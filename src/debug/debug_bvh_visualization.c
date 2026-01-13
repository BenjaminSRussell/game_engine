// BVH Structure Definition for Debug Visualization implementation
#include "debug/debug_bvh_visualization.h"
#include <OpenGL/gl3.h>
#include <core/logger.h>
#include <math/mat4.h>
#include <math/math.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global debug BVH visualization instance
DebugBVHVisualization *g_debug_bvh_visualization = NULL;

// MARK: - Helper Functions

static u64 get_current_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)(ts.tv_sec * 1000000000LL + ts.tv_nsec);
}

static u64 get_current_time_ms(void) { return get_current_time_ns() / 1000000; }

static float min_f(float a, float b) { return a < b ? a : b; }
static float max_f(float a, float b) { return a > b ? a : b; }

static void vec3_min(const Vec3 *a, const Vec3 *b, Vec3 *result) {
  result->x = min_f(a->x, b->x);
  result->y = min_f(a->y, b->y);
  result->z = min_f(a->z, b->z);
}

static void vec3_max(const Vec3 *a, const Vec3 *b, Vec3 *result) {
  result->x = max_f(a->x, b->x);
  result->y = max_f(a->y, b->y);
  result->z = max_f(a->z, b->z);
}

static bool vec3_equal(const Vec3 *a, const Vec3 *b, float epsilon) {
  return fabs(a->x - b->x) < epsilon && fabs(a->y - b->y) < epsilon &&
         fabs(a->z - b->z) < epsilon;
}

static u32 create_bounding_box_shader(void) {
  const char *vertex_shader_source =
      "#version 330 core\n"
      "layout (location = 0) in vec3 aPosition;\n"
      "layout (location = 1) in vec4 aColor;\n"
      "\n"
      "uniform mat4 uViewMatrix;\n"
      "uniform mat4 uProjectionMatrix;\n"
      "uniform mat4 uModelMatrix;\n"
      "\n"
      "out vec4 Color;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * "
      "vec4(aPosition, 1.0);\n"
      "    Color = aColor;\n"
      "}\n";

  const char *fragment_shader_source = "#version 330 core\n"
                                       "in vec4 Color;\n"
                                       "out vec4 FragColor;\n"
                                       "\n"
                                       "void main()\n"
                                       "{\n"
                                       "    FragColor = Color;\n"
                                       "}\n";

  // Compile and link shaders (simplified)
  u32 shader_program = glCreateProgram();
  // In a real implementation, you would compile and link the shaders properly

  return shader_program;
}

// MARK: - BVH Tree Management

bool bvh_tree_init(BVHTree *tree, u32 max_triangles, u32 max_nodes) {
  if (!tree || max_triangles == 0 || max_nodes == 0) {
    LOG_ERROR("Invalid parameters for bvh_tree_init");
    return false;
  }

  memset(tree, 0, sizeof(BVHTree));

  // Allocate triangle array
  tree->triangles = (Triangle *)calloc(max_triangles, sizeof(Triangle));
  if (!tree->triangles) {
    LOG_ERROR("Failed to allocate triangles array");
    return false;
  }
  tree->triangle_capacity = max_triangles;

  // Allocate node array
  tree->nodes = (BVHNode *)calloc(max_nodes, sizeof(BVHNode));
  if (!tree->nodes) {
    free(tree->triangles);
    LOG_ERROR("Failed to allocate nodes array");
    return false;
  }
  tree->node_capacity = max_nodes;

  // Set default build parameters
  tree->split_strategy = BVH_SPLIT_SAH;
  tree->max_triangles_per_leaf = 8;
  tree->max_depth_limit = 32;
  tree->sah_traversal_cost = 1.0f;
  tree->sah_intersection_cost = 1.0f;

  // Set default visualization colors
  tree->leaf_color = (Vec4){0.0f, 1.0f, 0.0f, 0.3f};      // Green
  tree->internal_color = (Vec4){1.0f, 0.0f, 0.0f, 0.3f};  // Red
  tree->highlight_color = (Vec4){1.0f, 1.0f, 0.0f, 0.5f}; // Yellow

  LOG_INFO("BVH tree initialized (triangles: %u, nodes: %u)", max_triangles,
           max_nodes);
  return true;
}

void bvh_tree_shutdown(BVHTree *tree) {
  if (!tree) {
    return;
  }

  free(tree->triangles);
  free(tree->nodes);

  memset(tree, 0, sizeof(BVHTree));
  LOG_INFO("BVH tree shutdown");
}

bool bvh_tree_build(BVHTree *tree, Triangle *triangles, u32 triangle_count) {
  if (!tree || !triangles || triangle_count == 0) {
    return false;
  }

  u64 start_time = get_current_time_ns();

  // Copy triangles to tree
  if (triangle_count > tree->triangle_capacity) {
    LOG_ERROR("Triangle count exceeds capacity");
    return false;
  }

  memcpy(tree->triangles, triangles, triangle_count * sizeof(Triangle));
  tree->triangle_count = triangle_count;

  // Create root node
  tree->root = bvh_node_create(tree, BVH_NODE_INTERNAL);
  if (!tree->root) {
    return false;
  }

  // Build tree recursively
  tree->max_depth = 0;
  bool success = bvh_tree_build_recursive(tree, tree->root, tree->triangles,
                                          triangle_count, 0);

  tree->total_build_time_ns = get_current_time_ns() - start_time;

  if (success) {
    bvh_tree_calculate_statistics(tree);
    LOG_INFO(
        "BVH tree built successfully (nodes: %u, depth: %u, time: %.2f ms)",
        tree->node_count, tree->max_depth,
        tree->total_build_time_ns / 1000000.0f);
  } else {
    LOG_ERROR("Failed to build BVH tree");
  }

  return success;
  // TODO(Jules): Add support for dynamic BVH updates in bvh_tree_build.
}

bool bvh_tree_build_recursive(BVHTree *tree, BVHNode *node, Triangle *triangles,
                              u32 triangle_count, u32 depth) {
  if (!node || !triangles || triangle_count == 0) {
    return false;
  }

  node->depth = depth;
  node->subtree_triangle_count = triangle_count;
  tree->max_depth = max_u32(tree->max_depth, depth);

  // Calculate bounds for this node
  bvh_node_calculate_bounds(node, triangles, triangle_count);

  // Check if we should create a leaf node
  if (triangle_count <= tree->max_triangles_per_leaf ||
      depth >= tree->max_depth_limit) {
    node->type = BVH_NODE_LEAF;
    node->data.leaf.triangles = triangles;
    node->data.leaf.triangle_count = triangle_count;
    node->data.leaf.first_triangle_index = (u32)(triangles - tree->triangles);

    // Set leaf color
    node->debug_color = tree->leaf_color;
    return true;
  }

  // Create internal node
  node->type = BVH_NODE_INTERNAL;
  node->debug_color = tree->internal_color;

  // Choose split axis (longest axis)
  Vec3 extent = {node->bounds.max.x - node->bounds.min.x,
                 node->bounds.max.y - node->bounds.min.y,
                 node->bounds.max.z - node->bounds.min.z};

  u32 split_axis = 0;
  if (extent.y > extent.x)
    split_axis = 1;
  if (extent.z > extent.y && extent.z > extent.x)
    split_axis = 2;
  node->data.internal.split_axis = split_axis;

  // Split triangles
  u32 split_index = 0;
  switch (tree->split_strategy) {
  case BVH_SPLIT_MIDDLE:
    split_index = bvh_split_middle(tree, triangles, triangle_count, split_axis);
    break;
  case BVH_SPLIT_MEDIAN:
    split_index = bvh_split_median(tree, triangles, triangle_count, split_axis);
    break;
  case BVH_SPLIT_SAH:
    split_index = bvh_split_sah(tree, triangles, triangle_count, split_axis);
    break;
  case BVH_SPLIT_OBJECT_MEDIAN:
    split_index =
        bvh_split_object_median(tree, triangles, triangle_count, split_axis);
    break;
  case BVH_SPLIT_SPATIAL_MEDIAN:
    split_index =
        bvh_split_spatial_median(tree, triangles, triangle_count, split_axis);
    break;
  }

  // Ensure valid split
  if (split_index == 0 || split_index >= triangle_count) {
    split_index = triangle_count / 2;
  }

  // Create child nodes
  node->data.internal.left = bvh_node_create(tree, BVH_NODE_INTERNAL);
  node->data.internal.right = bvh_node_create(tree, BVH_NODE_INTERNAL);

  if (!node->data.internal.left || !node->data.internal.right) {
    return false;
  }

  // Build sub-trees
  bool left_success = bvh_tree_build_recursive(
      tree, node->data.internal.left, triangles, split_index, depth + 1);
  bool right_success = bvh_tree_build_recursive(
      tree, node->data.internal.right, triangles + split_index,
      triangle_count - split_index, depth + 1);

  return left_success && right_success;
}

// MARK: - BVH Node Operations

BVHNode *bvh_node_create(BVHTree *tree, BVHNodeType type) {
  if (!tree || tree->node_count >= tree->node_capacity) {
    return NULL;
  }

  BVHNode *node = &tree->nodes[tree->node_count++];
  // TODO(Jules): Add memory pool allocator for BVH nodes to reduce
  // fragmentation.
  memset(node, 0, sizeof(BVHNode));

  node->type = type;
  node->node_id = tree->node_count - 1;
  node->is_visible = true;
  node->is_highlighted = false;

  // TODO(Jules): Implement SAH cost visualization coloring in bvh_node_create.
  return node;
}

void bvh_node_calculate_bounds(BVHNode *node, Triangle *triangles,
                               u32 triangle_count) {
  if (!node || !triangles || triangle_count == 0) {
    return;
  }

  // Initialize bounds to first triangle
  node->bounds.min = triangles[0].vertices[0];
  node->bounds.max = triangles[0].vertices[0];

  // Expand bounds to include all triangle vertices
  for (u32 i = 0; i < triangle_count; i++) {
    for (u32 j = 0; j < 3; j++) {
      Vec3 vertex = triangles[i].vertices[j];

      node->bounds.min.x = min_f(node->bounds.min.x, vertex.x);
      node->bounds.min.y = min_f(node->bounds.min.y, vertex.y);
      node->bounds.min.z = min_f(node->bounds.min.z, vertex.z);

      node->bounds.max.x = max_f(node->bounds.max.x, vertex.x);
      node->bounds.max.y = max_f(node->bounds.max.y, vertex.y);
      node->bounds.max.z = max_f(node->bounds.max.z, vertex.z);
    }
  }

  // Calculate surface area
  node->surface_area = bvh_calculate_surface_area(&node->bounds);
}

// MARK: - BVH Splitting Strategies

u32 bvh_split_middle(BVHTree *tree, Triangle *triangles, u32 triangle_count,
                     u32 axis) {
  if (!triangles || triangle_count == 0) {
    return 0;
  }

  // Find middle point of bounds
  float middle = 0.0f;
  if (axis == 0)
    middle = (tree->nodes[0].bounds.min.x + tree->nodes[0].bounds.max.x) * 0.5f;
  else if (axis == 1)
    middle = (tree->nodes[0].bounds.min.y + tree->nodes[0].bounds.max.y) * 0.5f;
  else
    middle = (tree->nodes[0].bounds.min.z + tree->nodes[0].bounds.max.z) * 0.5f;

  // Partition triangles
  u32 left = 0;
  u32 right = triangle_count - 1;

  while (left <= right) {
    float center = 0.0f;
    if (axis == 0) {
      center = (triangles[left].vertices[0].x + triangles[left].vertices[1].x +
                triangles[left].vertices[2].x) /
               3.0f;
    } else if (axis == 1) {
      center = (triangles[left].vertices[0].y + triangles[left].vertices[1].y +
                triangles[left].vertices[2].y) /
               3.0f;
    } else {
      center = (triangles[left].vertices[0].z + triangles[left].vertices[1].z +
                triangles[left].vertices[2].z) /
               3.0f;
    }

    if (center < middle) {
      left++;
    } else {
      // Swap with right
      Triangle temp = triangles[left];
      triangles[left] = triangles[right];
      triangles[right] = temp;
      right--;
    }
  }

  return left;
}

u32 bvh_split_median(BVHTree *tree, Triangle *triangles, u32 triangle_count,
                     u32 axis) {
  if (!triangles || triangle_count <= 1) {
    return triangle_count / 2;
  }

  // Sort triangles by centroid along axis (simplified bubble sort for
  // demonstration)
  for (u32 i = 0; i < triangle_count - 1; i++) {
    for (u32 j = 0; j < triangle_count - i - 1; j++) {
      float center1 = 0.0f, center2 = 0.0f;

      if (axis == 0) {
        center1 = (triangles[j].vertices[0].x + triangles[j].vertices[1].x +
                   triangles[j].vertices[2].x) /
                  3.0f;
        center2 =
            (triangles[j + 1].vertices[0].x + triangles[j + 1].vertices[1].x +
             triangles[j + 1].vertices[2].x) /
            3.0f;
      } else if (axis == 1) {
        center1 = (triangles[j].vertices[0].y + triangles[j].vertices[1].y +
                   triangles[j].vertices[2].y) /
                  3.0f;
        center2 =
            (triangles[j + 1].vertices[0].y + triangles[j + 1].vertices[1].y +
             triangles[j + 1].vertices[2].y) /
            3.0f;
      } else {
        center1 = (triangles[j].vertices[0].z + triangles[j].vertices[1].z +
                   triangles[j].vertices[2].z) /
                  3.0f;
        center2 =
            (triangles[j + 1].vertices[0].z + triangles[j + 1].vertices[1].z +
             triangles[j + 1].vertices[2].z) /
            3.0f;
      }

      if (center1 > center2) {
        Triangle temp = triangles[j];
        triangles[j] = triangles[j + 1];
        triangles[j + 1] = temp;
      }
    }
  }

  return triangle_count / 2;
}

u32 bvh_split_sah(BVHTree *tree, Triangle *triangles, u32 triangle_count,
                  u32 axis) {
  // Simplified SAH implementation - for a real implementation, you would
  // evaluate multiple split positions and choose the one with minimum cost
  return bvh_split_median(tree, triangles, triangle_count, axis);
}

u32 bvh_split_object_median(BVHTree *tree, Triangle *triangles,
                            u32 triangle_count, u32 axis) {
  return bvh_split_median(tree, triangles, triangle_count, axis);
}

u32 bvh_split_spatial_median(BVHTree *tree, Triangle *triangles,
                             u32 triangle_count, u32 axis) {
  return bvh_split_middle(tree, triangles, triangle_count, axis);
}

// MARK: - BVH Traversal and Intersection

bool bvh_tree_intersect_ray(BVHTree *tree, const Ray *ray,
                            RayTriangleIntersection *result) {
  if (!tree || !ray || !result) {
    return false;
  }

  tree->total_traversal_count++;

  result->hit = false;
  result->t = ray->t_max;

  if (tree->root) {
    return bvh_node_intersect_ray(tree->root, ray, result);
  }

  return false;
}

bool bvh_node_intersect_ray(BVHNode *node, const Ray *ray,
                            RayTriangleIntersection *result) {
  if (!node || !ray || !result) {
    return false;
  }

  node->traversal_count++;

  // Check ray against node bounds
  Vec3 inv_dir = {1.0f / ray->direction.x, 1.0f / ray->direction.y,
                  1.0f / ray->direction.z};

  float t1 = (node->bounds.min.x - ray->origin.x) * inv_dir.x;
  float t2 = (node->bounds.max.x - ray->origin.x) * inv_dir.x;
  float tmin = min_f(t1, t2);
  float tmax = max_f(t1, t2);

  t1 = (node->bounds.min.y - ray->origin.y) * inv_dir.y;
  t2 = (node->bounds.max.y - ray->origin.y) * inv_dir.y;
  tmin = max_f(tmin, min_f(t1, t2));
  tmax = min_f(tmax, max_f(t1, t2));

  t1 = (node->bounds.min.z - ray->origin.z) * inv_dir.z;
  t2 = (node->bounds.max.z - ray->origin.z) * inv_dir.z;
  tmin = max_f(tmin, min_f(t1, t2));
  tmax = min_f(tmax, max_f(t1, t2));

  if (tmax < max_f(tmin, ray->t_min) || tmin > ray->t_max) {
    return false;
  }

  if (node->type == BVH_NODE_LEAF) {
    bool hit = false;
    for (u32 i = 0; i < node->data.leaf.triangle_count; i++) {
      RayTriangleIntersection triangle_result;
      if (bvh_intersect_ray_triangle(ray, &node->data.leaf.triangles[i],
                                     &triangle_result)) {
        if (triangle_result.t < result->t) {
          *result = triangle_result;
          hit = true;
        }
      }
    }
    return hit;
  } else {
    // Traverse children
    bool left_hit =
        bvh_node_intersect_ray(node->data.internal.left, ray, result);
    bool right_hit =
        bvh_node_intersect_ray(node->data.internal.right, ray, result);
    return left_hit || right_hit;
  }
}

bool bvh_intersect_ray_triangle(const Ray *ray, const Triangle *triangle,
                                RayTriangleIntersection *result) {
  if (!ray || !triangle || !result) {
    return false;
  }

  // Mller-Trumbore ray-triangle intersection algorithm
  Vec3 edge1 = vec3_sub(triangle->vertices[1], triangle->vertices[0]);
  Vec3 edge2 = vec3_sub(triangle->vertices[2], triangle->vertices[0]);
  Vec3 h = vec3_cross(ray->direction, edge2);
  float a = vec3_dot(edge1, h);

  if (fabs(a) < 0.0001f) {
    // TODO(Jules): Optimize bvh_intersect_ray_triangle using SIMD instructions.
    return false; // Ray is parallel to triangle
  }

  float f = 1.0f / a;
  Vec3 s = vec3_sub(ray->origin, triangle->vertices[0]);
  float u = f * vec3_dot(s, h);

  if (u < 0.0f || u > 1.0f) {
    return false;
  }

  Vec3 q = vec3_cross(s, edge1);
  float v = f * vec3_dot(ray->direction, q);

  if (v < 0.0f || u + v > 1.0f) {
    return false;
  }

  float t = f * vec3_dot(edge2, q);

  if (t > ray->t_min && t < ray->t_max && t < result->t) {
    result->hit = true;
    result->t = t;
    result->hit_point = vec3_add(ray->origin, vec3_scale(ray->direction, t));
    result->normal = triangle->normal;
    result->barycentric.x = 1.0f - u - v;
    result->barycentric.y = u;
    result->barycentric.z = v;
    result->triangle_id = triangle->triangle_id;
    result->material_id = triangle->material_id;
    return true;
  }

  return false;
}

// MARK: - BVH Statistics and Analysis

void bvh_tree_calculate_statistics(BVHTree *tree) {
  if (!tree) {
    return;
  }

  u32 leaf_count = 0;
  u32 internal_count = 0;
  u64 total_depth = 0;
  u64 total_tris_in_leaves = 0;

  for (u32 i = 0; i < tree->node_count; i++) {
    BVHNode *node = &tree->nodes[i];
    total_depth += node->depth;

    if (node->type == BVH_NODE_LEAF) {
      leaf_count++;
      total_tris_in_leaves += node->data.leaf.triangle_count;
    } else {
      internal_count++;
    }
  }

  tree->average_depth =
      tree->node_count > 0 ? (float)total_depth / tree->node_count : 0.0f;
  tree->leaf_utilization = leaf_count > 0
                               ? (float)total_tris_in_leaves /
                                     (leaf_count * tree->max_triangles_per_leaf)
                               : 0.0f;
}

float bvh_calculate_surface_area(const BoundingBox *box) {
  if (!box) {
    return 0.0f;
  }

  float x = box->max.x - box->min.x;
  float y = box->max.y - box->min.y;
  float z = box->max.z - box->min.z;

  return 2.0f * (x * y + y * z + z * x);
}

void bvh_tree_print_statistics(BVHTree *tree) {
  if (!tree) {
    printf("BVH tree is NULL\n");
    return;
  }

  u32 leaf_count = 0;
  u32 internal_count = 0;

  for (u32 i = 0; i < tree->node_count; i++) {
    if (tree->nodes[i].type == BVH_NODE_LEAF) {
      leaf_count++;
    } else {
      internal_count++;
    }
  }

  printf("=== BVH TREE STATISTICS ===\n");
  printf("Total Nodes: %u\n", tree->node_count);
  printf("Leaf Nodes: %u\n", leaf_count);
  printf("Internal Nodes: %u\n", internal_count);
  printf("Max Depth: %u\n", tree->max_depth);
  printf("Average Depth: %.2f\n", tree->average_depth);
  printf("Total Triangles: %u\n", tree->triangle_count);
  printf("Build Time: %.2f ms\n", tree->total_build_time_ns / 1000000.0f);
  printf("Leaf Utilization: %.2f%%\n", tree->leaf_utilization * 100.0f);
  printf("Split Strategy: %u\n", tree->split_strategy);
  printf("Max Triangles per Leaf: %u\n", tree->max_triangles_per_leaf);
  printf("===========================\n");
}

// MARK: - Debug Visualization System

bool debug_bvh_visualization_init(DebugBVHVisualization *viz, u32 max_trees) {
  if (!viz || max_trees == 0) {
    LOG_ERROR("Invalid parameters for debug_bvh_visualization_init");
    return false;
  }

  memset(viz, 0, sizeof(DebugBVHVisualization));

  // Allocate trees array
  viz->bvh_trees = (BVHTree *)calloc(max_trees, sizeof(BVHTree));
  if (!viz->bvh_trees) {
    LOG_ERROR("Failed to allocate BVH trees array");
    return false;
  }
  viz->tree_capacity = max_trees;

  // Create shader
  viz->bounding_box_shader = create_bounding_box_shader();

  // Set default settings
  viz->wireframe_mode = false;
  viz->show_node_ids = false;
  viz->show_statistics = true;
  viz->animate_traversal = false;
  viz->line_width = 1.0f;
  viz->background_color = (Vec4){0.1f, 0.1f, 0.1f, 1.0f};
  viz->traversal_speed = 1.0f;

  // Generate OpenGL buffers
  glGenBuffers(1, &viz->line_vertex_buffer);
  glGenBuffers(1, &viz->line_index_buffer);
  glGenBuffers(1, &viz->triangle_vertex_buffer);
  glGenBuffers(1, &viz->triangle_index_buffer);

  // Set global instance
  g_debug_bvh_visualization = viz;

  LOG_INFO("Debug BVH visualization initialized (trees: %u)", max_trees);
  return true;
}

void debug_bvh_visualization_shutdown(DebugBVHVisualization *viz) {
  if (!viz) {
    return;
  }

  // Clean up trees
  if (viz->bvh_trees) {
    for (u32 i = 0; i < viz->tree_count; i++) {
      bvh_tree_shutdown(&viz->bvh_trees[i]);
    }
    free(viz->bvh_trees);
  }

  // Delete OpenGL resources
  if (viz->bounding_box_shader) {
    glDeleteProgram(viz->bounding_box_shader);
  }

  if (viz->line_vertex_buffer) {
    glDeleteBuffers(1, &viz->line_vertex_buffer);
  }

  if (viz->line_index_buffer) {
    glDeleteBuffers(1, &viz->line_index_buffer);
  }

  if (viz->triangle_vertex_buffer) {
    glDeleteBuffers(1, &viz->triangle_vertex_buffer);
  }

  if (viz->triangle_index_buffer) {
    glDeleteBuffers(1, &viz->triangle_index_buffer);
  }

  // Clear global instance
  g_debug_bvh_visualization = NULL;

  memset(viz, 0, sizeof(DebugBVHVisualization));
  LOG_INFO("Debug BVH visualization shutdown");
}

BVHTree *debug_bvh_add_tree(DebugBVHVisualization *viz, u32 max_triangles) {
  if (!viz || viz->tree_count >= viz->tree_capacity) {
    return NULL;
  }

  BVHTree *tree = &viz->bvh_trees[viz->tree_count++];
  if (!bvh_tree_init(tree, max_triangles, max_triangles * 2)) {
    viz->tree_count--;
    return NULL;
  }

  LOG_DEBUG("Added BVH tree to visualization");
  return tree;
}

bool debug_bvh_build_tree(DebugBVHVisualization *viz, BVHTree *tree,
                          Triangle *triangles, u32 triangle_count) {
  if (!viz || !tree || !triangles || triangle_count == 0) {
    return false;
  }

  bool success = bvh_tree_build(tree, triangles, triangle_count);

  if (success && viz->on_tree_built) {
    viz->on_tree_built(tree);
  }

  return success;
}

// MARK: - BVH Visualization Rendering

void debug_bvh_render(DebugBVHVisualization *viz, const Mat4 *view_matrix,
                      const Mat4 *projection_matrix) {
  if (!viz || !view_matrix || !projection_matrix) {
    return;
  }

  viz->last_render_time_ms = get_current_time_ms();
  viz->rendered_nodes_last_frame = 0;
  viz->rendered_triangles_last_frame = 0;

  // Enable wireframe mode if needed
  if (viz->wireframe_mode) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

  // Use shader
  glUseProgram(viz->bounding_box_shader);

  // Set uniforms
  GLint view_loc =
      glGetUniformLocation(viz->bounding_box_shader, "uViewMatrix");
  GLint proj_loc =
      glGetUniformLocation(viz->bounding_box_shader, "uProjectionMatrix");

  glUniformMatrix4fv(view_loc, 1, GL_FALSE, (const float *)view_matrix);
  glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (const float *)projection_matrix);

  // Render all trees
  for (u32 i = 0; i < viz->tree_count; i++) {
    if (viz->bvh_trees[i].root) {
      debug_bvh_render_tree(viz, &viz->bvh_trees[i], view_matrix,
                            projection_matrix);
    }
  }

  // Restore polygon mode
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void debug_bvh_render_tree(DebugBVHVisualization *viz, BVHTree *tree,
                           const Mat4 *view_matrix,
                           const Mat4 *projection_matrix) {
  if (!viz || !tree || !tree->root) {
    return;
  }

  debug_bvh_render_node(viz, tree->root, view_matrix, projection_matrix);
  // TODO(Jules): Add frustum culling to debug_bvh_render_tree to improve
  // performance.
}

void debug_bvh_render_node(DebugBVHVisualization *viz, BVHNode *node,
                           const Mat4 *view_matrix,
                           const Mat4 *projection_matrix) {
  if (!viz || !node || !node->is_visible) {
    return;
  }

  viz->rendered_nodes_last_frame++;

  // Render bounding box
  if (viz->show_bounds) {
    Vec4 color =
        node->is_highlighted ? tree->highlight_color : node->debug_color;
    debug_bvh_render_bounding_box(viz, &node->bounds, &color, mat4_identity());
  }

  // Render children if internal node
  if (node->type == BVH_NODE_INTERNAL) {
    debug_bvh_render_node(viz, node->data.internal.left, view_matrix,
                          projection_matrix);
    debug_bvh_render_node(viz, node->data.internal.right, view_matrix,
                          projection_matrix);
  } else {
    // Render triangles in leaf node
    viz->rendered_triangles_last_frame += node->data.leaf.triangle_count;
  }
}

void debug_bvh_render_bounding_box(DebugBVHVisualization *viz,
                                   const BoundingBox *box, const Vec4 *color,
                                   const Mat4 *model_matrix) {
  if (!viz || !box || !color) {
    return;
  }

  // Generate box vertices
  Vec3 vertices[8] = {{box->min.x, box->min.y, box->min.z},
                      {box->max.x, box->min.y, box->min.z},
                      {box->max.x, box->max.y, box->min.z},
                      {box->min.x, box->max.y, box->min.z},
                      {box->min.x, box->min.y, box->max.z},
                      {box->max.x, box->min.y, box->max.z},
                      {box->max.x, box->max.y, box->max.z},
                      {box->min.x, box->max.y, box->max.z}};

  // Generate line indices for wireframe box
  u32 indices[24] = {
      0, 1, 1, 2, 2, 3, 3, 0, // Front face
      4, 5, 5, 6, 6, 7, 7, 4, // Back face
      0, 4, 1, 5, 2, 6, 3, 7  // Connecting edges
  };

  // Upload vertex data
  glBindBuffer(GL_ARRAY_BUFFER, viz->line_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

  // Upload index data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, viz->line_index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_DYNAMIC_DRAW);

  // Set vertex attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void *)0);
  glEnableVertexAttribArray(0);

  // Set color uniform
  GLint color_loc = glGetUniformLocation(viz->bounding_box_shader, "uColor");
  glUniform4f(color_loc, color->x, color->y, color->z, color->w);

  // Set model matrix
  GLint model_loc =
      glGetUniformLocation(viz->bounding_box_shader, "uModelMatrix");
  glUniformMatrix4fv(model_loc, 1, GL_FALSE, (const float *)model_matrix);

  // Draw lines
  glLineWidth(viz->line_width);
  glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
}

// MARK: - Utility Functions

void debug_bvh_get_statistics(DebugBVHVisualization *viz, u32 *total_nodes,
                              u32 *leaf_nodes, u32 *internal_nodes,
                              u32 *total_triangles) {
  if (!viz) {
    return;
  }

  u32 total = 0, leaf = 0, internal = 0, tris = 0;

  for (u32 i = 0; i < viz->tree_count; i++) {
    BVHTree *tree = &viz->bvh_trees[i];
    total += tree->node_count;
    tris += tree->triangle_count;

    for (u32 j = 0; j < tree->node_count; j++) {
      if (tree->nodes[j].type == BVH_NODE_LEAF) {
        leaf++;
      } else {
        internal++;
      }
    }
  }

  if (total_nodes)
    *total_nodes = total;
  if (leaf_nodes)
    *leaf_nodes = leaf;
  if (internal_nodes)
    *internal_nodes = internal;
  if (total_triangles)
    *total_triangles = tris;
}

void debug_bvh_print_statistics(DebugBVHVisualization *viz) {
  if (!viz) {
    printf("Debug BVH visualization is NULL\n");
    return;
  }

  u32 total_nodes, leaf_nodes, internal_nodes, total_triangles;
  debug_bvh_get_statistics(viz, &total_nodes, &leaf_nodes, &internal_nodes,
                           &total_triangles);

  printf("=== DEBUG BVH VISUALIZATION STATISTICS ===\n");
  printf("Total Trees: %u\n", viz->tree_count);
  printf("Total Nodes: %u\n", total_nodes);
  printf("Leaf Nodes: %u\n", leaf_nodes);
  printf("Internal Nodes: %u\n", internal_nodes);
  printf("Total Triangles: %u\n", total_triangles);
  printf("Rendered Nodes (last frame): %u\n", viz->rendered_nodes_last_frame);
  printf("Rendered Triangles (last frame): %u\n",
         viz->rendered_triangles_last_frame);
  printf("Wireframe Mode: %s\n", viz->wireframe_mode ? "Enabled" : "Disabled");
  printf("Show Bounds: %s\n", viz->show_bounds ? "Enabled" : "Disabled");
  printf("Line Width: %.2f\n", viz->line_width);
  printf("=========================================\n");
}
