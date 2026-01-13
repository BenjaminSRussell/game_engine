// continuous_collision.c - CCD Implementation
#include <include/physics/continuous_collision.h>
#include <include/core/logger.h>
#include <include/math/vec3.h>
#include <include/math/quat.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CCD_OBJECTS 256
#define MAX_CCD_RAYS 1024

typedef struct {
  EntityID entity_id;
  CCDShapeType shape_type;
  Vec3 position;
  Quat rotation;
  Vec3 linear_velocity;
  Vec3 angular_velocity;
  Vec3 prev_position;
  Quat prev_rotation;
  float radius;
  bool enabled;
} CCDObject;

static CCDObject g_ccd_objects[MAX_CCD_OBJECTS];
static u32 g_ccd_object_count = 0;

bool ccd_init(void) {
  memset(g_ccd_objects, 0, sizeof(g_ccd_objects));
  g_ccd_object_count = 0;
  LOG_INFO("CCD system initialized");
  return true;
}

void ccd_shutdown(void) {
  memset(g_ccd_objects, 0, sizeof(g_ccd_objects));
  g_ccd_object_count = 0;
  LOG_INFO("CCD system shutdown");
}

u32 ccd_add_object(EntityID entity, CCDShapeType shape, const Vec3 *pos, const Vec3 *vel, float radius) {
  if (g_ccd_object_count >= MAX_CCD_OBJECTS) return 0;
  
  CCDObject *obj = &g_ccd_objects[g_ccd_object_count++];
  obj->entity_id = entity;
  obj->shape_type = shape;
  obj->position = *pos;
  obj->prev_position = *pos;
  obj->linear_velocity = vel ? *vel : (Vec3){0,0,0};
  obj->radius = radius;
  obj->enabled = true;
  
  return g_ccd_object_count;
}

void ccd_update(f32 delta_time) {
  for (u32 i = 0; i < g_ccd_object_count; i++) {
    CCDObject *obj = &g_ccd_objects[i];
    if (!obj->enabled) continue;
    
    obj->prev_position = obj->position;
    obj->position = vec3_add(obj->position, vec3_scale(obj->linear_velocity, delta_time));
  }
}

bool ccd_ray_cast(const Vec3 *start, const Vec3 *end, CCDCollisionResult *result) {
  if (!start || !end || !result) return false;
  
  Vec3 ray_dir = vec3_normalize(vec3_sub(*end, *start));
  float max_dist = vec3_distance(start, end);
  
  // Check against all CCD objects
  for (u32 i = 0; i < g_ccd_object_count; i++) {
    CCDObject *obj = &g_ccd_objects[i];
    if (!obj->enabled) continue;
    
    if (obj->shape_type == CCD_SHAPE_SPHERE) {
      if (ccd_ray_vs_sphere(start, &ray_dir, max_dist, &obj->position, obj->radius, result)) {
        result->entity_b = obj->entity_id;
        result->shape_b = obj->shape_type;
        return true;
      }
    }
  }
  
  return false;
}

bool ccd_ray_vs_sphere(const Vec3 *origin, const Vec3 *dir, float max_dist, 
                       const Vec3 *center, float radius, CCDCollisionResult *result) {
  Vec3 to_center = vec3_sub(*center, *origin);
  float projection = vec3_dot(&to_center, dir);
  
  if (projection < 0 || projection > max_dist) return false;
  
  Vec3 closest_point = vec3_add(*origin, vec3_scale(*dir, projection));
  Vec3 to_closest = vec3_sub(*center, closest_point);
  float dist_sq = vec3_length_sq(&to_closest);
  
  if (dist_sq <= radius * radius) {
    result->hit = true;
    result->time_of_impact = projection / max_dist;
    result->contact_point = closest_point;
    result->contact_normal = vec3_normalize(to_closest);
    result->penetration_depth = radius - sqrtf(dist_sq);
    return true;
  }
  
  return false;
}

bool ccd_sweep_sphere_vs_sphere(const CCDObject *a, const CCDObject *b, CCDCollisionResult *result) {
  Vec3 start_a = a->prev_position;
  Vec3 end_a = a->position;
  Vec3 start_b = b->prev_position;
  Vec3 end_b = b->position;
  
  // Relative motion
  Vec3 rel_start = vec3_sub(start_a, start_b);
  Vec3 rel_end = vec3_sub(end_a, end_b);
  Vec3 rel_dir = vec3_normalize(vec3_sub(rel_end, rel_start));
  
  // Check if spheres collide during motion
  float combined_radius = a->radius + b->radius;
  float max_dist = vec3_distance(&rel_start, &rel_end);
  
  return ccd_ray_vs_sphere(&rel_start, &rel_dir, max_dist, &(Vec3){0,0,0}, combined_radius, result);
}
