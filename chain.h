/*
 *    ARP ping
 *    Dan Demidow (dandemidow@gmail.com)
 */
#ifndef CHAIN_H
#define CHAIN_H
#include <pthread.h>

typedef struct chain {
  unsigned int addr;
  struct chain *next;
  struct chain *back;
} chain_t;

typedef struct {
  chain_t *main;
  chain_t *deleted;
  chain_t *current;
  pthread_mutex_t lock;
  size_t cycle;
} thrash_t;

void chain_init(thrash_t *tr, unsigned int first_addr, unsigned int last_addr);
chain_t *chain_del_value(thrash_t *tr, unsigned int value);
void chain_free(thrash_t *tr);
unsigned int chain_current(thrash_t *tr);
void chain_next(thrash_t *tr);

#endif  // CHAIN_H
