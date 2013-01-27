#include <stdio.h>
#include <unistd.h>
#include <curses.h>

#define MAX_ACTORS 4
#define MAX_MSG 10

//for SIP
typedef char *actors[3];
struct Msg {
  char *src;
  char *dst;
  char *desc;
};
struct SipGraph {
  struct Msg msg[MAX_MSG];
  unsigned short num_msg;
  actors a;
  unsigned short num_a;
};

//for drawing
typedef int pos;
void draw(char d, pos x, pos y, pos max_x, pos max_y);
void draw_graph(struct SipGraph sg, pos max_x, pos max_y);

int get_offset(int num_a, pos max_y) { 
  return max_y / num_a;
}

int get_init_offset(int num_a, pos max_y) {
  return offset(num_a,max_y) / 2;
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
  
  draw_msg(x, init_offset + from * offset, (from - to) * offset, dir, msg, max_x, max_y);
}

void draw_graph(struct SipGraph sg, pos max_x, pos max_y) {
  pos x = 0, y = 0;
  pos offset = get_offset(sg.num_a, max_y);
  pos init_offset = get_init_offset(sg.num_a, max_y);
  y = init_offset;
  int i;
  //titles
  for(i = 0; i < sg.num_a; i++) {
    move(x, y);
    insstr(sg.a[i]);
    y += offset;
  }
  //go one line below
  x++;
  draw_line(x++, 0, max_y, max_x, max_y);
  
  //draw bars
  for(;x < max_x;x++) {
    y = init_offset;
    for(i = 0; i < sg.num_a; i++) {
      move(x, y);
      delch();insch('|');
      y += offset;
    }
  }

  x = 2;y = 0;
  draw_sip_msg(0, 1, "INVITE", sg.num_a, x, max_x, max_y);
  x = 2;y = 0;
  draw_sip_msg(1, 2, "INVITE", sg.num_a, x, max_x, max_y);
  x = 2;y = 0;
  draw_sip_msg(2, 1, "ACK", sg.num_a, x, max_x, max_y);
  x = 2;y = 0;
  draw_sip_msg(1, 0, "ACK", sg.num_a, x, max_x, max_y);
}

int main() {
  struct SipGraph sg;
  sg.num_a = 3;
  sg.a[0] = "192.168.1.1";
  sg.a[1] = "192.168.1.2";
  sg.a[2] = "192.168.1.3";
  sg.num_msg = 8;
  sg.msg[0].src = "192.168.1.1";
  sg.msg[0].dst = "192.168.1.2";
  sg.msg[0].desc = "INVITE";
  sg.msg[1].src = "192.168.1.2";
  sg.msg[1].dst = "192.168.1.3";
  sg.msg[1].desc = "INVITE";
  sg.msg[2].src = "192.168.1.3";
  sg.msg[2].dst = "192.168.1.2";
  sg.msg[2].desc = "Ringing";
  sg.msg[3].src = "192.168.1.2";
  sg.msg[3].dst = "192.168.1.1";
  sg.msg[3].desc = "Ringing";
  sg.msg[4].src = "192.168.1.3";
  sg.msg[4].dst = "192.168.1.2";
  sg.msg[4].desc = "OK";
  sg.msg[5].src = "192.168.1.2";
  sg.msg[5].dst = "192.168.1.1";
  sg.msg[5].desc = "OK";
  sg.msg[6].src = "192.168.1.2";
  sg.msg[6].dst = "192.168.1.3";
  sg.msg[6].desc = "ACK";
  sg.msg[7].src = "192.168.1.1";
  sg.msg[7].dst = "192.168.1.2";
  sg.msg[7].desc = "ACK";

  pos x = 0, y = 0; //current row and column (r,c), upper left (0,0) 
  pos max_x,max_y; //number of total rows and cols in window 
  char d;

  WINDOW *wnd;
  wnd = initscr();
  
  cbreak(); //disables line buffering, making characters immediately available
  noecho(); //do not print characters on screen

  getmaxyx(wnd, max_x, max_y); //size of window
  
  clear(); //clear screen, send cursor to (0,0)
  refresh();
  draw_graph(sg, max_x, max_y);
  refresh();
  sleep(180);

  endwin();

  return 0;
}

void draw(char d, pos x, pos y, pos max_x, pos max_y) {
  if(x > max_x) x = max_x;
  if(y > max_y) y = max_y;

  move(x, y); //move to position
  delch(); insch(d);
}
