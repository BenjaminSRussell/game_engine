#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

typedef struct { float x, y, z; } vec3;
typedef struct { vec3 min, max; } AABB;
typedef struct { vec3 center; float radius; } Sphere;

typedef struct CollisionObject CollisionObject; // Opaque forward declaration for user?
// Actually define it if needed by API return types
struct CollisionObject {
    uint32_t id;
    AABB aabb;
    void *user_data;
    uint32_t layer_mask;
    struct CollisionObject *next_in_cell;
};

typedef struct ContactPoint {
    vec3 point;
    vec3 normal;
    float penetration_depth;
    uint32_t entity_a;
    uint32_t entity_b;
} ContactPoint;

typedef struct SpatialHash SpatialHash;

// Spatial hash functions
SpatialHash* spatial_hash_create(uint32_t initial_capacity);
void spatial_hash_insert(SpatialHash *hash, uint32_t id, AABB aabb, void *user_data, uint32_t layer_mask);
void spatial_hash_clear(SpatialHash *hash);
uint32_t query_potential_collisions(SpatialHash *hash, AABB query_aabb, 
                                     CollisionObject **results, uint32_t max_results);

// Collision system functions
bool collision_system_init(void);
void collision_system_shutdown(void);
void collision_system_update(float delta_time);
void collision_system_add_body(uint32_t id, AABB *bounds, vec3 *position);
uint32_t collision_system_get_contacts_for_body(uint32_t id, ContactPoint *contacts, uint32_t max_contacts);
void collision_get_statistics(uint32_t *out_broadphase_tests, uint32_t *out_narrowphase_tests, 
                            uint32_t *out_actual_collisions);

// Collision detection functions
bool collision_detect_aabb_vs_aabb(const AABB *a, const AABB *b);
bool collision_detect_sphere_vs_sphere(const vec3 *center_a, float radius_a,
                                       const vec3 *center_b, float radius_b);
bool collision_detect_sphere_vs_aabb(const vec3 *center, float radius, const AABB *aabb);
bool aabb_overlap(const AABB *a, const AABB *b);
bool aabb_contains_point(const AABB *aabb, const vec3 *point);
vec3 aabb_center(const AABB *aabb);

#endif