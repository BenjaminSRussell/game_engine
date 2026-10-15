/*
 * cluster_builder_test.m
 * Cluster DAG build, bounds, quantization, LOD, and streaming helpers.
 * Compiles cluster_builder.m directly and does not link Engine.
 */

#include "geometry/nanite/cluster_builder.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static int nearly(float a, float b, float eps) {
  return fabsf(a - b) <= eps;
}

static int bounds_contain(simd_float3 parent_center, simd_float3 parent_extent,
                          simd_float3 child_center, simd_float3 child_extent) {
  simd_float3 pmin = parent_center - parent_extent;
  simd_float3 pmax = parent_center + parent_extent;
  simd_float3 cmin = child_center - child_extent;
  simd_float3 cmax = child_center + child_extent;
  const float eps = 1e-3f;
  return cmin.x >= pmin.x - eps && cmax.x <= pmax.x + eps &&
         cmin.y >= pmin.y - eps && cmax.y <= pmax.y + eps &&
         cmin.z >= pmin.z - eps && cmax.z <= pmax.z + eps;
}

typedef struct {
  uint32_t a, b, c;
} tri_key_t;

static tri_key_t make_key(uint32_t i0, uint32_t i1, uint32_t i2) {
  uint32_t v[3] = {i0, i1, i2};
  for (int i = 0; i < 2; i++) {
    for (int j = i + 1; j < 3; j++) {
      if (v[j] < v[i]) {
        uint32_t tmp = v[i];
        v[i] = v[j];
        v[j] = tmp;
      }
    }
  }
  tri_key_t key = {v[0], v[1], v[2]};
  return key;
}

static int cmp_key(const void *lhs, const void *rhs) {
  const tri_key_t *a = (const tri_key_t *)lhs;
  const tri_key_t *b = (const tri_key_t *)rhs;
  if (a->a != b->a)
    return a->a < b->a ? -1 : 1;
  if (a->b != b->b)
    return a->b < b->b ? -1 : 1;
  if (a->c != b->c)
    return a->c < b->c ? -1 : 1;
  return 0;
}

static uint32_t expected_cluster_count(uint32_t tri_count) {
  uint32_t leaves =
      (tri_count + CLUSTER_TRIANGLE_COUNT - 1) / CLUSTER_TRIANGLE_COUNT;
  uint32_t total = leaves;
  uint32_t level = leaves;
  while (level > 1) {
    level = (level + 3) / 4;
    total += level;
  }
  return total;
}

static void fill_vertex(geometry_vertex_t *v, float x, float y, float z) {
  memset(v, 0, sizeof(*v));
  v->position.x = x;
  v->position.y = y;
  v->position.z = z;
  v->normal.x = 0.0f;
  v->normal.y = 1.0f;
  v->normal.z = 0.0f;
  v->texcoord.x = 0.25f;
  v->texcoord.y = 0.75f;
  v->tangent.w = 1.0f;
}

static void test_null_guards(void) {
  uint32_t ids[4] = {0};
  uint32_t count = 99;
  simd_float3 cam = {0.0f, 0.0f, 1.0f};

  cluster_mesh_free(NULL);
  cluster_mesh_quantize_vertices(NULL, NULL, 0);
  cluster_mesh_select_lod_clusters(NULL, &cam, 1.0f, ids, &count);
  if (count != 99) {
    fail("null lod selection wrote the count");
  }
  if (cluster_mesh_is_cluster_resident(NULL, 0)) {
    fail("null mesh reported resident");
  }
  if (cluster_mesh_should_cull_temporal(NULL, 0, 0)) {
    fail("null mesh reported temporal cull");
  }
}

static void test_single_triangle(id<MTLDevice> device) {
  geometry_vertex_t vertices[3];
  uint32_t indices[3] = {0, 1, 2};
  cluster_mesh_t *mesh;
  mesh_cluster_t *clusters;

  fill_vertex(&vertices[0], 0.0f, 0.0f, 0.0f);
  fill_vertex(&vertices[1], 1.0f, 0.0f, 0.0f);
  fill_vertex(&vertices[2], 0.0f, 1.0f, 0.0f);

  mesh = cluster_mesh_build(device, vertices, 3, indices, 3);
  if (!mesh) {
    fail("single triangle build");
  }
  if (mesh->cluster_count != 1 || mesh->total_triangles != 1 ||
      mesh->meshlet_count != 1) {
    fail("single triangle counts");
  }
  if (!mesh->vertex_buffer || !mesh->index_buffer || !mesh->cluster_buffer ||
      !mesh->meshlet_buffer) {
    fail("single triangle buffers");
  }

  clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
  if (!clusters || clusters[0].triangle_count != 1 ||
      clusters[0].child_count != 0 ||
      clusters[0].parent_cluster != 0xFFFFFFFFu || clusters[0].lod_error <= 0.0f) {
    fail("single triangle cluster");
  }
  if (!nearly(clusters[0].bounds_center.x, 0.5f, 1e-4f) ||
      !nearly(clusters[0].bounds_center.y, 0.5f, 1e-4f) ||
      !nearly(clusters[0].bounds_center.z, 0.0f, 1e-4f)) {
    fail("single triangle bounds");
  }

  cluster_mesh_free(mesh);
}

static void test_dag_and_helpers(id<MTLDevice> device) {
  enum { TRI_COUNT = 256, VERT_COUNT = TRI_COUNT * 3 };
  geometry_vertex_t *vertices;
  uint32_t *indices;
  uint32_t *materials;
  tri_key_t *original;
  tri_key_t *built;
  cluster_mesh_t *mesh;
  mesh_cluster_t *clusters;
  const uint32_t *built_indices;
  uint32_t expected;
  uint32_t leaf_tris;
  uint32_t stack[32];
  uint32_t sp;
  uint32_t seen;
  uint32_t leaves_seen;
  uint8_t visited[32];
  uint32_t visible[16];
  uint32_t visible_count;
  simd_float3 camera;
  float parent_error;
  uint32_t root;
  geometry_vertex_t dequant;
  quantized_vertex_t *quantized;
  bvh_node_t *bvh;
  cluster_blas_t *blas;
  cluster_animation_data_t anim;
  cluster_animation_data_t *anim_buffer;
  visibility_history_t *history;
  uint32_t i;

  vertices = (geometry_vertex_t *)calloc(VERT_COUNT, sizeof(*vertices));
  indices = (uint32_t *)malloc(sizeof(uint32_t) * VERT_COUNT);
  materials = (uint32_t *)malloc(sizeof(uint32_t) * VERT_COUNT);
  original = (tri_key_t *)malloc(sizeof(tri_key_t) * TRI_COUNT);
  built = (tri_key_t *)malloc(sizeof(tri_key_t) * TRI_COUNT);
  if (!vertices || !indices || !materials || !original || !built) {
    fail("alloc");
  }

  for (i = 0; i < TRI_COUNT; i++) {
    uint32_t base = i * 3;
    fill_vertex(&vertices[base], (float)i, 0.0f, 0.0f);
    fill_vertex(&vertices[base + 1], (float)i + 0.2f, 1.0f, 0.0f);
    fill_vertex(&vertices[base + 2], (float)i, 0.0f, 1.0f);
    indices[base] = base;
    indices[base + 1] = base + 1;
    indices[base + 2] = base + 2;
    original[i] = make_key(base, base + 1, base + 2);
    materials[base] = materials[base + 1] = materials[base + 2] = (i < 128) ? 4u : 9u;
  }

  mesh = cluster_mesh_build(device, vertices, VERT_COUNT, indices, VERT_COUNT);
  if (!mesh) {
    fail("dag build");
  }

  expected = expected_cluster_count(TRI_COUNT);
  if (mesh->total_triangles != TRI_COUNT || mesh->cluster_count != expected ||
      mesh->meshlet_count != expected || expected < 3) {
    fail("dag counts");
  }
  if (!mesh->cluster_buffer || !mesh->index_buffer) {
    fail("dag buffers");
  }

  clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
  built_indices = (const uint32_t *)[mesh->index_buffer contents];
  if (!clusters || !built_indices) {
    fail("dag contents");
  }

  root = mesh->cluster_count - 1;
  if (clusters[root].parent_cluster != 0xFFFFFFFFu ||
      clusters[root].child_count == 0) {
    fail("root cluster");
  }

  memset(visited, 0, sizeof(visited));
  leaf_tris = 0;
  leaves_seen = 0;
  seen = 0;
  sp = 0;
  stack[sp++] = root;
  while (sp > 0) {
    uint32_t idx = stack[--sp];
    mesh_cluster_t *cluster;
    if (idx >= mesh->cluster_count || visited[idx]) {
      fail("dag walk");
    }
    visited[idx] = 1;
    seen++;
    cluster = &clusters[idx];
    if (cluster->child_count == 0) {
      leaves_seen++;
      leaf_tris += cluster->triangle_count;
      if (cluster->triangle_count == 0 ||
          cluster->triangle_count > CLUSTER_TRIANGLE_COUNT) {
        fail("leaf triangle count");
      }
      continue;
    }
    if (cluster->child_count > 8) {
      fail("child count");
    }
    for (uint32_t c = 0; c < cluster->child_count; c++) {
      uint32_t child = cluster->child_clusters[c];
      if (child >= mesh->cluster_count ||
          clusters[child].parent_cluster != idx) {
        fail("parent link");
      }
      if (clusters[idx].lod_error + 1e-4f < clusters[child].lod_error) {
        fail("lod error did not grow toward the root");
      }
      if (!bounds_contain(clusters[idx].bounds_center, clusters[idx].bounds_extent,
                          clusters[child].bounds_center,
                          clusters[child].bounds_extent)) {
        fail("parent bounds");
      }
      stack[sp++] = child;
    }
  }
  if (seen != mesh->cluster_count || leaf_tris != TRI_COUNT || leaves_seen < 2) {
    fail("dag coverage");
  }

  for (i = 0; i < TRI_COUNT; i++) {
    built[i] = make_key(built_indices[i * 3], built_indices[i * 3 + 1],
                        built_indices[i * 3 + 2]);
  }
  qsort(original, TRI_COUNT, sizeof(tri_key_t), cmp_key);
  qsort(built, TRI_COUNT, sizeof(tri_key_t), cmp_key);
  if (memcmp(original, built, sizeof(tri_key_t) * TRI_COUNT) != 0) {
    fail("sorted indices dropped or duplicated a triangle");
  }

  cluster_mesh_quantize_vertices(mesh, vertices, VERT_COUNT);
  if (!mesh->quantized_vertex_buffer ||
      mesh->quantized_vertex_count != VERT_COUNT) {
    fail("quantize");
  }
  quantized = (quantized_vertex_t *)[mesh->quantized_vertex_buffer contents];
  if (!quantized || quantized[0].position[0] != 0 ||
      quantized[0].position[1] != 0 || quantized[0].position[2] != 0) {
    fail("origin vertex did not quantize to zero");
  }
  memset(&dequant, 0, sizeof(dequant));
  cluster_mesh_dequantize_vertices(mesh, &dequant, 0, 1);
  if (!nearly(dequant.position.x, 0.0f, 1e-5f) ||
      !nearly(dequant.position.y, 0.0f, 1e-5f) ||
      !nearly(dequant.position.z, 0.0f, 1e-5f) || dequant.normal.y < 0.9f ||
      !nearly(dequant.texcoord.x, 0.25f, 1e-3f) ||
      !nearly(dequant.texcoord.y, 0.75f, 1e-3f)) {
    fail("dequantize");
  }

  cluster_mesh_quantize_materials(mesh, materials, VERT_COUNT);
  if (clusters[0].material_info.material_id != 4 ||
      clusters[0].material_info.material_count != 1 ||
      clusters[1].material_info.material_id != 9 ||
      clusters[1].material_info.material_count != 1) {
    fail("material ids");
  }

  camera = (simd_float3){0.0f, 0.0f, 1000.0f};
  visible_count = 0;
  cluster_mesh_select_lod_clusters(mesh, &camera, 1.0e9f, visible, &visible_count);
  if (visible_count != leaves_seen) {
    fail("coarse lod should keep only leaves");
  }
  for (i = 0; i < visible_count; i++) {
    if (clusters[visible[i]].child_count != 0) {
      fail("coarse lod selected a parent");
    }
  }
  visible_count = 0;
  cluster_mesh_select_lod_clusters(mesh, &camera, 1.0f, visible, &visible_count);
  if (visible_count != mesh->cluster_count) {
    fail("fine screen threshold should accept every cluster");
  }

  parent_error = clusters[root].lod_error;
  if (clusters[root].triangle_count <= 10) {
    fail("parent triangle count");
  }
  cluster_mesh_simplify_parent_geometry(mesh, root, 10);
  if (clusters[root].triangle_count != 10 ||
      clusters[root].lod_error < parent_error) {
    fail("simplify parent");
  }
  cluster_mesh_simplify_parent_geometry(mesh, 100000u, 1);

  cluster_mesh_build_bvh(mesh);
  if (!mesh->bvh_buffer || mesh->bvh_node_count < mesh->cluster_count) {
    fail("bvh");
  }
  bvh = (bvh_node_t *)[mesh->bvh_buffer contents];
  if (!bvh ||
      !nearly(bvh[0].bounds_min.x,
              clusters[0].bounds_center.x - clusters[0].bounds_extent.x, 1e-3f) ||
      bvh[0].cluster_count != 1) {
    fail("bvh leaf");
  }

  cluster_mesh_init_lru_cache(mesh, 8, 16.0f);
  cluster_mesh_mark_cluster_accessed(mesh, 0, 3, 2);
  cluster_mesh_mark_cluster_accessed(mesh, 3, 4, 1);
  if (!cluster_mesh_is_cluster_resident(mesh, 0) ||
      !cluster_mesh_is_cluster_resident(mesh, 3) ||
      cluster_mesh_is_cluster_resident(mesh, 99)) {
    fail("lru residency");
  }
  cluster_mesh_update_lru_cache(mesh, 5);
  if (mesh->resident_cluster_count != 2 || mesh->current_frame != 5) {
    fail("lru update");
  }

  cluster_mesh_init_visibility_history(mesh);
  if (mesh->visibility_history_count != mesh->cluster_count ||
      !cluster_mesh_should_cull_temporal(mesh, 0, 0)) {
    fail("visibility init");
  }
  visible[0] = 0;
  for (i = 0; i < VISIBILITY_HISTORY_FRAMES; i++) {
    cluster_mesh_update_visibility_history(mesh, visible, 1, i);
  }
  history = (visibility_history_t *)[mesh->visibility_history_buffer contents];
  if (!history || history[0].visibility_score != VISIBILITY_HISTORY_FRAMES ||
      cluster_mesh_should_cull_temporal(mesh, 0, VISIBILITY_HISTORY_FRAMES) ||
      !cluster_mesh_should_cull_temporal(mesh, 1, VISIBILITY_HISTORY_FRAMES)) {
    fail("visibility history");
  }

  cluster_mesh_build_blas(mesh, device);
  if (mesh->blas_count != mesh->cluster_count || !mesh->blas_buffer) {
    fail("blas");
  }
  blas = (cluster_blas_t *)[mesh->blas_buffer contents];
  if (!blas || blas[0].triangle_count != CLUSTER_TRIANGLE_COUNT ||
      blas[0].cluster_offset != 0) {
    fail("blas entry");
  }

  cluster_mesh_init_animation_data(mesh);
  memset(&anim, 0, sizeof(anim));
  anim.bone_count = 4;
  anim.vertex_count = 12;
  anim.vertex_offset = 3;
  cluster_mesh_update_animation(mesh, &anim, 0);
  anim_buffer = (cluster_animation_data_t *)[mesh->animation_buffer contents];
  if (!anim_buffer || anim_buffer[0].bone_count != 4 ||
      anim_buffer[0].vertex_count != 12 || !clusters[0].is_animated ||
      clusters[0].animation_data_index != 0) {
    fail("animation");
  }

  cluster_mesh_enable_displacement(mesh, 0, true);
  cluster_mesh_enable_displacement(mesh, 9999u, true);
  if (!clusters[0].has_displacement || clusters[1].has_displacement) {
    fail("displacement");
  }

  cluster_mesh_free(mesh);
  free(vertices);
  free(indices);
  free(materials);
  free(original);
  free(built);
}

int main(void) {
  id<MTLDevice> device;

  @autoreleasepool {
    test_null_guards();
    device = MTLCreateSystemDefaultDevice();
    if (!device) {
      fail("Metal device");
    }
    test_single_triangle(device);
    test_dag_and_helpers(device);
  }

  printf("ok\n");
  return 0;
}
