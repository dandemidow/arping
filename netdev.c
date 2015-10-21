//
// Copyright (C) 2015, Danila Demidow
// Author: dandemidow@gmail.com (Danila Demidow)

#include "netdev.h"

#include <string.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */

void print_ip(struct sockaddr *addr)
{
  struct sockaddr_in *h = ((struct sockaddr_in*)addr);
  struct in_addr ad = h->sin_addr;

  printf("\t\taddress: %d -  <%s>\n", (int)(ad.s_addr),
         inet_ntoa(((struct sockaddr_in*)addr)->sin_addr));
}

void print_mac(struct sockaddr *addr)
{
  int i=0;
  struct sockaddr_ll *h = ((struct sockaddr_ll*)addr);
  printf("\t\tmac ");
  for (;i<6;i++) {
    printf("%02x", h->sll_addr[i]);
    if ( i!=5 ) printf(":");
  }
  printf("\n");
}

void init_target(struct ifaddrs *if_sender, char *target) {
  char *slash = strstr(target, "/");
  if_sender->ifa_addr = malloc(sizeof(struct sockaddr));
  if_sender->ifa_netmask = malloc(sizeof(struct sockaddr));
  struct sockaddr_in *ip_sender = (struct sockaddr_in *)if_sender->ifa_addr;
  struct sockaddr_in *mask_sender = (struct sockaddr_in *)if_sender->ifa_netmask;
  *slash = '\0';
  ip_sender->sin_addr.s_addr = inet_addr(target);
  memset(sockaddr_mac(ip_sender), 0xff, 6);
  *slash = '/';
  if (slash) {
    int maskbits = atoi(slash+1);
    if ( maskbits < 1 || maskbits > 30 ) {
      fprintf(stderr,"Invalid net mask bits (1-30): %d\n",maskbits);
      exit(1);
    }
    /* Create the netmask from the number of bits */
    int mask = 0, i;
    for ( i=0 ; i<maskbits ; i++ )
      mask |= 1<<(31-i);
    mask_sender->sin_addr.s_addr = htonl(mask);
  }
}

void free_target(struct ifaddrs *if_sender) {
  free(if_sender->ifa_netmask);
  free(if_sender->ifa_addr);
}


static int in_subnet(
    struct in_addr ip,
    struct ifaddrs *ifs) {
  if ((ip.s_addr &
       sockaddr_ip_addr(ifs->ifa_netmask).s_addr)
       ==
      (sockaddr_ip_addr(ifs->ifa_addr).s_addr &
       sockaddr_ip_addr(ifs->ifa_netmask).s_addr ))
    return 1;
  return 0;
}

static struct ifaddrs *find_mac_addr(struct ifaddrs *ifaddr, struct ifaddrs *pattern) {
  if ( !ifaddr ) return NULL;
  for (; ifaddr != NULL; ifaddr = ifaddr->ifa_next) {
    if ( ifaddr->ifa_addr->sa_family == AF_PACKET &&
         !strcmp(ifaddr->ifa_name, pattern->ifa_name))
      return ifaddr;
  }
  return NULL;
}

void init_local(char *ifname, struct ifaddrs *ifs, struct ifaddrs *local) {
  if ( !ifname ) {
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
      perror("getifaddrs");
      exit(EXIT_FAILURE);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
      if ( ifa->ifa_addr == NULL ) continue;
      if ( ifa->ifa_addr->sa_family == AF_INET ) {
        printf("check subnet %d ",
               in_subnet(sockaddr_ip_addr(ifa->ifa_addr), ifs));
        printf("%s - ", ifa->ifa_name);
        print_ip(ifa->ifa_addr);
        if ( in_subnet(sockaddr_ip_addr(ifa->ifa_addr), ifs) ) {
          memcpy(local, ifa, sizeof(struct ifaddr));
          local->ifa_addr = malloc(sizeof(struct sockaddr_ll));
          memcpy(local->ifa_addr, ifa->ifa_addr, sizeof(struct sockaddr));
          int ifa_name_len = strlen(ifa->ifa_name)+1;
          local->ifa_name = malloc(ifa_name_len);
          memcpy(local->ifa_name, ifa->ifa_name, ifa_name_len);
          local->ifa_dstaddr = malloc(sizeof(struct sockaddr_ll));
          memcpy(local->ifa_dstaddr, local->ifa_addr, sizeof(struct sockaddr));
          struct ifaddrs *mac_ifaddr = find_mac_addr(ifaddr, ifa);
          if ( mac_ifaddr )
            memcpy(ifaddr_mac(local), ifaddr_mac(mac_ifaddr), 6);
          ((struct sockaddr_ll*)local->ifa_dstaddr)->sll_ifindex = if_nametoindex(local->ifa_name);
          ((struct sockaddr_ll*)local->ifa_dstaddr)->sll_family = AF_PACKET;
          ((struct sockaddr_ll*)local->ifa_dstaddr)->sll_protocol = htons(ETH_P_ARP);
          break;
        }
      }
    }
    freeifaddrs(ifaddr);
  }
}

void free_local(struct ifaddrs* ifa) {
  free(ifa->ifa_addr);
  free(ifa->ifa_dstaddr);
}
