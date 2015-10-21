#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

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
  int sd, status, i;
  char ether_frame[ETH_FRAME_ARP];
  struct ifaddrs *local_endpoint = (ptr);
  if ((sd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    return NULL;
  }

  while ( !_rec_exit ) {
    delete_frame_type(ether_frame);
    arp_hdr *arphdr_rec = alloc_arphdr(ether_frame);
    while ( ( !is_frame_arp(ether_frame) || !is_arp_reply(arphdr_rec) ) &&
            !_rec_exit ) {

//      struct pollfd fd;
//      int ret = 0;
//      fd.fd = sd;
//      fd.events = POLLIN;
//      while((ret = poll(&fd, 1, 1000)) == 0) {
//        if ( ret < 0 ) {
//          printf("poll error\n");
//          return NULL;
//        }
//        if (_rec_exit) return NULL;
//      }
      if (wait_data(sd) < 0) {
        fprintf(stderr, "wait socket data error\n");
        return NULL;
      }
      if ( _rec_exit ) return NULL;
      if ((status = recv (sd, ether_frame, ETH_FRAME_ARP, 0)) < 0) {
        if (errno == EINTR) {
          memset (ether_frame, 0, ETH_FRAME_ARP * sizeof (uint8_t));
          continue;
        } else {
          perror ("recv() failed: ");
          return NULL;
        }
      }
    }
    print_mac(local_endpoint->ifa_addr);
    if ( strncmp ((char*)ifaddr_mac(local_endpoint), (char*)ether_frame, 6) != 0 ) continue;

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

  return NULL;
}


void exit_receiver() {
  _rec_exit = 1;
}
