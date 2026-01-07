#include "physics/core/physics_types.h"
#include <stdlib.h>
#include <string.h>

// Helper to access core raycast from physics_core.c
bool core_physics_world_raycast(PhysicsWorld *world, const float *origin, const float *dir, float max_dist, RayHit *out_hit);

bool physics_query_raycast(PhysicsWorld *world, const float *origin, const float *dir, float max_dist, RayHit *out_hit) {
    return core_physics_world_raycast(world, origin, dir, max_dist, out_hit);
}

// TODO: Implement other queries (shapecasts, overlaps) properly
// For now they are stubs in the core or need to be moved here.

void physics_query_overlap_sphere(PhysicsWorld *world, const float *origin, float radius) {
    // Stub
}

void physics_query_overlap_box(PhysicsWorld *world, const float *center, const float *extents) {
    // Stub
}
