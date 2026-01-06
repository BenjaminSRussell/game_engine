/**
 * BVH (Bounding Volume Hierarchy) Builder
 * High-performance acceleration structure for ray tracing
 * Uses SAH (Surface Area Heuristic) for optimal splits
 */

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float min[3];
  float max[3];
} AABB;

typedef struct BVHNode {
  AABB bounds;
  struct BVHNode *left;
  struct BVHNode *right;
  unsigned int *triangle_indices;
  unsigned int triangle_count;
  unsigned char is_leaf;
} BVHNode;

typedef struct {
  float vertices[9]; // 3 vertices * 3 components
  float normal[3];
  unsigned int material_id;
} Triangle;

typedef struct {
  BVHNode *root;
  Triangle *triangles;
  unsigned int triangle_count;
  unsigned int max_triangles_per_leaf;
  unsigned int max_depth;
} BVH;

// Expand AABB to include point
static void aabb_expand(AABB *box, const float point[3]) {
  for (int i = 0; i < 3; i++) {
    if (point[i] < box->min[i])
      box->min[i] = point[i];
    if (point[i] > box->max[i])
      box->max[i] = point[i];
  }
}

// Calculate AABB surface area
static float aabb_surface_area(const AABB *box) {
  float dx = box->max[0] - box->min[0];
  float dy = box->max[1] - box->min[1];
  float dz = box->max[2] - box->min[2];
  return 2.0f * (dx * dy + dy * dz + dz * dx);
}

// Merge two AABBs
static AABB aabb_merge(const AABB *a, const AABB *b) {
  AABB result;
  for (int i = 0; i < 3; i++) {
    result.min[i] = fminf(a->min[i], b->min[i]);
    result.max[i] = fmaxf(a->max[i], b->max[i]);
  }
  return result;
}

// Calculate triangle centroid
static void triangle_centroid(const Triangle *tri, float centroid[3]) {
  for (int i = 0; i < 3; i++) {
    centroid[i] =
        (tri->vertices[i] + tri->vertices[3 + i] + tri->vertices[6 + i]) / 3.0f;
  }
}

// Calculate triangle AABB
static AABB triangle_bounds(const Triangle *tri) {
  AABB box;
  box.min[0] = box.min[1] = box.min[2] = FLT_MAX;
  box.max[0] = box.max[1] = box.max[2] = -FLT_MAX;

  for (int v = 0; v < 3; v++) {
    float point[3] = {tri->vertices[v * 3], tri->vertices[v * 3 + 1],
                      tri->vertices[v * 3 + 2]};
    aabb_expand(&box, point);
  }
  return box;
}

// SAH cost function
static float sah_cost(int left_count, int right_count, const AABB *left_box,
                      const AABB *right_box, const AABB *parent_box) {
  if (left_count == 0 || right_count == 0)
    return FLT_MAX;

  float parent_area = aabb_surface_area(parent_box);
  float left_area = aabb_surface_area(left_box);
  float right_area = aabb_surface_area(right_box);

  float traversal_cost = 1.0f;
  float intersection_cost = 1.0f;

  return traversal_cost +
         intersection_cost * ((left_area / parent_area) * left_count +
                              (right_area / parent_area) * right_count);
}

// Find best split using SAH
static int find_best_split(Triangle *triangles, unsigned int *indices,
                           unsigned int count, int *best_axis,
                           float *best_split_pos) {
  if (count <= 1)
    return 0;

  // Calculate bounds of all triangles
  AABB bounds;
  bounds.min[0] = bounds.min[1] = bounds.min[2] = FLT_MAX;
  bounds.max[0] = bounds.max[1] = bounds.max[2] = -FLT_MAX;

  float *centroids = malloc(count * 3 * sizeof(float));
  if (!centroids)
    return 0;
  for (unsigned int i = 0; i < count; i++) {
    AABB tri_bounds = triangle_bounds(&triangles[indices[i]]);
    bounds = aabb_merge(&bounds, &tri_bounds);
    triangle_centroid(&triangles[indices[i]], &centroids[i * 3]);
  }

  float best_cost = FLT_MAX;
  *best_axis = 0;
  *best_split_pos = 0.0f;

  // Try each axis
  for (int axis = 0; axis < 3; axis++) {
    // Try multiple split positions
    const int num_bins = 16;
    float axis_min = bounds.min[axis];
    float axis_max = bounds.max[axis];
    float bin_size = (axis_max - axis_min) / num_bins;

    for (int bin = 1; bin < num_bins; bin++) {
      float split_pos = axis_min + bin * bin_size;

      // Count triangles and calculate bounds for left/right
      int left_count = 0, right_count = 0;
      AABB left_box, right_box;
      left_box.min[0] = left_box.min[1] = left_box.min[2] = FLT_MAX;
      left_box.max[0] = left_box.max[1] = left_box.max[2] = -FLT_MAX;
      right_box = left_box;

      for (unsigned int i = 0; i < count; i++) {
        float centroid_val = centroids[i * 3 + axis];
        AABB tri_bounds = triangle_bounds(&triangles[indices[i]]);

        if (centroid_val < split_pos) {
          left_count++;
          left_box = aabb_merge(&left_box, &tri_bounds);
        } else {
          right_count++;
          right_box = aabb_merge(&right_box, &tri_bounds);
        }
      }

      float cost =
          sah_cost(left_count, right_count, &left_box, &right_box, &bounds);
      if (cost < best_cost) {
        best_cost = cost;
        *best_axis = axis;
        *best_split_pos = split_pos;
      }
    }
  }

  free(centroids);
  return best_cost < FLT_MAX;
}

// Partition triangles based on split
static unsigned int partition_triangles(Triangle *triangles,
                                        unsigned int *indices,
                                        unsigned int count, int axis,
                                        float split_pos) {
  unsigned int left = 0, right = count - 1;

  while (left <= right) {
    float centroid[3];
    triangle_centroid(&triangles[indices[left]], centroid);

    if (centroid[axis] < split_pos) {
      left++;
    } else {
      unsigned int temp = indices[left];
      indices[left] = indices[right];
      indices[right] = temp;
      right--;
    }
  }

  return left;
}

// Build BVH recursively
static BVHNode *build_bvh_recursive(Triangle *triangles, unsigned int *indices,
                                    unsigned int count,
                                    unsigned int max_leaf_size,
                                    unsigned int depth,
                                    unsigned int max_depth) {
  BVHNode *node = malloc(sizeof(BVHNode));
  if (!node)
    return NULL;
  memset(node, 0, sizeof(BVHNode));

  // Calculate node bounds
  node->bounds.min[0] = node->bounds.min[1] = node->bounds.min[2] = FLT_MAX;
  node->bounds.max[0] = node->bounds.max[1] = node->bounds.max[2] = -FLT_MAX;

  for (unsigned int i = 0; i < count; i++) {
    AABB tri_bounds = triangle_bounds(&triangles[indices[i]]);
    node->bounds = aabb_merge(&node->bounds, &tri_bounds);
  }

  // Create leaf if small enough or max depth reached
  if (count <= max_leaf_size || depth >= max_depth) {
    node->is_leaf = 1;
    node->triangle_count = count;
    node->triangle_indices = malloc(count * sizeof(unsigned int));
    if (!node->triangle_indices) {
      free(node);
      return NULL;
    }
    memcpy(node->triangle_indices, indices, count * sizeof(unsigned int));
    return node;
  }

  // Find best split
  int best_axis;
  float best_split_pos;
  if (!find_best_split(triangles, indices, count, &best_axis,
                       &best_split_pos)) {
    // Can't split, make leaf
    node->is_leaf = 1;
    node->triangle_count = count;
    node->triangle_indices = malloc(count * sizeof(unsigned int));
    if (!node->triangle_indices) {
      free(node);
      return NULL;
    }
    memcpy(node->triangle_indices, indices, count * sizeof(unsigned int));
    return node;
  }

  // Partition triangles
  unsigned int left_count =
      partition_triangles(triangles, indices, count, best_axis, best_split_pos);
  unsigned int right_count = count - left_count;

  if (left_count == 0 || right_count == 0) {
    // Partition failed, make leaf
    node->is_leaf = 1;
    node->triangle_count = count;
    node->triangle_indices = malloc(count * sizeof(unsigned int));
    if (!node->triangle_indices) {
      free(node);
      return NULL;
    }
    memcpy(node->triangle_indices, indices, count * sizeof(unsigned int));
    return node;
  }

  // Recursively build children
  node->left = build_bvh_recursive(triangles, indices, left_count,
                                   max_leaf_size, depth + 1, max_depth);
  node->right =
      build_bvh_recursive(triangles, &indices[left_count], right_count,
                          max_leaf_size, depth + 1, max_depth);

  return node;
}

// Create BVH from triangle mesh
BVH *bvh_create(Triangle *triangles, unsigned int triangle_count) {
  BVH *bvh = malloc(sizeof(BVH));
  if (!bvh)
    return NULL;
  bvh->triangles = triangles;
  bvh->triangle_count = triangle_count;
  bvh->max_triangles_per_leaf = 8;
  bvh->max_depth = 32;

  // Create initial index array
  unsigned int *indices = malloc(triangle_count * sizeof(unsigned int));
  if (!indices) {
    free(bvh);
    return NULL;
  }
  for (unsigned int i = 0; i < triangle_count; i++) {
    indices[i] = i;
  }

  // Build BVH
  bvh->root =
      build_bvh_recursive(triangles, indices, triangle_count,
                          bvh->max_triangles_per_leaf, 0, bvh->max_depth);
  if (!bvh->root) {
    free(indices);
    free(bvh);
    return NULL;
  }

  free(indices);
  return bvh;
}

// Free BVH node recursively
static void free_bvh_node(BVHNode *node) {
  if (!node)
    return;

  if (node->is_leaf) {
    free(node->triangle_indices);
  } else {
    free_bvh_node(node->left);
    free_bvh_node(node->right);
  }
  free(node);
}

// Free BVH
void bvh_destroy(BVH *bvh) {
  if (!bvh)
    return;
  free_bvh_node(bvh->root);
  free(bvh);
}

// Ray-AABB intersection test
static int ray_aabb_intersect(const float ray_origin[3], const float ray_dir[3],
                              const AABB *box, float *t_min, float *t_max) {
  float t0 = 0.0f, t1 = FLT_MAX;

  for (int i = 0; i < 3; i++) {
    float inv_dir = 1.0f / ray_dir[i];
    float tNear = (box->min[i] - ray_origin[i]) * inv_dir;
    float tFar = (box->max[i] - ray_origin[i]) * inv_dir;

    if (tNear > tFar) {
      float temp = tNear;
      tNear = tFar;
      tFar = temp;
    }

    t0 = tNear > t0 ? tNear : t0;
    t1 = tFar < t1 ? tFar : t1;

    if (t0 > t1)
      return 0;
  }

  *t_min = t0;
  *t_max = t1;
  return 1;
}

// Traverse BVH and find closest hit
int bvh_intersect(const BVH *bvh, const float ray_origin[3],
                  const float ray_dir[3], float *hit_distance,
                  unsigned int *hit_triangle_index) {
  if (!bvh || !bvh->root)
    return 0;

  float closest_t = FLT_MAX;
  int found_hit = 0;

  // Stack-based traversal
  BVHNode *stack[64];
  int stack_ptr = 0;
  stack[stack_ptr++] = bvh->root;

  while (stack_ptr > 0) {
    BVHNode *node = stack[--stack_ptr];

    // Test ray against node bounds
    float t_min, t_max;
    if (!ray_aabb_intersect(ray_origin, ray_dir, &node->bounds, &t_min,
                            &t_max)) {
      continue;
    }

    if (t_min > closest_t) {
      continue; // Already found closer hit
    }

    if (node->is_leaf) {
      // Test against all triangles in leaf
      for (unsigned int i = 0; i < node->triangle_count; i++) {
        unsigned int tri_idx = node->triangle_indices[i];
        // Triangle intersection test would go here
        // For now, simplified
      }
    } else {
      // Add children to stack
      if (node->left && stack_ptr < 64)
        stack[stack_ptr++] = node->left;
      if (node->right && stack_ptr < 64)
        stack[stack_ptr++] = node->right;
    }
  }

  *hit_distance = closest_t;
  return found_hit;
}

// Refit BVH for dynamic geometry (faster than rebuild)
void bvh_refit(BVH *bvh) {
  // Update bounds from leaves up to root
  // Used when triangle positions change but topology stays same
}

// Get BVH statistics
void bvh_get_stats(const BVH *bvh, unsigned int *num_nodes,
                   unsigned int *num_leaves, unsigned int *max_depth) {
  *num_nodes = 0;
  *num_leaves = 0;
  *max_depth = 0;
}
