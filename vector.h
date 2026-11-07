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
  u64 elem_size, capacity, size;
  void *data;
  mem_arena *arena;
} vector;

typedef b8 (*__vec_eq_fn_t)(const void *, const void *);

vector *vector_create(mem_arena *arena, u64 elem_size);
void vector_free(vector *vec);

void *vector_get(vector *vec, u64 index);
void *vector_get_last(vector *vec);

void vector_grow(vector *vec);
void vector_extend(vector *vec, vector *extra_vec);
void *vector_append_get(vector *vec);

void vector_remove(vector *vec, u64 index);
void *vector_pop(vector *vec);

u64 vector_get_index(vector *vec, const void *needle, __vec_eq_fn_t eq);
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
#define VEC_PUSH(vec, T, x) (*(T *)vector_append_get((vec)) = (x))
#define VEC_GET(vec, T, i) (*(T *)vector_get(vec, i))
#define VEC_POP(vec, T) (*(T *)vector_pop(vec))

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

// Priority queue

typedef enum { lt, gt, eq } Order;
typedef Order (*compareFn)(const void *, const void *, const void *);

typedef struct {
  u64 item_size;
  u64 capacity;
  void *ctx;
  compareFn compare_fn;
  u64 size;
  u8 *items;
} PriorityQueue;

void __pq_sift_up(PriorityQueue *self, u64 start_index);
void __pq_sift_down(PriorityQueue *self, u64 start_index);
void __pq_set_item(PriorityQueue *self, void *item, u64 index);
void __pq_get_item(const PriorityQueue *self, void *dst, u64 index);

PriorityQueue *priority_queue_init(u64 cap, u64 item_size, void *ctx,
                                   compareFn compare_fn);
void priority_queue_push(PriorityQueue *self, void *item);
void priority_queue_deinit(PriorityQueue *self);
void *priority_queue_peek(const PriorityQueue *self);
void priority_queue_pop_index(PriorityQueue *self, void *dst, u64 index);
void priority_queue_pop(PriorityQueue *self, void *dst);

#define PRIORITY_QUEUE_CREATE(T, ctx, compareFn)                               \
  priority_queue_init(BASE_VEC_CAPACITY, sizeof(T), ctx, compareFn)

#ifdef VECTOR_IMPLEMENTATION

vector *vector_create(mem_arena *arena, u64 elem_size) {
  vector *vec;
  vec = (vector *)arena_alloc(arena, sizeof(vector));
  vec->size = 0;
  vec->elem_size = elem_size;
  vec->capacity = BASE_VEC_CAPACITY;
  vec->data = arena_alloc(arena, BASE_VEC_CAPACITY * elem_size);
  vec->arena = arena;
  return vec;
}

void *vector_get(vector *vec, u64 index) {
  assert(vec && index < vec->size);
  return (void *)((u8 *)vec->data + index * vec->elem_size);
}

void *vector_get_last(vector *vec) {
  if (vec->size == 0)
    return NULL;
  return vector_get(vec, vec->size - 1);
}

void *vector_pop(vector *vec) {
  assert(vec->size > 0);
  void *v = vector_get(vec, vec->size - 1);
  vec->size--;
  return v;
}

void vector_grow(vector *vec) {
  vec->capacity *= 2;
  vec->data =
      arena_realloc(vec->arena, vec->data, vec->capacity * vec->elem_size);
}

void *vector_append_get(vector *vec) {
  if (vec->size == vec->capacity)
    vector_grow(vec);
  return vector_get(vec, vec->size++);
}

void vector_remove(vector *vec, u64 index) {
  assert(index < vec->size);
  u64 i = index * vec->elem_size;
  u64 j = i + vec->elem_size;
  u8 *data = (u8 *)vec->data;
  for (u64 k = 0; j + k < vec->size * vec->elem_size; ++k) {
    data[i + k] = data[j + k];
  }
  vec->size--;
}

void vector_free(vector *vec) {
  if (!vec)
    return;
  arena_dealloc(vec->arena, vec->data);
  arena_dealloc(vec->arena, vec);
}

void vector_extend(vector *vec, vector *extra_vec) {
  assert(vec->elem_size == extra_vec->elem_size);
  u64 start_indx = vec->size;
  u64 new_size = start_indx + extra_vec->size;
  while (vec->capacity < new_size)
    vector_grow(vec);
  memcpy((u8 *)vec->data + start_indx * vec->elem_size, (u8 *)extra_vec->data,
         extra_vec->size * vec->elem_size);
  vec->size = new_size;
}

u64 vector_get_index(vector *vec, const void *needle, __vec_eq_fn_t eq) {
  for (u64 i = 0; i < vec->size; ++i) {
    u8 *vp = (u8 *)vec->data + i * vec->elem_size;
    if (eq(vp, needle))
      return i;
  }
  return vec->size;
}

b8 vector_contains(vector *vec, const void *needle, __vec_eq_fn_t eq) {
  return vector_get_index(vec, needle, eq) < vec->size;
}

void vector_sort(vector *vec, __compar_fn_t cmp_fn) {
  qsort(vec->data, vec->size, vec->elem_size, cmp_fn);
}

#ifdef STRING_IMPLEMENTATION

void str_split_whitespace(vector *vec, string8 s) {
  vec->size = 0;
  string8 s2 = str_trim(s);
  while (s2.size > 0) {
    u64 start = 0;
    u64 end = start;
    while (end < s2.size && !isspace(s2.str[end])) {
      end++;
    }
    string8 word = (string8){.str = s2.str, .size = end - start};
    VEC_PUSH(vec, string8, word);
    string8 r = (string8){.str = s2.str + end, .size = s2.size - end};
    s2 = str_trim_left(r);
  }
}

void str_split(vector *vec, string8 base, string8 sep) {
  vec->size = 0;
  u64 start = 0;
  u64 end = start;
  if (sep.size > base.size || sep.size == 0) {
    VEC_PUSH(vec, string8, base);
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
    VEC_PUSH(vec, string8, part);
    start = end + sep.size;
    end = start;
  }
  if (start < base.size) {
    string8 part =
        (string8){.str = base.str + start, .size = base.size - start};
    VEC_PUSH(vec, string8, part);
  }
}

#endif

PriorityQueue *priority_queue_init(u64 cap, u64 item_size, void *ctx,
                                   compareFn compare_fn) {
  PriorityQueue *self = (PriorityQueue *)malloc(sizeof(PriorityQueue));
  self->capacity = cap;
  self->item_size = item_size;
  self->ctx = ctx;
  self->compare_fn = compare_fn;
  self->size = 0;
  self->items = (u8 *)malloc(item_size * cap);
  return self;
}

void priority_queue_deinit(PriorityQueue *self) {
  free(self->items);
  free(self);
}

void __pq_set_item(PriorityQueue *self, void *item, u64 index) {
  assert(index < self->size);
  memcpy(self->items + self->item_size * index, item, self->item_size);
}

void __pq_get_item(const PriorityQueue *self, void *dst, u64 index) {
  assert(index < self->size);
  memcpy(dst, self->items + self->item_size * index, self->item_size);
}

void priority_queue_push(PriorityQueue *self, void *item) {
  if (self->size == self->capacity) {
    self->capacity *= 2;
    self->items = (u8 *)realloc(self->items, self->capacity * self->item_size);
  }
  self->size++;
  __pq_set_item(self, item, self->size - 1);
  __pq_sift_up(self, self->size - 1);
}

void __pq_sift_up(PriorityQueue *self, u64 start_index) {
  void *child = malloc(self->item_size);
  __pq_get_item(self, child, start_index);
  void *parent = malloc(self->item_size);
  u64 child_index = start_index;
  while (child_index > 0) {
    u64 parent_index = ((child_index - 1) >> 1);
    __pq_get_item(self, parent, parent_index);
    if (self->compare_fn(self->ctx, child, parent) != lt)
      break;
    __pq_set_item(self, parent, child_index);
    child_index = parent_index;
  }
  __pq_set_item(self, child, child_index);
  free(child);
  free(parent);
}

void __pq_sift_down(PriorityQueue *self, u64 target_index) {
  void *target_element = malloc(self->item_size);
  __pq_get_item(self, target_element, target_index);
  u64 index = target_index;
  while (1) {
    u64 lesser_child_i = (index * 2) | 1;
    if (lesser_child_i >= self->size)
      break;
    u64 next_child_i = lesser_child_i + 1;
    void *lesser_child = self->items + self->item_size * lesser_child_i;
    void *next_child = self->items + self->item_size * next_child_i;
    if (next_child_i < self->size &&
        self->compare_fn(self->ctx, next_child, lesser_child) == lt) {
      lesser_child_i = next_child_i;
    }

    lesser_child = self->items + self->item_size * lesser_child_i;

    if (self->compare_fn(self->ctx, target_element, lesser_child) == lt)
      break;
    __pq_set_item(self, lesser_child, index);
    index = lesser_child_i;
  }
  __pq_set_item(self, target_element, index);
  free(target_element);
}

void *priority_queue_peek(const PriorityQueue *self) {
  return self->size == 0 ? NULL : self->items;
}

void priority_queue_pop_index(PriorityQueue *self, void *dst, u64 index) {
  assert(self->size > index);
  __pq_get_item(self, dst, index);
  void *last = self->items + (self->size - 1) * self->item_size;
  __pq_set_item(self, last, index);
  self->size--;
  if (index == self->size) {
    // Last element removed, nothing more to do.
  } else if (index == 0) {
    __pq_sift_down(self, index);
  } else {
    u64 parent_index = ((index - 1) >> 1);
    void *parent = self->items + parent_index * self->item_size;
    if (self->compare_fn(self->ctx, last, parent) == gt) {
      __pq_sift_down(self, index);
    } else {
      __pq_sift_up(self, index);
    }
  }
}

void priority_queue_pop(PriorityQueue *self, void *dst) {
  priority_queue_pop_index(self, dst, 0);
}

#endif
#endif
