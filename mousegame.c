// Skeleton version of Lab #3, CS 306 Spring 2014.
// Original Author: Norman Carver
//Rewriten by Ryan Sample to use multiple processes and buttons
// Compile as:
//   gcc -Wall -std=gnu99 -omspeed lab3-skel.c -lncurses -lpthread
//4/21/2014
#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ncurses.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>

#define ITERATIONS 5
#define MAX_WAIT_TIME 10
#define BUTTON_TIME 3


// Prototypes:
void sigchld_handler(int signal);
int check_click(int active, int mousex, int mousey,  int rows, int cols);
void child_func(int number, int rows, int cols, sem_t *mutex, int *stateptr);


// Global Variables:
int running = 4;  //Global for signal handler, count of running children.


int main(int argc, char *argv[])
{
  sem_t *sem_mutex;  //Binary semaphore to use a mutex
  int *stateptr;     //Shared memory global int to store active button state
  int hit_count = 0, rows, cols;
  WINDOW *mainwin = NULL;
  int nextch;
  MEVENT event;
  struct sigaction act;

  //Create a binary semaphore to use as a mutex:
  //(will be inerited by children)
  if ((sem_mutex = sem_open("/mutex", O_CREAT|O_EXCL, 0600, 1)) == SEM_FAILED) {
    perror("Semaphore creation failed");
    return EXIT_FAILURE; }

  //Now unlink semaphore so it will be deleted in case of ^-C or crash:
  sem_unlink("/mutex");

  //Setup anonymous, shared memory for global int:
  if ((stateptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
    perror("Shared memory creation failed");
    return EXIT_FAILURE; }

  //Initialize button state (no active buttons):
  *stateptr = 0;

  //Initialize and setup the curses window:
  if ((mainwin = initscr()) == NULL) {
    fprintf(stderr,"Failed to initialize curses window\n");
    return EXIT_FAILURE; }
  getmaxyx(mainwin,rows,cols);
  mvprintw(rows/2,cols/2-18,"Click on the Corner Buttons to Score!",rows,cols);
  mvprintw(rows/2+2,cols/2-10,"(rows: %d  cols: %d)",rows,cols);
  refresh();

  //Setup signal handler for SIGCHLD signals:
  memset(&act,0,sizeof(act));
  act.sa_handler = sigchld_handler;
  sigemptyset(&act.sa_mask);
  sigaction(SIGCHLD,&act,NULL);

  //Create children:
  //****************
 
  for (int i=0; i < running; i++){//memory problem???
	  switch(fork()){
		  case -1:
			perror("fork failed");
			sigchld_handler(EXIT_FAILURE);
			endwin();//put this in here since failures don't close the window apparently.
			exit(EXIT_FAILURE);
		  case 0:
			
			child_func(1+i, rows, cols,sem_mutex,stateptr);//1+processcount allows for proper numbering.
			sigchld_handler(EXIT_SUCCESS);
			exit(EXIT_SUCCESS);
		  default: 
		    break;//no break leads to error apparently.
			//I got it now the switch statement will determine the parent function follows the default.
		}
  }

  //Setup curses to get mouse events:
  keypad(mainwin, TRUE);
  mousemask(ALL_MOUSE_EVENTS, NULL);

  //Loop catching mouse clicks while children are running:
    while (running > 0) {
    //nextch = wgetch(mainwin);
    if ((nextch = getch()) == KEY_MOUSE && getmouse(&event) == OK && (event.bstate & BUTTON1_PRESSED)) {
      //Check if user clicked on a label:
      if (check_click(*stateptr, event.x, event.y, rows, cols)) {
        //Clicked on current label:
        hit_count++;
        mvprintw(rows/2+5,cols/2-11,"Got #%d at (%3d,%3d)",*stateptr,event.x,event.y);
        wrefresh(curscr);  //Need this to ensure entire screen is redrawn despite child changes.
      } }
  }

  //Close curses window so terminal settings are restored to normal:
  endwin();

  //Print out results:
  printf("\nYour hit count was: %d\n\n",hit_count);

  //Collect all the children:
  //*************************
  //return exit success if child successfully exited.
  int status;
  int tempcount = 0;//made a temp count since I think running will be 0 by now.
  while(tempcount<4){
  wait(&status);// wait for children shouldn't all return status
 
  if(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS){
	tempcount++;//increment temp count.
  }else
	exit(EXIT_FAILURE);//return exit failure according to lecture.
  }
  exit(EXIT_SUCCESS);//completes while loop so returns exit success.
}


// Signal handler for SIGCHLD signals.
// Use to keep track of number of running children
// so can stop monitoring mouse clicks when done.
void sigchld_handler(int signal)
{
  running--;
  return;
}


// Checks if a mouse click was on an active button or not:
int check_click(int active, int mousex, int mousey,  int rows, int cols)
{
    //Check if click was on a button
    switch(active) {
      case 1:
        if (mousey >= 0 && mousey <= 1 && mousex >= 0 && mousex <= +10)
          return 1;
        break;
      case 2:
        if (mousey >= 0 && mousey <= 1 && mousex >= cols-10 && mousex <= cols)
          return 1;
        break;
      case 3:
        if (mousey >= rows-2 && mousey <= rows-1 && mousex >= cols-10 && mousex <= cols)
          return 1;
        break;
      case 4:
        if (mousey >= rows-2 && mousey <= rows-1 && mousex >= 0 && mousex <= 10)
          return 1;
        break; }

    return 0;
}


// Function to be run in each of the four children.
// Puts up "buttons" at random times in appropriate corner.
// child_num is to be between 1 and 4.
void child_func(int child_num, int rows, int cols, sem_t *mutex, int *stateptr)//might not be done with this check when not tired.
{
	int numexec = 0;
	srand(getpid());
  while(numexec<ITERATIONS){//make sure less than max iterations
	sleep(rand()%(MAX_WAIT_TIME-1)+1);
	
	sem_wait(mutex);//try to post if not then wait.
		wrefresh(curscr);//this was the problem
		*stateptr = child_num;//should change stateptr value with * value.
		
		
		switch(child_num){
			case 1:
			mvprintw(0,0,"1111111111");
			mvprintw(1,0,"1111111111");
			break;
			case 2:
			mvprintw(0,cols-11,"2222222222");
			mvprintw(1,cols-11,"2222222222");
			break;
			case 3:
			mvprintw(rows-2,cols-11,"3333333333");
			mvprintw(rows-1,cols-11,"3333333333");
			break;
			case 4:
			mvprintw(rows-2,0,"4444444444");
			mvprintw(rows-1,0,"4444444444");
			break;
			
			
		}
		refresh();
		sleep(BUTTON_TIME);
		
		switch(child_num){
			case 1://case1
			mvprintw(0,0,"          ");
			mvprintw(1,0,"          ");
			break;
			case 2://case2
			mvprintw(0,cols-10,"          ");
			mvprintw(1,cols-10,"          ");
			break;			
			case 3://case 3
			mvprintw(rows-2,cols-10,"          ");
			mvprintw(rows-1,cols-10,"          ");
			break;
			case 4://case4
			mvprintw(rows-2,0,"          ");
			mvprintw(rows-1,0,"          ");
			break;
		}//clear window.
		refresh();//refresh window
		numexec++;//increment current times this child has executed.
		*stateptr = 0;//set stateptr back to 0
		sem_post(mutex);//close mutex should be last
	}
  //send signal out
}

//EOF
