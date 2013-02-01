#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include "sipgraph.h"

/************************************************/
/* get offset between 2 vertical lines in graph */
/************************************************/
int get_offset(int num_a, pos max_y) { 
  return max_y / num_a;
}

/*********************************************************/
/* get left offset of first element from the left border */
/*********************************************************/
int get_init_offset(int num_a, pos max_y) {
  return get_offset(num_a,max_y) / 2;
}

/*********************************/
/* draw line from        	 */
/* (x,y) with a length of length */
/*********************************/
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

/******************************************/
/* draw an arrow; like line, but with dir */
/* '>' right				  */
/* '<' left				  */
/******************************************/
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

/******************************************************************/
/* draw a message; arrow with a message on top of the arrow, like */
/*       INVITE							  */
/* ===================>						  */
/******************************************************************/
void draw_msg(pos x, pos y, int length, char dir, char *msg, pos max_x, pos max_y) {
  if(x >= max_x) x = max_x - 2;
  if(y >= max_y) y = max_y - 1;
  if(y + length < max_y) {
    max_y = y + length;
  }
  
  int msglen = strlen(msg);
  move(x,y + (length - msglen) /2);
  addstr(msg); //like insstr, but replaces content underneath
  x++;
  draw_arrow(x,y,length,dir, max_x, max_y);
}

/*****************************************************************************************/
/* like draw_msg, but this time dependent on the graph implementation			 */
/* draw a msg from line "from" to line "to"						 */
/*   if to < from, internally, to and from are inversed and the direction '<' is chosen	 */
/*   takes from and to index, message to be displayed, number of actors in the sip graph */
/*****************************************************************************************/
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

/*******************************************************************/
/* draw a complete graph, beginning with the message at msg_offset */
/*******************************************************************/
void draw_graph(sip_graph_t *sg, unsigned short msg_offset, pos max_x, pos max_y) {
  if(sg == NULL) return;

  pos x = 0, y = 0;
  pos offset = get_offset(sg->num_a, max_y);
  pos init_offset = get_init_offset(sg->num_a, max_y);
  y = init_offset;
  int i;
  /***************/
  /* draw header */
  /***************/
  attron(A_BOLD);
  //titles
  for(i = 0; i < sg->num_a; i++) {
    move(x, y - strlen(sg->a[i]) / 2);
    insstr(sg->a[i]);
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
    for(i = 0; i < sg->num_a; i++) {
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
  if(sg->num_msg < max_msg) {
    max_msg = sg->num_msg;
  }
  
  for(i = msg_offset;i < max_msg;i++) {
    x += 2; 
    draw_sip_msg(sg->msg[i].src, sg->msg[i].dst, sg->msg[i].desc, sg->num_a, x, max_x, max_y);
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
