#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <pthread.h>
#include "sipgraph.h"

//SipGraph, share amongst everything
struct SipGraph sg[MAX_GRAPHS];
unsigned short num_sg;
int fd[2];
WINDOW * wnd;

void *do_graph(void *ptr) {
  pos max_x,max_y; //number of total rows and cols in window 
  int msg_offset = 0;
  int graph_offset = 0;
  int input;
  getmaxyx(wnd, max_x, max_y); //size of window
  
  while(true) {
    //draw frame
    clear();
    draw_graph(sg[graph_offset], msg_offset, max_x, max_y);
    refresh();
    
    //read integer from input pipe, block until input
    if( read(fd[0], &input, sizeof(input)) == -1 ) {
      perror("reading from pipe");
      exit(1);
    }
    
    //scroll up or down
    switch(input) {
    case 'r':
      //just refresh, do nothing
      break;
    case KEY_DOWN:
      if(msg_offset < sg[graph_offset].num_msg - 1) {
	msg_offset++;
      }
      break;
    case KEY_UP:
      if(msg_offset > 0) {
	msg_offset--;
	}
      break;
    case KEY_RIGHT:
      if(graph_offset < num_sg - 1) {
	graph_offset++;
	msg_offset = 0;
      }
      break;
    case KEY_LEFT:
      if(graph_offset > 0) {
	graph_offset--;
	msg_offset = 0;
      }
      break;
    }
  }

  return 0;
}

void *do_input(void *ptr) {
  int input;
  while( (input = getch())) { 
    write(fd[1], &input, sizeof(input));
  }

  return 0;
}

void *do_capture(void *ptr) {
  sleep(5);

  sg[0].msg[0].desc = "HOHOHO";;
  int input = 'r';
  write(fd[1], &input, sizeof(input)); 

  return 0;
}

int main() {
  #include "demo_data.h"

  //enable curses
  wnd = initscr();
  cbreak(); //disables line buffering, making characters immediately available
  noecho(); //do not print characters on screen
  keypad(stdscr, TRUE); // enable arrow keys, F1, ...
  curs_set(0);

  //create a pipe
  if( pipe(fd) == -1 ) {
    perror("Creating pipe\n");
    exit(1);
  }
  
  //thread input and output process
  pthread_t graph_thread, input_thread, capture_thread;
  if(pthread_create(&graph_thread, NULL, do_graph, NULL) != 0) {
    perror("Creating thread for graph");
    exit(1);
  }
  if(pthread_create(&input_thread, NULL, do_input, NULL) != 0) {
    perror("Creating thread for input");
    exit(1);
  }
  if(pthread_create(&capture_thread, NULL, do_capture, NULL) != 0) {
    perror("Creating thread for capture");
    exit(1);
  }

  pthread_join( graph_thread, NULL );
  pthread_join( input_thread, NULL );

  endwin();
  return 0;
}

void draw(char d, pos x, pos y, pos max_x, pos max_y) {
  if(x > max_x) x = max_x;
  if(y > max_y) y = max_y;

  move(x, y); //move to position
  delch(); insch(d);
}
