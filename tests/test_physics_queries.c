#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "physics/physics.h"
#include "physics/core/physics_types.h"

// Forward declarations of query functions we implemented
int physics_query_overlap_sphere(PhysicsWorld *world, const float *origin, float radius, RigidBody **out_bodies, int max_bodies);
int physics_query_overlap_box(PhysicsWorld *world, const float *center, const float *half_extents, const float* rotation, RigidBody **out_bodies, int max_bodies);
bool physics_query_sweep_sphere(PhysicsWorld *world, const float *origin, float radius, const float *direction, float max_dist, RayHit *out_hit);

void test_overlap_sphere() {
    printf("Testing Overlap Sphere...\n");
    PhysicsConfig config = physics_config_get_default();
    PhysicsWorld *world = physics_world_create(config);

    // Create a body at (0, 0, 5) with radius 1
    RigidBody *body = rigid_body_create(BODY_TYPE_DYNAMIC, (Vec3){0, 0, 5});
    Collider *collider = collider_create_sphere(1.0f);
    rigid_body_attach_collider(body, collider);
    physics_world_add_body(world, body);

    // Query at (0, 0, 5) radius 2 -> Should hit
    RigidBody *results[10];
    float origin1[3] = {0, 0, 5};
    int count = physics_query_overlap_sphere(world, origin1, 2.0f, results, 10);
    assert(count == 1);
    assert(results[0] == body);

    // Query at (0, 0, 0) radius 2 -> Should NOT hit (dist is 5, radii sum is 1+2=3)
    float origin2[3] = {0, 0, 0};
    count = physics_query_overlap_sphere(world, origin2, 2.0f, results, 10);
    assert(count == 0);

    // Query at (0, 0, 3.5) radius 2 -> Should hit (dist 1.5, radii sum 3)
    float origin3[3] = {0, 0, 3.5f};
    count = physics_query_overlap_sphere(world, origin3, 2.0f, results, 10);
    assert(count == 1);

    physics_world_destroy(world);
    printf("Overlap Sphere Passed.\n");
}

void test_sweep_sphere() {
    printf("Testing Sweep Sphere...\n");
    PhysicsConfig config = physics_config_get_default();
    PhysicsWorld *world = physics_world_create(config);

    // Body at (0, 0, 10), radius 1
    RigidBody *body = rigid_body_create(BODY_TYPE_DYNAMIC, (Vec3){0, 0, 10});
    Collider *collider = collider_create_sphere(1.0f);
    rigid_body_attach_collider(body, collider);
    physics_world_add_body(world, body);

    // Sweep from (0,0,0) towards (0,0,1) with radius 1
    // Distance to body center is 10.
    // Body radius 1, Sweep radius 1. Effective radius sum 2.
    // Should hit at distance 10 - 2 = 8.

    float origin[3] = {0, 0, 0};
    float dir[3] = {0, 0, 1};
    RayHit hit;
    bool result = physics_query_sweep_sphere(world, origin, 1.0f, dir, 20.0f, &hit);

    assert(result == true);
    assert(hit.body == body);
    printf("Sweep Hit Distance: %f (Expected ~8.0)\n", hit.distance);
    assert(hit.distance > 7.9f && hit.distance < 8.1f);

    physics_world_destroy(world);
    printf("Sweep Sphere Passed.\n");
}

int main() {
    test_overlap_sphere();
    test_sweep_sphere();
    return 0;
}
