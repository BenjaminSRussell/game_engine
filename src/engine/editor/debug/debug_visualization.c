#include "editor/debug/debug_visualization.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Placeholder light system structure
typedef struct {
  simd_float3 position;
  simd_float3 direction;
  simd_float4 color;
  float intensity;
  float radius;
  int type; // 0 = directional, 1 = point, 2 = spot
  bool enabled;
} debug_light_t;

// Shadow cascade structure (moved from local scope)
typedef struct {
  simd_float4x4 view_proj_matrix;
  float cascade_split;
  int cascade_index;
  bool enabled;
} shadow_cascade_t;

// Forward declarations
static void debug_viz_draw_directional_light(debug_renderer_t *dbg,
                                             const debug_light_t *light,
                                             simd_float4 color);
static void debug_viz_draw_point_light(debug_renderer_t *dbg,
                                       const debug_light_t *light,
                                       simd_float4 color);
static void debug_viz_draw_spot_light(debug_renderer_t *dbg,
                                      const debug_light_t *light,
                                      simd_float4 color);
static void debug_viz_draw_spot_cone(debug_renderer_t *dbg,
                                     simd_float3 position,
                                     simd_float3 direction, float length,
                                     float angle_degrees, simd_float4 color);
static void debug_draw_sphere_wireframe(debug_renderer_t *dbg,
                                        simd_float3 center, float radius,
                                        simd_float4 color);
static void debug_viz_draw_shadow_cascade(debug_renderer_t *dbg,
                                          const shadow_cascade_t *cascade,
                                          const debug_viz_config_t *config);
static void debug_viz_draw_frustum_faces(debug_renderer_t *dbg,
                                         const simd_float3 corners[8],
                                         simd_float4 color);
static void debug_viz_draw_split_plane(debug_renderer_t *dbg,
                                       const shadow_cascade_t *cascade,
                                       const debug_viz_config_t *config);
static void debug_draw_quad(debug_renderer_t *dbg, simd_float3 bl,
                            simd_float3 br, simd_float3 tr, simd_float3 tl,
                            simd_float4 color);
static void debug_viz_draw_bvh_split_plane(debug_renderer_t *dbg,
                                           const bvh_node_t *node,
                                           simd_float4 color);
static bvh_node_t *debug_viz_create_demo_bvh(void);
static void debug_viz_cleanup_demo_bvh(bvh_node_t *node);

// Helper for identity matrix
static inline simd_float4x4 debug_matrix_identity(void) {
  return (simd_float4x4){.columns[0] = {1, 0, 0, 0},
                         .columns[1] = {0, 1, 0, 0},
                         .columns[2] = {0, 0, 1, 0},
                         .columns[3] = {0, 0, 0, 1}};
}

void debug_viz_init_config(debug_viz_config_t *config) {
  if (!config)
    return;

  config->show_lights = false;
  config->show_shadow_frustums = false;
  config->show_probe_grid = false;
  config->show_cluster_grid = false;
  config->show_bvh = false;
  config->show_gbuffer_channels = false;

  config->light_sphere_radius = 0.2f;
  config->light_color = (simd_float4){1.0f, 1.0f, 0.0f, 0.8f}; // Yellow
  config->shadow_frustum_color =
      (simd_float4){1.0f, 0.0f, 1.0f, 0.5f};                   // Magenta
  config->probe_color = (simd_float4){0.0f, 1.0f, 1.0f, 0.6f}; // Cyan
  config->bvh_color = (simd_float4){0.0f, 1.0f, 0.0f, 0.4f};   // Green
}

void debug_viz_draw_lights(debug_renderer_t *dbg, const light_system_t *lights,
                           const debug_viz_config_t *config) {
  if (!dbg || !config || !config->show_lights)
    return;

  debug_light_t demo_lights[8] = {// Directional light (sun)
                                  {{10.0f, 20.0f, 10.0f},
                                   {-0.5f, -0.7f, -0.5f},
                                   {1.0f, 0.95f, 0.8f, 1.0f},
                                   1.0f,
                                   0.0f,
                                   0,
                                   true},
                                  // Point lights
                                  {{5.0f, 2.0f, 5.0f},
                                   {0, -1, 0},
                                   {1.0f, 0.2f, 0.2f, 1.0f},
                                   10.0f,
                                   3.0f,
                                   1,
                                   true},
                                  {{-5.0f, 2.0f, 5.0f},
                                   {0, -1, 0},
                                   {0.2f, 1.0f, 0.2f, 1.0f},
                                   10.0f,
                                   3.0f,
                                   1,
                                   true},
                                  {{5.0f, 2.0f, -5.0f},
                                   {0, -1, 0},
                                   {0.2f, 0.2f, 1.0f, 1.0f},
                                   10.0f,
                                   3.0f,
                                   1,
                                   true},
                                  {{-5.0f, 2.0f, -5.0f},
                                   {0, -1, 0},
                                   {1.0f, 1.0f, 0.2f, 1.0f},
                                   10.0f,
                                   3.0f,
                                   1,
                                   true},
                                  // Spot lights
                                  {{0.0f, 5.0f, 0.0f},
                                   {0, -1, 0},
                                   {1.0f, 0.5f, 0.0f, 1.0f},
                                   15.0f,
                                   5.0f,
                                   2,
                                   true},
                                  {{8.0f, 3.0f, 0.0f},
                                   {-0.7f, -0.3f, 0},
                                   {0.0f, 0.5f, 1.0f, 1.0f},
                                   12.0f,
                                   4.0f,
                                   2,
                                   true},
                                  {{-8.0f, 3.0f, 0.0f},
                                   {0.7f, -0.3f, 0},
                                   {1.0f, 0.0f, 0.5f, 1.0f},
                                   12.0f,
                                   4.0f,
                                   2,
                                   true}};

  for (int i = 0; i < 8; i++) {
    debug_light_t *light = &demo_lights[i];
    if (!light->enabled)
      continue;

    simd_float4 viz_color = light->color * light->intensity;
    viz_color.w = 1.0f;

    switch (light->type) {
    case 0:
      debug_viz_draw_directional_light(dbg, light, viz_color);
      break;
    case 1:
      debug_viz_draw_point_light(dbg, light, viz_color);
      break;
    case 2:
      debug_viz_draw_spot_light(dbg, light, viz_color);
      break;
    }

    char light_label[64];
    switch (light->type) {
    case 0:
      snprintf(light_label, sizeof(light_label), "DIR");
      break;
    case 1:
      snprintf(light_label, sizeof(light_label), "POINT");
      break;
    case 2:
      snprintf(light_label, sizeof(light_label), "SPOT");
      break;
    default:
      snprintf(light_label, sizeof(light_label), "LIGHT");
      break;
    }

    simd_float3 label_pos = light->position + simd_make_float3(0, 0.5f, 0);
    debug_draw_text_3d(dbg, label_pos, light_label, viz_color);
  }
}

static void debug_viz_draw_directional_light(debug_renderer_t *dbg,
                                             const debug_light_t *light,
                                             simd_float4 color) {
  simd_float3 sun_pos = light->position;
  float sun_radius = 0.5f;
  debug_draw_sphere(dbg, sun_pos, sun_radius, color);

  simd_float3 normalized_dir = simd_normalize(light->direction);
  float ray_length = 3.0f;

  for (int i = 0; i < 8; i++) {
    float angle = (float)i * M_PI * 2.0f / 8.0f;
    simd_float3 offset = simd_make_float3(cosf(angle) * sun_radius * 1.5f,
                                          sinf(angle) * sun_radius * 0.3f,
                                          sinf(angle) * sun_radius * 1.5f);
    simd_float3 ray_start = sun_pos + offset;
    simd_float3 ray_end = ray_start + normalized_dir * ray_length;

    debug_draw_line(dbg, ray_start, ray_end, color);

    simd_float3 arrowhead_base = ray_end - normalized_dir * 0.2f;
    simd_float3 arrowhead_left = arrowhead_base + simd_make_float3(0.1f, 0, 0);
    simd_float3 arrowhead_right = arrowhead_base - simd_make_float3(0.1f, 0, 0);

    debug_draw_line(dbg, ray_end, arrowhead_left, color);
    debug_draw_line(dbg, ray_end, arrowhead_right, color);
  }
}

static void debug_viz_draw_point_light(debug_renderer_t *dbg,
                                       const debug_light_t *light,
                                       simd_float4 color) {
  debug_draw_sphere(dbg, light->position, 0.2f, color);
  simd_float4 faded_color = color;
  faded_color.w *= 0.3f;
  debug_draw_sphere_wireframe(dbg, light->position, light->radius, faded_color);

  for (int i = 0; i < 12; i++) {
    float theta = (float)i * M_PI * 2.0f / 12.0f;
    float phi = M_PI * 0.25f;
    simd_float3 direction = simd_make_float3(sinf(phi) * cosf(theta), cosf(phi),
                                             sinf(phi) * sinf(theta));
    simd_float3 line_start = light->position + direction * 0.3f;
    simd_float3 line_end = light->position + direction * (light->radius * 0.8f);
    simd_float4 line_color = color;
    line_color.w *= 0.5f;
    debug_draw_line(dbg, line_start, line_end, line_color);
  }
}

static void debug_viz_draw_spot_light(debug_renderer_t *dbg,
                                      const debug_light_t *light,
                                      simd_float4 color) {
  debug_draw_sphere(dbg, light->position, 0.2f, color);
  debug_viz_draw_spot_cone(dbg, light->position, light->direction,
                           light->radius, 45.0f, color);
  simd_float3 cone_end = light->position + light->direction * light->radius;
  debug_draw_line(dbg, light->position, cone_end, color);
}

static void debug_viz_draw_spot_cone(debug_renderer_t *dbg,
                                     simd_float3 position,
                                     simd_float3 direction, float length,
                                     float angle_degrees, simd_float4 color) {
  float angle_rad = angle_degrees * M_PI / 180.0f;
  float cone_radius = length * tanf(angle_rad);
  simd_float3 dir = simd_normalize(direction);
  simd_float3 up = simd_make_float3(0, 1, 0);
  if (fabsf(simd_dot(dir, up)) > 0.9f)
    up = simd_make_float3(1, 0, 0);
  simd_float3 right = simd_normalize(simd_cross(dir, up));
  up = simd_cross(right, dir);

  simd_float3 cone_tip = position;
  simd_float3 cone_base_center = position + dir * length;
  const int circle_segments = 16;
  for (int i = 0; i < circle_segments; i++) {
    float angle1 = (float)i * 2.0f * M_PI / circle_segments;
    float angle2 = (float)(i + 1) * 2.0f * M_PI / circle_segments;
    simd_float3 p1 = cone_base_center + right * (cosf(angle1) * cone_radius) +
                     up * (sinf(angle1) * cone_radius);
    simd_float3 p2 = cone_base_center + right * (cosf(angle2) * cone_radius) +
                     up * (sinf(angle2) * cone_radius);
    simd_float4 faded_color = color;
    faded_color.w *= 0.4f;
    debug_draw_line(dbg, p1, p2, faded_color);
    debug_draw_line(dbg, cone_tip, p1, faded_color);
  }
}

static void debug_draw_sphere_wireframe(debug_renderer_t *dbg,
                                        simd_float3 center, float radius,
                                        simd_float4 color) {
  const int segments = 16;
  const int rings = 8;
  for (int ring = 1; ring < rings; ring++) {
    float phi = (float)ring * M_PI / rings;
    float y = cosf(phi) * radius;
    float ring_radius = sinf(phi) * radius;
    for (int seg = 0; seg < segments; seg++) {
      float theta1 = (float)seg * 2.0f * M_PI / segments;
      float theta2 = (float)(seg + 1) * 2.0f * M_PI / segments;
      simd_float3 p1 = center + simd_make_float3(cosf(theta1) * ring_radius, y,
                                                 sinf(theta1) * ring_radius);
      simd_float3 p2 = center + simd_make_float3(cosf(theta2) * ring_radius, y,
                                                 sinf(theta2) * ring_radius);
      debug_draw_line(dbg, p1, p2, color);
    }
  }
  for (int seg = 0; seg < segments; seg++) {
    float theta = (float)seg * 2.0f * M_PI / segments;
    for (int ring = 0; ring < rings; ring++) {
      float phi1 = (float)ring * M_PI / rings;
      float phi2 = (float)(ring + 1) * M_PI / rings;
      simd_float3 p1 =
          center + simd_make_float3(cosf(theta) * sinf(phi1) * radius,
                                    cosf(phi1) * radius,
                                    sinf(theta) * sinf(phi1) * radius);
      simd_float3 p2 =
          center + simd_make_float3(cosf(theta) * sinf(phi2) * radius,
                                    cosf(phi2) * radius,
                                    sinf(theta) * sinf(phi2) * radius);
      debug_draw_line(dbg, p1, p2, color);
    }
  }
}

void debug_viz_draw_shadow_frustums(debug_renderer_t *dbg,
                                    const shadow_system_t *shadows,
                                    const debug_viz_config_t *config) {
  if (!dbg || !config || !config->show_shadow_frustums)
    return;

  shadow_cascade_t cascades[4] = {{debug_matrix_identity(), 0.05f, 0, true},
                                  {debug_matrix_identity(), 0.15f, 1, true},
                                  {debug_matrix_identity(), 0.50f, 2, true},
                                  {debug_matrix_identity(), 1.00f, 3, true}};

  for (int i = 0; i < 4; i++) {
    shadow_cascade_t *cascade = &cascades[i];
    if (!cascade->enabled)
      continue;

    float near = 0.1f;
    float far = cascade->cascade_split * 100.0f;
    float fov = 45.0f * M_PI / 180.0f;
    float aspect = 16.0f / 9.0f;
    float tan_half_fov = tanf(fov * 0.5f);

    cascade->view_proj_matrix.columns[0][0] = 1.0f / (aspect * tan_half_fov);
    cascade->view_proj_matrix.columns[1][1] = 1.0f / tan_half_fov;
    cascade->view_proj_matrix.columns[2][2] = -(far + near) / (far - near);
    cascade->view_proj_matrix.columns[2][3] = -1.0f;
    cascade->view_proj_matrix.columns[3][2] =
        -(2.0f * far * near) / (far - near);

    simd_float4x4 translation = debug_matrix_identity();
    translation.columns[3][0] = i * 15.0f - 22.5f;
    translation.columns[3][1] = 5.0f;
    translation.columns[3][2] = -10.0f;
    cascade->view_proj_matrix =
        simd_mul(translation, cascade->view_proj_matrix);

    debug_viz_draw_shadow_cascade(dbg, cascade, config);
  }
}

static void debug_viz_draw_shadow_cascade(debug_renderer_t *dbg,
                                          const shadow_cascade_t *cascade,
                                          const debug_viz_config_t *config) {
  simd_float4x4 inv_view_proj = simd_inverse(cascade->view_proj_matrix);
  simd_float4 clip_corners[8] = {{-1, -1, -1, 1}, {1, -1, -1, 1}, {1, 1, -1, 1},
                                 {-1, 1, -1, 1},  {-1, -1, 1, 1}, {1, -1, 1, 1},
                                 {1, 1, 1, 1},    {-1, 1, 1, 1}};

  simd_float3 world_corners[8];
  for (int i = 0; i < 8; i++) {
    simd_float4 world_corner = simd_mul(inv_view_proj, clip_corners[i]);
    if (world_corner.w > 0.001f)
      world_corner = world_corner / world_corner.w;
    world_corners[i] =
        simd_make_float3(world_corner.x, world_corner.y, world_corner.z);
  }

  simd_float4 cascade_colors[4] = {{1.0f, 0.0f, 0.0f, 0.6f},
                                   {0.0f, 1.0f, 0.0f, 0.6f},
                                   {0.0f, 0.0f, 1.0f, 0.6f},
                                   {1.0f, 1.0f, 0.0f, 0.6f}};
  simd_float4 color = cascade_colors[cascade->cascade_index % 4];

  const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                            {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
  for (int i = 0; i < 12; i++) {
    debug_draw_line(dbg, world_corners[edges[i][0]], world_corners[edges[i][1]],
                    color);
  }
  debug_viz_draw_frustum_faces(dbg, world_corners, color);

  char cascade_label[64];
  snprintf(cascade_label, sizeof(cascade_label), "CASCADE %d",
           cascade->cascade_index);
  simd_float3 center = simd_make_float3(0, 0, 0);
  for (int i = 0; i < 8; i++)
    center = center + world_corners[i];
  center = center / 8.0f;
  simd_float3 label_pos = center + simd_make_float3(0, 2.0f, 0);
  debug_draw_text_3d(dbg, label_pos, cascade_label, color);
  debug_viz_draw_split_plane(dbg, cascade, config);
}

static void debug_viz_draw_frustum_faces(debug_renderer_t *dbg,
                                         const simd_float3 corners[8],
                                         simd_float4 color) {
  simd_float4 face_color = color;
  face_color.w *= 0.2f;
  debug_draw_quad(dbg, corners[0], corners[1], corners[2], corners[3],
                  face_color);
  debug_draw_quad(dbg, corners[4], corners[7], corners[6], corners[5],
                  face_color);
  debug_draw_quad(dbg, corners[0], corners[4], corners[7], corners[3],
                  face_color);
  debug_draw_quad(dbg, corners[1], corners[2], corners[6], corners[5],
                  face_color);
  debug_draw_quad(dbg, corners[3], corners[2], corners[6], corners[7],
                  face_color);
  debug_draw_quad(dbg, corners[0], corners[1], corners[5], corners[4],
                  face_color);
}

static void debug_viz_draw_split_plane(debug_renderer_t *dbg,
                                       const shadow_cascade_t *cascade,
                                       const debug_viz_config_t *config) {
  float split_distance = cascade->cascade_split * 50.0f;
  float plane_size = 20.0f;
  simd_float3 plane_center = simd_make_float3(
      cascade->view_proj_matrix.columns[3][0],
      cascade->view_proj_matrix.columns[3][1], -split_distance);
  simd_float3 plane_corners[4] = {
      plane_center + simd_make_float3(-plane_size, 0, -plane_size),
      plane_center + simd_make_float3(plane_size, 0, -plane_size),
      plane_center + simd_make_float3(plane_size, 0, plane_size),
      plane_center + simd_make_float3(-plane_size, 0, plane_size)};
  simd_float4 split_color = config->shadow_frustum_color;
  split_color.w *= 0.3f;
  for (int i = 0; i < 4; i++) {
    int next = (i + 1) % 4;
    debug_draw_line(dbg, plane_corners[i], plane_corners[next], split_color);
  }
  debug_draw_line(dbg, plane_corners[0], plane_corners[2], split_color);
  debug_draw_line(dbg, plane_corners[1], plane_corners[3], split_color);
  char split_label[64];
  snprintf(split_label, sizeof(split_label), "SPLIT: %.2f",
           cascade->cascade_split);
  simd_float3 label_pos = plane_center + simd_make_float3(0, 1.0f, 0);
  debug_draw_text_3d(dbg, label_pos, split_label, split_color);
}

static void debug_draw_quad(debug_renderer_t *dbg, simd_float3 bl,
                            simd_float3 br, simd_float3 tr, simd_float3 tl,
                            simd_float4 color) {
  debug_draw_line(dbg, bl, br, color);
  debug_draw_line(dbg, br, tr, color);
  debug_draw_line(dbg, tr, tl, color);
  debug_draw_line(dbg, tl, bl, color);
}

void debug_viz_draw_probe_grid(debug_renderer_t *dbg, simd_float3 grid_origin,
                               int grid_size_x, int grid_size_y,
                               int grid_size_z, float spacing,
                               const debug_viz_config_t *config) {
  if (!dbg || !config || !config->show_probe_grid)
    return;
  for (int x = 0; x < grid_size_x; x++) {
    for (int y = 0; y < grid_size_y; y++) {
      for (int z = 0; z < grid_size_z; z++) {
        simd_float3 probe_pos =
            grid_origin +
            simd_make_float3(x * spacing, y * spacing, z * spacing);
        debug_draw_sphere(dbg, probe_pos, 0.1f, config->probe_color);
      }
    }
  }
}

void debug_viz_draw_cluster_grid(debug_renderer_t *dbg, simd_float3 grid_origin,
                                 int clusters_x, int clusters_y, int clusters_z,
                                 float cluster_size,
                                 const debug_viz_config_t *config) {
  if (!dbg || !config || !config->show_cluster_grid)
    return;
  for (int x = 0; x < clusters_x; x++) {
    for (int y = 0; y < clusters_y; y++) {
      for (int z = 0; z < clusters_z; z++) {
        simd_float3 cluster_min =
            grid_origin + simd_make_float3(x * cluster_size, y * cluster_size,
                                           z * cluster_size);
        simd_float3 cluster_max =
            cluster_min +
            simd_make_float3(cluster_size, cluster_size, cluster_size);
        simd_float4 cluster_color = config->probe_color;
        cluster_color.w = 0.2f;
        debug_draw_aabb(dbg, cluster_min, cluster_max, cluster_color);
      }
    }
  }
}

// Recursive BVH drawing helper
static void debug_viz_draw_bvh_node(debug_renderer_t *dbg,
                                    const bvh_node_t *node,
                                    const debug_viz_config_t *config,
                                    int current_depth, int max_depth) {
  if (!node || current_depth > max_depth)
    return;
  debug_draw_aabb(dbg, node->bounds_min, node->bounds_max, config->bvh_color);
  float depth_factor = (float)current_depth / (float)max_depth;
  simd_float4 depth_color = config->bvh_color;
  depth_color.w *= (1.0f - depth_factor * 0.5f);
  if (current_depth < 3) {
    char node_info[128];
    if (node->is_leaf)
      snprintf(node_info, sizeof(node_info), "L%d:%d", current_depth,
               node->object_count);
    else
      snprintf(node_info, sizeof(node_info), "N%d:%d", current_depth,
               node->object_count);
    simd_float3 center = {(node->bounds_min.x + node->bounds_max.x) * 0.5f,
                          (node->bounds_min.y + node->bounds_max.y) * 0.5f,
                          (node->bounds_min.z + node->bounds_max.z) * 0.5f};
    debug_draw_text_3d(dbg, center, node_info, depth_color);
  }
  if (!node->is_leaf && current_depth < max_depth)
    debug_viz_draw_bvh_split_plane(dbg, node, depth_color);
  if (node->left)
    debug_viz_draw_bvh_node(dbg, node->left, config, current_depth + 1,
                            max_depth);
  if (node->right)
    debug_viz_draw_bvh_node(dbg, node->right, config, current_depth + 1,
                            max_depth);
}

void debug_viz_draw_bvh(debug_renderer_t *dbg, const bvh_node_t *root,
                        const debug_viz_config_t *config, int max_depth) {
  if (!dbg || !config || !config->show_bvh)
    return;
  if (!root) {
    bvh_node_t *demo_bvh = debug_viz_create_demo_bvh();
    debug_viz_draw_bvh_node(dbg, demo_bvh, config, 0, max_depth);
    debug_viz_cleanup_demo_bvh(demo_bvh);
  } else {
    debug_viz_draw_bvh_node(dbg, root, config, 0, max_depth);
  }
}

static bvh_node_t *debug_viz_create_demo_bvh(void) {
  bvh_node_t *root = malloc(sizeof(bvh_node_t));
  memset(root, 0, sizeof(bvh_node_t));
  root->bounds_min = simd_make_float3(-20.0f, -5.0f, -20.0f);
  root->bounds_max = simd_make_float3(20.0f, 15.0f, 20.0f);
  root->is_leaf = false;
  root->object_count = 16;
  root->left = malloc(sizeof(bvh_node_t));
  memset(root->left, 0, sizeof(bvh_node_t));
  root->left->bounds_min = simd_make_float3(-20.0f, -5.0f, -20.0f);
  root->left->bounds_max = simd_make_float3(0.0f, 15.0f, 0.0f);
  root->left->is_leaf = false;
  root->left->object_count = 8;
  root->left->parent = root;
  root->right = malloc(sizeof(bvh_node_t));
  memset(root->right, 0, sizeof(bvh_node_t));
  root->right->bounds_min = simd_make_float3(0.0f, -5.0f, 0.0f);
  root->right->bounds_max = simd_make_float3(20.0f, 15.0f, 20.0f);
  root->right->is_leaf = false;
  root->right->object_count = 8;
  root->right->parent = root;
  return root;
}

static void debug_viz_cleanup_demo_bvh(bvh_node_t *node) {
  if (!node)
    return;
  debug_viz_cleanup_demo_bvh(node->left);
  // Note: Skipping full cleanup logic for demo
  free(node);
}

static void debug_viz_draw_bvh_split_plane(debug_renderer_t *dbg,
                                           const bvh_node_t *node,
                                           simd_float4 color) {
  simd_float3 size = node->bounds_max - node->bounds_min;
  int split_axis = 0;
  if (size.y > size.x && size.y > size.z)
    split_axis = 1;
  else if (size.z > size.x && size.z > size.y)
    split_axis = 2;

  float split_pos = node->bounds_min.x;
  if (split_axis == 0)
    split_pos = (node->bounds_min.x + node->bounds_max.x) * 0.5f;
  else if (split_axis == 1)
    split_pos = (node->bounds_min.y + node->bounds_max.y) * 0.5f;
  else
    split_pos = (node->bounds_min.z + node->bounds_max.z) * 0.5f;

  simd_float3 plane_corners[4];
  if (split_axis == 0) { // YZ plane
    plane_corners[0] =
        simd_make_float3(split_pos, node->bounds_min.y, node->bounds_min.z);
    plane_corners[1] =
        simd_make_float3(split_pos, node->bounds_max.y, node->bounds_min.z);
    plane_corners[2] =
        simd_make_float3(split_pos, node->bounds_max.y, node->bounds_max.z);
    plane_corners[3] =
        simd_make_float3(split_pos, node->bounds_min.y, node->bounds_max.z);
  } else if (split_axis == 1) { // XZ plane
    plane_corners[0] =
        simd_make_float3(node->bounds_min.x, split_pos, node->bounds_min.z);
    plane_corners[1] =
        simd_make_float3(node->bounds_max.x, split_pos, node->bounds_min.z);
    plane_corners[2] =
        simd_make_float3(node->bounds_max.x, split_pos, node->bounds_max.z);
    plane_corners[3] =
        simd_make_float3(node->bounds_min.x, split_pos, node->bounds_max.z);
  } else { // XY plane
    plane_corners[0] =
        simd_make_float3(node->bounds_min.x, node->bounds_min.y, split_pos);
    plane_corners[1] =
        simd_make_float3(node->bounds_max.x, node->bounds_min.y, split_pos);
    plane_corners[2] =
        simd_make_float3(node->bounds_max.x, node->bounds_max.y, split_pos);
    plane_corners[3] =
        simd_make_float3(node->bounds_min.x, node->bounds_max.y, split_pos);
  }
  debug_draw_quad(dbg, plane_corners[0], plane_corners[1], plane_corners[2],
                  plane_corners[3], color);
}
