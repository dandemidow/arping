#include "chain.h"

#include <stdlib.h>
#include <stdio.h>

void chain_init(thrash_t *tr, unsigned int first_addr, unsigned int last_addr) {
  unsigned int i;
  chain_t *previous = NULL;
  pthread_mutex_init(&tr->lock, NULL);
  pthread_mutex_lock(&tr->lock);
  tr->deleted = NULL;
  tr->main = NULL;
  tr->cycle = 0;
  for(i=first_addr; i<=last_addr; ++i) {
    chain_t *p = malloc(sizeof(chain_t));
    p->addr = i;
    if (previous) {
      p->back = previous;
      previous->next = p;
    }
    else tr->main = p;
    previous = p;
  }
  tr->main->back = previous;
  previous->next = tr->main;
  tr->current = tr->main;
  pthread_mutex_unlock(&tr->lock);
}

static void chain_add_to_thrash(thrash_t *tr, chain_t *ch) {
  if ( tr->deleted ) {
    tr->deleted->back->next = ch;
    ch->back = tr->deleted->back;
    ch->next = tr->deleted;
    tr->deleted->back = ch;
  } else {
    tr->deleted = ch;
    tr->deleted->next =
    tr->deleted->back = ch;
  }
}

static chain_t *exclude_from_main(thrash_t *tr, chain_t *ch) {
  ch->back->next = ch->next;
  ch->next->back = ch->back;
  if ( ch == tr->main ) {
    if ( ch == tr->current ) tr->current = NULL;
    tr->main = ch->next;
  }
  if ( ch == tr->current ) tr->current = ch->back;
  ch->next = ch->back = NULL;
  return ch;
}

static chain_t *find_value(thrash_t *tr, unsigned int value) {
  chain_t *s_first;
  chain_t *s_last = tr->main->back;
  for(s_first=tr->main; s_first <= s_last; s_first=s_first->next) {
    if ( s_first->addr == value ) return s_first;
  }
  return NULL;
}

chain_t *chain_del_value(thrash_t *tr, unsigned int value) {
  pthread_mutex_lock(&tr->lock);
  chain_t *p = find_value(tr, value);
  exclude_from_main(tr, p);
  chain_add_to_thrash(tr, p);
  pthread_mutex_unlock(&tr->lock);
  return p;
}

static void free_chain(chain_t *ch) {
  ch->back->next = NULL;
  while ( ch->next != NULL ) {
    chain_t *ptr = ch;
    ch = ch->next;
    free(ptr);
  }
  free(ch);
}

void chain_free(thrash_t *tr) {
  pthread_mutex_lock(&tr->lock);
  free_chain(tr->main);
  free_chain(tr->deleted);
  tr->current = NULL;
  pthread_mutex_unlock(&tr->lock);
}

unsigned int chain_current(thrash_t *tr) {
  return tr->current ? tr->current->addr : tr->main->addr;
}

void chain_next(thrash_t *tr) {
  pthread_mutex_lock(&tr->lock);
  if ( tr->current ) {
    if ( tr->current->next == tr->main )
      ++tr->cycle;
    tr->current = tr->current->next;
  } else {
    tr->current = tr->main;
  }
  pthread_mutex_unlock(&tr->lock);
}
