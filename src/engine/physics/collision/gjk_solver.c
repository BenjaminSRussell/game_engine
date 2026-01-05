/**
 * =================================================================================================
 *                          GJK (GILBERT-JOHNSON-KEERTHI) SOLVER
 *                          Agent: AGENT_PHYSICS_1
 * =================================================================================================
 *
 * PURPOSE: Convex collision detection using Minkowski difference
 * PERFORMANCE TARGET: <1μs per pair
 * =================================================================================================
 */

#include <float.h>
#include <math.h>
#include <physics/physics_engine_core.h>
#include <stdbool.h>
#include <string.h>

// Maximum iterations to prevent infinite loops
#define GJK_MAX_ITERATIONS 64
#define GJK_EPSILON 1e-6f

// Simplex structure for GJK algorithm
typedef struct {
  float vertices[4][3]; // Up to 4 vertices (tetrahedron)
  uint32_t count;       // Current number of vertices
} GJKSimplex;

// Support point result
typedef struct {
  float point[3];   // Support point in world space
  float point_a[3]; // Point on shape A
  float point_b[3]; // Point on shape B
} SupportPoint;

// -----------------------------------------------------------------------------
// Vector Math Helpers
// -----------------------------------------------------------------------------

static inline float vec3_dot(const float *a, const float *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void vec3_sub(float *out, const float *a, const float *b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
}

static inline void vec3_add_out(float *out, const float *a, const float *b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
}

static inline void vec3_negate(float *out, const float *v) {
  out[0] = -v[0];
  out[1] = -v[1];
  out[2] = -v[2];
}

static inline void vec3_scale_out(float *out, const float *v, float s) {
  out[0] = v[0] * s;
  out[1] = v[1] * s;
  out[2] = v[2] * s;
}

static inline float vec3_length_sq(const float *v) {
  return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

static inline float vec3_length(const float *v) {
  return sqrtf(vec3_length_sq(v));
}

static inline void vec3_normalize(float *out, const float *v) {
  float len = vec3_length(v);
  if (len > GJK_EPSILON) {
    out[0] = v[0] / len;
    out[1] = v[1] / len;
    out[2] = v[2] / len;
  } else {
    out[0] = out[1] = out[2] = 0.0f;
  }
}

static inline void vec3_cross(float *out, const float *a, const float *b) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

static inline void vec3_copy(float *out, const float *v) {
  out[0] = v[0];
  out[1] = v[1];
  out[2] = v[2];
}

// Triple cross product: (a x b) x c
static inline void vec3_triple_cross(float *out, const float *a, const float *b,
                                     const float *c) {
  float temp[3];
  vec3_cross(temp, a, b);
  vec3_cross(out, temp, c);
}

// -----------------------------------------------------------------------------
// Support Function
// -----------------------------------------------------------------------------

// Get support point for a collision shape in given direction
static void shape_get_support(const CollisionShape *shape, const float *pos,
                              const float *rot, const float *dir, float *out) {
  switch (shape->type) {
  case COLLISION_SHAPE_SPHERE: {
    float norm_dir[3];
    vec3_normalize(norm_dir, dir);
    out[0] = pos[0] + norm_dir[0] * shape->data.sphere.radius;
    out[1] = pos[1] + norm_dir[1] * shape->data.sphere.radius;
    out[2] = pos[2] + norm_dir[2] * shape->data.sphere.radius;
    break;
  }
  case COLLISION_SHAPE_BOX: {
    // Box support: each axis gets +/- half extent based on direction sign
    out[0] =
        pos[0] + (dir[0] >= 0 ? 1.0f : -1.0f) * shape->data.box.half_extents[0];
    out[1] =
        pos[1] + (dir[1] >= 0 ? 1.0f : -1.0f) * shape->data.box.half_extents[1];
    out[2] =
        pos[2] + (dir[2] >= 0 ? 1.0f : -1.0f) * shape->data.box.half_extents[2];
    break;
  }
  case COLLISION_SHAPE_CAPSULE: {
    // Capsule: sphere at top or bottom of line segment
    float half_height = shape->data.capsule.height * 0.5f;
    float center_offset = (dir[1] >= 0) ? half_height : -half_height;

    float norm_dir[3];
    vec3_normalize(norm_dir, dir);
    out[0] = pos[0] + norm_dir[0] * shape->data.capsule.radius;
    out[1] = pos[1] + center_offset + norm_dir[1] * shape->data.capsule.radius;
    out[2] = pos[2] + norm_dir[2] * shape->data.capsule.radius;
    break;
  }
  case COLLISION_SHAPE_CONVEX_HULL: {
    // Find vertex with maximum dot product with direction
    float max_dot = -FLT_MAX;
    uint32_t max_idx = 0;

    for (uint32_t i = 0; i < shape->data.convex_hull.vertex_count; i++) {
      float *v = &shape->data.convex_hull.vertices[i * 3];
      float d = vec3_dot(v, dir);
      if (d > max_dot) {
        max_dot = d;
        max_idx = i;
      }
    }

    float *v = &shape->data.convex_hull.vertices[max_idx * 3];
    out[0] = pos[0] + v[0];
    out[1] = pos[1] + v[1];
    out[2] = pos[2] + v[2];
    break;
  }
  default:
    // Fallback: return position
    vec3_copy(out, pos);
    break;
  }
}

// Get Minkowski difference support point
static void gjk_support(const RigidBody *a, const RigidBody *b,
                        const float *dir, SupportPoint *out) {
  float neg_dir[3];
  vec3_negate(neg_dir, dir);

  // Support(A-B, d) = Support(A, d) - Support(B, -d)
  shape_get_support(a->shape, a->position, a->rotation, dir, out->point_a);
  shape_get_support(b->shape, b->position, b->rotation, neg_dir, out->point_b);

  vec3_sub(out->point, out->point_a, out->point_b);
}

// -----------------------------------------------------------------------------
// Simplex Operations
// -----------------------------------------------------------------------------

static void simplex_add_vertex(GJKSimplex *s, const float *v) {
  if (s->count < 4) {
    vec3_copy(s->vertices[s->count], v);
    s->count++;
  }
}

// Check if origin is contained and update simplex
static bool simplex_contains_origin(GJKSimplex *s, float *dir) {
  switch (s->count) {
  case 2: {
    // Line case
    float *a = s->vertices[1];
    float *b = s->vertices[0];

    float ab[3], ao[3];
    vec3_sub(ab, b, a);
    vec3_negate(ao, a);

    if (vec3_dot(ab, ao) > 0) {
      // Origin is between a and b, find perpendicular direction
      vec3_triple_cross(dir, ab, ao, ab);
    } else {
      // Origin is beyond a, keep only a
      s->count = 1;
      vec3_copy(s->vertices[0], a);
      vec3_copy(dir, ao);
    }
    return false;
  }

  case 3: {
    // Triangle case
    float *a = s->vertices[2];
    float *b = s->vertices[1];
    float *c = s->vertices[0];

    float ab[3], ac[3], ao[3], abc[3];
    vec3_sub(ab, b, a);
    vec3_sub(ac, c, a);
    vec3_negate(ao, a);
    vec3_cross(abc, ab, ac); // Triangle normal

    float abc_ac[3], ab_abc[3];
    vec3_cross(abc_ac, abc, ac);
    vec3_cross(ab_abc, ab, abc);

    if (vec3_dot(abc_ac, ao) > 0) {
      if (vec3_dot(ac, ao) > 0) {
        // Region AC
        s->count = 2;
        vec3_copy(s->vertices[0], c);
        vec3_copy(s->vertices[1], a);
        vec3_triple_cross(dir, ac, ao, ac);
      } else {
        // Region A or AB
        if (vec3_dot(ab, ao) > 0) {
          s->count = 2;
          vec3_copy(s->vertices[0], b);
          vec3_copy(s->vertices[1], a);
          vec3_triple_cross(dir, ab, ao, ab);
        } else {
          s->count = 1;
          vec3_copy(s->vertices[0], a);
          vec3_copy(dir, ao);
        }
      }
    } else if (vec3_dot(ab_abc, ao) > 0) {
      if (vec3_dot(ab, ao) > 0) {
        // Region AB
        s->count = 2;
        vec3_copy(s->vertices[0], b);
        vec3_copy(s->vertices[1], a);
        vec3_triple_cross(dir, ab, ao, ab);
      } else {
        s->count = 1;
        vec3_copy(s->vertices[0], a);
        vec3_copy(dir, ao);
      }
    } else {
      // Inside triangle projection, check which side of triangle
      if (vec3_dot(abc, ao) > 0) {
        vec3_copy(dir, abc);
      } else {
        // Flip winding
        vec3_copy(s->vertices[0], b);
        vec3_copy(s->vertices[1], c);
        vec3_negate(dir, abc);
      }
    }
    return false;
  }

  case 4: {
    // Tetrahedron case
    float *a = s->vertices[3];
    float *b = s->vertices[2];
    float *c = s->vertices[1];
    float *d = s->vertices[0];

    float ab[3], ac[3], ad[3], ao[3];
    vec3_sub(ab, b, a);
    vec3_sub(ac, c, a);
    vec3_sub(ad, d, a);
    vec3_negate(ao, a);

    float abc[3], acd[3], adb[3];
    vec3_cross(abc, ab, ac);
    vec3_cross(acd, ac, ad);
    vec3_cross(adb, ad, ab);

    // Check each face
    bool above_abc = vec3_dot(abc, ao) > 0;
    bool above_acd = vec3_dot(acd, ao) > 0;
    bool above_adb = vec3_dot(adb, ao) > 0;

    if (!above_abc && !above_acd && !above_adb) {
      // Origin is inside tetrahedron!
      return true;
    }

    // Remove point and continue with face closest to origin
    if (above_abc) {
      s->count = 3;
      vec3_copy(s->vertices[0], c);
      vec3_copy(s->vertices[1], b);
      vec3_copy(s->vertices[2], a);
      vec3_copy(dir, abc);
    } else if (above_acd) {
      s->count = 3;
      vec3_copy(s->vertices[0], d);
      vec3_copy(s->vertices[1], c);
      vec3_copy(s->vertices[2], a);
      vec3_copy(dir, acd);
    } else {
      s->count = 3;
      vec3_copy(s->vertices[0], b);
      vec3_copy(s->vertices[1], d);
      vec3_copy(s->vertices[2], a);
      vec3_copy(dir, adb);
    }
    return false;
  }

  default:
    return false;
  }
}

// -----------------------------------------------------------------------------
// GJK Algorithm
// -----------------------------------------------------------------------------

bool gjk_intersect(const RigidBody *a, const RigidBody *b,
                   GJKSimplex *simplex_out) {
  if (!a || !b || !a->shape || !b->shape)
    return false;

  GJKSimplex simplex = {0};
  float dir[3];

  // Initial direction: from A to B
  vec3_sub(dir, b->position, a->position);
  if (vec3_length_sq(dir) < GJK_EPSILON) {
    dir[0] = 1.0f;
    dir[1] = 0.0f;
    dir[2] = 0.0f;
  }
  vec3_normalize(dir, dir);

  // Get first support point
  SupportPoint support;
  gjk_support(a, b, dir, &support);
  simplex_add_vertex(&simplex, support.point);
  vec3_negate(dir, support.point);

  for (uint32_t iter = 0; iter < GJK_MAX_ITERATIONS; iter++) {
    // Get support point in current direction
    gjk_support(a, b, dir, &support);

    // Check if we passed the origin
    if (vec3_dot(support.point, dir) < 0) {
      return false; // No intersection
    }

    simplex_add_vertex(&simplex, support.point);

    // Check if simplex contains origin
    if (simplex_contains_origin(&simplex, dir)) {
      if (simplex_out)
        *simplex_out = simplex;
      return true; // Intersection!
    }

    // Prevent degenerate search direction
    if (vec3_length_sq(dir) < GJK_EPSILON) {
      return false;
    }
    vec3_normalize(dir, dir);
  }

  return false; // Max iterations reached
}

// Wrapper for collision_convex_convex_gjk in physics_engine_core.h
bool collision_convex_convex_gjk(const void *body_a, const void *body_b,
                                 ContactManifold *manifold) {
  const RigidBody *a = (const RigidBody *)body_a;
  const RigidBody *b = (const RigidBody *)body_b;

  GJKSimplex simplex;
  bool intersects = gjk_intersect(a, b, &simplex);

  if (intersects && manifold) {
    manifold->body_a = a->id;
    manifold->body_b = b->id;
    manifold->point_count = 0; // EPA will fill in contact points
  }

  return intersects;
}
