#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define DIM 2
#define MAXP 200

typedef double f64;
typedef uint32_t u32;
typedef u32 b32;
typedef struct cell_struct cell;

typedef struct {
  u32 n;
  f64 x[MAXP][DIM];
} leaf_data;

struct cell_struct {
  f64 xmin[DIM];
  f64 xmax[DIM];
  leaf_data *leaf;
  cell *sub;
};

void cell_init(cell *tree, const f64 *xmin, const f64 *xmax);
void cell_free(cell *c);
void cell_split(cell *tree);
b32 cell_contains(cell *c, const f64 *x);

cell *tree_create(const f64 *xmin, const f64 *xmax);
void tree_free(cell *c);
b32 tree_add(cell *c, const f64 *x);

int main(void) { return 0; }

cell *tree_create(const f64 *xmin, const f64 *xmax) {
  cell *tree = malloc(sizeof(cell));
  cell_init(tree, xmin, xmax);
  return tree;
}

void cell_init(cell *tree, const f64 *xmin, const f64 *xmax) {
  for (u32 i = 0; i < DIM; ++i) {
    tree->xmin[i] = xmin[i];
    tree->xmax[i] = xmax[i];
  }
  tree->leaf = malloc(sizeof(leaf_data));
  tree->leaf->n = 0;
  tree->sub = NULL;
}

b32 cell_contains(cell *c, const f64 *x) {
  for (u32 i = 0; i < DIM; ++i) {
    if (x[i] < c->xmin[i] || x[i] > c->xmax[i])
      return false;
  }
  return true;
}

b32 tree_add(cell *c, const f64 *x) {
  if (!cell_contains(c, x))
    return false;
  if (c->leaf) {
    u32 ip = c->leaf->n;
    if (ip < MAXP) {
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
    u32 nsub = 1 << DIM;
    for (u32 i = 0; i < nsub; ++i) {
      if (tree_add(&c->sub[i], x))
        return true;
    }
  }
  return false;
}

void cell_split(cell *c) {
  u32 nsub = 1 << DIM;
  c->sub = (cell *)malloc(nsub * sizeof(cell));
  f64 xc[DIM];
  for (u32 i = 0; i < DIM; ++i) {
    xc[i] = 0.5 * (c->xmin[i] + c->xmax[i]);
  }
  for (u32 i = 0; i < nsub; ++i) {
    f64 a[2] = {(i % 2) ? xc[0] : c->xmin[0], (i / 2) ? xc[1] : c->xmin[1]};
    f64 b[2] = {(i % 2) ? c->xmax[0] : xc[0], (i / 2) ? c->xmax[1] : xc[1]};
    cell_init(&c->sub[i], a, b);
  }
  free(c->leaf);
  c->leaf = NULL;
}

void tree_free(cell *c) {
  cell_free(c);
  free(c);
}

void cell_free(cell *c) {
  if (c->leaf)
    free(c->leaf);
  if (c->sub) {
    u32 nsub = 1 << DIM;
    for (u32 i = 0; i < nsub; ++i) {
      cell_free(&c->sub[i]);
    }
    free(c->sub);
  }
}
