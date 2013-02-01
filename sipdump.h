#ifndef SIPDUMP_H
#define SIPDUMP_H

#include <osip2/osip.h>
#include "sipgraph.h"

int sipdump(const char* payload, unsigned int payload_length, sip_graph_t *sg[], unsigned short *sg_length);
int ins_req_sip_graph(osip_event_t *oe, sip_graph_t *sg[], unsigned short *sg_length);
int ins_resp_sip_graph(osip_event_t *oe, sip_graph_t *sg[], unsigned short *sg_length);
int ins_sip_graph(osip_event_t *oe, sip_graph_t *sg[], unsigned short *sg_length);

#endif
