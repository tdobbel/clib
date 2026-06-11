#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint8_t u8;

typedef i32 b32;
typedef u8 b8;

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1)))
#define ARENA_BASE_POS (sizeof(mem_arena))
#define ARENA_ALIGN (sizeof(void *))
#define META_SIZE (sizeof(mem_region *))

typedef struct mem_region mem_region;

struct mem_region {
  u64 pos, size;
  b8 used;
  mem_region *next;
};

typedef struct {
  u64 size, pos;
  mem_region *start, *end;
} mem_arena;

void *reserve_memory(u64 size);
b32 release_memory(void *ptr, u64 size);

mem_region *new_region(u64 pos, u64 size);
mem_arena *arena_create(u64 size);

void *arena_alloc(mem_arena *arena, u64 size);
void arena_dealloc(mem_arena *arena, void *ptr);
void *arena_realloc(mem_arena *arena, void *ptr, u64 new_size);
void arena_free_regions(mem_arena *arena);
void arena_simplify_regions(mem_arena *arena);
void arena_clear(mem_arena *arena);
void arena_destroy(mem_arena *arena);
void arena_print_regions(mem_arena *arena);

#define BYTE_SIZE(ptr) ((*(((mem_region **)(ptr)) - 1))->size - META_SIZE)
#define ARRAY_SIZE(ptr, T) (BYTE_SIZE(ptr) / sizeof(T))
#define ALLOC_STRUCT(arena, T) (T *)arena_alloc((arena), sizeof(T))
#define ALLOC_ARRAY(arena, T, n) (T *)arena_alloc((arena), (n) * sizeof(T))
#define DEALLOC(arena, v)                                                      \
  do {                                                                         \
    arena_dealloc(arena, v);                                                   \
    v = NULL;                                                                  \
  } while (0);

#define REALLOC(arena, T, ptr, n)                                              \
  (T *)arena_realloc((arena), (ptr), sizeof(T) * (n))

#ifdef ARENA_IMPLEMENTATION

void arena_print_regions(mem_arena *arena) {
  u64 i = 0;
  mem_region *r = arena->start;
  printf("---\n");
  while (r) {
    printf("Region %lu: pos=%lu, size=%lu, used=%hu\n", i, r->pos, r->size,
           r->used);
    r = r->next;
    i++;
  }
}

void *reserve_memory(u64 size) {
  void *out = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (out == MAP_FAILED) {
    return NULL;
  }
  return out;
}

b32 release_memory(void *ptr, u64 size) {
  i32 ret = munmap(ptr, size);
  return ret == 0;
}

mem_arena *arena_create(u64 size) {
  u32 pagesize = (u32)sysconf(_SC_PAGESIZE);
  size = ALIGN_UP_POW2(size, pagesize);
  mem_arena *arena = (mem_arena *)reserve_memory(size);
  arena->size = size;
  arena->pos = ARENA_BASE_POS;
  arena->start = NULL;
  arena->end = NULL;
  return arena;
}

mem_region *new_region(u64 pos, u64 size) {
  mem_region *r = (mem_region *)malloc(sizeof(mem_region));
  r->pos = pos;
  r->size = size;
  r->used = false;
  r->next = NULL;
  return r;
}

b8 region_invalid(mem_region *r, u64 req_size) {
  return r->size < req_size || r->used;
}

void *arena_alloc(mem_arena *arena, u64 size) {
  u64 total_size = size + META_SIZE;
  u64 pos_aligned = ALIGN_UP_POW2(arena->pos, ARENA_ALIGN);

  mem_region *r = arena->start;
  while (r != NULL && region_invalid(r, total_size)) {
    r = r->next;
  }

  if (r == NULL) {
    assert(pos_aligned + total_size < arena->size);
    r = new_region(pos_aligned, total_size);
    if (arena->end == NULL) {
      arena->start = r;
      arena->end = arena->start;
    } else {
      arena->end->next = r;
      arena->end = r;
    }
    arena->pos = pos_aligned + total_size;
  } else {
    // Split regions
    u64 new_pos = ALIGN_UP_POW2(r->pos + total_size, ARENA_ALIGN);
    if (new_pos < r->pos + r->size) {
      u64 size = r->pos + r->size - new_pos;
      r->size = total_size;
      mem_region *rn = new_region(new_pos, size);
      rn->next = r->next;
      r->next = rn;
    }
  }

  mem_region **v = (mem_region **)(arena + r->pos);
  r->used = true;
  *v = r;
  return (void *)(v + 1);
}

void *arena_realloc(mem_arena *arena, void *ptr, u64 new_size) {
  u64 old_size = BYTE_SIZE(ptr);
  u8 *new_ptr = (u8 *)arena_alloc(arena, new_size);
  u8 *data = (u8 *)ptr;
  for (u64 i = 0; i < MIN(old_size, new_size); ++i) {
    new_ptr[i] = data[i];
  }
  arena_dealloc(arena, ptr);
  return (void *)new_ptr;
}

void arena_simplify_regions(mem_arena *arena) {
  mem_region *r = arena->start;
  while (r != NULL && r->next != NULL) {
    if (!r->used && !r->next->used) {
      u64 new_size = r->next->pos + r->next->size - r->pos;
      r->size = new_size;
      mem_region *next = r->next->next;
      free(r->next);
      r->next = next;
    }
    r = r->next;
  }
}

void arena_dealloc(mem_arena *arena, void *ptr) {
  mem_region **rp = ((mem_region **)ptr) - 1;
  (*rp)->used = false;
  arena_simplify_regions(arena);
}

void arena_clear(mem_arena *arena) {
  arena_free_regions(arena);
  arena->pos = ARENA_BASE_POS;
}

void arena_free_regions(mem_arena *arena) {
  mem_region *r = arena->start;
  while (r != NULL) {
    mem_region *next = r->next;
    free(r);
    r = next;
  }
}

void arena_destroy(mem_arena *arena) {
  arena_free_regions(arena);
  release_memory(arena, arena->size);
}

#endif
#endif
