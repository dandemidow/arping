/*
 *    ARP ping
 *    Dan Demidow (dandemidow@gmail.com)
 */
#ifndef _ARP_H_
#define _ARP_H_

#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <sys/socket.h>

#define MAXLEN 8192

#define IP4_HDRLEN 20      // IPv4 header length
#define ARP_HDRLEN 28      // ARP header length
#define ARPOP_REQUEST 1    // Taken from <linux/if_arp.h>
#define ARPOP_REPLY 2

#pragma pack(push, 1)

typedef struct _arp_hdr {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[ETHER_ADDR_LEN];
  unsigned int sender_ip;
  uint8_t target_mac[ETHER_ADDR_LEN];
  unsigned int target_ip;
}  arp_hdr;

#pragma pack(pop)

unsigned short csum(unsigned short *ptr,int nbytes);

int is_arp_reply(arp_hdr *);

#endif // _ARP_H_
