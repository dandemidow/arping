#include "frame.h"

#include <net/ethernet.h>
#include <string.h>

#include "netdev.h"

void init_frame(char *frame,
                struct ifaddrs *local,
                struct ifaddrs *remote) {
  // frame length about ETH_FRAME_ARP
  memcpy (frame, ifaddr_mac(remote), ETHER_ADDR_LEN);
  memcpy (frame + ETHER_ADDR_LEN, ifaddr_mac(local), ETHER_ADDR_LEN);
  frame[12] = ETH_P_ARP / 256;
  frame[13] = ETH_P_ARP % 256;
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
