/**
 * =================================================================================================
 *                          EPA (EXPANDING POLYTOPE ALGORITHM) SOLVER
 *                          Agent: AGENT_PHYSICS_1
 * =================================================================================================
 *
 * PURPOSE: Generate contact manifold (penetration depth/normal) after GJK
 * collision PERFORMANCE TARGET: <2μs per collision
 * =================================================================================================
 */

#include <float.h>
#include <math.h>
#include <physics/physics_engine_core.h>
#include <stdbool.h>
#include <string.h>

#define EPA_MAX_ITERATIONS 64
#define EPA_MAX_FACES 64
#define EPA_EPSILON 1e-6f

// EPA face structure
typedef struct {
  uint32_t indices[3]; // Indices into vertex array
  float normal[3];     // Outward-facing normal
  float distance;      // Distance from origin to face
} EPAFace;

// EPA polytope
typedef struct {
  float vertices[64][3]; // Vertex pool
  uint32_t vertex_count;
  EPAFace faces[EPA_MAX_FACES];
  uint32_t face_count;
} EPAPolytope;

// -----------------------------------------------------------------------------
// Vector Math Helpers (local copies for compilation)
// -----------------------------------------------------------------------------

static inline float epa_vec3_dot(const float *a, const float *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void epa_vec3_sub(float *out, const float *a, const float *b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
}

static inline void epa_vec3_cross(float *out, const float *a, const float *b) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

static inline float epa_vec3_length(const float *v) {
  return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static inline void epa_vec3_normalize(float *out, const float *v) {
  float len = epa_vec3_length(v);
  if (len > EPA_EPSILON) {
    out[0] = v[0] / len;
    out[1] = v[1] / len;
    out[2] = v[2] / len;
  } else {
    out[0] = out[1] = out[2] = 0.0f;
  }
}

static inline void epa_vec3_copy(float *out, const float *v) {
  out[0] = v[0];
  out[1] = v[1];
  out[2] = v[2];
}

static inline void epa_vec3_negate(float *out, const float *v) {
  out[0] = -v[0];
  out[1] = -v[1];
  out[2] = -v[2];
}

// -----------------------------------------------------------------------------
// Support Function (from gjk_solver.c)
// -----------------------------------------------------------------------------

extern void shape_get_support(const CollisionShape *shape, const float *pos,
                              const float *rot, const float *dir, float *out);

static void epa_support(const RigidBody *a, const RigidBody *b,
                        const float *dir, float *out) {
  float point_a[3], point_b[3], neg_dir[3];
  epa_vec3_negate(neg_dir, dir);

  shape_get_support(a->shape, a->position, a->rotation, dir, point_a);
  shape_get_support(b->shape, b->position, b->rotation, neg_dir, point_b);

  epa_vec3_sub(out, point_a, point_b);
}

// -----------------------------------------------------------------------------
// Face Operations
// -----------------------------------------------------------------------------

static void epa_compute_face_normal(EPAFace *face, const float vertices[][3]) {
  const float *a = vertices[face->indices[0]];
  const float *b = vertices[face->indices[1]];
  const float *c = vertices[face->indices[2]];

  float ab[3], ac[3];
  epa_vec3_sub(ab, b, a);
  epa_vec3_sub(ac, c, a);
  epa_vec3_cross(face->normal, ab, ac);
  epa_vec3_normalize(face->normal, face->normal);

  // Distance from origin to face plane
  face->distance = epa_vec3_dot(face->normal, a);

  // Ensure normal points away from origin
  if (face->distance < 0) {
    epa_vec3_negate(face->normal, face->normal);
    face->distance = -face->distance;
    // Swap winding
    uint32_t temp = face->indices[1];
    face->indices[1] = face->indices[2];
    face->indices[2] = temp;
  }
}

static int epa_find_closest_face(const EPAPolytope *poly) {
  int closest = -1;
  float min_dist = FLT_MAX;

  for (uint32_t i = 0; i < poly->face_count; i++) {
    if (poly->faces[i].distance < min_dist) {
      min_dist = poly->faces[i].distance;
      closest = (int)i;
    }
  }

  return closest;
}

// Check if face can "see" a point (point is in front of face)
static bool epa_face_can_see(const EPAFace *face, const float vertices[][3],
                             const float *point) {
  const float *a = vertices[face->indices[0]];
  float ap[3];
  epa_vec3_sub(ap, point, a);
  return epa_vec3_dot(face->normal, ap) > EPA_EPSILON;
}

// -----------------------------------------------------------------------------
// Polytope Initialization from GJK Simplex
// -----------------------------------------------------------------------------

static void epa_init_from_tetrahedron(EPAPolytope *poly,
                                      const float simplex[4][3]) {
  poly->vertex_count = 4;
  for (int i = 0; i < 4; i++) {
    epa_vec3_copy(poly->vertices[i], simplex[i]);
  }

  // Create 4 faces of tetrahedron
  poly->face_count = 4;

  // Face 0: BCD (opposite A)
  poly->faces[0].indices[0] = 1;
  poly->faces[0].indices[1] = 2;
  poly->faces[0].indices[2] = 3;

  // Face 1: ACD (opposite B)
  poly->faces[1].indices[0] = 0;
  poly->faces[1].indices[1] = 2;
  poly->faces[1].indices[2] = 3;

  // Face 2: ABD (opposite C)
  poly->faces[2].indices[0] = 0;
  poly->faces[2].indices[1] = 1;
  poly->faces[2].indices[2] = 3;

  // Face 3: ABC (opposite D)
  poly->faces[3].indices[0] = 0;
  poly->faces[3].indices[1] = 1;
  poly->faces[3].indices[2] = 2;

  // Compute normals
  for (uint32_t i = 0; i < poly->face_count; i++) {
    epa_compute_face_normal(&poly->faces[i], poly->vertices);
  }
}

// -----------------------------------------------------------------------------
// Edge Structure for Horizon
// -----------------------------------------------------------------------------

typedef struct {
  uint32_t a, b; // Vertex indices
} EPAEdge;

static bool epa_edge_equals(const EPAEdge *e1, const EPAEdge *e2) {
  return (e1->a == e2->a && e1->b == e2->b) ||
         (e1->a == e2->b && e1->b == e2->a);
}

// -----------------------------------------------------------------------------
// EPA Algorithm
// -----------------------------------------------------------------------------

bool epa_solve(const RigidBody *body_a, const RigidBody *body_b,
               const float simplex[4][3], float *normal_out, float *depth_out) {
  if (!body_a || !body_b || !body_a->shape || !body_b->shape)
    return false;

  EPAPolytope poly;
  epa_init_from_tetrahedron(&poly, simplex);

  for (uint32_t iter = 0; iter < EPA_MAX_ITERATIONS; iter++) {
    // Find closest face to origin
    int closest = epa_find_closest_face(&poly);
    if (closest < 0)
      return false;

    EPAFace *face = &poly.faces[closest];

    // Get support point in direction of closest face normal
    float new_point[3];
    epa_support(body_a, body_b, face->normal, new_point);

    // Check if we've converged
    float new_dist = epa_vec3_dot(face->normal, new_point);
    if (new_dist - face->distance < EPA_EPSILON) {
      // Converged - return result
      epa_vec3_copy(normal_out, face->normal);
      *depth_out = face->distance;
      return true;
    }

    // Add new vertex
    if (poly.vertex_count >= 64)
      return false;
    uint32_t new_idx = poly.vertex_count;
    epa_vec3_copy(poly.vertices[new_idx], new_point);
    poly.vertex_count++;

    // Find horizon edges (edges of faces that can see new point)
    EPAEdge horizon[64];
    uint32_t horizon_count = 0;

    // Mark and remove faces that can see new point
    for (uint32_t i = 0; i < poly.face_count;) {
      if (epa_face_can_see(&poly.faces[i], poly.vertices, new_point)) {
        // Add edges to horizon (or remove if duplicate)
        for (int e = 0; e < 3; e++) {
          EPAEdge edge = {poly.faces[i].indices[e],
                          poly.faces[i].indices[(e + 1) % 3]};

          // Check if edge already in horizon
          bool found = false;
          for (uint32_t h = 0; h < horizon_count; h++) {
            if (epa_edge_equals(&horizon[h], &edge)) {
              // Remove duplicate (shared edge)
              horizon[h] = horizon[horizon_count - 1];
              horizon_count--;
              found = true;
              break;
            }
          }

          if (!found && horizon_count < 64) {
            horizon[horizon_count++] = edge;
          }
        }

        // Remove face
        poly.faces[i] = poly.faces[poly.face_count - 1];
        poly.face_count--;
      } else {
        i++;
      }
    }

    // Create new faces from horizon edges to new vertex
    for (uint32_t h = 0; h < horizon_count; h++) {
      if (poly.face_count >= EPA_MAX_FACES)
        break;

      EPAFace *new_face = &poly.faces[poly.face_count];
      new_face->indices[0] = horizon[h].a;
      new_face->indices[1] = horizon[h].b;
      new_face->indices[2] = new_idx;

      epa_compute_face_normal(new_face, poly.vertices);
      poly.face_count++;
    }
  }

  return false; // Max iterations reached
}

// Wrapper for collision_convex_convex_epa in physics_engine_core.h
bool collision_convex_convex_epa(const void *body_a, const void *body_b,
                                 ContactManifold *manifold) {
  const RigidBody *a = (const RigidBody *)body_a;
  const RigidBody *b = (const RigidBody *)body_b;

  // First run GJK to get simplex
  extern bool gjk_intersect(const RigidBody *a, const RigidBody *b,
                            void *simplex_out);

  float simplex[4][3];
  // Note: gjk_intersect returns a GJKSimplex, need to extract vertices
  // For now, return false - full integration needed
  (void)a;
  (void)b;
  (void)manifold;
  (void)simplex;

  return false;
}

// Generate contact points from EPA result
void epa_generate_contacts(const RigidBody *a, const RigidBody *b,
                           const float *normal, float depth,
                           ContactManifold *manifold) {
  if (!manifold)
    return;

  manifold->body_a = a->id;
  manifold->body_b = b->id;
  epa_vec3_copy(manifold->normal, normal);

  // For now, generate single contact point at penetration center
  // TODO: Generate full contact manifold with multiple points
  if (manifold->point_count < 4) {
    ContactPoint *cp = &manifold->points[0];

    // Approximate contact point: center - normal * depth/2
    float half_depth = depth * 0.5f;
    float center[3];
    center[0] = (a->position[0] + b->position[0]) * 0.5f;
    center[1] = (a->position[1] + b->position[1]) * 0.5f;
    center[2] = (a->position[2] + b->position[2]) * 0.5f;

    cp->position_world_a[0] = center[0] + normal[0] * half_depth;
    cp->position_world_a[1] = center[1] + normal[1] * half_depth;
    cp->position_world_a[2] = center[2] + normal[2] * half_depth;

    cp->position_world_b[0] = center[0] - normal[0] * half_depth;
    cp->position_world_b[1] = center[1] - normal[1] * half_depth;
    cp->position_world_b[2] = center[2] - normal[2] * half_depth;

    epa_vec3_copy(cp->normal, normal);
    cp->penetration_depth = depth;
    cp->impulse_normal = 0.0f;
    cp->impulse_tangent[0] = 0.0f;
    cp->impulse_tangent[1] = 0.0f;
    cp->friction = (a->friction + b->friction) * 0.5f;
    cp->restitution = (a->restitution + b->restitution) * 0.5f;

    manifold->point_count = 1;
  }
}
