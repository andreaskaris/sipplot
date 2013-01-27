#ifndef SIPGRAPH_H
#define SIPGRAPH_H

#define MAX_ACTORS 5 /* maximum number of actors in the diagram */
#define MAX_MSG 50 /* maximum number of messages */
#define MAX_GRAPHS 10 /* maximum number of concurrent graphs */

/***********/
/* for SIP */
/***********/

typedef char *SipActors[MAX_ACTORS];

//a message, from src to dst with desc
//e.g. "INVITE" from 0 to 1
struct SipMsg {
  int src;
  int dst;
  char *desc;
};

//actors, e.g. 0 => "192.168.1.1", 1 => "192.168.1.2"
//messages, e.g. "INVITE" from 0 to 1
struct SipGraph {
  struct SipMsg msg[MAX_MSG];
  unsigned short num_msg;
  SipActors a;
  unsigned short num_a;
};

/***************/
/* for drawing */
/***************/
typedef unsigned int pos;

//get offset between vertical lines
int get_offset(int num_a, pos max_y);
//get initial offset
int get_init_offset(int num_a, pos max_y);
//draw a line
void draw_line(pos x, pos y, int length, pos max_x, pos max_y);
//draw an arrow
void draw_arrow(pos x, pos y, int length, char dir, pos max_x, pos max_y);
//draw an arrow with a msg on top of it
void draw_msg(pos x, pos y, int length, char dir, char *msg, pos max_x, pos max_y);
//draw a msg from element x to element y
void draw_sip_msg(int from, int to, char *msg, int num_a, int x, pos max_x, pos max_y);
//draw a complete sip graph
void draw_graph(struct SipGraph sg, unsigned short msg_offset, pos max_x, pos max_y);

#endif