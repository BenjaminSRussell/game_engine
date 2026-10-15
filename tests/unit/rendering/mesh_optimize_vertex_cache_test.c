/* Tests mesh_optimize_vertex_cache (src/engine/rendering/core/mesh.c), issue #190.
 * Compiles the unit directly because Engine does not build on this host.
 *
 * Checks that the optimizer:
 *   - permutes whole triangles and keeps each triangle's vertex order,
 *   - lowers the average cache miss ratio (ACMR) of shuffled grids and strips,
 *   - leaves malformed or empty meshes untouched.
 */

#include <rendering/mesh.h>
#include <rendering/mesh_optimizer.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failures++;                                                            \
    }                                                                          \
  } while (0)

typedef struct {
  uint32_t a, b, c;
} Tri;

static int compare_tri(const void *lhs, const void *rhs) {
  const Tri *p = lhs;
  const Tri *q = rhs;
  if (p->a != q->a) return p->a < q->a ? -1 : 1;
  if (p->b != q->b) return p->b < q->b ? -1 : 1;
  if (p->c != q->c) return p->c < q->c ? -1 : 1;
  return 0;
}

/* Average cache miss ratio for a FIFO post-transform cache of 32 entries. */
static double measure_acmr(const uint32_t *indices, uint32_t index_count) {
  enum { kCacheSize = 32 };
  uint32_t cache[kCacheSize];
  uint32_t used = 0;
  uint32_t misses = 0;
  for (uint32_t i = 0; i < index_count; i++) {
    int hit = 0;
    for (uint32_t j = 0; j < used; j++) {
      if (cache[j] == indices[i]) {
        hit = 1;
        break;
      }
    }
    if (!hit) {
      misses++;
      if (used < kCacheSize) used++;
      memmove(cache + 1, cache, (used - 1) * sizeof(uint32_t));
      cache[0] = indices[i];
    }
  }
  return (double)misses / (double)(index_count / 3);
}

static uint32_t g_seed = 12345u;

static uint32_t next_random(void) {
  g_seed = g_seed * 1664525u + 1013904223u;
  return g_seed >> 8;
}

/* Fisher-Yates shuffle with a fixed seed, so every run sees the same input. */
static void shuffle_tris(Tri *tris, uint32_t count) {
  for (uint32_t i = count - 1; i > 0; i--) {
    const uint32_t j = next_random() % (i + 1);
    const Tri tmp = tris[i];
    tris[i] = tris[j];
    tris[j] = tmp;
  }
}

/* Initialises mesh with vertex_count vertices and the given triangles. */
static void load_mesh(Mesh *mesh, uint32_t vertex_count, const Tri *tris,
                      uint32_t tri_count) {
  mesh_init(mesh, vertex_count, tri_count * 3);
  mesh->vertex_count = vertex_count;
  mesh->index_count = tri_count * 3;
  for (uint32_t i = 0; i < tri_count; i++) {
    mesh->indices[3 * i + 0] = tris[i].a;
    mesh->indices[3 * i + 1] = tris[i].b;
    mesh->indices[3 * i + 2] = tris[i].c;
  }
}

/* Checks that `after` holds the same triangles as `before` in any order, each with
 * its vertex order intact (so winding is preserved). */
static void expect_same_triangles(const uint32_t *before, const uint32_t *after,
                                  uint32_t index_count) {
  const uint32_t tri_count = index_count / 3;
  Tri *old_tris = malloc(tri_count * sizeof(Tri));
  Tri *new_tris = malloc(tri_count * sizeof(Tri));
  for (uint32_t i = 0; i < tri_count; i++) {
    old_tris[i] = (Tri){before[3 * i], before[3 * i + 1], before[3 * i + 2]};
    new_tris[i] = (Tri){after[3 * i], after[3 * i + 1], after[3 * i + 2]};
  }
  qsort(old_tris, tri_count, sizeof(Tri), compare_tri);
  qsort(new_tris, tri_count, sizeof(Tri), compare_tri);
  int same = 1;
  for (uint32_t i = 0; i < tri_count; i++) {
    if (compare_tri(&old_tris[i], &new_tris[i]) != 0) same = 0;
  }
  CHECK(same);
  free(old_tris);
  free(new_tris);
}

static void test_reorders_shuffled_grid(void) {
  /* 16 x 16 quads: 512 triangles over 289 vertices. */
  const uint32_t n = 16;
  const uint32_t side = n + 1;
  const uint32_t tri_count = n * n * 2;
  Tri *tris = malloc(tri_count * sizeof(Tri));
  uint32_t t = 0;
  for (uint32_t y = 0; y < n; y++) {
    for (uint32_t x = 0; x < n; x++) {
      const uint32_t v00 = y * side + x;
      tris[t++] = (Tri){v00, v00 + 1, v00 + side + 1};
      tris[t++] = (Tri){v00, v00 + side + 1, v00 + side};
    }
  }
  shuffle_tris(tris, tri_count);

  Mesh mesh;
  load_mesh(&mesh, side * side, tris, tri_count);
  free(tris);

  const uint32_t index_count = mesh.index_count;
  uint32_t *before = malloc(index_count * sizeof(uint32_t));
  memcpy(before, mesh.indices, index_count * sizeof(uint32_t));
  const double acmr_before = measure_acmr(before, index_count);

  mesh_optimize_vertex_cache(&mesh);

  const double acmr_after = measure_acmr(mesh.indices, index_count);
  printf("grid ACMR before %.3f, after %.3f\n", acmr_before, acmr_after);
  CHECK(mesh.index_count == index_count);
  CHECK(memcmp(before, mesh.indices, index_count * sizeof(uint32_t)) != 0);
  CHECK(acmr_before > 2.0); /* the shuffled input really is scrambled */
  CHECK(acmr_after < 1.0);  /* well under one cache miss per triangle */
  expect_same_triangles(before, mesh.indices, index_count);

  free(before);
  mesh_free(&mesh);
}

/* A triangle strip can be drawn with one new vertex per triangle (ACMR about 1.0).
 * A shuffled strip should come back close to that order. */
static void test_reorders_shuffled_strip(void) {
  const uint32_t tri_count = 1000;
  const uint32_t index_count = tri_count * 3;
  Tri *tris = malloc(tri_count * sizeof(Tri));
  for (uint32_t i = 0; i < tri_count; i++) {
    tris[i] = (Tri){i, i + 1, i + 2};
  }
  shuffle_tris(tris, tri_count);

  Mesh mesh;
  load_mesh(&mesh, tri_count + 2, tris, tri_count);
  free(tris);

  uint32_t *before = malloc(index_count * sizeof(uint32_t));
  memcpy(before, mesh.indices, index_count * sizeof(uint32_t));
  mesh_optimize_vertex_cache(&mesh);

  const double acmr_after = measure_acmr(mesh.indices, index_count);
  printf("strip ACMR after %.3f (optimal about 1.002)\n", acmr_after);
  CHECK(acmr_after < 1.05);
  expect_same_triangles(before, mesh.indices, index_count);

  free(before);
  mesh_free(&mesh);
}

/* Calls the optimizer on a mesh with the given contents and checks nothing changed. */
static void expect_untouched(uint32_t vertex_count, const uint32_t *indices,
                             uint32_t index_count) {
  Mesh mesh;
  mesh_init(&mesh, vertex_count, index_count);
  mesh.vertex_count = vertex_count;
  mesh.index_count = index_count;
  if (index_count > 0) {
    memcpy(mesh.indices, indices, index_count * sizeof(uint32_t));
  }

  mesh_optimize_vertex_cache(&mesh);

  CHECK(mesh.index_count == index_count);
  CHECK(index_count == 0 ||
        memcmp(mesh.indices, indices, index_count * sizeof(uint32_t)) == 0);
  mesh_free(&mesh);
}

static void test_malformed_meshes_are_untouched(void) {
  Mesh *none = NULL;
  mesh_optimize_vertex_cache(none); /* must not crash */

  Mesh no_indices;
  memset(&no_indices, 0, sizeof(no_indices));
  no_indices.vertex_count = 3;
  no_indices.index_count = 3;
  mesh_optimize_vertex_cache(&no_indices); /* must not crash */
  CHECK(no_indices.indices == NULL);

  const uint32_t single[3] = {0, 1, 2};
  expect_untouched(3, single, 3);

  const uint32_t not_triangles[4] = {0, 2, 1, 3};
  expect_untouched(4, not_triangles, 4);

  const uint32_t out_of_range[6] = {0, 1, 2, 0, 1, 7};
  expect_untouched(3, out_of_range, 6);

  const uint32_t no_vertices[3] = {0, 1, 2};
  expect_untouched(0, no_vertices, 3);

  expect_untouched(3, NULL, 0);
}

int main(void) {
  test_reorders_shuffled_grid();
  test_reorders_shuffled_strip();
  test_malformed_meshes_are_untouched();

  if (g_failures) {
    fprintf(stderr, "%d check(s) failed\n", g_failures);
    return 1;
  }
  printf("mesh_optimize_vertex_cache: all checks passed\n");
  return 0;
}
