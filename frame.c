#include "frame.h"

#include <net/ethernet.h>
#include <string.h>

#include "netdev.h"

void init_frame(char *frame,
                struct ifaddrs *local,
                struct ifaddrs *remote) {
  // frame length about ETH_FRAME_ARP
  arp_hdr *arphdr = alloc_arphdr(frame);
  memcpy (frame, ifaddr_mac(remote), ETHER_ADDR_LEN);
  memcpy (frame + ETHER_ADDR_LEN, ifaddr_mac(local), ETHER_ADDR_LEN);
  frame[12] = ETH_P_ARP / 256;
  frame[13] = ETH_P_ARP % 256;

  arphdr->htype = htons (1);
  arphdr->ptype = htons (ETH_P_IP);
  arphdr->hlen = 6;
  arphdr->plen = 4;
  arphdr->opcode = htons (ARPOP_REQUEST);

  memcpy(arphdr->sender_mac, ifaddr_mac(local), ETHER_ADDR_LEN);
  arphdr->sender_ip = ifaddr_ip_addr(local).s_addr;
  bzero (arphdr->target_mac, ETHER_ADDR_LEN);
  arphdr->target_ip = ifaddr_ip_addr(remote).s_addr;
}


arp_hdr *alloc_arphdr(char *frame) {
  return (arp_hdr *) (frame + ETH_HDRLEN);
}

void delete_frame_type(char *frame) {
  frame[12] = 0x00;
  frame[13] = 0x00;
}

int is_frame_arp(char *frame) {
  return ((((frame[12]) << 8) + frame[13]) == ETH_P_ARP);
}

int is_source_mac(char *frame, char *mac) {
  return (strncmp(mac, frame, ETHER_ADDR_LEN) == 0);
}


struct in_addr int_in_addr(unsigned int num) {
  struct in_addr fa;
  fa.s_addr = num;
  return fa;
}
