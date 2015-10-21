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

int init_device(net_dev_t *dev) {
  bzero(&dev->device, sizeof (struct sockaddr_ll));
  if ((dev->device.sll_ifindex = if_nametoindex (dev->interface)) == 0) {
    perror ("if_nametoindex() failed to obtain interface index ");
    return -1;
  }
  //printf ("Index for interface %s is %i\n", interface, device->sll_ifindex);
  // Fill out sockaddr_ll.
  dev->device.sll_family = AF_PACKET;
  //memcpy (device->sll_addr, localEndpoint.mac, sizeof (localEndpoint.mac));
  //device->sll_halen = 6;
  dev->device.sll_protocol=htons(ETH_P_ARP);
  return 0;
}




void find_all_interfaces()
{
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  /* Walk through linked list, maintaining head pointer so we
                can free list later */

  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    /* Display interface name and family (including symbolic
                    form of the latter for the common families) */

    {
      int i;
      printf("\n");
      for(i = 0; i < 6; i++)
        printf("%02X:", ifaddr_mac(ifa)[i]);
      printf("\n");
      //len+=sprintf(macp+len,"%02X%s",s->sll_addr[i],i < 5 ? ":":"");

    }

    printf("%-8s %s (%d)\n",
           ifa->ifa_name,
           (family == AF_PACKET) ? "AF_PACKET" :
                                   (family == AF_INET) ? "AF_INET" :
                                                         (family == AF_INET6) ? "AF_INET6" : "???",
           family);

    /* For an AF_INET* interface address, display the address */

    if (family == AF_INET || family == AF_INET6) {
      s = getnameinfo(ifa->ifa_addr,
                      (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                            sizeof(struct sockaddr_in6),
                      host, NI_MAXHOST,
                      NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }

      printf("\t\taddress: <%s>\n", host);

    } else if (family == AF_PACKET && ifa->ifa_data != NULL) {
      struct rtnl_link_stats *stats = ifa->ifa_data;

      printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
             "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
             stats->tx_packets, stats->rx_packets,
             stats->tx_bytes, stats->rx_bytes);
    }
  }

  freeifaddrs(ifaddr);
}


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
          local->ifa_addr = malloc(sizeof(struct sockaddr));
          memcpy(local->ifa_addr, ifa->ifa_addr, sizeof(struct sockaddr));
          struct ifaddrs *mac_ifaddr = find_mac_addr(ifaddr, ifa);
          if ( mac_ifaddr )
            memcpy(ifaddr_mac(local), ifaddr_mac(mac_ifaddr), 6);
          break;
        }
      }
    }
    freeifaddrs(ifaddr);
  }
}

void free_local(struct ifaddrs* ifa) {
  free(ifa->ifa_addr);
}
