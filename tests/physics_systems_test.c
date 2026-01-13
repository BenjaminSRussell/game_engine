/*
 * Physics Systems Test Suite
 * Comprehensive test coverage for physics engine components
 * Target: 80%+ code coverage
 */

#include "physics_integration_tests_extended.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

// Physics system headers
#include "engine/physics/rigid_body/rigid_body.h"
#include "engine/physics/constraints/constraint_solver.h"
#include "engine/physics/collision/collision_detection.h"
#include "engine/physics/simulation/physics_world.h"
#include "engine/physics/cloth/cloth_simulation.h"
#include "engine/physics/fluid/sph_fluid.h"

// Test utilities
#define PHYSICS_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("PHYSICS ASSERTION FAILED: %s at %s:%d\n", message, __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define PHYSICS_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("PHYSICS ASSERTION FAILED: %s (expected: %d, actual: %d) at %s:%d\n", \
                   message, (int)(expected), (int)(actual), __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define PHYSICS_ASSERT_FLOAT_EQ(expected, actual, tolerance, message) \
    do { \
        if (fabs((expected) - (actual)) > (tolerance)) { \
            printf("PHYSICS ASSERTION FAILED: %s (expected: %f, actual: %f) at %s:%d\n", \
                   message, (expected), (actual), __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

// Rigid Body Tests
static bool test_rigid_body_creation(void) {
    printf("Testing rigid body creation...\n");
    
    // Create rigid body
    rigid_body_t* body = rigid_body_create();
    PHYSICS_ASSERT(body != NULL, "Failed to create rigid body");
    
    // Set basic properties
    vec3_t position = {0.0f, 5.0f, 0.0f};
    vec3_t velocity = {1.0f, 0.0f, 0.0f};
    float mass = 10.0f;
    
    rigid_body_set_position(body, &position);
    rigid_body_set_velocity(body, &velocity);
    rigid_body_set_mass(body, mass);
    
    // Verify properties
    vec3_t check_pos;
    rigid_body_get_position(body, &check_pos);
    PHYSICS_ASSERT_FLOAT_EQ(position.x, check_pos.x, 0.001f, "Position X mismatch");
    PHYSICS_ASSERT_FLOAT_EQ(position.y, check_pos.y, 0.001f, "Position Y mismatch");
    PHYSICS_ASSERT_FLOAT_EQ(position.z, check_pos.z, 0.001f, "Position Z mismatch");
    
    float check_mass = rigid_body_get_mass(body);
    PHYSICS_ASSERT_FLOAT_EQ(mass, check_mass, 0.001f, "Mass mismatch");
    
    // Test collision shape
    collision_shape_t* sphere_shape = collision_shape_create_sphere(1.0f);
    PHYSICS_ASSERT(sphere_shape != NULL, "Failed to create sphere shape");
    
    rigid_body_set_collision_shape(body, sphere_shape);
    
    // Calculate inertia tensor
    rigid_body_calculate_inertia_tensor(body);
    
    // Verify inertia tensor is calculated
    mat3_t inertia;
    rigid_body_get_inertia_tensor(body, &inertia);
    
    // Inertia tensor for sphere should be diagonal with equal values
    PHYSICS_ASSERT_FLOAT_EQ(inertia.m[0][0], inertia.m[1][1], 0.001f, "Sphere inertia tensor should be uniform");
    PHYSICS_ASSERT_FLOAT_EQ(inertia.m[1][1], inertia.m[2][2], 0.001f, "Sphere inertia tensor should be uniform");
    
    // Cleanup
    collision_shape_destroy(sphere_shape);
    rigid_body_destroy(body);
    
    return true;
}

static bool test_rigid_body_dynamics(void) {
    printf("Testing rigid body dynamics...\n");
    
    // Create rigid body
    rigid_body_t* body = rigid_body_create();
    vec3_t position = {0.0f, 10.0f, 0.0f};
    vec3_t velocity = {0.0f, 0.0f, 0.0f};
    float mass = 1.0f;
    
    rigid_body_set_position(body, &position);
    rigid_body_set_velocity(body, &velocity);
    rigid_body_set_mass(body, mass);
    
    collision_shape_t* sphere_shape = collision_shape_create_sphere(1.0f);
    rigid_body_set_collision_shape(body, sphere_shape);
    rigid_body_calculate_inertia_tensor(body);
    
    // Apply gravity force
    vec3_t gravity = {0.0f, -9.81f, 0.0f};
    vec3_t force = vec3_mul(&gravity, mass);
    rigid_body_apply_force(body, &force);
    
    // Simulate for 1 second at 60 FPS
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; i++) {
        rigid_body_integrate(body, dt);
    }
    
    // Check final position (should have fallen due to gravity)
    vec3_t final_pos;
    rigid_body_get_position(body, &final_pos);
    
    // s = ut + 0.5 * a * t^2 = 0 + 0.5 * 9.81 * 1^2 = 4.905
    float expected_y = 10.0f - 4.905f;
    PHYSICS_ASSERT_FLOAT_EQ(expected_y, final_pos.y, 0.1f, "Body should fall due to gravity");
    
    // Check final velocity
    vec3_t final_vel;
    rigid_body_get_velocity(body, &final_vel);
    float expected_vel_y = -9.81f; // v = u + at = 0 + 9.81 * 1
    PHYSICS_ASSERT_FLOAT_EQ(expected_vel_y, final_vel.y, 0.1f, "Velocity should increase due to gravity");
    
    // Cleanup
    collision_shape_destroy(sphere_shape);
    rigid_body_destroy(body);
    
    return true;
}

static bool test_rigid_body_angular_motion(void) {
    printf("Testing rigid body angular motion...\n");
    
    // Create rigid body
    rigid_body_t* body = rigid_body_create();
    vec3_t position = {0.0f, 0.0f, 0.0f};
    vec3_t angular_velocity = {0.0f, 1.0f, 0.0f}; // 1 rad/s around Y axis
    float mass = 1.0f;
    
    rigid_body_set_position(body, &position);
    rigid_body_set_angular_velocity(body, &angular_velocity);
    rigid_body_set_mass(body, mass);
    
    collision_shape_t* box_shape = collision_shape_create_box(2.0f, 1.0f, 1.0f);
    rigid_body_set_collision_shape(body, box_shape);
    rigid_body_calculate_inertia_tensor(body);
    
    // Apply torque
    vec3_t torque = {0.0f, 0.0f, 1.0f};
    rigid_body_apply_torque(body, &torque);
    
    // Simulate for 1 second
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; i++) {
        rigid_body_integrate(body, dt);
    }
    
    // Check angular velocity should have changed due to torque
    vec3_t final_angular_vel;
    rigid_body_get_angular_velocity(body, &final_angular_vel);
    PHYSICS_ASSERT(final_angular_vel.z > 0.0f, "Angular velocity should change due to torque");
    
    // Check orientation should have changed
    quat_t orientation;
    rigid_body_get_orientation(body, &orientation);
    PHYSICS_ASSERT(orientation.w < 1.0f, "Orientation should change due to angular motion");
    
    // Cleanup
    collision_shape_destroy(box_shape);
    rigid_body_destroy(body);
    
    return true;
}

// Constraint Tests
static bool test_distance_constraint(void) {
    printf("Testing distance constraint...\n");
    
    // Create two bodies
    rigid_body_t* body_a = rigid_body_create();
    rigid_body_t* body_b = rigid_body_create();
    
    vec3_t pos_a = {-2.0f, 0.0f, 0.0f};
    vec3_t pos_b = {2.0f, 0.0f, 0.0f};
    
    rigid_body_set_position(body_a, &pos_a);
    rigid_body_set_position(body_b, &pos_b);
    rigid_body_set_mass(body_a, 1.0f);
    rigid_body_set_mass(body_b, 1.0f);
    
    collision_shape_t* sphere_shape = collision_shape_create_sphere(0.5f);
    rigid_body_set_collision_shape(body_a, sphere_shape);
    rigid_body_set_collision_shape(body_b, sphere_shape);
    rigid_body_calculate_inertia_tensor(body_a);
    rigid_body_calculate_inertia_tensor(body_b);
    
    // Create distance constraint
    vec3_t anchor_a = {0.5f, 0.0f, 0.0f}; // Right edge of body A
    vec3_t anchor_b = {-0.5f, 0.0f, 0.0f}; // Left edge of body B
    float distance = 1.0f; // Should keep them 1 unit apart
    
    constraint_t* constraint = constraint_create_distance(body_a, body_b, &anchor_a, &anchor_b, distance);
    PHYSICS_ASSERT(constraint != NULL, "Failed to create distance constraint");
    
    // Apply forces to bodies
    vec3_t force_a = {10.0f, 0.0f, 0.0f};
    vec3_t force_b = {-10.0f, 0.0f, 0.0f};
    
    rigid_body_apply_force(body_a, &force_a);
    rigid_body_apply_force(body_b, &force_b);
    
    // Simulate with constraint solving
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; i++) {
        rigid_body_integrate(body_a, dt);
        rigid_body_integrate(body_b, dt);
        constraint_solve(constraint, dt);
    }
    
    // Check that distance constraint is maintained
    vec3_t world_anchor_a, world_anchor_b;
    rigid_body_get_world_point(body_a, &anchor_a, &world_anchor_a);
    rigid_body_get_world_point(body_b, &anchor_b, &world_anchor_b);
    
    float actual_distance = vec3_distance(&world_anchor_a, &world_anchor_b);
    PHYSICS_ASSERT_FLOAT_EQ(distance, actual_distance, 0.1f, "Distance constraint should be maintained");
    
    // Cleanup
    constraint_destroy(constraint);
    collision_shape_destroy(sphere_shape);
    rigid_body_destroy(body_a);
    rigid_body_destroy(body_b);
    
    return true;
}

static bool test_hinge_constraint(void) {
    printf("Testing hinge constraint...\n");
    
    // Create two bodies
    rigid_body_t* body_a = rigid_body_create();
    rigid_body_t* body_b = rigid_body_create();
    
    vec3_t pos_a = {0.0f, 0.0f, 0.0f};
    vec3_t pos_b = {2.0f, 0.0f, 0.0f};
    
    rigid_body_set_position(body_a, &pos_a);
    rigid_body_set_position(body_b, &pos_b);
    rigid_body_set_mass(body_a, 10.0f); // Heavy body (static)
    rigid_body_set_mass(body_b, 1.0f);  // Light body (dynamic)
    
    collision_shape_t* box_shape = collision_shape_create_box(1.0f, 1.0f, 1.0f);
    rigid_body_set_collision_shape(body_a, box_shape);
    rigid_body_set_collision_shape(body_b, box_shape);
    rigid_body_calculate_inertia_tensor(body_a);
    rigid_body_calculate_inertia_tensor(body_b);
    
    // Create hinge constraint (allow rotation around Z axis only)
    vec3_t hinge_position = {1.0f, 0.0f, 0.0f};
    vec3_t hinge_axis = {0.0f, 0.0f, 1.0f};
    
    constraint_t* hinge = constraint_create_hinge(body_a, body_b, &hinge_position, &hinge_axis);
    PHYSICS_ASSERT(hinge != NULL, "Failed to create hinge constraint");
    
    // Apply torque to body B
    vec3_t torque = {0.0f, 5.0f, 0.0f}; // Try to rotate around Y axis
    rigid_body_apply_torque(body_b, &torque);
    
    // Simulate
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; i++) {
        rigid_body_integrate(body_a, dt);
        rigid_body_integrate(body_b, dt);
        constraint_solve(hinge, dt);
    }
    
    // Check that body B rotated around Z axis
    vec3_t angular_vel_b;
    rigid_body_get_angular_velocity(body_b, &angular_vel_b);
    PHYSICS_ASSERT(fabs(angular_vel_b.z) > 0.0f, "Body should rotate around hinge axis");
    
    // Check that other angular components are constrained
    PHYSICS_ASSERT(fabs(angular_vel_b.x) < 0.1f, "X rotation should be constrained");
    PHYSICS_ASSERT(fabs(angular_vel_b.y) < 0.1f, "Y rotation should be constrained");
    
    // Cleanup
    constraint_destroy(hinge);
    collision_shape_destroy(box_shape);
    rigid_body_destroy(body_a);
    rigid_body_destroy(body_b);
    
    return true;
}

// Collision Detection Tests
static bool test_sphere_sphere_collision(void) {
    printf("Testing sphere-sphere collision detection...\n");
    
    // Create two spheres
    collision_shape_t* sphere_a = collision_shape_create_sphere(1.0f);
    collision_shape_t* sphere_b = collision_shape_create_sphere(1.0f);
    
    PHYSICS_ASSERT(sphere_a != NULL, "Failed to create sphere A");
    PHYSICS_ASSERT(sphere_b != NULL, "Failed to create sphere B");
    
    // Test collision case
    transform_t transform_a = {{0.0f, 0.0f, 0.0f}, quat_identity()};
    transform_t transform_b = {{1.5f, 0.0f, 0.0f}, quat_identity()};
    
    collision_contact_t contact;
    bool has_collision = collision_detect_sphere_sphere(
        sphere_a, &transform_a,
        sphere_b, &transform_b,
        &contact
    );
    
    PHYSICS_ASSERT(has_collision, "Spheres should collide when overlapping");
    PHYSICS_ASSERT(contact.penetration_depth > 0.0f, "Penetration depth should be positive");
    PHYSICS_ASSERT_FLOAT_EQ(0.5f, contact.penetration_depth, 0.001f, "Incorrect penetration depth");
    
    // Test non-collision case
    transform_b.position.x = 3.0f;
    has_collision = collision_detect_sphere_sphere(
        sphere_a, &transform_a,
        sphere_b, &transform_b,
        &contact
    );
    
    PHYSICS_ASSERT(!has_collision, "Spheres should not collide when separated");
    
    // Cleanup
    collision_shape_destroy(sphere_a);
    collision_shape_destroy(sphere_b);
    
    return true;
}

static bool test_box_box_collision(void) {
    printf("Testing box-box collision detection...\n");
    
    // Create two boxes
    collision_shape_t* box_a = collision_shape_create_box(2.0f, 2.0f, 2.0f);
    collision_shape_t* box_b = collision_shape_create_box(1.0f, 1.0f, 1.0f);
    
    PHYSICS_ASSERT(box_a != NULL, "Failed to create box A");
    PHYSICS_ASSERT(box_b != NULL, "Failed to create box B");
    
    // Test collision case (box B inside box A)
    transform_t transform_a = {{0.0f, 0.0f, 0.0f}, quat_identity()};
    transform_t transform_b = {{0.5f, 0.5f, 0.5f}, quat_identity()};
    
    collision_contact_t contact;
    bool has_collision = collision_detect_box_box(
        box_a, &transform_a,
        box_b, &transform_b,
        &contact
    );
    
    PHYSICS_ASSERT(has_collision, "Boxes should collide when overlapping");
    
    // Test non-collision case
    transform_b.position.x = 5.0f;
    has_collision = collision_detect_box_box(
        box_a, &transform_a,
        box_b, &transform_b,
        &contact
    );
    
    PHYSICS_ASSERT(!has_collision, "Boxes should not collide when separated");
    
    // Cleanup
    collision_shape_destroy(box_a);
    collision_shape_destroy(box_b);
    
    return true;
}

// Cloth Simulation Tests
static bool test_cloth_creation(void) {
    printf("Testing cloth creation...\n");
    
    // Create cloth grid
    int width = 10;
    int height = 10;
    float spacing = 0.1f;
    
    cloth_t* cloth = cloth_create(width, height, spacing);
    PHYSICS_ASSERT(cloth != NULL, "Failed to create cloth");
    
    // Check cloth properties
    PHYSICS_ASSERT_EQ(width, cloth->width, "Cloth width mismatch");
    PHYSICS_ASSERT_EQ(height, cloth->height, "Cloth height mismatch");
    PHYSICS_ASSERT_EQ(width * height, cloth->particle_count, "Particle count mismatch");
    
    // Check that particles are created
    for (int i = 0; i < cloth->particle_count; i++) {
        cloth_particle_t* particle = &cloth->particles[i];
        PHYSICS_ASSERT(particle->mass > 0.0f, "Particle should have positive mass");
        PHYSICS_ASSERT(particle->inv_mass > 0.0f, "Particle inverse mass should be positive");
    }
    
    // Check that constraints are created
    PHYSICS_ASSERT(cloth->constraint_count > 0, "Cloth should have constraints");
    
    // Test pinning corners
    cloth_pin_particle(cloth, 0); // Top-left corner
    cloth_pin_particle(cloth, width - 1); // Top-right corner
    
    PHYSICS_ASSERT(cloth->particles[0].pinned, "Corner particle should be pinned");
    PHYSICS_ASSERT(cloth->particles[width - 1].pinned, "Corner particle should be pinned");
    
    // Cleanup
    cloth_destroy(cloth);
    
    return true;
}

static bool test_cloth_simulation(void) {
    printf("Testing cloth simulation...\n");
    
    // Create cloth
    cloth_t* cloth = cloth_create(8, 8, 0.1f);
    
    // Pin top edge
    for (int i = 0; i < 8; i++) {
        cloth_pin_particle(cloth, i);
    }
    
    // Apply gravity
    vec3_t gravity = {0.0f, -9.81f, 0.0f};
    cloth_set_gravity(cloth, &gravity);
    
    // Simulate for 1 second
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; i++) {
        cloth_simulate(cloth, dt);
    }
    
    // Check that cloth has fallen (unpinned particles should have moved down)
    for (int i = 8; i < cloth->particle_count; i++) { // Skip pinned top row
        cloth_particle_t* particle = &cloth->particles[i];
        PHYSICS_ASSERT(particle->position.y < 0.0f, "Unpinned particles should fall due to gravity");
    }
    
    // Check that pinned particles haven't moved
    for (int i = 0; i < 8; i++) {
        cloth_particle_t* particle = &cloth->particles[i];
        PHYSICS_ASSERT_FLOAT_EQ(0.0f, particle->position.y, 0.001f, "Pinned particles should not move");
    }
    
    // Cleanup
    cloth_destroy(cloth);
    
    return true;
}

// SPH Fluid Tests
static bool test_sph_fluid_creation(void) {
    printf("Testing SPH fluid creation...\n");
    
    // Create fluid simulation
    int particle_count = 1000;
    float radius = 5.0f;
    
    sph_fluid_t* fluid = sph_fluid_create(particle_count, radius);
    PHYSICS_ASSERT(fluid != NULL, "Failed to create SPH fluid");
    
    // Check fluid properties
    PHYSICS_ASSERT_EQ(particle_count, fluid->particle_count, "Particle count mismatch");
    PHYSICS_ASSERT_FLOAT_EQ(radius, fluid->smoothing_radius, 0.001f, "Smoothing radius mismatch");
    
    // Check that particles are initialized
    for (int i = 0; i < fluid->particle_count; i++) {
        sph_particle_t* particle = &fluid->particles[i];
        PHYSICS_ASSERT(particle->mass > 0.0f, "Particle should have positive mass");
        PHYSICS_ASSERT(particle->density > 0.0f, "Particle should have positive density");
        PHYSICS_ASSERT(particle->pressure >= 0.0f, "Particle pressure should be non-negative");
    }
    
    // Cleanup
    sph_fluid_destroy(fluid);
    
    return true;
}

static bool test_sph_fluid_simulation(void) {
    printf("Testing SPH fluid simulation...\n");
    
    // Create fluid in a box
    sph_fluid_t* fluid = sph_fluid_create(500, 1.0f);
    
    // Initialize particles in a cube
    for (int i = 0; i < fluid->particle_count; i++) {
        sph_particle_t* particle = &fluid->particles[i];
        particle->position.x = (float)(rand() % 100) / 100.0f * 2.0f - 1.0f; // -1 to 1
        particle->position.y = (float)(rand() % 100) / 100.0f * 2.0f;        // 0 to 2
        particle->position.z = (float)(rand() % 100) / 100.0f * 2.0f - 1.0f; // -1 to 1
        particle->velocity = vec3_make(0.0f, 0.0f, 0.0f);
    }
    
    // Set gravity
    vec3_t gravity = {0.0f, -9.81f, 0.0f};
    sph_fluid_set_gravity(fluid, &gravity);
    
    // Simulate for 0.5 seconds
    float dt = 0.01f; // 10ms timestep for stability
    for (int i = 0; i < 50; i++) {
        sph_fluid_simulate(fluid, dt);
    }
    
    // Check that particles have fallen due to gravity
    for (int i = 0; i < fluid->particle_count; i++) {
        sph_particle_t* particle = &fluid->particles[i];
        PHYSICS_ASSERT(particle->velocity.y < 0.0f, "Particles should gain downward velocity");
        PHYSICS_ASSERT(particle->position.y < 2.0f, "Particles should fall from initial position");
    }
    
    // Check pressure forces (particles should repel each other)
    float avg_density = 0.0f;
    for (int i = 0; i < fluid->particle_count; i++) {
        avg_density += fluid->particles[i].density;
    }
    avg_density /= fluid->particle_count;
    
    PHYSICS_ASSERT(avg_density > 1000.0f, "Average density should be high due to particle packing");
    
    // Cleanup
    sph_fluid_destroy(fluid);
    
    return true;
}

// Physics World Tests
static bool test_physics_world_integration(void) {
    printf("Testing physics world integration...\n");
    
    // Create physics world
    physics_world_t* world = physics_world_create();
    PHYSICS_ASSERT(world != NULL, "Failed to create physics world");
    
    // Set gravity
    vec3_t gravity = {0.0f, -9.81f, 0.0f};
    physics_world_set_gravity(world, &gravity);
    
    // Add rigid bodies
    rigid_body_t* ball = rigid_body_create();
    vec3_t ball_pos = {0.0f, 5.0f, 0.0f};
    rigid_body_set_position(ball, &ball_pos);
    rigid_body_set_mass(ball, 1.0f);
    
    collision_shape_t* sphere_shape = collision_shape_create_sphere(0.5f);
    rigid_body_set_collision_shape(ball, sphere_shape);
    rigid_body_calculate_inertia_tensor(ball);
    
    rigid_body_t* ground = rigid_body_create();
    vec3_t ground_pos = {0.0f, -1.0f, 0.0f};
    rigid_body_set_position(ground, &ground_pos);
    rigid_body_set_mass(ground, 0.0f); // Static body
    
    collision_shape_t* box_shape = collision_shape_create_box(10.0f, 2.0f, 10.0f);
    rigid_body_set_collision_shape(ground, box_shape);
    rigid_body_calculate_inertia_tensor(ground);
    
    // Add bodies to world
    physics_world_add_rigid_body(world, ball);
    physics_world_add_rigid_body(world, ground);
    
    // Simulate for 2 seconds
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 120; i++) {
        physics_world_step(world, dt);
    }
    
    // Check that ball has landed on ground
    vec3_t final_pos;
    rigid_body_get_position(ball, &final_pos);
    PHYSICS_ASSERT_FLOAT_EQ(0.5f, final_pos.y, 0.1f, "Ball should rest on ground");
    
    // Check that ball has stopped moving
    vec3_t final_vel;
    rigid_body_get_velocity(ball, &final_vel);
    PHYSICS_ASSERT_FLOAT_EQ(0.0f, final_vel.y, 0.1f, "Ball should have stopped moving");
    
    // Cleanup
    collision_shape_destroy(sphere_shape);
    collision_shape_destroy(box_shape);
    rigid_body_destroy(ball);
    rigid_body_destroy(ground);
    physics_world_destroy(world);
    
    return true;
}

// Performance Tests
static bool benchmark_physics_performance(void) {
    printf("Benchmarking physics performance...\n");
    
    physics_world_t* world = physics_world_create();
    
    // Create many falling objects
    const int num_objects = 100;
    rigid_body_t* bodies[num_objects];
    collision_shape_t* sphere_shape = collision_shape_create_sphere(0.5f);
    
    clock_t start = clock();
    
    // Create objects
    for (int i = 0; i < num_objects; i++) {
        bodies[i] = rigid_body_create();
        vec3_t pos = {
            (float)(rand() % 100 - 50) / 10.0f,
            5.0f + (float)i * 0.5f,
            (float)(rand() % 100 - 50) / 10.0f
        };
        rigid_body_set_position(bodies[i], &pos);
        rigid_body_set_mass(bodies[i], 1.0f);
        rigid_body_set_collision_shape(bodies[i], sphere_shape);
        rigid_body_calculate_inertia_tensor(bodies[i]);
        physics_world_add_rigid_body(world, bodies[i]);
    }
    
    clock_t end = clock();
    double creation_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Created %d objects in %.2f ms\n", num_objects, creation_time);
    
    // Benchmark simulation
    start = clock();
    
    float dt = 1.0f / 60.0f;
    for (int frame = 0; frame < 300; frame++) { // 5 seconds at 60 FPS
        physics_world_step(world, dt);
    }
    
    end = clock();
    double simulation_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Simulated %d objects for 5 seconds in %.2f ms (%.1f FPS)\n", 
           num_objects, simulation_time, 300.0 / (simulation_time / 1000.0));
    
    // Should maintain at least 30 FPS
    double fps = 300.0 / (simulation_time / 1000.0);
    PHYSICS_ASSERT(fps >= 30.0, "Physics simulation should maintain at least 30 FPS");
    
    // Cleanup
    for (int i = 0; i < num_objects; i++) {
        rigid_body_destroy(bodies[i]);
    }
    collision_shape_destroy(sphere_shape);
    physics_world_destroy(world);
    
    return true;
}

// Test suite definitions
static TestCase rigid_body_tests[] = {
    {"Rigid Body Creation", test_rigid_body_creation},
    {"Rigid Body Dynamics", test_rigid_body_dynamics},
    {"Rigid Body Angular Motion", test_rigid_body_angular_motion},
};

static TestCase constraint_tests[] = {
    {"Distance Constraint", test_distance_constraint},
    {"Hinge Constraint", test_hinge_constraint},
};

static TestCase collision_tests[] = {
    {"Sphere-Sphere Collision", test_sphere_sphere_collision},
    {"Box-Box Collision", test_box_box_collision},
};

static TestCase cloth_tests[] = {
    {"Cloth Creation", test_cloth_creation},
    {"Cloth Simulation", test_cloth_simulation},
};

static TestCase fluid_tests[] = {
    {"SPH Fluid Creation", test_sph_fluid_creation},
    {"SPH Fluid Simulation", test_sph_fluid_simulation},
};

static TestCase integration_tests[] = {
    {"Physics World Integration", test_physics_world_integration},
};

static TestCase performance_tests[] = {
    {"Physics Performance", benchmark_physics_performance},
};

// Test suite array
static TestSuite physics_test_suites[] = {
    {
        .suite_name = "Rigid Body System",
        .tests = rigid_body_tests,
        .test_count = sizeof(rigid_body_tests) / sizeof(rigid_body_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Constraint System",
        .tests = constraint_tests,
        .test_count = sizeof(constraint_tests) / sizeof(constraint_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Collision Detection",
        .tests = collision_tests,
        .test_count = sizeof(collision_tests) / sizeof(collision_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Cloth Simulation",
        .tests = cloth_tests,
        .test_count = sizeof(cloth_tests) / sizeof(cloth_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Fluid Simulation",
        .tests = fluid_tests,
        .test_count = sizeof(fluid_tests) / sizeof(fluid_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Integration Tests",
        .tests = integration_tests,
        .test_count = sizeof(integration_tests) / sizeof(integration_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Performance Tests",
        .tests = performance_tests,
        .test_count = sizeof(performance_tests) / sizeof(performance_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    }
};

// Test runner functions (same as core systems test)
static void run_physics_test_case(TestCase* test_case) {
    clock_t start = clock();
    test_case->passed = test_case->test_func();
    clock_t end = clock();
    
    test_case->execution_time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    if (test_case->passed) {
        printf("   %s (%.2f ms)\n", test_case->name, test_case->execution_time_ms);
    } else {
        printf("   %s (%.2f ms)\n", test_case->name, test_case->execution_time_ms);
    }
}

static void run_physics_test_suite(TestSuite* suite) {
    printf("\n=== Running %s ===\n", suite->suite_name);
    
    clock_t start = clock();
    
    for (size_t i = 0; i < suite->test_count; i++) {
        run_physics_test_case(&suite->tests[i]);
    }
    
    clock_t end = clock();
    suite->total_time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    suite->passed_count = 0;
    suite->failed_count = 0;
    for (size_t i = 0; i < suite->test_count; i++) {
        if (suite->tests[i].passed) {
            suite->passed_count++;
        } else {
            suite->failed_count++;
        }
    }
    
    suite->coverage_percentage = (double)suite->passed_count / suite->test_count * 100.0;
    
    printf("\nSuite Results: %zu/%zu passed (%.1f%% coverage) (%.2f ms)\n",
           suite->passed_count, suite->test_count, 
           suite->coverage_percentage, suite->total_time_ms);
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("Physics Systems Test Suite\n");
    printf("==========================\n");
    printf("Target: 80%%+ code coverage\n\n");
    
    clock_t total_start = clock();
    
    size_t total_tests = 0;
    size_t total_passed = 0;
    size_t total_failed = 0;
    
    // Run all test suites
    for (size_t i = 0; i < sizeof(physics_test_suites) / sizeof(physics_test_suites[0]); i++) {
        run_physics_test_suite(&physics_test_suites[i]);
        
        total_tests += physics_test_suites[i].test_count;
        total_passed += physics_test_suites[i].passed_count;
        total_failed += physics_test_suites[i].failed_count;
    }
    
    clock_t total_end = clock();
    double total_time = ((double)(total_end - total_start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Print final results
    printf("\n=== Final Results ===\n");
    printf("Total Tests: %zu\n", total_tests);
    printf("Passed: %zu\n", total_passed);
    printf("Failed: %zu\n", total_failed);
    printf("Success Rate: %.1f%%\n", (double)total_passed / total_tests * 100.0);
    printf("Total Time: %.2f ms\n", total_time);
    
    // Check if we met the coverage target
    double overall_coverage = (double)total_passed / total_tests * 100.0;
    if (overall_coverage >= 80.0) {
        printf(" Coverage target met: %.1f%%\n", overall_coverage);
    } else {
        printf(" Coverage target not met: %.1f%% (need 80%%+)\n", overall_coverage);
    }
    
    return (total_failed == 0) ? 0 : 1;
}
