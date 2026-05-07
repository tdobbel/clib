#ifndef _KDTREE_H_
#define _KDTREE_H_

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef DIM
#define DIM 2
#endif
#define NSUB (1 << DIM)

#define MAXP 200

typedef double f64;
typedef uint32_t u32;
typedef uint64_t u64;
typedef u32 b32;
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
  u64 size;
  cell *root;
} kdtree;

static inline f64 distance(const f64 *xa, const f64 *xb) {
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

static void add_point(u64 id, f64 d, u64 *n, u64 *nalloc, u64 **ids_ptr,
                      f64 **dist_ptr) {
  if (*n == *nalloc) {
    (*nalloc) *= 2;
    *ids_ptr = (u64 *)realloc(*ids_ptr, *nalloc * sizeof(u64));
    *dist_ptr = (f64 *)realloc(*dist_ptr, *nalloc * sizeof(f64));
  }
  u64 *ids = *ids_ptr;
  f64 *dist = *dist_ptr;
  u64 i;
  for (i = 0; i < *n && dist[i] < d; ++i)
    ;
  for (u64 j = *n; j > i; --j) {
    ids[j] = ids[j - 1];
    dist[j] = dist[j - 1];
  }
  ids[i] = id;
  dist[i] = d;
  *n += 1;
}

void cell_init(cell *c, const f64 *xmin, const f64 *xmax);
b32 cell_add(cell *c, u64 id, const f64 *x);
void cell_free(cell *c);
void cell_split(cell *tree);
void cell_search_radius(cell *c, const f64 *xp, f64 radius, u64 *n, u64 *nalloc,
                        u64 **ids, f64 **distances);

kdtree tree_create(const f64 *xmin, const f64 *xmax);
void tree_free(kdtree *tree);
b32 tree_add(kdtree *tree, const f64 *x);
void search_radius(kdtree *tree, const f64 *xp, f64 radius, u64 *n, u64 **ids,
                   f64 **distances);
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

void search_radius(kdtree *tree, const f64 *xp, f64 radius, u64 *n, u64 **ids,
                   f64 **distances) {
  *n = 0;
  u64 nalloc = 1024;
  *ids = (u64 *)malloc(sizeof(u64) * nalloc);
  *distances = (f64 *)malloc(sizeof(f64) * nalloc);
  cell_search_radius(tree->root, xp, radius, n, &nalloc, ids, distances);
}

void cell_search_radius(cell *c, const f64 *xp, f64 radius, u64 *n, u64 *nalloc,
                        u64 **ids, f64 **distances) {
  if (!within_radius(c, xp, radius)) {
    return;
  }
  if (c->leaf) {
    for (u32 i = 0; i < MAXP; ++i) {
      f64 d = distance(xp, c->leaf->x[i]);
      if (d < radius)
        add_point(c->leaf->id[i], d, n, nalloc, ids, distances);
    }
  }
  if (c->sub) {
    for (u32 i = 0; i < NSUB; ++i) {
      cell_search_radius(&c->sub[i], xp, radius, n, nalloc, ids, distances);
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
