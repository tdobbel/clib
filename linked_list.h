#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint8_t u8 ;
typedef uint64_t u64 ;
typedef struct node_struct ll_node;

struct node_struct {
  ll_node *next, *prev;
  u8 *data;
};

typedef struct {
  u64 elem_size, size;
  ll_node *head, *tail;
} linked_list;

linked_list *ll_init(u64 elem_size);
void ll_deinit(linked_list *ll);
void ll_push(linked_list *ll, void *item);
void *ll_pop(linked_list *ll);
void *ll_pop_first(linked_list *ll);

#define LL_CREATE(T) ll_init(sizeof(T))
#define LL_POP(ll, T) *(T *)ll_pop(ll)
#define LL_POP_FIRST(ll, T) *(T *)ll_pop_first(ll)

#ifdef LINKED_LIST_IMPLEMENTATION

linked_list *ll_init(u64 elem_size) {
  linked_list *ll = (linked_list *)malloc(sizeof(linked_list));
  ll->size = 0;
  ll->elem_size = elem_size;
  ll->head = NULL;
  ll->tail = NULL;
  return ll;
}

void ll_deinit(linked_list *ll) {
  ll_node *node = ll->head;
  ll_node *tmp;
  while (node) {
    tmp = node->next;
    free(node->data);
    free(node);
    node = tmp;
  }
  free(ll);
}

void *ll_pop(linked_list *ll) {
  ll_node *last = ll->tail;
  if (last == NULL)
    return NULL; // empty
  ll_node *penult = last->prev;
  ll->tail = penult;
  if (penult == NULL) { // no prev
    ll->head = NULL;
  } else {
    penult->next = NULL;
  }
  ll->size--;
  u8 *data = last->data;
  free(last);
  return data;
}

void *ll_pop_first(linked_list *ll) {
  ll_node *first = ll->head;
  if (first == NULL)
    return NULL;
  ll_node *second = first->next;
  ll->head = second;
  if (second == NULL)
    ll->tail = NULL;
  else
    second->prev = NULL;
  ll->size--;
  u8 *data = first->data;
  free(first);
  return data;
}

void ll_push(linked_list *ll, void *item) {
  ll_node *node = (ll_node *)malloc(sizeof(ll_node));
  node->data = (u8 *)malloc(ll->elem_size);
  memcpy(node->data, item, ll->elem_size);
  if (ll->tail == NULL) {
    node->prev = NULL;
    node->next = NULL;
    ll->head = node;
    ll->tail = node;
  } else {
    ll_node *tmp = ll->tail;
    node->prev = tmp;
    tmp->next = node;
    node->next = NULL;
    ll->tail = node;
  }
  ll->size++;
}

#endif
#endif
