#include <core/containers_advanced/octree.h>

#include <include/math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct OctreeNode {
  OctreeAABB bounds;
  OctreeItem *items;
  uint32_t item_count;
  uint32_t item_capacity;
  struct OctreeNode *children[8];
  uint32_t depth;
  bool is_leaf;
} OctreeNode;

struct Octree {
  OctreeNode *root;
  uint32_t max_depth;
  uint32_t max_items;
};

static OctreeNode *octree_node_create(OctreeAABB bounds, uint32_t depth,
                                      uint32_t capacity) {
  OctreeNode *node = (OctreeNode *)calloc(1, sizeof(OctreeNode));
  if (!node) {
    return NULL;
  }

  node->bounds = bounds;
  node->items = (OctreeItem *)calloc(capacity, sizeof(OctreeItem));
  if (!node->items) {
    free(node);
    return NULL;
  }
  node->item_capacity = capacity;
  node->item_count = 0;
  node->depth = depth;
  node->is_leaf = true;
  for (int i = 0; i < 8; ++i) {
    node->children[i] = NULL;
  }
  return node;
}

static void octree_node_destroy(OctreeNode *node) {
  if (!node) {
    return;
  }
  for (int i = 0; i < 8; ++i) {
    octree_node_destroy(node->children[i]);
  }
  free(node->items);
  free(node);
}

static void octree_node_add_depth(OctreeNode *node, uint32_t delta) {
  if (!node) {
    return;
  }
  node->depth += delta;
  for (int i = 0; i < 8; ++i) {
    octree_node_add_depth(node->children[i], delta);
  }
}

static bool aabb_contains(const OctreeAABB *outer, const OctreeAABB *inner) {
  return inner->min[0] >= outer->min[0] && inner->max[0] <= outer->max[0] &&
         inner->min[1] >= outer->min[1] && inner->max[1] <= outer->max[1] &&
         inner->min[2] >= outer->min[2] && inner->max[2] <= outer->max[2];
}

static bool aabb_intersects(const OctreeAABB *a, const OctreeAABB *b) {
  return !(a->max[0] < b->min[0] || a->min[0] > b->max[0] ||
           a->max[1] < b->min[1] || a->min[1] > b->max[1] ||
           a->max[2] < b->min[2] || a->min[2] > b->max[2]);
}

static OctreeAABB octree_child_bounds(const OctreeAABB *bounds, int index) {
  float center[3] = {(bounds->min[0] + bounds->max[0]) * 0.5f,
                     (bounds->min[1] + bounds->max[1]) * 0.5f,
                     (bounds->min[2] + bounds->max[2]) * 0.5f};

  OctreeAABB child = *bounds;
  for (int axis = 0; axis < 3; ++axis) {
    if (index & (1 << axis)) {
      child.min[axis] = center[axis];
    } else {
      child.max[axis] = center[axis];
    }
  }
  return child;
}

static int octree_pick_child(const OctreeAABB *node_bounds,
                             const OctreeAABB *item_bounds) {
  float center[3] = {(node_bounds->min[0] + node_bounds->max[0]) * 0.5f,
                     (node_bounds->min[1] + node_bounds->max[1]) * 0.5f,
                     (node_bounds->min[2] + node_bounds->max[2]) * 0.5f};

  int index = 0;
  if (item_bounds->min[0] >= center[0]) {
    index |= 1;
  }
  if (item_bounds->min[1] >= center[1]) {
    index |= 2;
  }
  if (item_bounds->min[2] >= center[2]) {
    index |= 4;
  }

  OctreeAABB child_bounds = octree_child_bounds(node_bounds, index);
  if (aabb_contains(&child_bounds, item_bounds)) {
    return index;
  }

  return -1;
}

static void octree_subdivide(OctreeNode *node, uint32_t max_items) {
  if (!node || !node->is_leaf) {
    return;
  }

  for (int i = 0; i < 8; ++i) {
    OctreeAABB child_bounds = octree_child_bounds(&node->bounds, i);
    node->children[i] =
        octree_node_create(child_bounds, node->depth + 1, max_items);
  }
  node->is_leaf = false;

  uint32_t keep_count = 0;
  for (uint32_t i = 0; i < node->item_count; ++i) {
    OctreeItem item = node->items[i];
    int child_index = octree_pick_child(&node->bounds, &item.bounds);
    if (child_index >= 0 && node->children[child_index]) {
      OctreeNode *child = node->children[child_index];
      if (child->item_count < child->item_capacity) {
        child->items[child->item_count++] = item;
        continue;
      }
    }
    node->items[keep_count++] = item;
  }
  node->item_count = keep_count;
}

static bool octree_node_insert(OctreeNode *node, OctreeItem item,
                               uint32_t max_depth, uint32_t max_items) {
  if (!node) {
    return false;
  }

  if (!aabb_contains(&node->bounds, &item.bounds)) {
    return false;
  }

  if (node->is_leaf) {
    if (node->item_count < max_items || node->depth >= max_depth) {
      node->items[node->item_count++] = item;
      return true;
    }
    octree_subdivide(node, max_items);
  }

  int child_index = octree_pick_child(&node->bounds, &item.bounds);
  if (child_index >= 0 && node->children[child_index]) {
    return octree_node_insert(node->children[child_index], item, max_depth,
                              max_items);
  }

  if (node->item_count < node->item_capacity) {
    node->items[node->item_count++] = item;
    return true;
  }

  return false;
}

static bool octree_item_equals(const OctreeItem *a, const OctreeItem *b) {
  if (a->payload != b->payload) {
    return false;
  }
  return memcmp(&a->bounds, &b->bounds, sizeof(OctreeAABB)) == 0;
}

static bool octree_node_remove(OctreeNode *node, const OctreeItem *item) {
  if (!node || !item) {
    return false;
  }

  for (uint32_t i = 0; i < node->item_count; ++i) {
    if (octree_item_equals(&node->items[i], item)) {
      node->items[i] = node->items[node->item_count - 1];
      node->item_count--;
      return true;
    }
  }

  if (!node->is_leaf) {
    for (int i = 0; i < 8; ++i) {
      if (octree_node_remove(node->children[i], item)) {
        return true;
      }
    }
  }

  return false;
}

static bool octree_ray_intersects_aabb(const OctreeRay *ray,
                                       const OctreeAABB *box) {
  float tmin = -INFINITY;
  float tmax = INFINITY;

  for (int axis = 0; axis < 3; ++axis) {
    float origin = ray->origin[axis];
    float direction = ray->direction[axis];
    if (fabsf(direction) < 1e-6f) {
      if (origin < box->min[axis] || origin > box->max[axis]) {
        return false;
      }
    } else {
      float inv = 1.0f / direction;
      float t1 = (box->min[axis] - origin) * inv;
      float t2 = (box->max[axis] - origin) * inv;
      if (t1 > t2) {
        float tmp = t1;
        t1 = t2;
        t2 = tmp;
      }
      if (t1 > tmin) {
        tmin = t1;
      }
      if (t2 < tmax) {
        tmax = t2;
      }
      if (tmin > tmax) {
        return false;
      }
    }
  }

  return tmax >= 0.0f;
}

static bool sphere_intersects_aabb(const float center[3], float radius,
                                   const OctreeAABB *box) {
  float dist_sq = 0.0f;
  for (int axis = 0; axis < 3; ++axis) {
    float v = center[axis];
    if (v < box->min[axis]) {
      float d = box->min[axis] - v;
      dist_sq += d * d;
    } else if (v > box->max[axis]) {
      float d = v - box->max[axis];
      dist_sq += d * d;
    }
  }
  return dist_sq <= radius * radius;
}

static bool octree_frustum_intersects_aabb(const OctreeFrustum *frustum,
                                           const OctreeAABB *box) {
  for (int i = 0; i < 6; ++i) {
    const float *p = frustum->planes[i];
    float x = p[0] >= 0.0f ? box->max[0] : box->min[0];
    float y = p[1] >= 0.0f ? box->max[1] : box->min[1];
    float z = p[2] >= 0.0f ? box->max[2] : box->min[2];
    if (p[0] * x + p[1] * y + p[2] * z + p[3] < 0.0f) {
      return false;
    }
  }
  return true;
}

static void octree_query_node(const OctreeNode *node,
                              bool (*bounds_test)(const OctreeAABB *bounds,
                                                  const void *query),
                              const void *query, OctreeVisitFn visit,
                              void *user) {
  if (!node || !bounds_test) {
    return;
  }

  if (!bounds_test(&node->bounds, query)) {
    return;
  }

  for (uint32_t i = 0; i < node->item_count; ++i) {
    if (!visit(&node->items[i], user)) {
      return;
    }
  }

  if (!node->is_leaf) {
    for (int i = 0; i < 8; ++i) {
      octree_query_node(node->children[i], bounds_test, query, visit, user);
    }
  }
}

static bool bounds_test_frustum(const OctreeAABB *bounds, const void *query) {
  return octree_frustum_intersects_aabb((const OctreeFrustum *)query, bounds);
}

static bool bounds_test_ray(const OctreeAABB *bounds, const void *query) {
  return octree_ray_intersects_aabb((const OctreeRay *)query, bounds);
}

typedef struct SphereQuery {
  float center[3];
  float radius;
} SphereQuery;

static bool bounds_test_sphere(const OctreeAABB *bounds, const void *query) {
  const SphereQuery *sphere = (const SphereQuery *)query;
  return sphere_intersects_aabb(sphere->center, sphere->radius, bounds);
}

static bool bounds_test_aabb(const OctreeAABB *bounds, const void *query) {
  return aabb_intersects(bounds, (const OctreeAABB *)query);
}

Octree *octree_create(OctreeAABB bounds, uint32_t max_depth,
                      uint32_t max_items) {
  if (max_depth == 0) {
    max_depth = 4;
  }
  if (max_items == 0) {
    max_items = 8;
  }

  Octree *tree = (Octree *)calloc(1, sizeof(Octree));
  if (!tree) {
    return NULL;
  }

  tree->root = octree_node_create(bounds, 0, max_items);
  if (!tree->root) {
    free(tree);
    return NULL;
  }

  tree->max_depth = max_depth;
  tree->max_items = max_items;
  return tree;
}

void octree_destroy(Octree *tree) {
  if (!tree) {
    return;
  }
  octree_node_destroy(tree->root);
  free(tree);
}

static bool octree_expand_root(Octree *tree, const OctreeItem *item) {
  if (aabb_contains(&tree->root->bounds, &item->bounds)) {
    return true;
  }

  OctreeAABB bounds = tree->root->bounds;
  float center[3] = {(bounds.min[0] + bounds.max[0]) * 0.5f,
                     (bounds.min[1] + bounds.max[1]) * 0.5f,
                     (bounds.min[2] + bounds.max[2]) * 0.5f};
  float half[3] = {(bounds.max[0] - bounds.min[0]) * 0.5f,
                   (bounds.max[1] - bounds.min[1]) * 0.5f,
                   (bounds.max[2] - bounds.min[2]) * 0.5f};

  while (!aabb_contains(&bounds, &item->bounds)) {
    half[0] *= 2.0f;
    half[1] *= 2.0f;
    half[2] *= 2.0f;
    bounds.min[0] = center[0] - half[0];
    bounds.max[0] = center[0] + half[0];
    bounds.min[1] = center[1] - half[1];
    bounds.max[1] = center[1] + half[1];
    bounds.min[2] = center[2] - half[2];
    bounds.max[2] = center[2] + half[2];
  }

  OctreeNode *new_root = octree_node_create(bounds, 0, tree->max_items);
  if (!new_root) {
    return false;
  }

  new_root->is_leaf = false;
  int child_index = octree_pick_child(&bounds, &tree->root->bounds);
  if (child_index < 0) {
    child_index = 0;
  }
  for (int i = 0; i < 8; ++i) {
    OctreeAABB child_bounds = octree_child_bounds(&bounds, i);
    new_root->children[i] =
        octree_node_create(child_bounds, 1, tree->max_items);
  }
  if (new_root->children[child_index]) {
    octree_node_destroy(new_root->children[child_index]);
    new_root->children[child_index] = tree->root;
    octree_node_add_depth(new_root->children[child_index], 1);
  }

  tree->root = new_root;
  return true;
}

bool octree_insert(Octree *tree, OctreeItem item) {
  if (!tree) {
    return false;
  }

  if (!octree_expand_root(tree, &item)) {
    return false;
  }

  return octree_node_insert(tree->root, item, tree->max_depth, tree->max_items);
}

bool octree_remove(Octree *tree, const OctreeItem *item) {
  if (!tree || !item) {
    return false;
  }
  return octree_node_remove(tree->root, item);
}

bool octree_update(Octree *tree, const OctreeItem *item,
                   OctreeAABB new_bounds) {
  if (!tree || !item) {
    return false;
  }

  OctreeItem updated = *item;
  updated.bounds = new_bounds;
  octree_remove(tree, item);
  return octree_insert(tree, updated);
}

void octree_query_frustum(const Octree *tree, const OctreeFrustum *frustum,
                          OctreeVisitFn visit, void *user) {
  if (!tree || !frustum || !visit) {
    return;
  }
  octree_query_node(tree->root, bounds_test_frustum, frustum, visit, user);
}

void octree_query_ray(const Octree *tree, const OctreeRay *ray,
                      OctreeVisitFn visit, void *user) {
  if (!tree || !ray || !visit) {
    return;
  }
  octree_query_node(tree->root, bounds_test_ray, ray, visit, user);
}

void octree_query_sphere(const Octree *tree, const float center[3],
                         float radius, OctreeVisitFn visit, void *user) {
  if (!tree || !center || !visit) {
    return;
  }
  SphereQuery sphere;
  memcpy(sphere.center, center, sizeof(float) * 3);
  sphere.radius = radius;
  octree_query_node(tree->root, bounds_test_sphere, &sphere, visit, user);
}

void octree_query_aabb(const Octree *tree, OctreeAABB bounds,
                       OctreeVisitFn visit, void *user) {
  if (!tree || !visit) {
    return;
  }
  octree_query_node(tree->root, bounds_test_aabb, &bounds, visit, user);
}

static uint64_t expand_bits(uint32_t v) {
  uint64_t x = v & 0x1fffff;
  x = (x | (x << 32)) & 0x1f00000000ffff;
  x = (x | (x << 16)) & 0x1f0000ff0000ff;
  x = (x | (x << 8)) & 0x100f00f00f00f00f;
  x = (x | (x << 4)) & 0x10c30c30c30c30c3;
  x = (x | (x << 2)) & 0x1249249249249249;
  return x;
}

uint64_t octree_linear_key(const float position[3], uint32_t depth,
                           const OctreeAABB *bounds) {
  if (!bounds || depth == 0) {
    return 0;
  }

  float size[3] = {bounds->max[0] - bounds->min[0],
                   bounds->max[1] - bounds->min[1],
                   bounds->max[2] - bounds->min[2]};
  float scale = (float)((1u << depth) - 1u);

  uint32_t xi = (uint32_t)(((position[0] - bounds->min[0]) / size[0]) * scale);
  uint32_t yi = (uint32_t)(((position[1] - bounds->min[1]) / size[1]) * scale);
  uint32_t zi = (uint32_t)(((position[2] - bounds->min[2]) / size[2]) * scale);

  return (expand_bits(xi) << 2) | (expand_bits(yi) << 1) | expand_bits(zi);
}

static void octree_dump_node(const OctreeNode *node, FILE *out) {
  if (!node || !out) {
    return;
  }

  fprintf(out, "depth=%u items=%u bounds=[%.2f %.2f %.2f]-[%.2f %.2f %.2f]\n",
          node->depth, node->item_count, node->bounds.min[0],
          node->bounds.min[1], node->bounds.min[2], node->bounds.max[0],
          node->bounds.max[1], node->bounds.max[2]);

  for (int i = 0; i < 8; ++i) {
    octree_dump_node(node->children[i], out);
  }
}

bool octree_debug_dump(const Octree *tree, const char *path) {
  if (!tree || !path) {
    return false;
  }

  FILE *out = fopen(path, "w");
  if (!out) {
    return false;
  }

  octree_dump_node(tree->root, out);
  fclose(out);
  return true;
}

static bool count_visit(const OctreeItem *item, void *user) {
  (void)item;
  uint32_t *count = (uint32_t *)user;
  (*count)++;
  return true;
}

bool octree_self_test(void) {
  OctreeAABB bounds = {{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
  Octree *tree = octree_create(bounds, 4, 2);
  if (!tree) {
    return false;
  }

  OctreeItem item = {{{1.0f, 1.0f, 1.0f}, {2.0f, 2.0f, 2.0f}}, (void *)1};
  octree_insert(tree, item);

  uint32_t count = 0;
  octree_query_aabb(tree, bounds, count_visit, &count);

  octree_destroy(tree);
  return count == 1;
}
