/*
 * nanite_render_test.c
 * Cluster hierarchy parents must stay inside the level that produced them,
 * leaf bounds come from mesh positions, and LOD selection keeps a single
 * level whose projected error fits the pixel threshold.
 * Compiles nanite_render_cpu.c directly and does not link Engine.
 */

#include "geometry/nanite/nanite_render_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static int near(f32 a, f32 b) {
  f32 d = a - b;
  if (d < 0.0f) {
    d = -d;
  }
  return d <= 0.0001f;
}

static int child_listed(const ClusterHierarchyNode *node, u32 child_id) {
  u32 i;
  u32 hits = 0;
  for (i = 0; i < node->child_count; i++) {
    if (node->child_ids[i] == child_id) {
      hits++;
    }
  }
  return hits == 1;
}

static void assert_parent_links(const ClusterHierarchyBuilder *builder) {
  u32 i;
  u32 orphans = 0;
  for (i = 0; i < builder->node_count; i++) {
    const ClusterHierarchyNode *node = &builder->nodes[i];
    u32 c;
    u32 seen = 0;
    u32 walk = i;
    u32 steps = 0;
    for (c = 0; c < node->child_count; c++) {
      u32 child_id = node->child_ids[c];
      if (child_id >= builder->node_count) {
        fail("child index out of range");
      }
      if (builder->nodes[child_id].parent_id != i) {
        fail("child parent_id does not match the node that lists it");
      }
      if (child_id >= i) {
        fail("parent lists a node that was not built earlier");
      }
    }
    if (node->parent_id == UINT32_MAX) {
      orphans++;
    } else if (node->parent_id >= builder->node_count ||
               !child_listed(&builder->nodes[node->parent_id], i)) {
      fail("parent does not list this node");
    }
    while (walk != UINT32_MAX && steps <= builder->node_count) {
      walk = builder->nodes[walk].parent_id;
      steps++;
    }
    if (steps > builder->node_count) {
      fail("parent cycle");
    }
    (void)seen;
  }
  if (orphans != 1) {
    fail("expected a single root");
  }
  if (builder->root_node_id >= builder->node_count ||
      builder->nodes[builder->root_node_id].parent_id != UINT32_MAX) {
    fail("root_node_id is not the orphan root");
  }
}

static void test_hierarchy_groups_current_level_only(void) {
  enum { kClusters = 5, kVerts = 2 };
  NaniteMesh mesh;
  NaniteCluster clusters[kClusters];
  Vec3 positions[kClusters * kVerts];
  ClusterHierarchyBuilder *builder;
  u32 i;
  u32 leaf_hits[kClusters];
  const ClusterHierarchyNode *parent_a;
  const ClusterHierarchyNode *parent_b;
  const ClusterHierarchyNode *root;

  memset(&mesh, 0, sizeof(mesh));
  memset(clusters, 0, sizeof(clusters));
  memset(leaf_hits, 0, sizeof(leaf_hits));
  for (i = 0; i < kClusters; i++) {
    positions[i * 2] = vec3_create((f32)i * 10.0f, 0.0f, 0.0f);
    positions[i * 2 + 1] = vec3_create((f32)i * 10.0f, 2.0f, 0.0f);
    clusters[i].vertex_offset = i * 2;
    clusters[i].vertex_count = 2;
  }
  mesh.positions = positions;
  mesh.vertex_count = kClusters * kVerts;
  mesh.clusters = clusters;
  mesh.cluster_count = kClusters;

  builder = cluster_hierarchy_builder_create(32);
  if (!builder) {
    fail("create hierarchy");
  }
  cluster_hierarchy_build(builder, &mesh);

  if (builder->node_count != 8) {
    fail("5 clusters should build 3 parents");
  }
  assert_parent_links(builder);
  parent_a = &builder->nodes[5];
  parent_b = &builder->nodes[6];
  root = &builder->nodes[7];
  if (parent_a->child_count != 4 || parent_b->child_count != 1 ||
      root->child_count != 2) {
    fail("group sizes");
  }
  if (parent_b->child_ids[0] != 4) {
    fail("partial group absorbed a parent from the same level");
  }
  if (builder->leaf_count != kClusters) {
    fail("internal nodes stayed in the leaf list");
  }
  for (i = 0; i < builder->leaf_count; i++) {
    u32 id = builder->leaf_nodes[i];
    if (id >= kClusters || !builder->nodes[id].is_leaf) {
      fail("leaf list");
    }
    leaf_hits[id]++;
  }
  for (i = 0; i < kClusters; i++) {
    if (leaf_hits[i] != 1) {
      fail("missing leaf");
    }
    if (!near(builder->nodes[i].bounds_center.x, (f32)i * 10.0f) ||
        !near(builder->nodes[i].bounds_center.y, 1.0f) ||
        !near(builder->nodes[i].bounds_radius, 1.0f)) {
      fail("leaf bounds were not taken from positions");
    }
  }
  if (parent_a->is_leaf || parent_b->is_leaf || root->is_leaf) {
    fail("parent still marked leaf");
  }
  if (!near(parent_a->bounds_center.x, 15.0f) ||
      !near(parent_a->bounds_center.y, 1.0f) ||
      !near(parent_a->bounds_radius, 1.5f) ||
      !near(parent_b->bounds_center.x, 40.0f) ||
      !near(root->bounds_center.x, 27.5f) || !near(root->bounds_radius, 2.25f) ||
      !near(root->error_metric, 0.0144f)) {
    fail("parent bounds or error");
  }
  if (builder->root_node_id != 7) {
    fail("root");
  }
  cluster_hierarchy_destroy(builder);
  printf("PASS: hierarchy groups the current level and bounds leaves\n");
}

static void test_hierarchy_edges(void) {
  NaniteMesh mesh;
  NaniteCluster cluster;
  Vec3 positions[1];
  ClusterHierarchyBuilder *builder;
  Vec3 center;

  if (cluster_hierarchy_builder_create(0) != NULL) {
    fail("zero capacity should fail");
  }
  builder = cluster_hierarchy_builder_create(4);
  if (!builder) {
    fail("create");
  }
  if (cluster_hierarchy_add_node(builder, UINT32_MAX, NULL, 1.0f, 0, 0.0f) !=
      UINT32_MAX) {
    fail("null center accepted");
  }
  cluster_hierarchy_build(NULL, NULL);
  cluster_hierarchy_build(builder, NULL);
  if (builder->node_count != 0) {
    fail("null build mutated the builder");
  }

  memset(&mesh, 0, sizeof(mesh));
  memset(&cluster, 0, sizeof(cluster));
  positions[0] = vec3_create(3.0f, 4.0f, 0.0f);
  cluster.vertex_offset = 100;
  cluster.vertex_count = 4;
  mesh.positions = positions;
  mesh.vertex_count = 1;
  mesh.clusters = &cluster;
  mesh.cluster_count = 1;
  cluster_hierarchy_build(builder, &mesh);
  if (builder->node_count != 1 || builder->leaf_count != 1 ||
      !near(builder->nodes[0].bounds_radius, 0.0f) ||
      !near(builder->nodes[0].bounds_center.x, 0.0f)) {
    fail("out-of-range vertices should not invent bounds");
  }
  cluster_hierarchy_destroy(builder);

  center = vec3_create(1.0f, 2.0f, 3.0f);
  builder = cluster_hierarchy_builder_create(2);
  if (cluster_hierarchy_add_node(builder, UINT32_MAX, &center, 1.0f, 0, 0.2f) !=
      0) {
    fail("root id");
  }
  if (cluster_hierarchy_add_node(builder, 0, &center, 0.5f, 0, 0.1f) != 1) {
    fail("child id");
  }
  if (builder->nodes[0].is_leaf || builder->nodes[0].child_count != 1 ||
      builder->leaf_count != 1 || builder->leaf_nodes[0] != 1 ||
      builder->nodes[1].parent_id != 0) {
    fail("direct parent link did not clear the parent leaf");
  }
  if (cluster_hierarchy_add_node(builder, 0, &center, 0.1f, 0, 0.0f) !=
      UINT32_MAX) {
    fail("capacity");
  }
  cluster_hierarchy_destroy(builder);
  cluster_hierarchy_destroy(NULL);
  printf("PASS: hierarchy edges\n");
}

static void test_lod_selection(void) {
  NaniteCluster clusters[3];
  NaniteMesh mesh;
  Vec3 far_cam;
  Vec3 near_cam;
  Vec3 mid_cam;
  u32 start = 99;
  u32 count = 99;

  memset(clusters, 0, sizeof(clusters));
  memset(&mesh, 0, sizeof(mesh));
  clusters[0].bounds_min = vec3_create(-1.0f, -1.0f, -1.0f);
  clusters[0].bounds_max = vec3_create(1.0f, 1.0f, 1.0f);
  clusters[0].lod_error = 0.05f;
  clusters[1].bounds_min = clusters[0].bounds_min;
  clusters[1].bounds_max = clusters[0].bounds_max;
  clusters[1].lod_error = 0.05f;
  clusters[2].bounds_min = clusters[0].bounds_min;
  clusters[2].bounds_max = clusters[0].bounds_max;
  clusters[2].lod_error = 2.0f;
  mesh.clusters = clusters;
  mesh.cluster_count = 3;
  mesh.lod_count = 2;
  mesh.lod_offsets[0] = 0;
  mesh.lod_offsets[1] = 2;

  nanite_lod_cluster_range(&mesh, 0, &start, &count);
  if (start != 0 || count != 2) {
    fail("fine lod range");
  }
  nanite_lod_cluster_range(&mesh, 1, &start, &count);
  if (start != 2 || count != 1) {
    fail("coarse lod range");
  }

  far_cam = vec3_create(0.0f, 0.0f, 1000.0f);
  near_cam = vec3_create(0.0f, 0.0f, 10.0f);
  mid_cam = vec3_create(0.0f, 0.0f, 100.0f);
  if (nanite_select_lod(&mesh, &far_cam, 1.0f, 100.0f) != 1) {
    fail("far camera should keep the coarse lod");
  }
  if (nanite_select_lod(&mesh, &near_cam, 1.0f, 100.0f) != 0) {
    fail("near camera should keep the fine lod");
  }
  if (nanite_select_lod(&mesh, &mid_cam, 1.0f, 100.0f) != 0) {
    fail("mid camera still exceeds the coarse error");
  }

  clusters[2].lod_error = 1.0f;
  if (nanite_select_lod(&mesh, &mid_cam, 1.0f, 100.0f) != 1) {
    fail("error equal to the threshold should be accepted");
  }

  if (nanite_select_lod(NULL, &far_cam, 1.0f, 100.0f) != 0 ||
      nanite_select_lod(&mesh, NULL, 1.0f, 100.0f) != 0 ||
      nanite_select_lod(&mesh, &far_cam, -1.0f, 100.0f) != 0 ||
      nanite_select_lod(&mesh, &far_cam, 1.0f, 0.0f) != 0) {
    fail("lod selection rejected bad input");
  }

  mesh.lod_offsets[0] = 5;
  mesh.lod_offsets[1] = 1;
  mesh.cluster_count = 5;
  nanite_lod_cluster_range(&mesh, 0, &start, &count);
  if (count != 0 || start != 5) {
    fail("inverted lod offsets underflowed");
  }
  printf("PASS: lod selection\n");
}

int main(void) {
  test_hierarchy_groups_current_level_only();
  test_hierarchy_edges();
  test_lod_selection();
  printf("PASS: nanite render cpu\n");
  return 0;
}
