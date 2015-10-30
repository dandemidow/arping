/*
 *    ARP ping
 *    Dan Demidow (dandemidow@gmail.com)
 */

#include "arp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

unsigned short csum(unsigned short *ptr, int nbytes) {
  register long sum;
  unsigned short oddbyte;
  register short answer;

  sum=0;
  while ( nbytes > 1 ) {
    sum += *ptr++;
    nbytes -= 2;
  }
  if ( nbytes == 1 ) {
    oddbyte=0;
    *((unsigned char*)&oddbyte)=*(unsigned char*)ptr;
    sum+=oddbyte;
  }

  sum = (sum>>16)+(sum & 0xffff);
  sum = sum + (sum>>16);
  answer=(unsigned short)~sum;

  return answer;
}


int is_arp_reply(arp_hdr *arp) {
  return ( ntohs (arp->opcode) == ARPOP_REPLY);
}
