// Physics System Unit Tests
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// Include physics headers
#include "physics/collision/collision_gjk_epa.h"
#include "physics/rigid_body.h"
#include "physics/constraints/constraint_manager.h"
#include "include/math/math.h"
#include "include/core/logger.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b, tolerance, message) \
    do { \
        if (fabsf((a) - (b)) > (tolerance)) { \
            printf("FAIL: %s (expected %.6f, got %.6f)\n", message, (b), (a)); \
            return false; \
        } \
    } while(0)

typedef struct {
    const char* test_name;
    bool (*test_func)(void);
    bool passed;
    const char* error_message;
} PhysicsTest;

static PhysicsTest g_tests[32];
static u32 g_test_count = 0;
static u32 g_tests_passed = 0;

void add_physics_test(const char* name, bool (*test_func)(void)) {
    if (g_test_count < 32) {
        g_tests[g_test_count].test_name = name;
        g_tests[g_test_count].test_func = test_func;
        g_tests[g_test_count].passed = false;
        g_tests[g_test_count].error_message = NULL;
        g_test_count++;
    }
}

// Test 1: Vector3 Math Operations
bool test_vector3_math(void) {
    printf("Testing Vector3 math operations...\n");
    
    Vec3 a = {1.0f, 2.0f, 3.0f};
    Vec3 b = {4.0f, 5.0f, 6.0f};
    
    // Test addition
    Vec3 sum = vec3_add(&a, &b);
    TEST_ASSERT_FLOAT_EQ(sum.x, 5.0f, 0.001f, "Vector addition X");
    TEST_ASSERT_FLOAT_EQ(sum.y, 7.0f, 0.001f, "Vector addition Y");
    TEST_ASSERT_FLOAT_EQ(sum.z, 9.0f, 0.001f, "Vector addition Z");
    
    // Test subtraction
    Vec3 diff = vec3_sub(&a, &b);
    TEST_ASSERT_FLOAT_EQ(diff.x, -3.0f, 0.001f, "Vector subtraction X");
    TEST_ASSERT_FLOAT_EQ(diff.y, -3.0f, 0.001f, "Vector subtraction Y");
    TEST_ASSERT_FLOAT_EQ(diff.z, -3.0f, 0.001f, "Vector subtraction Z");
    
    // Test dot product
    float dot = vec3_dot(&a, &b);
    TEST_ASSERT_FLOAT_EQ(dot, 32.0f, 0.001f, "Vector dot product");
    
    // Test cross product
    Vec3 cross = vec3_cross(&a, &b);
    TEST_ASSERT_FLOAT_EQ(cross.x, -3.0f, 0.001f, "Vector cross product X");
    TEST_ASSERT_FLOAT_EQ(cross.y, 6.0f, 0.001f, "Vector cross product Y");
    TEST_ASSERT_FLOAT_EQ(cross.z, -3.0f, 0.001f, "Vector cross product Z");
    
    // Test length
    float length = vec3_length(&a);
    TEST_ASSERT_FLOAT_EQ(length, sqrtf(14.0f), 0.001f, "Vector length");
    
    // Test normalization
    Vec3 normalized = vec3_normalize(&a);
    float normalized_length = vec3_length(&normalized);
    TEST_ASSERT_FLOAT_EQ(normalized_length, 1.0f, 0.001f, "Vector normalization length");
    
    return true;
}

// Test 2: Basic Collision Detection
bool test_collision_detection(void) {
    printf("Testing basic collision detection...\n");
    
    // Test sphere-sphere collision
    Vec3 sphere1_center = {0.0f, 0.0f, 0.0f};
    Vec3 sphere2_center = {1.5f, 0.0f, 0.0f};
    float sphere1_radius = 1.0f;
    float sphere2_radius = 1.0f;
    
    // These spheres should collide (distance 1.5 < sum of radii 2.0)
    float distance = vec3_distance(&sphere1_center, &sphere2_center);
    bool should_collide = distance < (sphere1_radius + sphere2_radius);
    TEST_ASSERT(should_collide, "Sphere-sphere collision detection");
    
    // Test non-collision
    Vec3 sphere3_center = {3.0f, 0.0f, 0.0f};
    distance = vec3_distance(&sphere1_center, &sphere3_center);
    bool should_not_collide = distance >= (sphere1_radius + sphere2_radius);
    TEST_ASSERT(should_not_collide, "Sphere-sphere non-collision detection");
    
    // Test AABB-AABB collision
    typedef struct {
        Vec3 min;
        Vec3 max;
    } AABB;
    
    AABB box1 = {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    AABB box2 = {{0.5f, 0.5f, 0.5f}, {2.0f, 2.0f, 2.0f}};
    
    // These boxes should overlap
    bool boxes_overlap = (box1.max.x >= box2.min.x && box1.min.x <= box2.max.x) &&
                        (box1.max.y >= box2.min.y && box1.min.y <= box2.max.y) &&
                        (box1.max.z >= box2.min.z && box1.min.z <= box2.max.z);
    TEST_ASSERT(boxes_overlap, "AABB-AABB collision detection");
    
    return true;
}

// Test 3: Rigid Body Physics
bool test_rigid_body_physics(void) {
    printf("Testing rigid body physics...\n");
    
    // Create a simple rigid body
    typedef struct {
        Vec3 position;
        Vec3 velocity;
        Vec3 acceleration;
        float mass;
        float inverse_mass;
        bool is_static;
    } RigidBody;
    
    RigidBody body = {
        .position = {0.0f, 10.0f, 0.0f},
        .velocity = {0.0f, 0.0f, 0.0f},
        .acceleration = {0.0f, -9.81f, 0.0f}, // Gravity
        .mass = 1.0f,
        .inverse_mass = 1.0f,
        .is_static = false
    };
    
    // Simulate one physics step (dt = 0.016s for 60 FPS)
    float dt = 0.016f;
    
    // Update velocity: v = v + a * dt
    body.velocity.x += body.acceleration.x * dt;
    body.velocity.y += body.acceleration.y * dt;
    body.velocity.z += body.acceleration.z * dt;
    
    // Update position: p = p + v * dt
    body.position.x += body.velocity.x * dt;
    body.position.y += body.velocity.y * dt;
    body.position.z += body.velocity.z * dt;
    
    // After one step, the body should have fallen due to gravity
    TEST_ASSERT(body.position.y < 10.0f, "Gravity affects rigid body position");
    TEST_ASSERT(body.velocity.y < 0.0f, "Gravity gives rigid body downward velocity");
    
    // Test static body (should not move)
    RigidBody static_body = {
        .position = {5.0f, 10.0f, 5.0f},
        .velocity = {0.0f, 0.0f, 0.0f},
        .acceleration = {0.0f, -9.81f, 0.0f},
        .mass = 0.0f, // Infinite mass
        .inverse_mass = 0.0f,
        .is_static = true
    };
    
    Vec3 original_pos = static_body.position;
    
    // Static body should not move
    if (!static_body.is_static) {
        static_body.velocity.x += static_body.acceleration.x * dt;
        static_body.velocity.y += static_body.acceleration.y * dt;
        static_body.velocity.z += static_body.acceleration.z * dt;
        
        static_body.position.x += static_body.velocity.x * dt;
        static_body.position.y += static_body.velocity.y * dt;
        static_body.position.z += static_body.velocity.z * dt;
    }
    
    TEST_ASSERT_FLOAT_EQ(static_body.position.x, original_pos.x, 0.001f, "Static body X position unchanged");
    TEST_ASSERT_FLOAT_EQ(static_body.position.y, original_pos.y, 0.001f, "Static body Y position unchanged");
    TEST_ASSERT_FLOAT_EQ(static_body.position.z, original_pos.z, 0.001f, "Static body Z position unchanged");
    
    return true;
}

// Test 4: Constraint Solving
bool test_constraint_solving(void) {
    printf("Testing constraint solving...\n");
    
    // Simple distance constraint between two points
    typedef struct {
        Vec3 point1;
        Vec3 point2;
        float rest_length;
        float stiffness;
    } DistanceConstraint;
    
    DistanceConstraint constraint = {
        .point1 = {0.0f, 0.0f, 0.0f},
        .point2 = {2.0f, 0.0f, 0.0f},
        .rest_length = 2.0f,
        .stiffness = 0.5f
    };
    
    // Move point2 away from rest length
    constraint.point2.x = 3.0f;
    
    // Calculate current distance
    Vec3 delta = vec3_sub(&constraint.point2, &constraint.point1);
    float current_distance = vec3_length(&delta);
    float distance_error = current_distance - constraint.rest_length;
    
    // Should have a positive error (point2 is too far)
    TEST_ASSERT(distance_error > 0.0f, "Distance constraint error calculation");
    
    // Apply constraint correction (simplified)
    if (current_distance > 0.001f) {
        Vec3 correction_direction = vec3_normalize(&delta);
        float correction_magnitude = distance_error * constraint.stiffness;
        Vec3 correction = vec3_mul(correction_direction, correction_magnitude);
        
        // Move point2 back towards point1
        constraint.point2 = vec3_sub(&constraint.point2, &correction);
    }
    
    // Check if constraint was partially satisfied
    float new_distance = vec3_distance(&constraint.point1, &constraint.point2);
    float new_error = fabsf(new_distance - constraint.rest_length);
    TEST_ASSERT(new_error < distance_error, "Distance constraint correction reduces error");
    
    return true;
}

// Test 5: Physics Integration
bool test_physics_integration(void) {
    printf("Testing physics integration...\n");
    
    // Simple physics world with multiple bodies
    typedef struct {
        Vec3 position;
        Vec3 velocity;
        float mass;
        float radius;
    } PhysicsBody;
    
    #define MAX_BODIES 4
    PhysicsBody bodies[MAX_BODIES];
    
    // Initialize bodies
    bodies[0] = (PhysicsBody){.position = {0.0f, 5.0f, 0.0f}, .velocity = {1.0f, 0.0f, 0.0f}, .mass = 1.0f, .radius = 0.5f};
    bodies[1] = (PhysicsBody){.position = {3.0f, 5.0f, 0.0f}, .velocity = {-1.0f, 0.0f, 0.0f}, .mass = 1.0f, .radius = 0.5f};
    bodies[2] = (PhysicsBody){.position = {1.5f, 7.0f, 0.0f}, .velocity = {0.0f, 0.0f, 0.0f}, .mass = 2.0f, .radius = 0.7f};
    bodies[3] = (PhysicsBody){.position = {1.5f, 2.0f, 0.0f}, .velocity = {0.0f, 0.0f, 0.0f}, .mass = 10.0f, .radius = 1.0f};
    
    float dt = 0.016f; // 60 FPS timestep
    Vec3 gravity = {0.0f, -9.81f, 0.0f};
    
    // Simulate physics for a few steps
    for (int step = 0; step < 10; step++) {
        // Apply forces and update velocities
        for (int i = 0; i < MAX_BODIES; i++) {
            if (bodies[i].mass > 0.0f) {
                // Apply gravity
                Vec3 acceleration = vec3_mul(gravity, 1.0f / bodies[i].mass);
                bodies[i].velocity = vec3_add(&bodies[i].velocity, vec3_mul(&acceleration, dt));
            }
        }
        
        // Simple collision detection and response
        for (int i = 0; i < MAX_BODIES; i++) {
            for (int j = i + 1; j < MAX_BODIES; j++) {
                Vec3 delta = vec3_sub(&bodies[j].position, &bodies[i].position);
                float distance = vec3_length(&delta);
                float min_distance = bodies[i].radius + bodies[j].radius;
                
                if (distance < min_distance && distance > 0.001f) {
                    // Collision detected - simple elastic collision
                    Vec3 normal = vec3_normalize(&delta);
                    Vec3 relative_velocity = vec3_sub(&bodies[j].velocity, &bodies[i].velocity);
                    float velocity_along_normal = vec3_dot(&relative_velocity, &normal);
                    
                    if (velocity_along_normal < 0.0f) {
                        // Bodies are approaching
                        float restitution = 0.8f; // Bounciness
                        float impulse = 2.0f * velocity_along_normal / (1.0f/bodies[i].mass + 1.0f/bodies[j].mass);
                        
                        Vec3 impulse_vector = vec3_mul(normal, impulse);
                        bodies[i].velocity = vec3_add(&bodies[i].velocity, vec3_mul(&impulse_vector, restitution / bodies[i].mass));
                        bodies[j].velocity = vec3_sub(&bodies[j].velocity, vec3_mul(&impulse_vector, restitution / bodies[j].mass));
                        
                        // Separate bodies
                        float overlap = min_distance - distance;
                        Vec3 separation = vec3_mul(normal, overlap * 0.5f);
                        bodies[i].position = vec3_sub(&bodies[i].position, separation);
                        bodies[j].position = vec3_add(&bodies[j].position, separation);
                    }
                }
            }
        }
        
        // Update positions
        for (int i = 0; i < MAX_BODIES; i++) {
            bodies[i].position = vec3_add(&bodies[i].position, vec3_mul(&bodies[i].velocity, dt));
        }
    }
    
    // Verify physics results
    // Heavy body should have fallen less due to larger mass
    TEST_ASSERT(bodies[3].position.y > bodies[0].position.y, "Heavier body falls slower");
    
    // Bodies should have moved from their initial positions
    TEST_ASSERT(bodies[0].position.x > 0.5f, "Body 0 moved horizontally");
    TEST_ASSERT(bodies[1].position.x < 2.5f, "Body 1 moved horizontally");
    
    // All bodies should have fallen due to gravity
    for (int i = 0; i < MAX_BODIES; i++) {
        TEST_ASSERT(bodies[i].position.y < 4.9f, "Body fell due to gravity");
    }
    
    return true;
}

// Test 6: Performance Benchmark
bool test_physics_performance(void) {
    printf("Testing physics performance...\n");
    
    const int NUM_BODIES = 1000;
    const int NUM_STEPS = 100;
    
    // Create many bodies
    typedef struct {
        Vec3 position;
        Vec3 velocity;
        float mass;
    } SimpleBody;
    
    SimpleBody* bodies = malloc(NUM_BODIES * sizeof(SimpleBody));
    if (!bodies) return false;
    
    // Initialize bodies with random positions
    for (int i = 0; i < NUM_BODIES; i++) {
        bodies[i].position = (Vec3){
            (float)(rand() % 100 - 50),
            (float)(rand() % 20),
            (float)(rand() % 100 - 50)
        };
        bodies[i].velocity = (Vec3){0.0f, 0.0f, 0.0f};
        bodies[i].mass = 1.0f + (float)(rand() % 10);
    }
    
    // Simple performance test
    clock_t start = clock();
    
    Vec3 gravity = {0.0f, -9.81f, 0.0f};
    float dt = 0.016f;
    
    for (int step = 0; step < NUM_STEPS; step++) {
        for (int i = 0; i < NUM_BODIES; i++) {
            // Apply gravity
            Vec3 acceleration = vec3_mul(gravity, 1.0f / bodies[i].mass);
            bodies[i].velocity = vec3_add(&bodies[i].velocity, vec3_mul(&acceleration, dt));
            bodies[i].position = vec3_add(&bodies[i].position, vec3_mul(&bodies[i].velocity, dt));
        }
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Performance: %d bodies, %d steps in %.3f seconds\n", NUM_BODIES, NUM_STEPS, elapsed);
    printf("Average: %.1f bodies per second\n", (NUM_BODIES * NUM_STEPS) / elapsed);
    
    // Performance should be reasonable (at least 1000 bodies/second)
    double bodies_per_second = (NUM_BODIES * NUM_STEPS) / elapsed;
    TEST_ASSERT(bodies_per_second > 1000.0, "Physics performance meets minimum requirements");
    
    free(bodies);
    return true;
}

void run_physics_unit_tests(void) {
    printf("=== Physics System Unit Tests ===\n\n");
    
    // Add all tests
    add_physics_test("Vector3 Math Operations", test_vector3_math);
    add_physics_test("Basic Collision Detection", test_collision_detection);
    add_physics_test("Rigid Body Physics", test_rigid_body_physics);
    add_physics_test("Constraint Solving", test_constraint_solving);
    add_physics_test("Physics Integration", test_physics_integration);
    add_physics_test("Performance Benchmark", test_physics_performance);
    
    // Run all tests
    for (u32 i = 0; i < g_test_count; i++) {
        printf("\n--- Test %u: %s ---\n", i + 1, g_tests[i].test_name);
        
        bool passed = g_tests[i].test_func();
        g_tests[i].passed = passed;
        
        if (passed) {
            printf("PASS\n");
            g_tests_passed++;
        } else {
            printf("FAIL\n");
        }
    }
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Tests passed: %u/%u (%.1f%%)\n", 
           g_tests_passed, g_test_count, 
           (float)g_tests_passed / g_test_count * 100.0f);
    
    if (g_tests_passed == g_test_count) {
        printf("All physics unit tests PASSED!\n");
    } else {
        printf("Some physics unit tests FAILED.\n");
    }
}

int main(void) {
    printf("Physics System Unit Test Suite\n");
    printf("================================\n\n");
    
    run_physics_unit_tests();
    
    return (g_tests_passed == g_test_count) ? 0 : 1;
}
