#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <pthread.h>
#include <pcap.h>
#include <getopt.h>
#include <stdbool.h>
#include "sipgraph.h"
#include "sipdump.h"

#define MAX_FILTER_LEN 128 /* for pcap_filter */

/**************************/
/* command line arguments */
/**************************/
char *capture_interface = NULL;
char *capture_file = NULL;
char *capture_port = NULL;
bool is_udp = true;
 
/********************/
/* global variables */
/********************/
sip_graph_t *sg[MAX_GRAPHS]; /* collection of sip_graphs */
unsigned short sg_length = 0; /* length of sip_graph */
int pipe_fd[2]; /* thread communication from user input and capture to graph thread (pipe) */
WINDOW * wnd; /* the main window (curses) */
WINDOW * info; /* the sub info window */

/**************************/
/* 3 threads for 	  */
/*   a) graphing 	  */
/*   b) user input 	  */
/*   c) libpcap capturing */
/**************************/
void *do_graph(void *ptr);
void *do_input(void *ptr);
void *do_capture(void *ptr);

/*******************************************************************/
/* libpcap loopback, executed whenever libpcap sniffs a new packet */
/*******************************************************************/
void do_pcap_loop(u_char *arg, const struct pcap_pkthdr *header, const u_char *frame);

/**********************************/
/* usage instructions for options */
/**********************************/
void print_usage();

/************************************/
/* callback which is called on exit */
/* end curses windows session	    */
/************************************/
void atexit_callback();

int main(int argc, char *argv[]) {  
  int option = 0;
  while((option = getopt(argc, argv, "huti:f:p:")) != -1) {
    switch (option) {
    case 'i':
      capture_interface = optarg;
      break;
    case 'f':
      capture_file = optarg;
      break;
    case 'p':
      capture_port = optarg;
      break;
    case 'u':
      is_udp = true;
      break;
    case 't':
      printf("TCP currently not supported\n\n");
    case 'h': 
    default: 
      print_usage();
      exit(0);
    }
  }
  if(capture_interface == NULL && capture_file == NULL) {
    print_usage();
    exit(0);
  }

  //register atexit handler that ends curses
  if(atexit(atexit_callback)) {
    fprintf(stderr, "Could not register atexit callback");
    exit(1);
  }

  //enable curses
  wnd = initscr();
  cbreak(); //disables line buffering, making characters immediately available
  noecho(); //do not print characters on screen
  keypad(stdscr, true); // enable arrow keys, F1, ...
  curs_set(0); //disable the cursor
  //set colors
  if(has_colors() == false) {
    printf("Your terminal does not support colors\n");
    exit(1);
  } 
  start_color();
  
  //create a pipe
  if( pipe(pipe_fd) == -1 ) {
    perror("Creating pipe\n");
    exit(1);
  }
  
  //create input, output and capture thread
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

  //wait for all threads before quitting the application
  pthread_join( graph_thread, NULL );
  pthread_join( input_thread, NULL );
  pthread_join( capture_thread, NULL );

  //close curses
  endwin();

  return 0;
}

/************************************/
/* graph thread			    */
/* uses curses to draw SIP graphs   */
/* listens to messages from pipe_fd */
/************************************/ 
void *do_graph(void *ptr) {
  pos max_x,max_y; //number of total rows and cols in window 
  unsigned short msg_offset = 0; /* shift the messages downwards */
  unsigned short msg_highlight = 0; /* highlight a particular message */
  unsigned short graph_offset = 0; /* which graph */
  int input;
  getmaxyx(wnd, max_x, max_y); //size of window

  while(true) {
    //draw frame
    clear();
    draw_graph(sg[graph_offset], msg_highlight, msg_offset, max_x, max_y);
    refresh();
    
    //read integer from input pipe, block until input
    if( read(pipe_fd[0], &input, sizeof(input)) == -1 ) {
      perror("reading from pipe");
      exit(1);
    }
    
    //scroll up or down
    switch(input) {
    case 'r':
      //just refresh, do nothing
      break;
    case KEY_DOWN:
      if(msg_highlight < sg[graph_offset]->num_msg - 1) {
	msg_highlight++;
	int max_msg = get_max_msg(sg[graph_offset], msg_offset, max_x);
	if(msg_highlight >= max_msg) {
	  msg_offset++;
	}
      }
      break;
    case KEY_UP:
      if(msg_highlight > 0) {
	msg_highlight--;
	if(msg_highlight < msg_offset) {
	  msg_offset--;
	}
      }
      break;
    case KEY_RIGHT:
      if(graph_offset < sg_length - 1) {
	graph_offset++;
	msg_offset = 0;
	msg_highlight = 0;
      }
      break;
    case KEY_LEFT:
      if(graph_offset > 0) {
	graph_offset--;
	msg_offset = 0;
	msg_highlight = 0;
      }
      break;
    case '\n':
      info = subwin(wnd, max_x - 10, max_y - 10, 5, 5);
      wbkgd(info, A_NORMAL | ' ');
      wrefresh(info);
      sleep(10);
      delwin(info);
      break;
    } //switch
  } //while

  return 0;
}

/*****************************************************************/
/* input thread							 */
/* listens to keyboard input and redirects the input to the pipe */
/* where the graph thread can process it			 */
/*****************************************************************/
void *do_input(void *ptr) {
  int input;
  while( (input = getch())) { 
    write(pipe_fd[1], &input, sizeof(input));
  }

  return 0;
}

/*************************************************/
/* capture thread				 */
/* listen for incoming packets or read from file */
/* modify sg					 */
/* notify graph thread of changes		 */
/*************************************************/
void *do_capture(void *ptr) {
  char *dev, *port;
  if(capture_port != NULL) {
    port = capture_port;
  } else {
    port = "5060";
  }

  char errbuf[PCAP_ERRBUF_SIZE];
  char filter[MAX_FILTER_LEN];
  sprintf(filter, "udp and port %s", port);

  osip_t *osip;
  int err_no = 0;
  if( (err_no = osip_init(&osip)) != 0) {
    perror("Initializing the osip state machine and parser");
    exit(err_no);
  }
  
  //open sniffing session, non-promiscuous
  pcap_t *handle;
  if(capture_interface != NULL) {
    dev = capture_interface;
    handle = pcap_open_live(dev, BUFSIZ, 0, 1000, errbuf);
  } else {
    dev = capture_file;
    handle = pcap_open_offline(dev, errbuf);
  }
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

/*************************************************************************/
/* pcap callback function, executed whenever a packet matches the filter */
/*************************************************************************/
void do_pcap_loop(u_char *arg, const struct pcap_pkthdr *header, const u_char *frame) {
  if( sipdump(frame, header->len, sg, &sg_length) == -1 ) {
    //perror("Error in function sipdump\n");
    return;
  }

  //send update to display thread
  int input = 'r';
  write(pipe_fd[1], &input, sizeof(input)); 
}

/**********************************/
/* usage instructions for options */
/**********************************/
void print_usage() {
  printf("Usage: sipplot [OPTION]\n");
  printf("  -u UDP capture\n");
  printf("  -t TCP capture (currently not supported)\n");
  printf("  -i <interface> live capture on interface\n");
  printf("  -f <filename> capture from pcap file\n");
}

/************************************/
/* callback which is called on exit */
/* end curses windows session	    */
/************************************/
void atexit_callback() {
  endwin(); /* weird console behaviour if curses is not terminated */
}
