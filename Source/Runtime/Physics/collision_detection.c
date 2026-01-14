// collision_detection.c - Collision Detection Implementation
#include <include/physics/collision/broadphase.h>
#include <include/physics/collision/narrowphase.h>
#include <include/core/logger.h>
#include <include/math/vec3.h>
#include <include/math/aabb.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COLLISION_PAIRS 4096
#define MAX_CONTACT_POINTS 8192
#define MAX_BODIES 1024

typedef struct {
  u32 entity_a;
  u32 entity_b;
  AABB bounds_a;
  AABB bounds_b;
  bool potential_collision;
} CollisionPair;

// Match definition in physics/core/physics_types.h roughly
// Note: physics_types.h has float normal[3].
// We use Vec3 here. We need to be careful when copying.
// Actually, we should try to match physics_types.h ContactPoint if possible,
// but this file defines its own ContactPoint struct!
// physics_world_stubs.c includes physics_types.h which defines ContactPoint.
// This file defines ContactPoint struct locally.
// This is a conflict if they are linked.
// But physics_world_stubs.c includes physics_types.h.
// collision_detection.c defines it locally.
// If physics_world_stubs.c uses functions from here that return ContactPoint*, the types must match.
// collision_system_get_contacts_for_body will be the bridge.

typedef struct {
  Vec3 point;
  Vec3 normal;
  f32 penetration_depth;
  u32 entity_a;
  u32 entity_b;
} InternalContactPoint;

typedef struct {
  u32 id;
  AABB bounds;
  Vec3 position;
} CollisionBody;

typedef struct {
  CollisionPair pairs[MAX_COLLISION_PAIRS];
  u32 pair_count;
  
  InternalContactPoint contacts[MAX_CONTACT_POINTS];
  u32 contact_count;
  
  CollisionBody bodies[MAX_BODIES];
  u32 body_count;

  // Spatial hashing for broadphase
  u32 spatial_hash[1024];
  Vec3 hash_cell_size;
  
  // Performance metrics
  u32 broadphase_tests;
  u32 narrowphase_tests;
  u32 actual_collisions;
} CollisionSystem;

// Forward declarations
void collision_broadphase_update(void);
void collision_narrowphase_update(void);
bool collision_detect_aabb_vs_aabb(const AABB *a, const AABB *b);
void collision_generate_contacts(CollisionPair *pair);

static CollisionSystem g_collision_system = {0};

bool collision_system_init(void) {
  memset(&g_collision_system, 0, sizeof(CollisionSystem));
  g_collision_system.hash_cell_size = (Vec3){16.0f, 16.0f, 16.0f};
  LOG_INFO_CAT(LOG_CAT_PHYSICS, "Collision detection system initialized");
  return true;
}

void collision_system_shutdown(void) {
  memset(&g_collision_system, 0, sizeof(CollisionSystem));
  LOG_INFO_CAT(LOG_CAT_PHYSICS, "Collision detection system shutdown");
}

void collision_system_clear_bodies(void) {
  g_collision_system.body_count = 0;
}

void collision_system_add_body(u32 id, const AABB *bounds, Vec3 position) {
  if (g_collision_system.body_count < MAX_BODIES && bounds) {
    g_collision_system.bodies[g_collision_system.body_count++] = (CollisionBody){id, *bounds, position};
  }
}

void collision_system_update(f32 delta_time) {
  // Reset counters
  g_collision_system.pair_count = 0;
  g_collision_system.contact_count = 0;
  g_collision_system.broadphase_tests = 0;
  g_collision_system.narrowphase_tests = 0;
  g_collision_system.actual_collisions = 0;
  
  // Clear spatial hash
  memset(g_collision_system.spatial_hash, 0, sizeof(g_collision_system.spatial_hash));
  
  // Perform broadphase collision detection
  collision_broadphase_update();
  
  // Perform narrowphase collision detection
  collision_narrowphase_update();
  
  LOG_DEBUG_CAT(LOG_CAT_PHYSICS, "Collision update: %d broadphase tests, %d narrowphase tests, %d actual collisions",
            g_collision_system.broadphase_tests, g_collision_system.narrowphase_tests, 
            g_collision_system.actual_collisions);
}

void collision_broadphase_update(void) {
  g_collision_system.pair_count = 0;
  
  // Use registered bodies
  for (u32 i = 0; i < g_collision_system.body_count; i++) {
    for (u32 j = i + 1; j < g_collision_system.body_count; j++) {
      g_collision_system.broadphase_tests++;
      
      CollisionBody *b1 = &g_collision_system.bodies[i];
      CollisionBody *b2 = &g_collision_system.bodies[j];

      // Quick AABB overlap test
      if (aabb_intersects_aabb(b1->bounds, b2->bounds)) {
        if (g_collision_system.pair_count < MAX_COLLISION_PAIRS) {
          CollisionPair *pair = &g_collision_system.pairs[g_collision_system.pair_count++];
          pair->entity_a = b1->id;
          pair->entity_b = b2->id;
          pair->bounds_a = b1->bounds;
          pair->bounds_b = b2->bounds;
          pair->potential_collision = true;
        }
      }
    }
  }
  
  LOG_DEBUG_CAT(LOG_CAT_PHYSICS, "Broadphase found %d potential collision pairs", g_collision_system.pair_count);
}

void collision_narrowphase_update(void) {
  for (u32 i = 0; i < g_collision_system.pair_count; i++) {
    CollisionPair *pair = &g_collision_system.pairs[i];
    
    if (!pair->potential_collision) continue;
    
    g_collision_system.narrowphase_tests++;
    
    if (collision_detect_aabb_vs_aabb(&pair->bounds_a, &pair->bounds_b)) {
      g_collision_system.actual_collisions++;
      // Generate contact points
      collision_generate_contacts(pair);
    }
  }
  
  LOG_DEBUG_CAT(LOG_CAT_PHYSICS, "Narrowphase resolved %d actual collisions", g_collision_system.actual_collisions);
}

// Helper to find body position from ID (slow, O(N))
static Vec3 get_body_position(u32 id) {
    for(u32 i=0; i<g_collision_system.body_count; i++) {
        if(g_collision_system.bodies[i].id == id) return g_collision_system.bodies[i].position;
    }
    return (Vec3){0};
}

bool collision_detect_aabb_vs_aabb(const AABB *a, const AABB *b) {
  return (a->max.x >= b->min.x && a->min.x <= b->max.x &&
          a->max.y >= b->min.y && a->min.y <= b->max.y &&
          a->max.z >= b->min.z && a->min.z <= b->max.z);
}

void collision_generate_contacts(CollisionPair *pair) {
  if (!pair) return;
  
  Vec3 center_a = aabb_center(pair->bounds_a);
  Vec3 center_b = aabb_center(pair->bounds_b);
  
  // Calculate penetration on each axis
  Vec3 penetration = {
    fminf(pair->bounds_a.max.x - pair->bounds_b.min.x, pair->bounds_b.max.x - pair->bounds_a.min.x),
    fminf(pair->bounds_a.max.y - pair->bounds_b.min.y, pair->bounds_b.max.y - pair->bounds_a.min.y),
    fminf(pair->bounds_a.max.z - pair->bounds_b.min.z, pair->bounds_b.max.z - pair->bounds_a.min.z)
  };
  
  // Find the axis with minimum penetration (collision normal)
  f32 min_penetration = penetration.x;
  Vec3 normal = {1.0f, 0.0f, 0.0f};
  
  if (penetration.y < min_penetration) {
    min_penetration = penetration.y;
    normal = (Vec3){0.0f, 1.0f, 0.0f};
  }
  
  if (penetration.z < min_penetration) {
    min_penetration = penetration.z;
    normal = (Vec3){0.0f, 0.0f, 1.0f};
  }
  
  // Correct direction of normal (point from A to B)
  Vec3 dir = vec3_sub(center_b, center_a);
  if (vec3_dot(dir, normal) < 0) {
      normal = vec3_mul(normal, -1.0f);
  }

  if (g_collision_system.contact_count < MAX_CONTACT_POINTS) {
    InternalContactPoint *contact = &g_collision_system.contacts[g_collision_system.contact_count++];
    
    // Approximate contact point
    Vec3 contact_pos = vec3_mul(vec3_add(center_a, center_b), 0.5f);
    
    contact->point = contact_pos;
    contact->normal = normal;
    contact->penetration_depth = min_penetration;
    contact->entity_a = pair->entity_a;
    contact->entity_b = pair->entity_b;
    
    LOG_DEBUG_CAT(LOG_CAT_PHYSICS, "Generated contact: entity %d-%d, normal(%.2f,%.2f,%.2f), depth=%.3f",
              pair->entity_a, pair->entity_b, normal.x, normal.y, normal.z, min_penetration);
  }
}

// Bridge function for external API using physics_types.h ContactPoint
// Assuming struct layout compatibility is NOT guaranteed, so we copy field by field.
// We need to match the struct definition in physics_types.h
// physics_types.h ContactPoint has: float position_world_a[3], float normal[3], float penetration_depth...
typedef struct {
  float position_world_a[3];
  float position_world_b[3];
  float position_local_a[3];
  float position_local_b[3];
  float normal[3];
  float penetration_depth;
  float impulse_normal;
  float impulse_tangent[2];
  float friction;
  float restitution;
} ExternalContactPoint;

u32 collision_system_get_contacts_for_body(u32 body_id, void *out_contacts, u32 max_count) {
    if (!out_contacts || max_count == 0) return 0;

    ExternalContactPoint *contacts = (ExternalContactPoint*)out_contacts;
    u32 count = 0;

    for (u32 i = 0; i < g_collision_system.contact_count && count < max_count; i++) {
        InternalContactPoint *ic = &g_collision_system.contacts[i];
        if (ic->entity_a == body_id || ic->entity_b == body_id) {
            ExternalContactPoint *ec = &contacts[count++];

            // Fill external contact
            ec->normal[0] = ic->normal.x;
            ec->normal[1] = ic->normal.y;
            ec->normal[2] = ic->normal.z;

            // Normal points A->B.
            // If we are A, we want response force B->A (towards A). So flip.
            // If we are B, we want response force A->B (towards B). So don't flip.
            if (ic->entity_a == body_id) {
                 ec->normal[0] *= -1.0f;
                 ec->normal[1] *= -1.0f;
                 ec->normal[2] *= -1.0f;
            }

            ec->penetration_depth = ic->penetration_depth;
            // Point
            ec->position_world_a[0] = ic->point.x;
            ec->position_world_a[1] = ic->point.y;
            ec->position_world_a[2] = ic->point.z;
            // Simplified copy
            memcpy(ec->position_world_b, ec->position_world_a, sizeof(float)*3);
        }
    }
    return count;
}

// ... helper functions omitted for brevity if unused ...
// Kept for compilation if needed
bool collision_detect_sphere_vs_sphere(const Vec3 *center_a, f32 radius_a,
                                       const Vec3 *center_b, f32 radius_b) {
  Vec3 diff = vec3_sub(*center_a, *center_b);
  f32 distance_sq = vec3_length_sq(diff);
  f32 radius_sum = radius_a + radius_b;
  return distance_sq <= radius_sum * radius_sum;
}

bool collision_ray_vs_sphere(const Vec3 *ray_origin, const Vec3 *ray_dir, 
                            const Vec3 *sphere_center, f32 radius, f32 *out_distance) {
  if (!ray_origin || !ray_dir || !sphere_center) return false;
  
  Vec3 to_sphere = vec3_sub(*sphere_center, *ray_origin);
  f32 projection = vec3_dot(to_sphere, *ray_dir);
  
  if (projection < 0) {
    return false; // Sphere is behind ray origin
  }
  
  Vec3 closest_point = vec3_add(*ray_origin, vec3_mul(*ray_dir, projection));
  Vec3 to_closest = vec3_sub(*sphere_center, closest_point);
  f32 distance_sq = vec3_length_sq(to_closest);
  
  if (distance_sq > radius * radius) {
    return false;
  }
  
  if (out_distance) {
    f32 offset = sqrtf(radius * radius - distance_sq);
    *out_distance = projection - offset;
    if (*out_distance < 0) *out_distance = projection + offset;
  }
  
  return true;
}

// Broadphase Stubs
BroadphaseResult* broadphase_create(void) { return NULL; }
void broadphase_destroy(BroadphaseResult *result) {}
u32 broadphase_find_potential_pairs(BroadphaseResult *result, const AABB *bounds, u32 count) { return 0; }

// Narrowphase Stubs
NarrowphaseResult* narrowphase_create(void) { return NULL; }
void narrowphase_destroy(NarrowphaseResult *result) {}
u32 narrowphase_generate_contacts(NarrowphaseResult *result, const CollisionPair *pairs, u32 pair_count) { return 0; }

// Utils stubs
void collision_get_statistics(u32 *a, u32 *b, u32 *c) {}
const InternalContactPoint* collision_get_contacts(u32 *out_count) { if(out_count)*out_count=0; return NULL; }
void collision_clear_contacts(void) {}
bool collision_query_point(const Vec3 *point, u32 *out_entity) { return false; }
bool collision_query_aabb(const AABB *bounds, u32 *out_entities, u32 max_entities, u32 *out_count) { return false; }
bool collision_query_sphere(const Vec3 *center, f32 radius, u32 *out_entities, u32 max_entities, u32 *out_count) { return false; }
bool collision_detect_sphere_vs_aabb(const Vec3 *center, f32 radius, const AABB *aabb) { return false; }
