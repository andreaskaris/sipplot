#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include "sipgraph.h"

//SipGraph, share amongst everything
struct SipGraph sg[MAX_GRAPHS];
unsigned short num_sg;

void do_graph(WINDOW * wnd, int fd) {
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
    if( read(fd, &input, sizeof(input)) == -1 ) {
      perror("reading from pipe");
      exit(1);
    }
    
    //scroll up or down
    switch(input) {
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
}

void do_get_input(int fd) {
  int c;
  while( (c = getch())) {
    write(fd, &c, sizeof(c));
  }
}

void do_capture(int fd) {

}

int main() {
  #include "demo_data.h"

  //enable curses
  WINDOW * wnd;
  wnd = initscr();
  cbreak(); //disables line buffering, making characters immediately available
  noecho(); //do not print characters on screen
  keypad(stdscr, TRUE); // enable arrow keys, F1, ...
  curs_set(0);
  
  //setup pipe for interprocess communication
  int fd[2];
  if(pipe(fd) == - 1) {
    perror("creating pipe\n");
    exit(1);
  }

  //fork input and output process
  int pid;
  if( (pid = fork()) == -1 ) {
    perror("Error forking process\n");
    exit(1);
  } else if(pid) { //parent
    close(fd[1]);
    close(0); //close 0 = stdin
    do_graph(wnd, fd[0]);
  } else { //child
    close(fd[0]);
    close(1); //close 1 = stdout
    do_get_input(fd[1]);
  }

  endwin();
  return 0;
}

void draw(char d, pos x, pos y, pos max_x, pos max_y) {
  if(x > max_x) x = max_x;
  if(y > max_y) y = max_y;

  move(x, y); //move to position
  delch(); insch(d);
}
