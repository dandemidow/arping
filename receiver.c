#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <ifaddrs.h>

#include "frame.h"
#include "netdev.h"
#include "chain.h"

static int _rec_exit = 0;

static int wait_data(int sd) {
  struct pollfd fd;
  int ret = 0;
  fd.fd = sd;
  fd.events = POLLIN;
  while((ret = poll(&fd, 1, 1000)) == 0 && !_rec_exit) {
    if ( ret < 0 ) {
      printf("poll error\n");
      return -1;
    }
  }
  return 0;
}

void *receive_arp( void *ptr ) {
  int sd, status;
  char ether_frame[ETH_FRAME_ARP];
  struct ifaddrs *local_endpoint = *((struct ifaddrs **)(ptr));
  thrash_t *addrs = *((thrash_t **)(ptr)+1);
  if ((sd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    return NULL;
  }

  if ( !quiet ) printf("IP-address\tMAC-address\n");

  while ( !_rec_exit ) {
    delete_frame_type(ether_frame);
    arp_hdr *arphdr_rec = alloc_arphdr(ether_frame);
    while ( ( !is_frame_arp(ether_frame) ||
              !is_arp_reply(arphdr_rec) ) &&
            !_rec_exit ) {
      if (wait_data(sd) < 0) {
        fprintf(stderr, "wait socket data error\n");
        return NULL;
      } else if ( _rec_exit ) return NULL;
      if ((status = recv (sd, ether_frame, ETH_FRAME_ARP, 0)) < 0) {
        if (errno == EINTR) {
          memset (ether_frame, 0, ETH_FRAME_ARP);
          continue;
        } else {
          perror ("recv() failed: ");
          return NULL;
        }
      }
    }
    if ( !is_source_mac(ether_frame, (char*)ifaddr_mac(local_endpoint)) ) continue;

    chain_del_value(addrs, ntohl(arphdr_rec->sender_ip));

    if ( !quiet ) {
      printf ("%s\t[", inet_ntoa(int_in_addr(arphdr_rec->sender_ip)));
      print_macstr(arphdr_rec->sender_mac);
      printf("]\n");

//      printf ("Target:\t%s\t", inet_ntoa(int_in_addr(arphdr_rec->target_ip)));
//      print_macstr(arphdr_rec->target_mac);
//      printf("\n");
    }
  }

  return NULL;
}


void exit_receiver() {
  _rec_exit = 1;
}
