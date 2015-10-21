#ifndef _RECEIVER_H_
#define _RECEIVER_H_

void exit_receiver();

// thread function
void *receive_arp( void *ptr );

#endif  // _RECEIVER_H_
