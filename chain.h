/*
 *    ARP ping
 *    Dan Demidow (dandemidow@gmail.com)
 */
#ifndef CHAIN_H
#define CHAIN_H
#include <pthread.h>

typedef struct chain {
  int ip;
  struct chain *next;
  struct chain *back;
} chain_t;

chain_t* chain_init(int count);
chain_t *chain_add(chain_t *last);
chain_t *chain_del_current(chain_t *curr);
void chain_swap(chain_t *curr);
void chain_del_number(int value);
void chain_free();
chain_t *chain_get();
void chain_next();
chain_t *get_find_chain();

#endif  // CHAIN_H
