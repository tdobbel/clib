#ifndef _KDTREE_H_
#define _KDTREE_H_

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef VECTOR_IMPLEMENTATION
#define VECTOR_IMPLEMENTATION
#endif

#include "vector.h"

#ifndef DIM
#define DIM 2
#endif
#define NSUB (1 << DIM)

#ifndef MAXP
#define MAXP 200
#endif

typedef double f64;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t b32;
typedef struct cell_struct cell;

typedef struct {
  u32 n;
  u64 id[MAXP];
  f64 x[MAXP][DIM];
} leaf_data;

struct cell_struct {
  f64 xmin[DIM];
  f64 xmax[DIM];
  leaf_data *leaf;
  cell *sub;
};

typedef struct {
  u64 index;
  f64 distance;
} kdt_pair;

typedef struct {
  u64 size;
  cell *root;
} kdtree;

static inline f64 compute_distance(const f64 *xa, const f64 *xb) {
  f64 d2 = 0;
  for (u32 i = 0; i < DIM; ++i) {
    d2 += (xa[i] - xb[i]) * (xa[i] - xb[i]);
  }
  return sqrt(d2);
}

static b32 within_radius(cell *c, const f64 *x, f64 r) {
  for (u32 i = 0; i < DIM; ++i) {
    if (x[i] < c->xmin[i] - r || x[i] > c->xmax[i] + r)
      return false;
  }
  return true;
}

static int compare_pairs(const void *a, const void *b) {
  f64 da = ((kdt_pair *)a)->distance;
  f64 db = ((kdt_pair *)b)->distance;
  if (da < db)
    return -1;
  if (da > db)
    return 1;
  return 0;
}

void cell_init(cell *c, const f64 *xmin, const f64 *xmax);
b32 cell_add(cell *c, u64 id, const f64 *x);
void cell_free(cell *c);
void cell_split(cell *tree);
void cell_search_radius(cell *c, const f64 *xp, f64 radius, vector *result);

kdtree tree_create(const f64 *xmin, const f64 *xmax);
void tree_free(kdtree *tree);
b32 tree_add(kdtree *tree, const f64 *x);
void search_radius(kdtree *tree, const f64 *xp, f64 radius, vector *result);
kdtree tree_from_points(u64 n, const f64 *x, f64 buffer);


#ifdef KDTREE_IMPLEMENTATION

kdtree tree_create(const f64 *xmin, const f64 *xmax) {
  cell *root = (cell *)malloc(sizeof(cell));
  cell_init(root, xmin, xmax);
  return (kdtree){.size = 0, .root = root};
}

void cell_init(cell *c, const f64 *xmin, const f64 *xmax) {
  for (u32 i = 0; i < DIM; ++i) {
    c->xmin[i] = xmin[i];
    c->xmax[i] = xmax[i];
  }
  c->leaf = (leaf_data *)malloc(sizeof(leaf_data));
  c->leaf->n = 0;
  c->sub = NULL;
}

b32 tree_add(kdtree *tree, const f64 *x) {
  b32 added = cell_add(tree->root, tree->size, x);
  if (added)
    tree->size++;
  return added;
}

b32 cell_add(cell *c, const u64 id, const f64 *x) {
  if (!within_radius(c, x, 0))
    return false;
  if (c->leaf) {
    u32 ip = c->leaf->n;
    if (ip < MAXP) {
      c->leaf->id[ip] = id;
      f64 *xp = c->leaf->x[ip];
      for (u32 i = 0; i < DIM; ++i) {
        xp[i] = x[i];
      }
      c->leaf->n++;
      return true;
    }
    cell_split(c);
  }
  if (c->sub) {
    for (u32 i = 0; i < NSUB; ++i) {
      if (cell_add(&c->sub[i], id, x))
        return true;
    }
  }
  return false;
}

void cell_split(cell *c) {
  c->sub = (cell *)malloc(NSUB * sizeof(cell));
  f64 xc[DIM];
  for (u32 i = 0; i < DIM; ++i) {
    xc[i] = 0.5 * (c->xmin[i] + c->xmax[i]);
  }
  for (u32 sub = 0; sub < NSUB; ++sub) {
    f64 a[DIM];
    f64 b[DIM];
    for (u32 i = 0; i < DIM; ++i) {
      a[i] = sub & (1 << i) ? c->xmin[i] : xc[i];
      b[i] = sub & (1 << i) ? xc[i] : c->xmax[i];
    }
    cell_init(&c->sub[sub], a, b);
  }
  for (u32 j = 0; j < c->leaf->n; ++j) {
    for (u32 i = 0; i < NSUB; ++i) {
      b32 added = cell_add(&c->sub[i], c->leaf->id[j], c->leaf->x[j]);
      if (added)
        break;
    }
  }
  free(c->leaf);
  c->leaf = NULL;
}

void tree_free(kdtree *tree) {
  cell_free(tree->root);
  free(tree->root);
}

void cell_free(cell *c) {
  if (c->leaf)
    free(c->leaf);
  if (c->sub) {
    for (u32 i = 0; i < NSUB; ++i) {
      cell_free(&c->sub[i]);
    }
    free(c->sub);
  }
}

void search_radius(kdtree *tree, const f64 *xp, f64 radius, vector *result) {
  result->size = 0;
  cell_search_radius(tree->root, xp, radius, result);
  vector_sort(result, compare_pairs);
}

void cell_search_radius(cell *c, const f64 *xp, f64 radius, vector *result) {
  if (!within_radius(c, xp, radius)) {
    return;
  }
  if (c->leaf) {
    for (u32 i = 0; i < c->leaf->n; ++i) {
      f64 d = compute_distance(xp, c->leaf->x[i]);
      if (d < radius) {
        kdt_pair pair = (kdt_pair){.index = c->leaf->id[i], .distance = d};
        VEC_PUSH(result, kdt_pair, pair);
      }
    }
  }
  if (c->sub) {
    for (u32 i = 0; i < NSUB; ++i) {
      cell_search_radius(&c->sub[i], xp, radius, result);
    }
  }
}

kdtree tree_from_points(u64 n, const f64 *x, f64 buffer) {
  f64 xmin[DIM];
  f64 xmax[DIM];
  for (u32 i = 0; i < DIM; ++i) {
    xmin[i] = INFINITY;
    xmax[i] = -INFINITY;
  }
  for (u64 i = 0; i < n; ++i) {
    for (u32 j = 0; j < DIM; ++j) {
      xmin[j] = fmin(xmin[j], x[DIM * i + j]);
      xmax[j] = fmax(xmax[j], x[DIM * i + j]);
    }
  }
  for (u32 i = 0; i < DIM; ++i) {
    xmin[i] -= buffer;
    xmax[i] += buffer;
  }
  kdtree tree = tree_create(xmin, xmax);
  for (u64 i = 0; i < n; ++i) {
    tree_add(&tree, x + DIM * i);
  }
  return tree;
}

#endif
#endif
