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

static int _trans_exit = 0;

extern void *receive_arp(void*);

static void usage() {
  printf("arping start_ip/netmast [-r cycles]\n");
}

static void sighandler(int signum) {
  fprintf(stderr, "Signal caught, exiting, %d!\n", signum);
  _trans_exit = 1;
  exit_receiver();
}
 
int main (int argc, char *argv[]) {
  struct sigaction sigact;
  int sd;
  char ether_frame[ETH_FRAME_ARP];
  int c;
  size_t cycles = 1;
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
      cycles = atoi(optarg);
      break;
    case 'i':
        ifacename = optarg;
        break;

    default: break;
      usage();
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

  init_frame((char*)ether_frame, &local, &sender);

  struct in_addr ia_last;
  ia_last.s_addr = htonl((ntohl(ifaddr_ip_addr(&sender).s_addr) |
                    (~ntohl(ifaddr_netmask(&sender).s_addr)))-1);

//  printf("first %s\n", inet_ntoa(ia_last));
  thrash_t addr_container;
  
  chain_init(&addr_container, ntohl(ifaddr_ip_addr(&sender).s_addr), ntohl(ia_last.s_addr));

//  while(addr_container.cycle == 0) {
//    struct in_addr ia;
//    ia.s_addr = htonl(chain_current(&addr_container));
//    printf("first %s\n", inet_ntoa(ia));
//    chain_next(&addr_container);
//  }

  void *args[2] = { &local, &addr_container };

  if(pthread_create( &receiver, NULL, &receive_arp, args)) {
    fprintf(stderr,"Error - pthread_create()\n");
    exit(EXIT_FAILURE);
  }

  if ((sd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }
  
  while(addr_container.cycle < cycles &&
        !_trans_exit) {
    *((unsigned int*)alloc_arphdr(ether_frame)->target_ip) =
        htonl(chain_current(&addr_container));
    if (sendto(sd, ether_frame, ETH_FRAME_ARP, 0,
                         local.ifa_dstaddr,
                         sizeof (struct sockaddr_ll)) <= 0) {
      perror("sendto() failed");
      exit(EXIT_FAILURE);
    }
    usleep(10000+rand()%50000);
    chain_next(&addr_container);
  }
  
  chain_t *find = addr_container.deleted;
  find->back->next = NULL;
  
  struct in_addr fa;
  while ( find->next != NULL ) {
    fa.s_addr = htonl(find->addr);
    printf("find %s\n", inet_ntoa(fa) );
    find = find->next;
  }
  fa.s_addr = htonl(find->addr);
  printf("find %s\n", inet_ntoa(fa));
  
  chain_free(&addr_container);
  free_local(&local);
  free_target(&sender);
  exit_receiver();
  printf("wait receiver\n");
  pthread_join(receiver, NULL);
  printf("wait receiver done\n");
  close (sd);
  return (EXIT_SUCCESS);     
}
