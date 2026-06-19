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

typedef struct {
  u64 size, capacity;
} vec_meta;

typedef struct {
  u64 elem_size, capacity, size;
  void *data;
  mem_arena *arena;
} vector;

vector *vector_create(mem_arena *arena, u64 elem_size);
void vector_free(vector *vec);
void *vector_get(vector *vec, u64 index);
void vector_grow(vector *vec);
void *vector_append_get(vector *vec);
void vector_remove(vector *vec, u64 index);
b8 vector_contains(vector *vec, const void *x);
void vector_extend(vector *vec, vector *extra_vec);

#ifdef STRING_IMPLEMENTATION
void split_whitespace(vector *vec, string8 s);
void split(vector *vec, string8 base, string8 sep);
#endif

#ifndef BASE_VEC_CAPACITY
#define BASE_VEC_CAPACITY 64
#endif

#define VEC_CREATE(T) vector_create(NULL, sizeof(T))
#define VEC_ARENA_CREATE(a, T) vector_create(a, sizeof(T))
#define VEC_PUSH(vec, T, x) (*(T *)vector_append_get((vec)) = (x))
#define VEC_GET(vec, T, i) (*(T *)vector_get(vec, i))

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

b8 vector_contains(vector *vec, const void *x) {
  u8 *needle = (u8 *)x;
  for (u64 i = 0; i < vec->size; ++i) {
    u8 *elem = (u8 *)vec->data + i * vec->elem_size;
    u64 j;
    for (j = 0; j < vec->elem_size && elem[j] == needle[j]; ++j) {
      ;
    }
    if (j == vec->elem_size)
      return true;
  }
  return false;
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

#ifdef STRING_IMPLEMENTATION

void split_whitespace(vector *vec, string8 s) {
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

void split(vector *vec, string8 base, string8 sep) {
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
    if (end > start) {
      string8 part = (string8){.str = base.str + start, .size = end - start};
      VEC_PUSH(vec, string8, part);
    }
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

#endif
#endif
