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
    *((u_char*)&oddbyte)=*(u_char*)ptr;
    sum+=oddbyte;
  }

  sum = (sum>>16)+(sum & 0xffff);
  sum = sum + (sum>>16);
  answer=(unsigned short)~sum;

  return(answer);
}


struct endpoint get_endpoint(char *iface) {
  int sd;
  struct endpoint epnt;
  struct ifreq ifr;

  if ((sd = socket (AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
    perror ("socket() failed to get socket descriptor for using ioctl() ");
    exit (EXIT_FAILURE);
  }
  // Use ioctl() to look up interface name and get its MAC address.
  bzero(&ifr, sizeof (ifr));
  snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", iface);
  if (ioctl (sd, SIOCGIFHWADDR, &ifr) < 0) {
    perror ("ioctl() failed to get source MAC address ");
    exit(EXIT_FAILURE);
  }
  memcpy (epnt.mac, ifr.ifr_hwaddr.sa_data, 6 * sizeof (uint8_t));
  if (ioctl(sd, SIOCGIFADDR, &ifr)==-1) {
    perror ("ioctl() failed to get source MAC address ");
    exit (EXIT_FAILURE);
  }
  close(sd);
  struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
  memcpy (epnt.ip, inet_ntoa(ipaddr->sin_addr), sizeof (epnt.ip));
  return epnt;
}
