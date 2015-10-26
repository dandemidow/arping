#ifndef _FARAME_H_
#define _FARAME_H_

#include <ifaddrs.h>

#include "arp.h"

#define ETH_HDRLEN 14      // Ethernet header length
#define ETH_FRAME_ARP ETH_HDRLEN+ARP_HDRLEN

void init_frame(char *frame,
                struct ifaddrs *local,
                struct ifaddrs *remote);

void delete_frame_type(char *frame);
arp_hdr *alloc_arphdr(char *frame);
int is_frame_arp(char *frame);

int is_source_mac(char *frame, char *mac);

#endif  // _FARAME_H_
