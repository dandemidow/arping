//
// Copyright (C) 2015, Danila Demidow
// Author: dandemidow@gmail.com (Danila Demidow)

#ifndef NETDEV_H
#define NETDEV_H

#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <sys/socket.h>

#define sockaddr_mac(addr) ((struct sockaddr_ll*)(addr))->sll_addr
#define sockaddr_ip_addr(addr) ((struct sockaddr_in*)(addr))->sin_addr

#define ifaddr_mac(ifa) sockaddr_mac((ifa)->ifa_addr)
#define ifaddr_ip_addr(ifa) sockaddr_ip_addr((ifa)->ifa_addr)
#define ifaddr_netmask(ifa) sockaddr_ip_addr((ifa)->ifa_netmask)

extern int verbose;
extern int quiet;

void print_ip(struct sockaddr *);
void print_mac(struct sockaddr *);
void print_macstr(unsigned char *);

void init_target(struct ifaddrs *if_sender, char *target);
void free_target(struct ifaddrs *if_sender);

int init_local(char *ifname, struct ifaddrs *ifs, struct ifaddrs *local);
void free_local(struct ifaddrs*);

#endif // NETDEV_H
