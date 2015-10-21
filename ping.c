/*
 *    ARP ping
 *    Dan Demidow (dandemidow@gmail.com)
 */
#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>

#include <net/if.h>
#include <ifaddrs.h>

#include "arp.h"
#include "chain.h"
#include "netdev.h"

int global_exit = 0;

static void sighandler(int signum) {
  fprintf(stderr, "Signal caught, exiting, %d!\n", signum);
  global_exit = 1;
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

void *receive_arp( void *ptr ) {
  int sd, status, i;
  uint8_t ether_frame[IP_MAXPACKET];
  struct ifaddrs *local_endpoint = (ptr);
  if ((sd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    return NULL;
  }
//  if(fcntl(sd, F_SETFL, fcntl(sd, F_GETFL) | O_NONBLOCK) < 0) {
//    perror ("fcntl() failed ");
//    return NULL;
//  }

  while(1) {
  ether_frame[12] = 0x00;
  ether_frame[13] = 0x00;
  arp_hdr * arphdr_rec;
  arphdr_rec = (arp_hdr *) (ether_frame + ETH_HDRLEN);
  while (((((ether_frame[12]) << 8) + ether_frame[13]) != ETH_P_ARP) ||
         (ntohs (arphdr_rec->opcode) != ARPOP_REPLY)) {

    struct pollfd fd;
    int ret = 0;

    fd.fd = sd;//your socket handler
    fd.events = POLLIN;
    while(ret==0) {
    ret = poll(&fd, 1, 1000); // 1 second for timeout
    switch (ret){
        case -1:
            printf("poll error\n");
            return NULL;
        break;
        case 0:
            printf("timeout\n");
        break;
        default:
        break;
    }
    if (global_exit) return NULL;
    if ( ret != 0 ) break;
    }

    if ((status = recv (sd, ether_frame, IP_MAXPACKET, 0)) < 0) {
      if (errno == EINTR) {
        memset (ether_frame, 0, IP_MAXPACKET * sizeof (uint8_t));
        continue;  // Something weird happened, but let's try again.
      } else {
        perror ("recv() failed: ");
        exit (EXIT_FAILURE);
      }
    }
  }
  printf("receive smth \n");
  print_mac(local_endpoint->ifa_addr);
  if ( strncmp (ifaddr_mac(local_endpoint), ether_frame, 6) != 0 ) continue;

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
  struct sigaction sigact;
  int i, frame_length, sd, bytes;
  arp_hdr arphdr;
  uint8_t ether_frame[IP_MAXPACKET];
  int c;
  char *ifacename = NULL;
  char *target;
  struct ifaddrs sender, local;
  pthread_t thread1;
  int  iret1;

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
      break;
    case 'r':
      //atoi(optarg);
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
  print_mac(local.ifa_addr);

  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);

  // ARP header
  arphdr.htype = htons (1);
  arphdr.ptype = htons (ETH_P_IP);
  arphdr.hlen = 6;
  arphdr.plen = 4;
  arphdr.opcode = htons (ARPOP_REQUEST);
  
  memcpy (&arphdr.sender_ip, &ifaddr_ip_addr(&local), sizeof(arphdr.sender_ip));
  memcpy (&arphdr.sender_mac, ifaddr_mac(&local), sizeof (arphdr.sender_mac));
  bzero (&arphdr.target_mac, sizeof (arphdr.target_mac));
  memcpy (&arphdr.target_ip, &ifaddr_ip_addr(&sender), sizeof (arphdr.target_ip));

  frame_length = ETH_HDRLEN + ARP_HDRLEN;

  init_ethernet_frame((unsigned char*)ether_frame, &local, &sender);
  memcpy (ether_frame + ETH_HDRLEN, &arphdr, ARP_HDRLEN * sizeof (uint8_t));
  
  chain_init(254);

  iret1 = pthread_create( &thread1, NULL, &receive_arp, &local);
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
  while(i++ < 10*254 && !global_exit) {
    //printf("ip %d\n", chain_get()->ip);
    ether_frame[41] = chain_get()->ip;
    if ((bytes = sendto (sd, ether_frame, frame_length, 0,
                         local.ifa_addr,
                         sizeof (struct sockaddr_ll))) <= 0) {
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
  free_local(&local);
  free_target(&sender);
  global_exit = 1;
  // receive
  pthread_join(thread1, NULL);
  close (sd);
  return (EXIT_SUCCESS);     
}
