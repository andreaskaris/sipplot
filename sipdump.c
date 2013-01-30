#include <stdlib.h>
#include <osip2/osip.h>
#include "sipdump.h"
#include "sipgraph.h"

int sipdump(const char* payload, unsigned int payload_length, sip_graph_t *sg[]) {
  osip_event_t *oe = osip_parse(payload, payload_length);
  if(oe == NULL) {
    //perror("Error parsing SIP");
    return -1;
  }

  if( ins_sip_graph(oe, sg) == -1) {
    //perror("Could not insert message into sip_graph");
    return -1;
  }

  return 0;
}

int ins_sip_graph(osip_event_t *oe, sip_graph_t *sg[]) {
  switch(oe->type) {
  case TIMEOUT_A: case TIMEOUT_B: case TIMEOUT_D: case TIMEOUT_E: case TIMEOUT_F: case TIMEOUT_K: case TIMEOUT_G: case TIMEOUT_H: case TIMEOUT_I: case TIMEOUT_J:
    //printf("timeout %s\n", oe->sip->sip_method);
    break;
  case RCV_REQINVITE: case SND_REQINVITE: 
    return ins_req_sip_graph(oe, sg);
    break;
  case RCV_REQUEST: case SND_REQUEST: case RCV_REQACK: case SND_REQACK:
    break;
  case RCV_STATUS_1XX: case RCV_STATUS_2XX: case RCV_STATUS_3456XX:  case SND_STATUS_1XX: case SND_STATUS_2XX: case SND_STATUS_3456XX:
    return ins_recv_sip_graph(oe, sg);
    break;
  case KILL_TRANSACTION:
    //printf("kill transaction %s\n", oe->sip->sip_method);
    break;
  case UNKNOWN_EVT:
    //printf("unknown event %s\n", oe->sip->sip_method);
    break;
  }

  return 0;
}


int ins_req_sip_graph(osip_event_t *oe, sip_graph_t *sg[]) {
  int pos; /* for for loops */
  osip_via_t *via; /* for via headers */
  char vias[10][1024];
  int num_vias;
  osip_contact_t *contact; /* for contact headers */  
  char contacts[10][1024];
  int num_contacts;
  char *method;
  char *call_id_number;
  char *cseq_method;
  char *cseq_number;
  char status_code[1024];
  char req_uri[1024];

  method = oe->sip->sip_method; /* char* */
  sprintf(req_uri, "%s:%s", oe->sip->req_uri->host, oe->sip->req_uri->port); /* char* */
  call_id_number = oe->sip->call_id->number; /* osip_call_id_t */
  num_vias = oe->sip->vias.nb_elt;
  for(pos = 0;pos < num_vias;pos++) {
    via = (osip_via_t *) osip_list_get(&oe->sip->vias, pos);
    sprintf(vias[pos], "%s:%s",via->host, via->port);
  }
  num_contacts = oe->sip->contacts.nb_elt;
  for(pos = 0;pos < num_contacts;pos++) {
    contact = (osip_contact_t *) osip_list_get(&oe->sip->contacts, pos);
    sprintf(contacts[pos], "%s:%s",contact->url->host, contact->url->port);
  }
  cseq_method = oe->sip->cseq->method;
  cseq_number = oe->sip->cseq->number;
  
  int i;
  for(i = 0;i < MAX_GRAPHS;i++) {
    //append a new dialog to the graph
    if(sg[i] == NULL) {
      sip_graph_t *new_graph = malloc(sizeof(sip_graph_t));
      new_graph->num_a = 2;
      new_graph->a[0] = contacts[0];
      new_graph->a[1] = req_uri;
      new_graph->num_msg = 1;
      new_graph->msg[0].src = 0; 
      new_graph->msg[0].dst = 1;
      new_graph->msg[0].desc = method;
      new_graph->msg[0].event = oe;
      sg[i] = new_graph;
      
      return 0;
    }
  }

  return 0;
}

int ins_recv_sip_graph(osip_event_t *oe, sip_graph_t *sg[]) {
  int pos; /* for for loops */
  osip_via_t *via; /* for via headers */
  char vias[10][1024];
  int num_vias;
  osip_contact_t *contact; /* for contact headers */  
  char contacts[10][1024];
  int num_contacts;
  char *method;
  char *call_id_number;
  char *cseq_method;
  char *cseq_number;
  char status_code[1024];
  char req_uri[1024];
  
  
  sprintf(status_code,"%d %s", oe->sip->status_code, oe->sip->reason_phrase);
  call_id_number = oe->sip->call_id->number; /* osip_call_id_t */
  for(pos = 0;pos < oe->sip->vias.nb_elt;pos++) {
    via = (osip_via_t *) osip_list_get(&oe->sip->vias, pos);
    sprintf(vias[pos], "%s:%s",via->host, via->port);
  }
  for(pos = 0;pos < oe->sip->contacts.nb_elt;pos++) {
    contact = (osip_contact_t *) osip_list_get(&oe->sip->contacts, pos);
    sprintf(contacts[pos], "%s:%s",contact->url->host, contact->url->port);
  }
  cseq_method = oe->sip->cseq->method;
  cseq_number = oe->sip->cseq->number;

  return 0;
}

