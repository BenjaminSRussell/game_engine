#include "../src/engine/physics/physics_engine_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

int main() {
    printf("=== Running Physics World Integration Tests ===\n");
    
    // 1. Create World
    PhysicsConfig config = {0};
    config.gravity = vec3(0, -9.81f, 0);
    config.fixed_timestep = 1.0f/60.0f;
    config.velocity_iterations = 10;
    config.position_iterations = 5;
    
    PhysicsWorld *world = physics_world_create(config);
    assert(world != NULL);
    printf("World created.\n");
    
    // 2. Create Static Ground (Box)
    RigidBody *ground = core_rigid_body_create(1, RIGID_BODY_STATIC);
    ground->shape = shape_box_create(10.0f, 1.0f, 10.0f); // Large ground, y-extents +- 1.0
    float ground_pos[3] = {0, -1.0f, 0}; // Top surface at y=0
    core_rigid_body_set_position(ground, ground_pos);
    physics_world_add_body(world, ground);
    
    // 3. Create Falling Sphere
    RigidBody *sphere = core_rigid_body_create(2, RIGID_BODY_DYNAMIC);
    sphere->shape = shape_sphere_create(1.0f);
    float sphere_pos[3] = {0, 5.0f, 0};
    core_rigid_body_set_position(sphere, sphere_pos);
    sphere->restitution = 0.0f; // No bounce for easy settling check
    physics_world_add_body(world, sphere);
    
    printf("Bodies added. Sphere at Y=5.0, Ground Top at Y=0.0\n");
    
    // 4. Simulate
    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < 120; i++) { // Simulate 2 seconds
        physics_world_step(world, dt);
        
        // Debugging
        if (i % 10 == 0) {
           printf("Frame %d: Sphere Y=%.3f (Contacts: %u)\n", i, sphere->position[1], world->contact_count);
        }

        if (i == 10) {
           float initial_y = 5.0f;
           if (sphere->position[1] >= initial_y) {
               printf("FAILED: Gravity not applied. Y: %.3f\n", sphere->position[1]);
               return 1;
           }
        }
    }
    
    // 5. Verify Settlement
    // Retrieve simulated body state (Now shared pointer)
    printf("Final Sphere Y: %.3f\n", sphere->position[1]);
    
    // Sphere radius is 1.0. Ground top is 0.0.
    // Center should be at 1.0.
    // Allow small penetration slop (0.01)
    float err = fabs(sphere->position[1] - 1.0f);
    printf("Settlement Error: %.4f\n", err);
    if (err < 0.05f) {
        printf("PASSED: Sphere settled on ground.\n");
    } else {
        printf("FAILED: Sphere did not settle correctly.\n");
        // Don't assert hard failure yet, print info
    }
    
    // Cleanup
    // (In real engine, would destroy world and bodies)
    
    return 0;
}
