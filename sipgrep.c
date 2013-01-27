#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

#define MAX_ACTORS 3
#define MAX_MSG 50

//for SIP
typedef char *SipActors[MAX_ACTORS];
struct SipMsg {
  int src;
  int dst;
  char *desc;
};
struct SipGraph {
  struct SipMsg msg[MAX_MSG];
  unsigned short num_msg;
  SipActors a;
  unsigned short num_a;
};

//for drawing
typedef int pos;

int get_offset(int num_a, pos max_y) { 
  return max_y / num_a;
}

int get_init_offset(int num_a, pos max_y) {
  return get_offset(num_a,max_y) / 2;
}

void draw_line(pos x, pos y, int length, pos max_x, pos max_y) {
  if(x >= max_x) x = max_x - 1;
  if(y >= max_y) y = max_y - 1;
  if(y + length < max_y) {
    max_y = y + length;
  }

  for(; y < max_y;y++) {
    move(x, y);
    delch();insch('=');
  }
}

void draw_arrow(pos x, pos y, int length, char dir, pos max_x, pos max_y) {
  if(x >= max_x) x = max_x - 1;
  if(y >= max_y) y = max_y - 1;
  if(y + length < max_y) {
    max_y = y + length;
  }

  draw_line(x,y,length,max_x, max_y);
  if(dir == '<') {
    move(x,y);
    delch();insch(dir);
  } else if(dir == '>') {
    move(x,y+length - 1);
    delch();insch(dir);
  }
}

void draw_msg(pos x, pos y, int length, char dir, char *msg, pos max_x, pos max_y) {
  if(x >= max_x) x = max_x - 2;
  if(y >= max_y) y = max_y - 1;
  if(y + length < max_y) {
    max_y = y + length;
  }
  
  move(x,y);
  int msglen = 0;
  while(msg[msglen++] != '\0') { delch(); }
  move(x,y + (length - msglen) /2);
  insstr(msg);
  x++;
  draw_arrow(x,y,length,dir, max_x, max_y);
}

void draw_sip_msg(int from, int to, char *msg, int num_a, int x, pos max_x, pos max_y) {
  char dir;
  if(from > to) {
    dir = '<';
    //normalize
    int buf = to;
    to = from;
    from = buf;
  } else {
    dir = '>';
  }

  int offset = get_offset(num_a, max_y);
  int init_offset = get_init_offset(num_a, max_y);
  
  draw_msg(x, init_offset + from * offset + 1, (to - from) * offset - 1, dir, msg, max_x, max_y);
}

void draw_graph(struct SipGraph sg, unsigned short msg_offset, pos max_x, pos max_y) {
  pos x = 0, y = 0;
  pos offset = get_offset(sg.num_a, max_y);
  pos init_offset = get_init_offset(sg.num_a, max_y);
  y = init_offset;
  int i;
  /***************/
  /* draw header */
  /***************/
  attron(A_BOLD);
  //titles
  for(i = 0; i < sg.num_a; i++) {
    move(x, y - strlen(sg.a[i]) / 2);
    insstr(sg.a[i]);
    y += offset;
  }
  //go one line below
  x++;
  draw_line(x++, 0, max_y, max_x, max_y);
  attroff(A_BOLD);

  /*************/
  /* draw bars */
  /*************/
  for(;x < max_x;x++) {
    y = init_offset;
    for(i = 0; i < sg.num_a; i++) {
      move(x, y);
      delch();insch('|');
      y += offset;
    }
  }

  /*********************/
  /* draw sip messages */
  /*********************/
  x = 0;y = 0;
  int max_msg = max_x / 2 - 1 + msg_offset;
  if(sg.num_msg < max_msg) {
    max_msg = sg.num_msg;
  }
  
  for(i = msg_offset;i < max_msg;i++) {
    x += 2; 
    draw_sip_msg(sg.msg[i].src, sg.msg[i].dst, sg.msg[i].desc, sg.num_a, x, max_x, max_y);
  }

  /*********************/
  /* draw line numbers */
  /*********************/
  x = 0; y = 0;
  for(i = msg_offset; i < max_msg;i++) {
    x += 2;
    move(x,y);
    if(i < 10) {
      delch();insch('0' + i);
    } else {
      delch();delch();insch('0' + i % 10);insch('0' + i / 10);
    }
  }
}

int *msg_offset;

void do_graph(struct SipGraph sg, unsigned short msg_offset) {
  pos max_x,max_y; //number of total rows and cols in window 
  int d = 0;

  WINDOW *wnd;
  wnd = initscr();
  cbreak(); //disables line buffering, making characters immediately available
  noecho(); //do not print characters on screen
  keypad(stdscr, TRUE); // enable arrow keys, F1, ...
  curs_set(0); //invisible cursor
  getmaxyx(wnd, max_x, max_y); //size of window
  
  clear(); //clear screen, send cursor to (0,0)
  refresh();
  do {
    switch(d) {
    case KEY_DOWN:
      msg_offset++;
      clear();
      draw_graph(sg, msg_offset, max_x, max_y);
      refresh();
      break;
    case KEY_UP:
      if(msg_offset > 0) {
	msg_offset--;
	clear();
	draw_graph(sg, msg_offset, max_x, max_y);
	refresh();
      }
      break;
    }
  } while( (d = getch()) != 'q');

  endwin();
}

int main() {
  struct SipGraph sg;
  sg.num_a = 3;
  sg.a[0] = "192.168.1.1";
  sg.a[1] = "192.168.1.2";
  sg.a[2] = "192.168.1.3";
  sg.num_msg = 12;
  sg.msg[0].src = 0;
  sg.msg[0].dst = 1;
  sg.msg[0].desc = "INVITE";
  sg.msg[1].src = 1;
  sg.msg[1].dst = 2;
  sg.msg[1].desc = "INVITE";
  sg.msg[2].src = 2;
  sg.msg[2].dst = 1;
  sg.msg[2].desc = "Ringing";
  sg.msg[3].src = 1;
  sg.msg[3].dst = 0;
  sg.msg[3].desc = "Ringing";
  sg.msg[4].src = 2;
  sg.msg[4].dst = 1;
  sg.msg[4].desc = "OK";
  sg.msg[5].src = 1;
  sg.msg[5].dst = 0;
  sg.msg[5].desc = "OK";
  sg.msg[6].src = 1;
  sg.msg[6].dst = 2;
  sg.msg[6].desc = "ACK";
  sg.msg[7].src = 0;
  sg.msg[7].dst = 1;
  sg.msg[7].desc = "ACK";
  sg.msg[8].src = 0;
  sg.msg[8].dst = 1;
  sg.msg[8].desc = "Hangup";
  sg.msg[9].src = 1;
  sg.msg[9].dst = 2;
  sg.msg[9].desc = "Hangup";
  sg.msg[10].src = 2;
  sg.msg[10].dst = 1;
  sg.msg[10].desc = "OK";
  sg.msg[11].src = 1;
  sg.msg[11].dst = 0;
  sg.msg[11].desc = "ACK";
  sg.msg[12].src = 0;
  sg.msg[12].dst = 1;
  sg.msg[12].desc = "OK";

  msg_offset = malloc(sizeof(int));
  *msg_offset = 0;
  int fd[2];
  if( pipe(fd) == -1 ) {
    perror("Error creating pipe\n");
    exit(1);
  }
  int pid;
  if( (pid = fork()) == -1 ) {
    perror("Error forking process\n");
    exit(1);
  } else if(pid) { //parent
    close(fd[1]); //close write end of pipe
    close(0); //close 0 = stdin
    dup(fd[0]); //hook up 0 with read end of pipe
    //char buf[100];
    //read(0, buf, 100);
    //printf("%s\n", buf);
    do_graph(sg, *msg_offset);
  } else { //child
    close(fd[0]); //close read end of pipe
    close(1); //close 1 = stdout
    dup(fd[1]); //hook up 1 with write end
    
    int d = 0;
    while( (d = getchar()) != 'q') {
      switch(d) {
      case KEY_DOWN:
	*msg_offset++;
	break;
      case KEY_UP:
	if(*msg_offset > 0) {
	  *msg_offset--;
	}
	break;
      }
      putchar(d);
    }
  }

  return 0;
}

void draw(char d, pos x, pos y, pos max_x, pos max_y) {
  if(x > max_x) x = max_x;
  if(y > max_y) y = max_y;

  move(x, y); //move to position
  delch(); insch(d);
}
