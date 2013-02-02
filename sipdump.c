#include <stdbool.h>
#include <stdlib.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <osip2/osip.h>
#include "sipdump.h"
#include "sipgraph.h"

/***********************************************************************/
/* takes a complete ethernet frame with IP and UDP headers and payload */
/* if 								       */
/*   the payload can't be parsed as SIP, 			       */
/*   or if the message can't be inserted into the graph, return -1     */
/* else return 0 						       */
/***********************************************************************/
int sipdump(const u_char* frame, unsigned int frame_length, sip_graph_t *sg[], unsigned short *sg_length) {
  //header part
  //struct ether_header *ether_header = (struct ether_header *) frame;
  struct iphdr *ip_header = (struct iphdr *) (frame + sizeof(struct ether_header));
  int ip_header_length = ip_header->ihl * 4;   //ip header size may be variable
  //  struct udphdr *udp_header = (struct udphdr *) (frame + sizeof(struct ether_header) + ip_header_length);
  unsigned int header_length = (sizeof(struct ether_header) + ip_header_length + sizeof(struct udphdr));
  
  //payload part
  const char *payload = (const char *) (frame + header_length);
  unsigned int payload_length = frame_length - header_length;

  osip_event_t *oe = osip_parse(payload, payload_length);
  if(oe == NULL) {
    //perror("Error parsing SIP");
    return -1;
  }

  if( ins_sip_graph(ip_header, oe, sg, sg_length) == -1) {
    //perror("Could not insert message into sip_graph");
    return -1;
  }

  return 0;
}

/**********************************************************************************/
/* Analysis the IP addresses and SIP headers of this SIP message		  */
/* Uses call_id only to match an existing SIP dialog - this might result in wrong */
/* graphs, but for now it's an approximate, good enough solution		  */
/* src and destination addresses are taken from the IP header			  */
/* message descriptions from the SIP headers (request uri or status message)	  */
/* 										  */
/* if the call_id does not yet exist, create a new graph element		  */
/* else insert a new message into an existing graph				  */
/**********************************************************************************/
int ins_sip_graph(struct iphdr *ip_header, osip_event_t *oe, sip_graph_t *sg[], unsigned short *sg_length) {
  /*****************************************/
  /* to be inserted into sip_graph message */
  /*****************************************/
  char *desc = malloc(SIP_DUMP_MAX_CHARACTERS);
  char *from = malloc(SIP_DUMP_MAX_CHARACTERS);
  char *to = malloc(SIP_DUMP_MAX_CHARACTERS);

  /*****************/
  /* ip parameters */
  /*****************/
  struct in_addr in;
  in.s_addr = ip_header->saddr;
  strcpy(from, inet_ntoa(in));
  in.s_addr = ip_header->daddr;
  strcpy(to, inet_ntoa(in));

  /********************************/
  /* sip parameters (some unused) */
  /********************************/
  osip_via_t *via; /* for via headers */
  char *vias[10];
  osip_contact_t *contact; /* for contact headers */  
  char *contacts[10];
  char *call_id_number;
  int num_vias = oe->sip->vias.nb_elt;
  int num_contacts = oe->sip->contacts.nb_elt;
  //char *cseq_method;
  //char *cseq_number;

  //cseq_method = oe->sip->cseq->method;
  //cseq_number = oe->sip->cseq->number;
  int pos; /* for for loops */
  call_id_number = oe->sip->call_id->number; /* osip_call_id_t */
  for(pos = 0;pos < num_vias;pos++) {
    via = (osip_via_t *) osip_list_get(&oe->sip->vias, pos);
    vias[pos] = malloc(SIP_DUMP_MAX_CHARACTERS);
    sprintf(vias[pos], "%s:%s",via->host, via->port);
  }
  for(pos = 0;pos < num_contacts;pos++) {
    contact = (osip_contact_t *) osip_list_get(&oe->sip->contacts, pos);
    contacts[pos] = malloc(SIP_DUMP_MAX_CHARACTERS);
    sprintf(contacts[pos], "%s:%s",contact->url->host, contact->url->port);
  }

  /*******************************************************/
  /* switch on the SIP event type - request or response? */
  /*******************************************************/
  switch(oe->type) {
  case TIMEOUT_A: case TIMEOUT_B: case TIMEOUT_D: case TIMEOUT_E: case TIMEOUT_F: case TIMEOUT_K: case TIMEOUT_G: case TIMEOUT_H: case TIMEOUT_I: case TIMEOUT_J:
    break;
  case RCV_REQINVITE: case SND_REQINVITE: case RCV_REQUEST: case SND_REQUEST: case RCV_REQACK: case SND_REQACK:
    desc = oe->sip->sip_method; /* char* */
    //from = contacts[0];
    //sprintf(to, "%s:%s", oe->sip->req_uri->host, oe->sip->req_uri->port); /* char* */
    break;
  case RCV_STATUS_1XX: case RCV_STATUS_2XX: case RCV_STATUS_3456XX:  case SND_STATUS_1XX: case SND_STATUS_2XX: case SND_STATUS_3456XX:
    sprintf(desc,"%d %s", oe->sip->status_code, oe->sip->reason_phrase);
    //from = contacts[0];
    //to = vias[0];
    break;
  case KILL_TRANSACTION:
    break;
  case UNKNOWN_EVT:
    break;
  } /* end switch */

  /********************************************************************/
  /* if call_id does not match any other call_id: create new graph    */
  /* if call_id does match another call_id: insert message into graph */
  /********************************************************************/
  int i;
  for(i = 0;i < MAX_GRAPHS;i++) {
    /*****************************/
    /* append a new dialog-graph */
    /*****************************/
    if(i >= *sg_length) {
      if(sg[i] != NULL) {
	free(sg[i]);
      }
      sip_graph_t *new_graph = malloc(sizeof(sip_graph_t));
      new_graph->num_a = 2;
      new_graph->a[0] = from;
      new_graph->a[1] = to;
      new_graph->call_id = call_id_number;
      new_graph->num_msg = 1;
      new_graph->msg[0].src = 0; 
      new_graph->msg[0].dst = 1;
      new_graph->msg[0].desc = desc;
      new_graph->msg[0].event = oe;
      //new_graph->msg[0].contact = contacts[0];
      sg[i] = new_graph;
      (*sg_length)++;

      return 0;
    } 
    /*************************************/
    /* call_id already exists		 */
    /* append a msg to an existing graph */
    /*************************************/
    else if(strcmp(sg[i]->call_id, call_id_number) == 0) { 
      //determine index for msg_src
      bool found = false;
      int msg_src;
      for(msg_src = 0; msg_src < sg[i]->num_a; msg_src++) {
	if(strcmp(sg[i]->a[msg_src], from) == 0) {
	  found = true;
	  break;
	} 
      }
      //if no index found, append this actor to the list of actors
      if(!found) {
	msg_src = sg[i]->num_a++;
	sg[i]->a[msg_src] = from;
      }
      //determine index for msg_dst
      found = false;
      int msg_dst;
      for(msg_dst = 0; msg_dst < sg[i]->num_a; msg_dst++) {
	if(strcmp(sg[i]->a[msg_dst], to) == 0) {
	  found = true;
	  break;
	}
      }
      //if no index found, append this actor to the list of actors
      if(!found) {
	msg_dst = sg[i]->num_a++;
	sg[i]->a[msg_dst] = from;
      }
      //now, insert everything into the graph
      sg[i]->msg[sg[i]->num_msg].src = msg_src;
      sg[i]->msg[sg[i]->num_msg].dst = msg_dst;
      sg[i]->msg[sg[i]->num_msg].desc = desc;
      sg[i]->msg[sg[i]->num_msg].event = oe;
      sg[i]->num_msg++;
      return 0;
    } /* end else if */
  } /* end for */

  return 0;
} /* end ins_sip_graph */
