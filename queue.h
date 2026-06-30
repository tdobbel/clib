#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARENA_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#endif

#include "arena.h"

typedef uint64_t u64;
typedef uint8_t u8;
typedef u8 b8;

#ifndef BASE_QUEUE_CAPACITY
#define BASE_QUEUE_CAPACITY 64
#endif

typedef struct {
  u8 *data;
  u64 elem_size;
  u64 start, size, capacity;
  mem_arena *arena;
} queue;

typedef struct {
  u64 capacity, size;
  u64 start;
} queue_meta;

queue *queue_create(mem_arena *arena, u64 elem_size);
u8 *queue_push(queue *q);
u8 *queue_pop(queue *q);
void queue_free(queue *q);
void queue_empty(queue *q);
b8 queue_contains(queue *q, const void *x);

#define QUEUE_CREATE(T) queue_create(NULL, sizeof(T))
#define QUEUE_ARENA_CREATE(arena, T) queue_create(arena, sizeof(T))
#define QUEUE_POP(q, T) *(T *)queue_pop(q)
#define QUEUE_PUSH(q, T, x) (*(T *)queue_push(q) = (x))

#define HQUEUE_LEN(q) ((queue_meta *)(q) - 1)->size

#define HQUEUE_PUSH(q, x)                                                      \
  do {                                                                         \
    if ((q) == NULL) {                                                         \
      (q) = malloc(sizeof(*(q)) * BASE_QUEUE_CAPACITY + sizeof(queue_meta));   \
      queue_meta *meta = (queue_meta *)(q);                                    \
      meta->capacity = BASE_QUEUE_CAPACITY;                                    \
      meta->size = 0;                                                          \
      meta->start = 0;                                                         \
      (q) = (void *)(meta + 1);                                                \
    }                                                                          \
    queue_meta *meta = (queue_meta *)(q) - 1;                                  \
    if (meta->size == meta->capacity) {                                        \
      u64 old_cap = meta->capacity;                                            \
      u64 ncopy = (meta->start + meta->size) & (meta->capacity - 1);           \
      meta->capacity *= 2;                                                     \
      meta =                                                                   \
          realloc(meta, sizeof(*(q)) * meta->capacity + sizeof(queue_meta));   \
      (q) = (void *)(meta + 1);                                                \
      memcpy((q) + old_cap, (q), ncopy * sizeof(*(q)));                        \
    }                                                                          \
    (q)[(meta->start + meta->size++) & (meta->capacity - 1)] = (x);            \
  } while (0)

#define HQUEUE_POP(q, v)                                                       \
  do {                                                                         \
    queue_meta *meta = (queue_meta *)(q) - 1;                                  \
    if (meta->size > 0) {                                                      \
      *(v) = q[meta->start];                                                   \
      meta->size--;                                                            \
      meta->start = (meta->start + 1) & (meta->capacity - 1);                  \
    }                                                                          \
  } while (0)

#define HQUEUE_FREE(q) free((queue_meta *)(q) - 1)

#ifdef QUEUE_IMPLEMENTATION

queue *queue_create(mem_arena *arena, u64 elem_size) {
  queue *q = (queue *)arena_alloc(arena, sizeof(queue));
  q->data = (u8 *)arena_alloc(arena, BASE_QUEUE_CAPACITY * elem_size);
  q->elem_size = elem_size;
  q->size = 0;
  q->start = 0;
  q->capacity = BASE_QUEUE_CAPACITY;
  q->arena = arena;
  return q;
}

u8 *queue_pop(queue *q) {
  if (q->size == 0)
    return NULL;
  u8 *v = q->data + q->elem_size * q->start;
  q->start = (q->start + 1) & (q->capacity - 1);
  q->size--;
  return v;
}

u8 *queue_push(queue *q) {
  if (q->size == q->capacity) {
    u64 old_cap = q->capacity;
    q->capacity *= 2;
    q->data =
        (u8 *)arena_realloc(q->arena, q->data, q->capacity * q->elem_size);
    u64 ncopy = (q->start + q->size) & (old_cap - 1);
    memcpy(q->data + q->elem_size * old_cap, q->data, ncopy * q->elem_size);
  }
  u64 indx = (q->start + q->size) & (q->capacity - 1);
  q->size++;
  return q->data + indx * q->elem_size;
}

void queue_empty(queue *q) {
  q->size = 0;
  q->start = 0;
}

void queue_free(queue *q) {
  arena_dealloc(q->arena, q->data);
  arena_dealloc(q->arena, q);
}

b8 queue_contains(queue *q, const void *x) {
  u8 *needle = (u8 *)x;
  u64 cntr;
  for (u64 i = 0; i < q->size; i++) {
    u8 *elem = q->data + q->elem_size * ((q->start + i) & (q->capacity - 1));
    cntr = 0;
    for (u64 ibit = 0; ibit < q->elem_size && elem[ibit] == needle[ibit];
         ++ibit) {
      cntr++;
    }
    if (cntr == q->elem_size)
      return true;
  }
  return false;
}

#endif

#endif
