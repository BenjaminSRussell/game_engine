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

typedef struct {
  u32 entity_a;
  u32 entity_b;
  AABB bounds_a;
  AABB bounds_b;
  bool potential_collision;
} CollisionPair;

typedef struct {
  Vec3 point;
  Vec3 normal;
  f32 penetration_depth;
  u32 entity_a;
  u32 entity_b;
} ContactPoint;

typedef struct {
  CollisionPair pairs[MAX_COLLISION_PAIRS];
  u32 pair_count;
  
  ContactPoint contacts[MAX_CONTACT_POINTS];
  u32 contact_count;
  
  // Spatial hashing for broadphase
  u32 spatial_hash[1024];
  Vec3 hash_cell_size;
  
  // Performance metrics
  u32 broadphase_tests;
  u32 narrowphase_tests;
  u32 actual_collisions;
} CollisionSystem;

static CollisionSystem g_collision_system = {0};

bool collision_system_init(void) {
  memset(&g_collision_system, 0, sizeof(CollisionSystem));
  
  g_collision_system.hash_cell_size = (Vec3){16.0f, 16.0f, 16.0f};
  
  LOG_INFO("Collision detection system initialized");
  return true;
}

void collision_system_shutdown(void) {
  memset(&g_collision_system, 0, sizeof(CollisionSystem));
  LOG_INFO("Collision detection system shutdown");
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
  
  LOG_DEBUG("Collision update: %d broadphase tests, %d narrowphase tests, %d actual collisions",
            g_collision_system.broadphase_tests, g_collision_system.narrowphase_tests, 
            g_collision_system.actual_collisions);
}

void collision_broadphase_update(void) {
  // This would integrate with the entity system to get all collidable entities
  // For now, we'll simulate with some test entities
  
  // Clear previous pairs
  g_collision_system.pair_count = 0;
  
  // Simulate entity AABBs (in real implementation, this would come from ECS)
  AABB test_aabbs[4] = {
    {{-5.0f, -5.0f, -5.0f}, {5.0f, 5.0f, 5.0f}},     // Entity 0
    {{3.0f, 3.0f, 3.0f}, {8.0f, 8.0f, 8.0f}},         // Entity 1
    {{10.0f, 10.0f, 10.0f}, {15.0f, 15.0f, 15.0f}}, // Entity 2
    {{4.0f, -2.0f, 4.0f}, {9.0f, 3.0f, 9.0f}}       // Entity 3
  };
  
  // Perform broadphase collision detection using spatial hashing
  for (u32 i = 0; i < 4; i++) {
    for (u32 j = i + 1; j < 4; j++) {
      g_collision_system.broadphase_tests++;
      
      // Quick AABB overlap test
      if (aabb_overlap(&test_aabbs[i], &test_aabbs[j])) {
        if (g_collision_system.pair_count < MAX_COLLISION_PAIRS) {
          CollisionPair *pair = &g_collision_system.pairs[g_collision_system.pair_count++];
          pair->entity_a = i;
          pair->entity_b = j;
          pair->bounds_a = test_aabbs[i];
          pair->bounds_b = test_aabbs[j];
          pair->potential_collision = true;
        }
      }
    }
  }
  
  LOG_DEBUG("Broadphase found %d potential collision pairs", g_collision_system.pair_count);
}

void collision_narrowphase_update(void) {
  for (u32 i = 0; i < g_collision_system.pair_count; i++) {
    CollisionPair *pair = &g_collision_system.pairs[i];
    
    if (!pair->potential_collision) continue;
    
    g_collision_system.narrowphase_tests++;
    
    // Perform detailed collision detection
    // For now, we'll use simple AABB collision as narrowphase
    // In a real implementation, this would use shape-specific algorithms
    
    if (collision_detect_aabb_vs_aabb(&pair->bounds_a, &pair->bounds_b)) {
      g_collision_system.actual_collisions++;
      
      // Generate contact points
      collision_generate_contacts(pair);
    }
  }
  
  LOG_DEBUG("Narrowphase resolved %d actual collisions", g_collision_system.actual_collisions);
}

bool collision_detect_aabb_vs_aabb(const AABB *a, const AABB *b) {
  return (a->max.x >= b->min.x && a->min.x <= b->max.x &&
          a->max.y >= b->min.y && a->min.y <= b->max.y &&
          a->max.z >= b->min.z && a->min.z <= b->max.z);
}

bool collision_detect_sphere_vs_sphere(const Vec3 *center_a, f32 radius_a,
                                       const Vec3 *center_b, f32 radius_b) {
  Vec3 diff = vec3_sub(*center_a, *center_b);
  f32 distance_sq = vec3_length_sq(&diff);
  f32 radius_sum = radius_a + radius_b;
  return distance_sq <= radius_sum * radius_sum;
}

bool collision_detect_sphere_vs_aabb(const Vec3 *center, f32 radius, const AABB *aabb) {
  // Find closest point on AABB to sphere center
  Vec3 closest_point = {
    fmaxf(aabb->min.x, fminf(center->x, aabb->max.x)),
    fmaxf(aabb->min.y, fminf(center->y, aabb->max.y)),
    fmaxf(aabb->min.z, fminf(center->z, aabb->max.z))
  };
  
  // Check if closest point is within sphere
  Vec3 diff = vec3_sub(*center, closest_point);
  f32 distance_sq = vec3_length_sq(&diff);
  
  return distance_sq <= radius * radius;
}

void collision_generate_contacts(CollisionPair *pair) {
  if (!pair) return;
  
  // Generate contact points for AABB vs AABB collision
  // This is a simplified implementation
  
  Vec3 center_a = aabb_center(&pair->bounds_a);
  Vec3 center_b = aabb_center(&pair->bounds_b);
  
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
  
  // Create contact point
  if (g_collision_system.contact_count < MAX_CONTACT_POINTS) {
    ContactPoint *contact = &g_collision_system.contacts[g_collision_system.contact_count++];
    
    // Contact point is the average of the two centers projected onto the collision surface
    Vec3 contact_pos = vec3_scale(vec3_add(center_a, center_b), 0.5f);
    
    // Project contact point onto collision surface
    Vec3 to_contact = vec3_sub(contact_pos, center_a);
    f32 dot_product = vec3_dot(&to_contact, &normal);
    contact_pos = vec3_add(center_a, vec3_scale(normal, dot_product));
    
    contact->point = contact_pos;
    contact->normal = normal;
    contact->penetration_depth = min_penetration;
    contact->entity_a = pair->entity_a;
    contact->entity_b = pair->entity_b;
    
    LOG_DEBUG("Generated contact: entity %d-%d, normal(%.2f,%.2f,%.2f), depth=%.3f",
              pair->entity_a, pair->entity_b, normal.x, normal.y, normal.z, min_penetration);
  }
}

bool collision_ray_vs_aabb(const Vec3 *ray_origin, const Vec3 *ray_dir, const AABB *aabb, f32 *out_distance) {
  if (!ray_origin || !ray_dir || !aabb) return false;
  
  Vec3 inv_dir = {1.0f / ray_dir->x, 1.0f / ray_dir->y, 1.0f / ray_dir->z};
  
  f32 t1 = (aabb->min.x - ray_origin->x) * inv_dir.x;
  f32 t2 = (aabb->max.x - ray_origin->x) * inv_dir.x;
  f32 t3 = (aabb->min.y - ray_origin->y) * inv_dir.y;
  f32 t4 = (aabb->max.y - ray_origin->y) * inv_dir.y;
  f32 t5 = (aabb->min.z - ray_origin->z) * inv_dir.z;
  f32 t6 = (aabb->max.z - ray_origin->z) * inv_dir.z;
  
  f32 tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
  f32 tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));
  
  if (tmax < 0 || tmin > tmax) {
    return false;
  }
  
  if (out_distance) {
    *out_distance = tmin < 0 ? tmax : tmin;
  }
  
  return true;
}

bool collision_ray_vs_sphere(const Vec3 *ray_origin, const Vec3 *ray_dir, 
                            const Vec3 *sphere_center, f32 radius, f32 *out_distance) {
  if (!ray_origin || !ray_dir || !sphere_center) return false;
  
  Vec3 to_sphere = vec3_sub(*sphere_center, *ray_origin);
  f32 projection = vec3_dot(&to_sphere, ray_dir);
  
  if (projection < 0) {
    return false; // Sphere is behind ray origin
  }
  
  Vec3 closest_point = vec3_add(*ray_origin, vec3_scale(*ray_dir, projection));
  Vec3 to_closest = vec3_sub(*sphere_center, closest_point);
  f32 distance_sq = vec3_length_sq(&to_closest);
  
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

u32 collision_spatial_hash(const Vec3 *position) {
  if (!position) return 0;
  
  i32 x = (i32)floorf(position->x / g_collision_system.hash_cell_size.x);
  i32 y = (i32)floorf(position->y / g_collision_system.hash_cell_size.y);
  i32 z = (i32)floorf(position->z / g_collision_system.hash_cell_size.z);
  
  // Simple hash function
  return ((u32)x * 73856093) ^ ((u32)y * 19349663) ^ ((u32)z * 83492791);
}

void collision_add_to_spatial_hash(u32 entity_id, const AABB *bounds) {
  if (!bounds) return;
  
  Vec3 center = aabb_center(bounds);
  u32 hash = collision_spatial_hash(&center);
  
  // In a real implementation, this would add to a hash bucket
  // For now, we just store the entity in the hash table
  g_collision_system.spatial_hash[hash % 1024] = entity_id;
}

u32 collision_get_potential_collisions(const Vec3 *position, f32 radius, u32 *out_entities, u32 max_entities) {
  if (!position || !out_entities || max_entities == 0) return 0;
  
  u32 count = 0;
  
  // Check surrounding hash cells
  Vec3 min_pos = vec3_sub(*position, (Vec3){radius, radius, radius});
  Vec3 max_pos = vec3_add(*position, (Vec3){radius, radius, radius});
  
  for (f32 x = min_pos.x; x <= max_pos.x; x += g_collision_system.hash_cell_size.x) {
    for (f32 y = min_pos.y; y <= max_pos.y; y += g_collision_system.hash_cell_size.y) {
      for (f32 z = min_pos.z; z <= max_pos.z; z += g_collision_system.hash_cell_size.z) {
        Vec3 cell_pos = {x, y, z};
        u32 hash = collision_spatial_hash(&cell_pos);
        
        // Add entity from this hash cell
        u32 entity = g_collision_system.spatial_hash[hash % 1024];
        if (entity != 0 && count < max_entities) {
          out_entities[count++] = entity;
        }
      }
    }
  }
  
  return count;
}

// Broadphase API implementation
BroadphaseResult* broadphase_create(void) {
  BroadphaseResult *result = (BroadphaseResult*)calloc(1, sizeof(BroadphaseResult));
  if (!result) {
    LOG_ERROR("Failed to create broadphase result");
    return NULL;
  }
  
  LOG_DEBUG("Created broadphase result");
  return result;
}

void broadphase_destroy(BroadphaseResult *result) {
  if (result) {
    free(result);
    LOG_DEBUG("Destroyed broadphase result");
  }
}

u32 broadphase_find_potential_pairs(BroadphaseResult *result, const AABB *bounds, u32 count) {
  if (!result || !bounds || count == 0) return 0;
  
  u32 pair_count = 0;
  
  // Use spatial hashing to find potential collision pairs
  for (u32 i = 0; i < count; i++) {
    collision_add_to_spatial_hash(i, &bounds[i]);
  }
  
  // Find overlapping pairs
  for (u32 i = 0; i < count && pair_count < MAX_COLLISION_PAIRS; i++) {
    u32 potential_entities[32];
    u32 potential_count = collision_get_potential_collisions(
      &aabb_center(&bounds[i]), 16.0f, potential_entities, 32);
    
    for (u32 j = 0; j < potential_count && pair_count < MAX_COLLISION_PAIRS; j++) {
      u32 other_entity = potential_entities[j];
      
      if (other_entity > i) { // Avoid duplicate pairs
        if (aabb_overlap(&bounds[i], &bounds[other_entity])) {
          // Store pair in result
          pair_count++;
        }
      }
    }
  }
  
  LOG_DEBUG("Broadphase found %u potential pairs from %u entities", pair_count, count);
  return pair_count;
}

// Narrowphase API implementation
NarrowphaseResult* narrowphase_create(void) {
  NarrowphaseResult *result = (NarrowphaseResult*)calloc(1, sizeof(NarrowphaseResult));
  if (!result) {
    LOG_ERROR("Failed to create narrowphase result");
    return NULL;
  }
  
  LOG_DEBUG("Created narrowphase result");
  return result;
}

void narrowphase_destroy(NarrowphaseResult *result) {
  if (result) {
    free(result);
    LOG_DEBUG("Destroyed narrowphase result");
  }
}

u32 narrowphase_generate_contacts(NarrowphaseResult *result, const CollisionPair *pairs, u32 pair_count) {
  if (!result || !pairs || pair_count == 0) return 0;
  
  u32 contact_count = 0;
  
  for (u32 i = 0; i < pair_count && contact_count < MAX_CONTACT_POINTS; i++) {
    const CollisionPair *pair = &pairs[i];
    
    // Generate contacts for this pair
    collision_generate_contacts((CollisionPair*)pair);
    contact_count++;
  }
  
  LOG_DEBUG("Narrowphase generated %u contacts from %u pairs", contact_count, pair_count);
  return contact_count;
}

// Utility functions
void collision_get_statistics(u32 *out_broadphase_tests, u32 *out_narrowphase_tests, 
                            u32 *out_actual_collisions) {
  if (out_broadphase_tests) *out_broadphase_tests = g_collision_system.broadphase_tests;
  if (out_narrowphase_tests) *out_narrowphase_tests = g_collision_system.narrowphase_tests;
  if (out_actual_collisions) *out_actual_collisions = g_collision_system.actual_collisions;
}

const ContactPoint* collision_get_contacts(u32 *out_count) {
  if (out_count) *out_count = g_collision_system.contact_count;
  return g_collision_system.contacts;
}

void collision_clear_contacts(void) {
  g_collision_system.contact_count = 0;
}

bool collision_query_point(const Vec3 *point, u32 *out_entity) {
  if (!point || !out_entity) return false;
  
  // Check if point is inside any entity's bounds
  // This would integrate with the entity system
  
  // For now, check against test entities
  AABB test_aabbs[4] = {
    {{-5.0f, -5.0f, -5.0f}, {5.0f, 5.0f, 5.0f}},
    {{3.0f, 3.0f, 3.0f}, {8.0f, 8.0f, 8.0f}},
    {{10.0f, 10.0f, 10.0f}, {15.0f, 15.0f, 15.0f}},
    {{4.0f, -2.0f, 4.0f}, {9.0f, 3.0f, 9.0f}}
  };
  
  for (u32 i = 0; i < 4; i++) {
    if (aabb_contains_point(&test_aabbs[i], point)) {
      *out_entity = i;
      return true;
    }
  }
  
  return false;
}

bool collision_query_aabb(const AABB *bounds, u32 *out_entities, u32 max_entities, u32 *out_count) {
  if (!bounds || !out_entities || max_entities == 0) return false;
  
  u32 count = 0;
  
  // Check against test entities
  AABB test_aabbs[4] = {
    {{-5.0f, -5.0f, -5.0f}, {5.0f, 5.0f, 5.0f}},
    {{3.0f, 3.0f, 3.0f}, {8.0f, 8.0f, 8.0f}},
    {{10.0f, 10.0f, 10.0f}, {15.0f, 15.0f, 15.0f}},
    {{4.0f, -2.0f, 4.0f}, {9.0f, 3.0f, 9.0f}}
  };
  
  for (u32 i = 0; i < 4 && count < max_entities; i++) {
    if (aabb_overlap(bounds, &test_aabbs[i])) {
      out_entities[count++] = i;
    }
  }
  
  if (out_count) *out_count = count;
  return count > 0;
}

bool collision_query_sphere(const Vec3 *center, f32 radius, u32 *out_entities, u32 max_entities, u32 *out_count) {
  if (!center || !out_entities || max_entities == 0) return false;
  
  u32 count = 0;
  
  // Check against test entities
  AABB test_aabbs[4] = {
    {{-5.0f, -5.0f, -5.0f}, {5.0f, 5.0f, 5.0f}},
    {{3.0f, 3.0f, 3.0f}, {8.0f, 8.0f, 8.0f}},
    {{10.0f, 10.0f, 10.0f}, {15.0f, 15.0f, 15.0f}},
    {{4.0f, -2.0f, 4.0f}, {9.0f, 3.0f, 9.0f}}
  };
  
  for (u32 i = 0; i < 4 && count < max_entities; i++) {
    if (collision_detect_sphere_vs_aabb(center, radius, &test_aabbs[i])) {
      out_entities[count++] = i;
    }
  }
  
  if (out_count) *out_count = count;
  return count > 0;
}
