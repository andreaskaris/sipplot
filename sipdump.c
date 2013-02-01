#include <stdlib.h>
#include <osip2/osip.h>
#include "sipdump.h"
#include "sipgraph.h"

#define MAX_CHARACTERS 128

int sipdump(const char* payload, unsigned int payload_length, sip_graph_t *sg[], unsigned short *sg_length) {
  osip_event_t *oe = osip_parse(payload, payload_length);
  if(oe == NULL) {
    //perror("Error parsing SIP");
    return -1;
  }

  if( ins_sip_graph(oe, sg, sg_length) == -1) {
    //perror("Could not insert message into sip_graph");
    return -1;
  }

  return 0;
}

int ins_sip_graph(osip_event_t *oe, sip_graph_t *sg[], unsigned short *sg_length) {
  switch(oe->type) {
  case TIMEOUT_A: case TIMEOUT_B: case TIMEOUT_D: case TIMEOUT_E: case TIMEOUT_F: case TIMEOUT_K: case TIMEOUT_G: case TIMEOUT_H: case TIMEOUT_I: case TIMEOUT_J:
    //printf("timeout %s\n", oe->sip->sip_method);
    break;
  case RCV_REQINVITE: case SND_REQINVITE: 
    return ins_req_sip_graph(oe, sg, sg_length);
    break;
  case RCV_REQUEST: case SND_REQUEST: case RCV_REQACK: case SND_REQACK:
    break;
  case RCV_STATUS_1XX: case RCV_STATUS_2XX: case RCV_STATUS_3456XX:  case SND_STATUS_1XX: case SND_STATUS_2XX: case SND_STATUS_3456XX:
    return ins_resp_sip_graph(oe, sg, sg_length);
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


int ins_req_sip_graph(osip_event_t *oe, sip_graph_t *sg[], unsigned short *sg_length) {
  int pos; /* for for loops */
  osip_via_t *via; /* for via headers */
  char *vias[10];
  int num_vias;
  osip_contact_t *contact; /* for contact headers */  
  char *contacts[10];
  int num_contacts;
  char *method;
  char *call_id_number;
  //char *cseq_method;
  //char *cseq_number;
  char *req_uri = malloc(MAX_CHARACTERS);

  method = oe->sip->sip_method; /* char* */
  sprintf(req_uri, "%s:%s", oe->sip->req_uri->host, oe->sip->req_uri->port); /* char* */
  call_id_number = oe->sip->call_id->number; /* osip_call_id_t */
  num_vias = oe->sip->vias.nb_elt;
  for(pos = 0;pos < num_vias;pos++) {
    via = (osip_via_t *) osip_list_get(&oe->sip->vias, pos);
    vias[pos] = malloc(MAX_CHARACTERS);
    sprintf(vias[pos], "%s:%s",via->host, via->port);
  }
  num_contacts = oe->sip->contacts.nb_elt;
  for(pos = 0;pos < num_contacts;pos++) {
    contact = (osip_contact_t *) osip_list_get(&oe->sip->contacts, pos);
    contacts[pos] = malloc(MAX_CHARACTERS);
    sprintf(contacts[pos], "%s:%s",contact->url->host, contact->url->port);
  }
  //cseq_method = oe->sip->cseq->method;
  //cseq_number = oe->sip->cseq->number;
  
  int i;
  for(i = 0;i < MAX_GRAPHS;i++) {
    //append a new dialog-graph
    if(i >= *sg_length) {
      if(sg[i] != NULL) {
	free(sg[i]);
      }
      sip_graph_t *new_graph = malloc(sizeof(sip_graph_t));
      new_graph->num_a = 2;
      new_graph->a[0] = contacts[0];
      new_graph->a[1] = req_uri;
      new_graph->call_id = call_id_number;
      new_graph->num_msg = 1;
      new_graph->msg[0].src = 0; 
      new_graph->msg[0].dst = 1;
      new_graph->msg[0].desc = method;
      new_graph->msg[0].event = oe;
      new_graph->msg[0].contact = contacts[0];
      sg[i] = new_graph;
      (*sg_length)++;

      return 0;
    } else if(strcmp(sg[i]->call_id, call_id_number) == 0) { //append a msg to an existing graph
      if(strcmp(sg[i]->msg[0].contact, contacts[0]) == 0) {
	sg[i]->msg[sg[i]->num_msg].src = 0;
	sg[i]->msg[sg[i]->num_msg].dst = 1;
      } else {
	sg[i]->msg[sg[i]->num_msg].src = 1;
	sg[i]->msg[sg[i]->num_msg].dst = 0;
      }
      sg[i]->msg[sg[i]->num_msg].desc = method;
      sg[i]->msg[sg[i]->num_msg].event = oe;
      sg[i]->num_msg++;
    }
  }

  return 0;
}

int ins_resp_sip_graph(osip_event_t *oe, sip_graph_t *sg[], unsigned short *sg_length) {
  int pos; /* for for loops */
  osip_via_t *via; /* for via headers */
  char *vias[10];
  int num_vias = oe->sip->vias.nb_elt;
  osip_contact_t *contact; /* for contact headers */  
  char *contacts[10];
  int num_contacts = oe->sip->contacts.nb_elt;
  char *call_id_number;
  //char *cseq_method;
  //char *cseq_number;
  char *status_code = malloc(MAX_CHARACTERS);
  
  sprintf(status_code,"%d %s", oe->sip->status_code, oe->sip->reason_phrase);
  call_id_number = oe->sip->call_id->number; /* osip_call_id_t */
  for(pos = 0;pos < num_vias;pos++) {
    via = (osip_via_t *) osip_list_get(&oe->sip->vias, pos);
    vias[pos] = malloc(MAX_CHARACTERS);
    sprintf(vias[pos], "%s:%s",via->host, via->port);
  }
  for(pos = 0;pos < num_contacts;pos++) {
    contact = (osip_contact_t *) osip_list_get(&oe->sip->contacts, pos);
    contacts[pos] = malloc(MAX_CHARACTERS);
    sprintf(contacts[pos], "%s:%s",contact->url->host, contact->url->port);
  }
  //cseq_method = oe->sip->cseq->method;
  //cseq_number = oe->sip->cseq->number;

  int i;
  for(i = 0;i < MAX_GRAPHS;i++) {
    //append a new dialog-graph
    if(i >= *sg_length) {
      if(sg[i] != NULL) {
	free(sg[i]);
      }

      sip_graph_t *new_graph = malloc(sizeof(sip_graph_t));
      new_graph->num_a = 2;
      new_graph->a[0] = contacts[0];
      new_graph->a[1] = vias[0];
      new_graph->call_id = call_id_number;
      new_graph->num_msg = 1;
      new_graph->msg[0].src = 0; 
      new_graph->msg[0].dst = 1;
      new_graph->msg[0].desc = status_code;
      new_graph->msg[0].event = oe;
      new_graph->msg[0].contact = contacts[0];
      sg[i] = new_graph;
      (*sg_length)++;

      return 0;
    } else if(strcmp(sg[i]->call_id, call_id_number) == 0) { //append a msg to an existing graph
      if(strcmp(sg[i]->msg[0].contact, contacts[0]) == 0) {
	sg[i]->msg[sg[i]->num_msg].src = 0;
	sg[i]->msg[sg[i]->num_msg].dst = 1;
      } else {
	sg[i]->msg[sg[i]->num_msg].src = 1;
	sg[i]->msg[sg[i]->num_msg].dst = 0;
      }
      sg[i]->msg[sg[i]->num_msg].desc = status_code;
      sg[i]->msg[sg[i]->num_msg].event = oe;
      sg[i]->num_msg++;
    }
  }

  return 0;
}

