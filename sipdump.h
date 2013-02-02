#ifndef SIPDUMP_H
#define SIPDUMP_H

#include <linux/ip.h>
#include <linux/udp.h>
#include <net/ethernet.h>
#include <osip2/osip.h>
#include "sipgraph.h"

#define SIP_DUMP_MAX_CHARACTERS 128

int sipdump(const u_char* frame, unsigned int frame_length, sip_graph_t *sg[], unsigned short *sg_length);
int ins_sip_graph(struct iphdr *ip_header,osip_event_t *oe, sip_graph_t *sg[], unsigned short *sg_length);

#endif
