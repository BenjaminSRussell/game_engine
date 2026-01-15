#include "../include/editor/selection_system.h"
#include "../include/ecs/components/transform.h"
#include "../include/math/vec3.h"
#include <float.h>
#include <stdio.h>

static SelectionState g_selection_state;
static World *g_world = NULL;

void selection_init(World *world) {
  g_world = world;
  g_selection_state.has_selection = false;
  g_selection_state.selected_entity = 0;
  g_selection_state.intersection_distance = FLT_MAX;
}

void selection_shutdown(void) {
  selection_clear();
  g_world = NULL;
}

void selection_clear(void) {
  g_selection_state.has_selection = false;
  g_selection_state.selected_entity = 0;
}

SelectionState selection_get_state(void) { return g_selection_state; }

// Simple Ray-Sphere intersection
static bool ray_sphere_intersect(Vec3 ray_origin, Vec3 ray_dir,
                                 Vec3 sphere_center, float radius, float *t) {
  Vec3 oc = vec3_sub(ray_origin, sphere_center);
  float a = vec3_dot(ray_dir, ray_dir);
  float b = 2.0f * vec3_dot(oc, ray_dir);
  float c = vec3_dot(oc, oc) - radius * radius;
  float discriminant = b * b - 4 * a * c;

  if (discriminant < 0) {
    return false;
  } else {
    *t = (-b - sqrtf(discriminant)) / (2.0f * a);
    if (*t < 0)
      return false; // Behind ray
    return true;
  }
}

bool selection_raycast(SelectionRay ray) {
  if (!g_world)
    return false;

  bool hit_any = false;
  float min_dist = ray.max_distance;
  EntityID best_entity = 0;

  // Get Transform Component ID
  ECSComponentID transform_id = transform_component_get_id(g_world);

  // Create Query
  QueryDesc desc = {.all_components = &transform_id, .all_count = 1};
  Query *q = ecs_query_create(g_world, &desc);
  if (!q)
    return false;

  Entity entity;
  void *components[1]; // Array of component pointers

  while (ecs_query_next(q, &entity, components)) {
    TransformComponent *transform = (TransformComponent *)components[0];

    float t;
    // Approximation: Radius = larger of scale.x/y/z or 1.0 default
    float radius = 1.0f;
    if (transform->scale.x > radius)
      radius = transform->scale.x;

    if (ray_sphere_intersect(ray.origin, ray.direction, transform->position,
                             radius, &t)) {
      if (t < min_dist) {
        min_dist = t;
        best_entity = entity.id; // access ID from struct
        hit_any = true;
      }
    }
  }

  ecs_query_destroy(g_world, q);

  if (hit_any) {
    g_selection_state.has_selection = true;
    g_selection_state.selected_entity = best_entity;
    g_selection_state.intersection_distance = min_dist;
    g_selection_state.intersection_point =
        vec3_add(ray.origin, vec3_mul(ray.direction, min_dist));
  }

  return hit_any;
}

SelectionRay selection_ray_from_camera(Vec3 cam_pos, Vec3 cam_fwd, Vec3 cam_up,
                                       float fov, float aspect, float mouse_x,
                                       float mouse_y) {
  SelectionRay ray;
  ray.origin = cam_pos;
  ray.direction = cam_fwd; // Placeholder
  ray.max_distance = 1000.0f;
  return ray;
}
