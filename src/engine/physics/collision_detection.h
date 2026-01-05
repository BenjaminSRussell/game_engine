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

typedef struct SpatialHash SpatialHash;

SpatialHash* spatial_hash_create(uint32_t initial_capacity);
void spatial_hash_insert(SpatialHash *hash, uint32_t id, AABB aabb, void *user_data, uint32_t layer_mask);
void spatial_hash_clear(SpatialHash *hash);
uint32_t query_potential_collisions(SpatialHash *hash, AABB query_aabb, 
                                     CollisionObject **results, uint32_t max_results);

#endif
