/*
 *    ARP ping
 *    Dan Demidow (dandemidow@gmail.com)
 */
#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>

#include <ifaddrs.h>

#include "arp.h"
#include "chain.h"
#include "netdev.h"
#include "frame.h"
#include "receiver.h"

int _trans_exit = 0;

extern void *receive_arp(void*);

static void sighandler(int signum) {
  fprintf(stderr, "Signal caught, exiting, %d!\n", signum);
  _trans_exit = 1;
  exit_receiver();
}
 
int main (int argc, char *argv[]) {
  struct sigaction sigact;
  int i, sd, bytes;
  arp_hdr arphdr;
  char ether_frame[ETH_FRAME_ARP];
  int c;
  char *ifacename = NULL;
  char *target;
  struct ifaddrs sender, local;
  pthread_t receiver;

  if (argc < 2) {
    printf("too few arguments %d\n", argc);
    // usage();
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

  init_frame((char*)ether_frame, &local, &sender);
  memcpy (ether_frame + ETH_HDRLEN, &arphdr, ARP_HDRLEN * sizeof (uint8_t));
  
  chain_init(254);

  if(pthread_create( &receiver, NULL, &receive_arp, &local)) {
    fprintf(stderr,"Error - pthread_create()\n");
    exit(EXIT_FAILURE);
  }

  if ((sd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }
  
  // send
  i = 0;
  while(i++ < 10*254 && !_trans_exit) {
    //printf("ip %d\n", chain_get()->ip);
    ether_frame[41] = chain_get()->ip;
    if ((bytes = sendto (sd, ether_frame, ETH_FRAME_ARP, 0,
                         local.ifa_dstaddr,
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
  exit_receiver();
  printf("wait receiver\n");
  pthread_join(receiver, NULL);
  printf("wait receiver done\n");
  close (sd);
  return (EXIT_SUCCESS);     
}
