#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <assert.h>
#include <stdbool.h>
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

// Vector

typedef struct {
  u64 size, capacity;
} vec_meta;

typedef struct {
  u64 item_size, capacity, size;
  u8 *items;
  mem_arena *arena;
} vector;

typedef b8 (*__vec_eq_fn_t)(const void *, const void *);

vector *vector_create(mem_arena *arena, u64 item_size);
void vector_deinit(vector *vec);

void vector_get(vector *vec, void *item, u64 index);
void *vector_get_ref_unsafe(vector *vec, u64 index);
void *vector_get_ref(vector *vec, u64 index);
void vector_get_last(vector *vec, void *item);
void vector_set_unsafe(vector *self, void *item, u64 index);
void vector_set(vector *self, void *item, u64 index);

void vector_grow(vector *vec);
void vector_push(vector *vec, void *item);
void vector_extend(vector *vec, vector *extra_vec);

void vector_remove(vector *vec, u64 index);
b8 vector_pop(vector *vec, void *item);

u64 vector_index(vector *vec, const void *needle, __vec_eq_fn_t eq);
b8 vector_contains(vector *vec, const void *needle, __vec_eq_fn_t eq);

void vector_sort(vector *vec, __compar_fn_t cmp_fn);

#ifdef STRING_IMPLEMENTATION
void str_split_whitespace(vector *vec, string8 s);
void str_split(vector *vec, string8 base, string8 sep);
#endif

#ifndef BASE_VEC_CAPACITY
#define BASE_VEC_CAPACITY 64
#endif

#define VEC_CREATE(T) vector_create(NULL, sizeof(T))
#define VEC_ARENA_CREATE(a, T) vector_create(a, sizeof(T))
#define VEC_CAST(v, T) ((T *)(v)->items)

#define HVEC_LEN(vec) ((vec_meta *)(vec) - 1)->size

#define HVEC_PUSH(vec, x)                                                      \
  do {                                                                         \
    if ((vec) == NULL) {                                                       \
      (vec) = malloc(sizeof(*(vec)) * BASE_VEC_CAPACITY + sizeof(vec_meta));   \
      vec_meta *meta = (vec_meta *)(vec);                                      \
      meta->capacity = BASE_VEC_CAPACITY;                                      \
      meta->size = 0;                                                          \
      (vec) = (void *)(meta + 1);                                              \
    }                                                                          \
    vec_meta *meta = (vec_meta *)(vec) - 1;                                    \
    if (meta->size >= meta->capacity) {                                        \
      meta->capacity *= 2;                                                     \
      meta =                                                                   \
          realloc(meta, sizeof(*(vec)) * meta->capacity + sizeof(vec_meta));   \
      (vec) = (void *)(meta + 1);                                              \
    }                                                                          \
    (vec)[meta->size++] = (x);                                                 \
  } while (0)

#define HVEC_FREE(vec) free((vec_meta *)(vec) - 1)

// Queue

typedef struct {
  vector *items;
  u64 start;
} Queue;

Queue *queue_init(u64 item_size);
void queue_deinit(Queue *q);

u64 queue_len(Queue *self);
void queue_push(Queue *self, void *item);
b8 queue_pop(Queue *self, void *item);

#define QUEUE_CREATE(T) queue_init(sizeof(T))

// Priority queue

typedef enum { lt, gt, eq } Order;
typedef Order (*compareFn)(const void *, const void *, const void *);

typedef struct {
  void *ctx;
  compareFn compare_fn;
  vector *items;
} PriorityQueue;

void __pq_sift_up(PriorityQueue *self, u64 start_index);
void __pq_sift_down(PriorityQueue *self, u64 start_index);

PriorityQueue *priority_queue_init(u64 item_size, void *ctx,
                                   compareFn compare_fn);
void priority_queue_deinit(PriorityQueue *self);

u64 priority_queue_len(PriorityQueue *self);
void priority_queue_push(PriorityQueue *self, void *item);
void *priority_queue_peek(const PriorityQueue *self);
void priority_queue_pop_index(PriorityQueue *self, void *dst, u64 index);
b8 priority_queue_pop(PriorityQueue *self, void *dst);

#define PRIORITY_QUEUE_CREATE(T, ctx, compareFn)                               \
  priority_queue_init(sizeof(T), ctx, compareFn)

// Linked list

typedef struct ll_node Node;

struct ll_node {
  u8 *item;
  Node *previous, *next;
};

typedef struct {
  u64 item_size, size;
  Node *head, *tail;
} LinkedList;

LinkedList *ll_init(u64 item_size);
void ll_deinit(LinkedList *self);

Node *__ll_get_node(LinkedList *self, u64 index);
Node *__ll_node_init(LinkedList *self, const void *item);

void ll_push(LinkedList *self, const void *item);
void ll_push_first(LinkedList *self, const void *item);
void ll_insert(LinkedList *self, const void *item, u64 index);

void ll_get(LinkedList *self, void *item, u64 index);
void *ll_get_ref(LinkedList *self, u64 index);

void ll_pop_index(LinkedList *self, void *item, u64 index);
b8 ll_pop_first(LinkedList *self, void *item);
b8 ll_pop_last(LinkedList *self, void *item);

#define LINKED_LIST_CREATE(T) ll_init(sizeof(T))

#ifdef VECTOR_IMPLEMENTATION

vector *vector_create(mem_arena *arena, u64 elem_size) {
  vector *vec;
  vec = (vector *)arena_alloc(arena, sizeof(vector));
  vec->size = 0;
  vec->item_size = elem_size;
  vec->capacity = BASE_VEC_CAPACITY;
  vec->items = (u8 *)arena_alloc(arena, BASE_VEC_CAPACITY * elem_size);
  vec->arena = arena;
  return vec;
}

void vector_get(vector *vec, void *item, u64 index) {
  assert(vec && index < vec->size);
  memcpy(item, vec->items + index * vec->item_size, vec->item_size);
}

void *vector_get_ref_unsafe(vector *vec, u64 index) {
  if (index >= vec->capacity)
    return NULL;
  return vec->items + index * vec->item_size;
}

void *vector_get_ref(vector *vec, u64 index) {
  if (index >= vec->size)
    return NULL;
  return vec->items + index * vec->item_size;
}

void vector_get_last(vector *vec, void *item) {
  vector_get(vec, item, vec->size - 1);
}

void vector_set_unsafe(vector *vec, void *item, u64 index) {
  assert(index < vec->capacity);
  memcpy(vec->items + index * vec->item_size, item, vec->item_size);
}

void vector_set(vector *vec, void *item, u64 index) {
  assert(index < vec->size);
  memcpy(vec->items + index * vec->item_size, item, vec->item_size);
}

b8 vector_pop(vector *vec, void *item) {
  if (vec->size == 0)
    return false;
  vector_get_last(vec, item);
  vec->size--;
  return true;
}

void vector_grow(vector *vec) {
  vec->capacity *= 2;
  vec->items = (u8 *)arena_realloc(vec->arena, vec->items,
                                   vec->capacity * vec->item_size);
}

void vector_push(vector *vec, void *item) {
  if (vec->size == vec->capacity) {
    vector_grow(vec);
  }
  vec->size++;
  vector_set(vec, item, vec->size - 1);
}

void vector_remove(vector *vec, u64 index) {
  assert(index < vec->size);
  u64 i = index * vec->item_size;
  u64 j = i + vec->item_size;
  u8 *data = vec->items;
  for (u64 k = 0; j + k < vec->size * vec->item_size; ++k) {
    data[i + k] = data[j + k];
  }
  vec->size--;
}

void vector_deinit(vector *vec) {
  if (!vec)
    return;
  arena_dealloc(vec->arena, vec->items);
  arena_dealloc(vec->arena, vec);
}

void vector_extend(vector *vec, vector *extra_vec) {
  assert(vec->item_size == extra_vec->item_size);
  u64 start_indx = vec->size;
  u64 new_size = start_indx + extra_vec->size;
  while (vec->capacity < new_size)
    vector_grow(vec);
  memcpy(vec->items + start_indx * vec->item_size, extra_vec->items,
         extra_vec->size * vec->item_size);
  vec->size = new_size;
}

u64 vector_index(vector *vec, const void *needle, __vec_eq_fn_t eq) {
  for (u64 i = 0; i < vec->size; ++i) {
    u8 *vp = vec->items + i * vec->item_size;
    if (eq(vp, needle))
      return i;
  }
  return vec->size;
}

b8 vector_contains(vector *vec, const void *needle, __vec_eq_fn_t eq) {
  return vector_index(vec, needle, eq) < vec->size;
}

void vector_sort(vector *vec, __compar_fn_t cmp_fn) {
  qsort(vec->items, vec->size, vec->item_size, cmp_fn);
}

#ifdef STRING_IMPLEMENTATION

void str_split_whitespace(vector *vec, string8 s) {
  assert(vec->item_size == sizeof(string8));
  vec->size = 0;
  string8 s2 = str_trim(s);
  while (s2.size > 0) {
    u64 start = 0;
    u64 end = start;
    while (end < s2.size && !isspace(s2.str[end])) {
      end++;
    }
    string8 word = (string8){.str = s2.str, .size = end - start};
    vector_push(vec, &word);
    string8 r = (string8){.str = s2.str + end, .size = s2.size - end};
    s2 = str_trim_left(r);
  }
}

void str_split(vector *vec, string8 base, string8 sep) {
  assert(vec->item_size == sizeof(string8));
  vec->size = 0;
  u64 start = 0;
  u64 end = start;
  if (sep.size > base.size || sep.size == 0) {
    vector_push(vec, &base);
    return;
  }
  while (end <= base.size - sep.size) {
    string8 test = (string8){.str = base.str + end, .size = sep.size};
    if (!str_equal(test, sep)) {
      end++;
      continue;
    }
    // test matched sep
    string8 part = (string8){.str = base.str + start, .size = end - start};
    vector_push(vec, &part);
    start = end + sep.size;
    end = start;
  }
  if (start < base.size) {
    string8 part =
        (string8){.str = base.str + start, .size = base.size - start};
    vector_push(vec, &part);
  }
}

#endif

Queue *queue_init(u64 item_size) {
  Queue *self = (Queue *)malloc(sizeof(Queue));
  self->items = vector_create(NULL, item_size);
  self->start = 0;
  return self;
}

void queue_deinit(Queue *self) {
  vector_deinit(self->items);
  free(self);
}

u64 queue_len(Queue *self) { return self->items->size; }

void queue_push(Queue *self, void *item) {
  if (self->items->size == self->items->capacity) {
    u64 old_cap = self->items->capacity;
    vector_grow(self->items);
    u64 n_item = (self->start + queue_len(self)) & (old_cap - 1);
    void *dst = vector_get_ref_unsafe(self->items, old_cap);
    void *src = vector_get_ref(self->items, 0);
    memcpy(dst, src, n_item * self->items->item_size);
  }
  u64 index = (self->start + self->items->size) & (self->items->capacity - 1);
  self->items->size++;
  vector_set_unsafe(self->items, item, index);
}

b8 queue_pop(Queue *self, void *item) {
  if (queue_len(self) == 0)
    return false;
  void *src = vector_get_ref_unsafe(self->items, self->start);
  memcpy(item, src, self->items->item_size);
  self->start = (self->start + 1) & (self->items->capacity - 1);
  self->items->size--;
  return true;
}

PriorityQueue *priority_queue_init(u64 item_size, void *ctx,
                                   compareFn compare_fn) {
  PriorityQueue *self = (PriorityQueue *)malloc(sizeof(PriorityQueue));
  self->ctx = ctx;
  self->compare_fn = compare_fn;
  self->items = vector_create(NULL, item_size);
  return self;
}

void priority_queue_deinit(PriorityQueue *self) {
  vector_deinit(self->items);
  free(self);
}

u64 priority_queue_len(PriorityQueue *self) { return self->items->size; }

void priority_queue_push(PriorityQueue *self, void *item) {
  vector_push(self->items, item);
  __pq_sift_up(self, self->items->size - 1);
}

void __pq_sift_up(PriorityQueue *self, u64 start_index) {
  void *child = malloc(self->items->item_size);
  vector_get(self->items, child, start_index);
  void *parent = malloc(self->items->item_size);
  u64 child_index = start_index;
  while (child_index > 0) {
    u64 parent_index = ((child_index - 1) >> 1);
    vector_get(self->items, parent, parent_index);
    if (self->compare_fn(self->ctx, child, parent) != lt)
      break;
    vector_set(self->items, parent, child_index);
    child_index = parent_index;
  }
  vector_set(self->items, child, child_index);
  free(child);
  free(parent);
}

void __pq_sift_down(PriorityQueue *self, u64 target_index) {
  void *target_element = malloc(self->items->item_size);
  vector_get(self->items, target_element, target_index);
  u64 index = target_index;
  while (1) {
    u64 lesser_child_i = (index * 2) | 1;
    if (lesser_child_i >= self->items->size)
      break;
    void *lesser_child = vector_get_ref(self->items, lesser_child_i);
    u64 next_child_i = lesser_child_i + 1;
    void *next_child = vector_get_ref(self->items, next_child_i);
    if (next_child_i < self->items->size &&
        self->compare_fn(self->ctx, next_child, lesser_child) == lt) {
      lesser_child_i = next_child_i;
    }

    lesser_child = vector_get_ref(self->items, lesser_child_i);

    if (self->compare_fn(self->ctx, target_element, lesser_child) == lt)
      break;
    vector_set(self->items, lesser_child, index);
    index = lesser_child_i;
  }
  vector_set(self->items, target_element, index);
  free(target_element);
}

void *priority_queue_peek(const PriorityQueue *self) {
  return vector_get_ref(self->items, 0);
}

void priority_queue_pop_index(PriorityQueue *self, void *dst, u64 index) {
  vector_get(self->items, dst, index);
  void *last = vector_get_ref(self->items, self->items->size - 1);
  vector_set(self->items, last, index);
  self->items->size--;

  if (index == self->items->size) {
    // Last element removed, nothing more to do.
  } else if (index == 0) {
    __pq_sift_down(self, index);
  } else {
    u64 parent_index = ((index - 1) >> 1);
    void *parent = vector_get_ref(self->items, parent_index);
    if (self->compare_fn(self->ctx, last, parent) == gt) {
      __pq_sift_down(self, index);
    } else {
      __pq_sift_up(self, index);
    }
  }
}

b8 priority_queue_pop(PriorityQueue *self, void *dst) {
  if (priority_queue_len(self) == 0)
    return false;
  priority_queue_pop_index(self, dst, 0);
  return true;
}

LinkedList *ll_init(u64 item_size) {
  LinkedList *self = malloc(sizeof(LinkedList));
  self->item_size = item_size;
  self->size = 0;
  self->head = NULL;
  self->tail = NULL;
  return self;
}

void ll_deinit(LinkedList *self) {
  Node *node = self->head;
  Node *tmp;
  while (node) {
    tmp = node->next;
    free(node->item);
    free(node);
    node = tmp;
  }
  free(self);
}

Node *__ll_node_init(LinkedList *self, const void *item) {
  Node *node = (Node *)malloc(sizeof(Node));
  node->item = (u8 *)malloc(self->item_size);
  memcpy(node->item, item, self->item_size);
  node->next = NULL;
  node->previous = NULL;
  return node;
}

void ll_push(LinkedList *self, const void *item) {
  Node *node = __ll_node_init(self, item);
  self->size++;
  if (self->tail) {
    Node *prev = self->tail;
    prev->next = node;
    node->previous = prev;
    self->tail = node;
    return;
  }
  assert(self->size == 1);
  self->head = node;
  self->tail = node;
}

void ll_push_first(LinkedList *self, const void *item) {
  Node *node = __ll_node_init(self, item);
  self->size++;
  if (self->head) {
    Node *next = self->head;
    node->next = next;
    next->previous = node;
    self->head = node;
    return;
  }
  assert(self->size == 1);
  self->head = node;
  self->tail = node;
}

void ll_insert(LinkedList *self, const void *item, u64 index) {
  assert(index <= self->size);
  if (index == 0) {
    ll_push_first(self, item);
    return;
  }
  if (index == self->size) {
    ll_push(self, item);
    return;
  }
  Node *new_node = __ll_node_init(self, item);
  Node *old_node = __ll_get_node(self, index);
  new_node->previous = old_node->previous;
  new_node->next = old_node;
  old_node->previous = new_node;
  self->size++;
}

Node *__ll_get_node(LinkedList *self, u64 index) {
  assert(index < self->size);
  Node *node;
  if (index <= self->size / 2) {
    node = self->head;
    for (u64 i = 0; i < index; ++i) {
      node = node->next;
    }
    return node;
  }
  node = self->tail;
  for (u64 i = self->size - 1; i > index; --i) {
    node = node->previous;
  }
  return node;
}

void ll_get(LinkedList *self, void *item, u64 index) {
  Node *node = __ll_get_node(self, index);
  memcpy(item, node->item, self->item_size);
}

void *ll_get_ref(LinkedList *self, u64 index) {
  Node *node = __ll_get_node(self, index);
  return node->item;
}

void ll_pop_index(LinkedList *self, void *item, u64 index) {
  Node *node = __ll_get_node(self, index);
  memcpy(item, node->item, self->item_size);
  Node *prev = node->previous;
  Node *next = node->next;
  if (prev)
    prev->next = next;
  else // we are first
    self->head = next;
  if (next)
    next->previous = prev;
  else // we are last
    self->tail = prev;
  self->size--;
  free(node->item);
  free(node);
}

b8 ll_pop_first(LinkedList *self, void *item) {
  if (self->size == 0)
    return false;
  ll_pop_index(self, item, 0);
  return true;
}

b8 ll_pop_last(LinkedList *self, void *item) {
  if (self->size == 0)
    return false;
  ll_pop_index(self, item, self->size - 1);
  return true;
}

#endif
#endif
