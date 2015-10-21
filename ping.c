/*
 *    ARP ping
 *    Dan Demidow (dandemidow@gmail.com)
 */
#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>

#include <net/if.h>
#include <ifaddrs.h>

#include "arp.h"
#include "chain.h"
#include "netdev.h"

struct endpoint get_remote(char *target) {
    // Fill out hints for getaddrinfo().
  int status;
  struct endpoint epnt;
  struct addrinfo hints, *res;
  struct sockaddr_in *ipv4;
  
  bzero (&hints, sizeof (struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = hints.ai_flags | AI_CANONNAME;

  // Resolve target using getaddrinfo().
  if ((status = getaddrinfo (target, NULL, &hints, &res)) != 0) {
    fprintf (stderr, "getaddrinfo() failed: %s\n", gai_strerror (status));
    exit (EXIT_FAILURE);
  }
  ipv4 = (struct sockaddr_in *) res->ai_addr;
  memcpy (epnt.ip, &ipv4->sin_addr, sizeof (epnt.ip));
  freeaddrinfo (res);
  
  memset (epnt.mac, 0xff, sizeof (epnt.mac));
  return epnt;
}
 
void init_ethernet_frame(char *frame,
                         struct ifaddrs *local,
                         struct ifaddrs *remote) {
  // Destination and Source MAC addresses
  int mac_len = 6; // fixme magic numbers
  memcpy (frame, ifaddr_mac(remote), mac_len);
  memcpy (frame + mac_len, ifaddr_mac(local), mac_len);
  frame[12] = ETH_P_ARP / 256;
  frame[13] = ETH_P_ARP % 256;
}

struct find_ip {
  unsigned char addr[255];
  int count;
} ips;

struct arg {
  struct endpoint *ep;
  chain_t *first;
  chain_t *last;
};
 
void *receive_arp( void *ptr ) {
  int sd, status, i;
  uint8_t ether_frame[IP_MAXPACKET];
  struct arg *a = ptr;
  struct endpoint *local_endpoint = a->ep;
  if ((sd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }
  while(1) {
  ether_frame[12] = 0x00;
  ether_frame[13] = 0x00;
  arp_hdr * arphdr_rec;
  arphdr_rec = (arp_hdr *) (ether_frame + ETH_HDRLEN);
  while (((((ether_frame[12]) << 8) + ether_frame[13]) != ETH_P_ARP) || (ntohs (arphdr_rec->opcode) != ARPOP_REPLY)) {
    if ((status = recv (sd, ether_frame, IP_MAXPACKET, 0)) < 0) {
      if (errno == EINTR) {
        memset (ether_frame, 0, IP_MAXPACKET * sizeof (uint8_t));
        continue;  // Something weird happened, but let's try again.
      } else {
        perror ("recv() failed:");
        exit (EXIT_FAILURE);
      }
    }
  }
  if ( strncmp (local_endpoint->mac, ether_frame, 6) != 0 ) continue;

  chain_del_number(arphdr_rec->sender_ip[3]);
  
  printf ("\n");
  for (i=0; i<5; i++) 
    printf ("%02x:", ether_frame[i]);
  printf ("%02x", ether_frame[5]);
  printf (" <- ");
  for (i=0; i<5; i++)
    printf ("%02x:", ether_frame[i+6]);
  printf ("%02x\n", ether_frame[11]);
  
  printf ("Sender:\t%u.%u.%u.%u\t",
    arphdr_rec->sender_ip[0], arphdr_rec->sender_ip[1], arphdr_rec->sender_ip[2], arphdr_rec->sender_ip[3]);
  for (i=0; i<5; i++)
    printf ("%02x:", arphdr_rec->sender_mac[i]);
  printf ("%02x\n", arphdr_rec->sender_mac[5]);
  
  printf ("Target:\t");
  printf ("%u.%u.%u.%u\t",
    arphdr_rec->target_ip[0], arphdr_rec->target_ip[1], arphdr_rec->target_ip[2], arphdr_rec->target_ip[3]);
  for (i=0; i<5; i++)
    printf ("%02x:", arphdr_rec->target_mac[i]);
  printf ("%02x\n", arphdr_rec->target_mac[5]);
  }
}
 
int main (int argc, char *argv[]) {
  int i, j, status, frame_length, sd, bytes;
  arp_hdr arphdr;
  uint8_t ether_frame[IP_MAXPACKET];
  net_dev_t dev;
  struct endpoint localEndpoint, remoteEndpoint;
  int c;
  int verbose = 0;
  char *ifacename = NULL;
  char *target;
  struct ifaddrs sender, local;

  if (argc < 2) {
    printf("too few arguments %d\n", argc);
    // usage;
    exit(EXIT_FAILURE);
  }

  target = argv[1];
  init_target(&sender, target);

  printf("target: %s\n", target);
  print_ip(sender.ifa_netmask);

  while ((c = getopt(argc, argv, "vr:i:")) != EOF) {
    switch (c) {
    case 'v':
      verbose = 1;
      break;
    case 'r':
      atoi(optarg);
      break;
    case 'i':
        ifacename = optarg;
        break;

    default: break;
      //usage();
    }
  }

  printf("iface name %s\n", ifacename);

  init_local(ifacename, &sender, &local);

  printf("%s ", local.ifa_name);
  print_ip(local.ifa_addr);
  
  ips.count = 0;
  
  strcpy (dev.interface, "wlan0");
  
  init_device(&dev);
  localEndpoint = get_endpoint(dev.interface);
  remoteEndpoint = get_remote(target);
 
  /*for (i=0; i<5; i++) {
    printf ("%02x:", localEndpoint.mac[i]);
  }
  printf ("%02x\n", localEndpoint.mac[5]);
  printf("IP address: %s\n", localEndpoint.ip);*/

  // ARP header
  arphdr.htype = htons (1);
  arphdr.ptype = htons (ETH_P_IP);
  arphdr.hlen = 6;
  arphdr.plen = 4;
  arphdr.opcode = htons (ARPOP_REQUEST);
  
  // Source IP address
  if ((status = inet_pton (AF_INET, localEndpoint.ip, &arphdr.sender_ip)) != 1) {
    fprintf (stderr, "inet_pton() failed for source IP address.\nError message: %s", strerror (status));
    exit (EXIT_FAILURE);
  }
  memcpy (&arphdr.sender_mac, localEndpoint.mac, sizeof (localEndpoint.mac));
  bzero (&arphdr.target_mac, sizeof (arphdr.target_mac));
  memcpy (&arphdr.target_ip, remoteEndpoint.ip, sizeof (remoteEndpoint.ip));

  frame_length = ETH_HDRLEN + ARP_HDRLEN;

  init_ethernet_frame(ether_frame, &local, &sender);
  memcpy (ether_frame + ETH_HDRLEN, &arphdr, ARP_HDRLEN * sizeof (uint8_t));
  
  pthread_t thread1;
  int  iret1;
  
  chain_init(254);
  
  struct arg a;
  a.ep = &localEndpoint;
  a.first = NULL;
  a.last = NULL;
  
  iret1 = pthread_create( &thread1, NULL, receive_arp, (void*)&a);
  if(iret1) {
    fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
    exit(EXIT_FAILURE);
  }

  if ((sd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }
  
  // send
  i = 0;
  while(i++ < 10*254) {
    //printf("ip %d\n", chain_get()->ip);
    ether_frame[41] = chain_get()->ip;
    if ((bytes = sendto (sd, ether_frame, frame_length, 0, (struct sockaddr *) &dev.device, sizeof (dev.device))) <= 0) {
      perror ("sendto() failed");
      exit (EXIT_FAILURE);
    }
    usleep(rand()%50000);
    chain_next();
  }
  
  usleep(1000000);
  
  chain_t *find = get_find_chain();
  find->back->next = NULL;
  
  while ( find->next != NULL ) {
    printf("find %d\n", find->ip);
    find = find->next;
  }
  printf("find %d\n", find->ip);
  
  chain_free();
  exit(1);
  // receive
  pthread_join(thread1, NULL);
  close (sd);
  return (EXIT_SUCCESS);     
}
