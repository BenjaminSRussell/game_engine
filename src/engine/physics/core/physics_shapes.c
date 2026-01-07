#include "physics/core/physics_types.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <include/math/math.h>

/* =================================================================================================
 *                                    COLLISION SHAPES
 * =================================================================================================
 */

static void pvec3_zero(float *v) { v[0] = v[1] = v[2] = 0.0f; }
static void pvec3_copy(float *dst, const float *src) {
  memcpy(dst, src, 3 * sizeof(float));
}

CollisionShape *shape_sphere_create(float radius) {
  CollisionShape *s = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!s) return NULL;
  s->type = COLLISION_SHAPE_SPHERE;
  s->data.sphere.radius = radius;
    
  // Set bounds
  s->bounds_min[0] = -radius; s->bounds_min[1] = -radius; s->bounds_min[2] = -radius;
  s->bounds_max[0] = radius;  s->bounds_max[1] = radius;  s->bounds_max[2] = radius;
    
  return s;
}

CollisionShape *shape_box_create(float hx, float hy, float hz) {
  CollisionShape *s = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!s) return NULL;
  s->type = COLLISION_SHAPE_BOX;
  s->data.box.half_extents[0] = hx;
  s->data.box.half_extents[1] = hy;
  s->data.box.half_extents[2] = hz;
    
  // Set bounds
  s->bounds_min[0] = -hx; s->bounds_min[1] = -hy; s->bounds_min[2] = -hz;
  s->bounds_max[0] = hx;  s->bounds_max[1] = hy;  s->bounds_max[2] = hz;
    
  return s;
}

CollisionShape *shape_capsule_create(float radius, float height) {
  CollisionShape *shape = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!shape) return NULL;
  shape->type = COLLISION_SHAPE_CAPSULE;
  shape->data.capsule.radius = radius;
  shape->data.capsule.height = height;
  return shape;
}

CollisionShape *shape_cylinder_create(float radius, float height) {
  CollisionShape *shape = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!shape) return NULL;
  shape->type = COLLISION_SHAPE_CYLINDER;
  shape->data.cylinder.radius = radius;
  shape->data.cylinder.height = height;
  return shape;
}

CollisionShape *shape_cone_create(float radius, float height) {
  CollisionShape *shape = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!shape) return NULL;
  shape->type = COLLISION_SHAPE_CONE;
  shape->data.cone.radius = radius;
  shape->data.cone.height = height;
  return shape;
}

CollisionShape *shape_convex_hull_create(float *vertices, uint32_t count) {
  CollisionShape *shape = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!shape) return NULL;
  shape->type = COLLISION_SHAPE_CONVEX_HULL;
  shape->data.convex_hull.vertices = (float *)malloc(count * 3 * sizeof(float));
  if (!shape->data.convex_hull.vertices) {
    free(shape);
    return NULL;
  }
  memcpy(shape->data.convex_hull.vertices, vertices, count * 3 * sizeof(float));
  shape->data.convex_hull.vertex_count = count;
  return shape;
}

CollisionShape *shape_mesh_create(float *vertices, uint32_t v_count,
                                  uint32_t *indices, uint32_t i_count) {
  CollisionShape *shape = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!shape) return NULL;
  shape->type = COLLISION_SHAPE_MESH;
  shape->data.mesh.vertices = (float *)malloc(v_count * 3 * sizeof(float));
  shape->data.mesh.indices = (uint32_t *)malloc(i_count * sizeof(uint32_t));
  if (!shape->data.mesh.vertices || !shape->data.mesh.indices) {
    if (shape->data.mesh.vertices) free(shape->data.mesh.vertices);
    if (shape->data.mesh.indices) free(shape->data.mesh.indices);
    free(shape);
    return NULL;
  }
  memcpy(shape->data.mesh.vertices, vertices, v_count * 3 * sizeof(float));
  memcpy(shape->data.mesh.indices, indices, i_count * sizeof(uint32_t));
  shape->data.mesh.tri_count = i_count / 3;
  return shape;
}

CollisionShape *shape_heightfield_create(float *heights, uint32_t width,
                                         uint32_t depth, float scale_x,
                                         float scale_y, float scale_z) {
  CollisionShape *shape = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!shape) return NULL;
  shape->type = COLLISION_SHAPE_HEIGHTFIELD;
  shape->data.heightfield.width = width;
  shape->data.heightfield.depth = depth;
  shape->data.heightfield.heights = (float *)malloc(width * depth * sizeof(float));
  if (!shape->data.heightfield.heights) {
    free(shape);
    return NULL;
  }
  memcpy(shape->data.heightfield.heights, heights, width * depth * sizeof(float));
  shape->data.heightfield.scale[0] = scale_x;
  shape->data.heightfield.scale[1] = scale_y;
  shape->data.heightfield.scale[2] = scale_z;
  return shape;
}

CollisionShape *shape_compound_create(void) {
  CollisionShape *shape = (CollisionShape *)calloc(1, sizeof(CollisionShape));
  if (!shape) return NULL;
  shape->type = COLLISION_SHAPE_COMPOUND;
  return shape;
}

void shape_destroy(CollisionShape *shape) {
  if (!shape) return;
  if (shape->type == COLLISION_SHAPE_CONVEX_HULL)
    free(shape->data.convex_hull.vertices);
  if (shape->type == COLLISION_SHAPE_MESH) {
    free(shape->data.mesh.vertices);
    free(shape->data.mesh.indices);
  }
  if (shape->type == COLLISION_SHAPE_HEIGHTFIELD)
    free(shape->data.heightfield.heights);
  free(shape);
}

void shape_calculate_bounds(CollisionShape *shape) {
  if (!shape) return;
  switch (shape->type) {
    case COLLISION_SHAPE_SPHERE: {
        float r = shape->data.sphere.radius;
        shape->bounds_min[0] = -r; shape->bounds_min[1] = -r; shape->bounds_min[2] = -r;
        shape->bounds_max[0] = r;  shape->bounds_max[1] = r;  shape->bounds_max[2] = r;
        break;
    }
    case COLLISION_SHAPE_BOX: {
        float *h = shape->data.box.half_extents;
        shape->bounds_min[0] = -h[0]; shape->bounds_min[1] = -h[1]; shape->bounds_min[2] = -h[2];
        shape->bounds_max[0] = h[0];  shape->bounds_max[1] = h[1];  shape->bounds_max[2] = h[2];
        break;
    }
    case COLLISION_SHAPE_CAPSULE: {
        float r = shape->data.capsule.radius;
        float h = shape->data.capsule.height * 0.5f;
        shape->bounds_min[0] = -r; shape->bounds_min[1] = -r - h; shape->bounds_min[2] = -r;
        shape->bounds_max[0] = r;  shape->bounds_max[1] = r + h;  shape->bounds_max[2] = r;
        break;
    }
    case COLLISION_SHAPE_MESH: {
        if (!shape->data.mesh.vertices || shape->data.mesh.tri_count == 0) {
            pvec3_zero(shape->bounds_min);
            pvec3_zero(shape->bounds_max);
            break;
        }
        shape->bounds_min[0] = -1000.0f; shape->bounds_min[1] = -1000.0f; shape->bounds_min[2] = -1000.0f;
        shape->bounds_max[0] = 1000.0f;  shape->bounds_max[1] = 1000.0f;  shape->bounds_max[2] = 1000.0f;
        break;
    }
    default:
        shape->bounds_min[0] = -1.0f; shape->bounds_min[1] = -1.0f; shape->bounds_min[2] = -1.0f;
        shape->bounds_max[0] = 1.0f;  shape->bounds_max[1] = 1.0f;  shape->bounds_max[2] = 1.0f;
        break;
  }
}

void shape_calculate_inertia(CollisionShape *shape, float mass, float *inertia) {
  inertia[0] = mass;
  inertia[4] = mass;
  inertia[8] = mass;
}

void shape_support_point(CollisionShape *shape, const float *dir, float *out_point) {
  pvec3_copy(out_point, shape->bounds_min);
}

bool shape_raycast(CollisionShape *shape, const float *origin, const float *dir,
                   float max_dist, float *out_hit) {
  if (!shape) return false;
  switch (shape->type) {
      case COLLISION_SHAPE_SPHERE: {
          float r = shape->data.sphere.radius;
          float oc[3] = { origin[0], origin[1], origin[2] };
          float b = 2.0f * (oc[0]*dir[0] + oc[1]*dir[1] + oc[2]*dir[2]);
          float c = (oc[0]*oc[0] + oc[1]*oc[1] + oc[2]*oc[2]) - r*r;
          float discrim = b*b - 4*c;
          if (discrim < 0) return false;
          float t = (-b - sqrtf(discrim)) * 0.5f;
          if (t < 0) t = (-b + sqrtf(discrim)) * 0.5f;
          if (t >= 0 && t <= max_dist) {
              *out_hit = t;
              return true;
          }
          break;
      }
      case COLLISION_SHAPE_BOX: {
          float tmin = -FLT_MAX;
          float tmax = FLT_MAX;
          float *bounds_min = shape->bounds_min;
          float *bounds_max = shape->bounds_max;
          for (int i = 0; i < 3; i++) {
              if (fabsf(dir[i]) < 0.00001f) {
                  if (origin[i] < bounds_min[i] || origin[i] > bounds_max[i]) return false;
              } else {
                  float inv_d = 1.0f / dir[i];
                  float t1 = (bounds_min[i] - origin[i]) * inv_d;
                  float t2 = (bounds_max[i] - origin[i]) * inv_d;
                  if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                  if (t1 > tmin) tmin = t1;
                  if (t2 < tmax) tmax = t2;
                  if (tmin > tmax) return false;
              }
          }
          if (tmin > 0 && tmin <= max_dist) {
              *out_hit = tmin;
              return true;
          }
          break;
      }
      default: break;
  }
  return false;
}
