#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <pthread.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <pcap.h>
#include "sipgraph.h"
#include "sipdump.h"

#define MAX_FILTER_LEN 128 /* for pcap_filter */
 
//SipGraph, share amongst everything
sip_graph_t *sg[MAX_GRAPHS];
unsigned short sg_length = 0;
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
      if(msg_offset < sg[graph_offset]->num_msg - 1) {
	msg_offset++;
      }
      break;
    case KEY_UP:
      if(msg_offset > 0) {
	msg_offset--;
	}
      break;
    case KEY_RIGHT:
      if(graph_offset < sg_length - 1) {
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
    } //switch
  } //while

  return 0;
}

void *do_input(void *ptr) {
  int input;
  while( (input = getch())) { 
    write(fd[1], &input, sizeof(input));
  }

  return 0;
}

void do_pcap_loop(u_char *arg, const struct pcap_pkthdr *header, const u_char *frame) {
  //header part
  //struct ether_header *ether_header = (struct ether_header *) frame;
  struct iphdr *ip_header = (struct iphdr *) (frame + sizeof(struct ether_header));
  int ip_header_length = ip_header->ihl * 4;   //ip header size may be variable
  //struct udphdr *udp_header = (struct udphdr *) (frame + sizeof(struct ether_header) + ip_header_length);
  unsigned int header_length = (sizeof(struct ether_header) + ip_header_length + sizeof(struct udphdr));
  
  //payload part
  const char *payload = (const char *) (frame + header_length);
  unsigned int payload_length = header->len - header_length;

  if( sipdump(payload, payload_length, sg, &sg_length) == -1 ) {
    //perror("Error in function sipdump\n");
    return;
  }

  //send update to display thread
  int input = 'r';
  write(fd[1], &input, sizeof(input)); 
}

void *do_capture(void *ptr) {
  char **argv = (char **) ptr;
  char *dev = argv[1], *port = argv[2];
  char errbuf[PCAP_ERRBUF_SIZE];
  char filter[MAX_FILTER_LEN];
  sprintf(filter, "udp and port %s", port);

  printf("Device: %s, Filter: %s\n", dev, filter);

  osip_t *osip;
  int err_no = 0;
  if( (err_no = osip_init(&osip)) != 0) {
    perror("Initializing the osip state machine and parser");
    exit(err_no);
  }
  
  //open sniffing session, non-promiscuous
  pcap_t *handle;
  //handle = pcap_open_live(dev, BUFSIZ, 0, 1000, errbuf);
  handle = pcap_open_offline(dev, errbuf);
  if( handle == NULL) {
    fprintf(stderr, "Couldn't open device %s: %s", dev, errbuf);
    exit(1);
  }
  //compile filter expression into a filter program
  struct bpf_program fp;
  //ignore ipv4 broadcast
  if(pcap_compile(handle, &fp, filter, 0, PCAP_NETMASK_UNKNOWN) == -1) { 
    fprintf(stderr, "Couldn't parse filter %s: %s\n", filter, pcap_geterr(handle));
    exit(1);
  }

  if(pcap_setfilter(handle, &fp) == -1) {
    fprintf(stderr, "Couldn't install filter %s: %s\n", filter, pcap_geterr(handle));
    exit(1);
  }

  if( pcap_loop(handle, -1, do_pcap_loop, NULL) == -1) {
    fprintf(stderr, "Error while processing of packets: %s\n", pcap_geterr(handle));
    exit(1);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  /* #include "demo_data.h" */

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
  if(pthread_create(&capture_thread, NULL, do_capture, argv) != 0) {
    perror("Creating thread for capture");
    exit(1);
  }

  pthread_join( graph_thread, NULL );
  pthread_join( input_thread, NULL );

  endwin();
  return 0;
}

