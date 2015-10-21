#include "chain.h"

#include <stdlib.h>

static chain_t *main_chain;
static chain_t *find_chain;
static pthread_mutex_t lock;

chain_t *chain_init(int count) {
  int i;
  chain_t *first, *p;
  find_chain = NULL;
  first = malloc(sizeof(chain_t));
  first->ip = 1;
  p = first;
  for(i=0; i<count-1; ++i) {
    p->next = malloc(sizeof(chain_t));
    p->next->back = p;
    p = p->next;
    p->ip = p->back->ip+1;
  }
  p->next = first;
  first->back = p;
  main_chain = first;
  return first;
}


chain_t *chain_add(chain_t *last) {
  pthread_mutex_lock(&lock);
  if ( find_chain == NULL ) {
    find_chain = last;
    find_chain->next = NULL;
    find_chain->back = NULL;
  } else {
    if ( find_chain->next == NULL && find_chain->back == NULL ) {
      find_chain->next = last;
      find_chain->back = last;
      last->next = find_chain;
      last->back = find_chain;
    } else {
      chain_t *p = find_chain->next;
      find_chain->next = last;
      last->back = find_chain;
      p->back = last;
      last->next = p;
    }
  }
  pthread_mutex_unlock(&lock);
  return find_chain;
}


chain_t *chain_del_current(chain_t *curr) {
  pthread_mutex_lock(&lock);
  chain_t *p = curr->next;
  curr->back->next = curr->next;
  curr->next->back = curr->back;
  if ( main_chain == curr ) main_chain = p;
  pthread_mutex_unlock(&lock);
  return p;
}


void chain_swap(chain_t *curr) {
  chain_del_current(curr);
  chain_add(curr);
}


void chain_del_number(int value) {
  chain_t *last = main_chain;
  chain_t *first = main_chain;
  int i = 0;
  do {
    i++;
    if ( first->ip == value ) {
      chain_swap(first);
      return;
    }
    first = first->next;
  } while ( first != last );
}


void chain_free() {
  main_chain->back->next = NULL;
  chain_t *p = main_chain;
  while ( p->next == NULL ) {
    chain_t *ptr = p;
    p = p->next;
    free(ptr);
  }
  free(p);
}


chain_t *chain_get() {
  return main_chain;
}


void chain_next() {
  pthread_mutex_lock(&lock);
  main_chain = main_chain->next;
  pthread_mutex_unlock(&lock);
}


chain_t *get_find_chain() {
  return find_chain;
}
