#ifndef _COLLECTIONS_H_
#define _COLLECTIONS_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;

typedef struct {
  u64 size, capacity;
} vec_meta;

typedef struct {
  u64 elem_size, capacity, size;
  void *data;
} vector;

vector *vector_create(u64 elem_size);
void vector_free(vector *vec);
void *vector_get(vector *vec, u64 index);
void *vector_append_get(vector *vec);

#ifdef STRING_IMPLEMENTATION
vector *split_whitespace(string8 s);
vector *split(string8 base, string8 sep);
#endif

#define BASE_CAPACITY 64
#define VEC_CREATE(T) vector_create(sizeof(T))
#define VEC_PUSH(vec, T, x) (*(T *)vector_append_get((vec)) = (x))

#define HVEC_LEN(vec) ((vec_meta *)(vec) - 1)->size

#define HVEC_PUSH(vec, x)                                                      \
  do {                                                                         \
    if ((vec) == NULL) {                                                       \
      (vec) = malloc(sizeof(*(vec)) * BASE_CAPACITY + sizeof(vec_meta));       \
      vec_meta *meta = (vec_meta *)(vec);                                      \
      meta->capacity = BASE_CAPACITY;                                          \
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

vector *vector_create(u64 elem_size) {
  vector *vec = (vector *)malloc(sizeof(vector));
  vec->size = 0;
  vec->elem_size = elem_size;
  vec->capacity = BASE_CAPACITY;
  vec->data = malloc(BASE_CAPACITY * elem_size);
  return vec;
}

void *vector_get(vector *vec, u64 index) {
  assert(vec && index < vec->size);
  return (void *)((char *)vec->data + index * vec->elem_size);
}

void *vector_append_get(vector *vec) {
  if (vec->size == vec->capacity) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, vec->capacity * vec->elem_size);
  }
  return vector_get(vec, vec->size++);
}

void vector_free(vector *vec) {
  if (!vec)
    return;
  free(vec->data);
  free(vec);
}

#ifdef STRING_IMPLEMENTATION

vector *split_whitespace(string8 s) {
  vector *vec = VEC_CREATE(string8);
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
  return vec;
}

vector *split(string8 base, string8 sep) {
  vector *vec = VEC_CREATE(string8);
  u64 start = 0;
  u64 end = start;
  if (sep.size > base.size || sep.size == 0) {
    VEC_PUSH(vec, string8, base);
    return vec;
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
  return vec;
}

#endif

#endif
#endif
