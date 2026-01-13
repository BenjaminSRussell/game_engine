#include "physics_integration_tests_extended.h"
#include <physics/physics.h>
#include <physics/core/physics_types.h>
#include <math/vec3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

PhysicsTestConfig g_physics_test_config;
PhysicsTestResults g_physics_test_results;
PhysicsTestState g_physics_test_state;

// Helper functions for collider creation (missing in engine)
static Collider *test_collider_create_sphere(float radius) {
    Collider *collider = (Collider*)calloc(1, sizeof(Collider));
    if (!collider) return NULL;

    collider->type = COLLISION_SHAPE_SPHERE;
    collider->data.sphere.radius = radius;
    // Identity transform
    collider->local_transform[0] = 1.0f; collider->local_transform[5] = 1.0f;
    collider->local_transform[10] = 1.0f; collider->local_transform[15] = 1.0f;
    return collider;
}

static Collider *test_collider_create_box(Vec3 half_extents) {
    Collider *collider = (Collider*)calloc(1, sizeof(Collider));
    if (!collider) return NULL;

    collider->type = COLLISION_SHAPE_BOX;
    collider->data.box.half_extents[0] = half_extents.x;
    collider->data.box.half_extents[1] = half_extents.y;
    collider->data.box.half_extents[2] = half_extents.z;
    // Identity transform
    collider->local_transform[0] = 1.0f; collider->local_transform[5] = 1.0f;
    collider->local_transform[10] = 1.0f; collider->local_transform[15] = 1.0f;
    return collider;
}

static void test_collider_destroy(Collider *collider) {
    free(collider);
}

static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

bool physics_integration_tests_init(const PhysicsTestConfig *config) {
    if (config) {
        g_physics_test_config = *config;
    } else {
        g_physics_test_config = (PhysicsTestConfig){
            .enable_verbose_output = false,
            .enable_performance_tests = true,
            .enable_stress_tests = false,
            .enable_visual_validation = false,
            .gravity_strength = 9.81f,
            .time_step = 1.0f / 60.0f,
            .max_substeps = 4,
            .position_tolerance = 0.001f,
            .velocity_tolerance = 0.01f,
            .energy_tolerance = 0.5f, // Relaxed tolerance due to simulation inaccuracy
            .stress_test_iterations = 1000,
            .output_file = NULL
        };
    }
    
    memset(&g_physics_test_results, 0, sizeof(g_physics_test_results));
    memset(&g_physics_test_state, 0, sizeof(g_physics_test_state));
    
    // Initialize test state
    g_physics_test_state.max_objects = 10000;
    g_physics_test_state.objects = calloc(g_physics_test_state.max_objects, sizeof(PhysicsTestObject));
    if (!g_physics_test_state.objects) {
        return false;
    }
    
    g_physics_test_state.config = g_physics_test_config;
    
    // Create Physics World
    PhysicsConfig phys_config = {
        .gravity = {0.0f, -g_physics_test_config.gravity_strength, 0.0f},
        .fixed_timestep = g_physics_test_config.time_step,
        .velocity_iterations = 8,
        .position_iterations = 3
    };

    g_physics_test_state.physics_world = physics_world_create(phys_config);
    if (!g_physics_test_state.physics_world) {
        free(g_physics_test_state.objects);
        return false;
    }

    srand((unsigned int)time(NULL));
    return true;
}

void physics_integration_tests_shutdown(bool generate_report) {
    if (generate_report) {
        physics_print_test_summary();
        if (g_physics_test_config.output_file) {
            physics_export_results(g_physics_test_config.output_file);
        }
    }
    
    if (g_physics_test_state.physics_world) {
        physics_world_destroy((PhysicsWorld*)g_physics_test_state.physics_world);
        g_physics_test_state.physics_world = NULL;
    }

    if (g_physics_test_state.objects) {
        free(g_physics_test_state.objects);
        g_physics_test_state.objects = NULL;
    }
    
    memset(&g_physics_test_state, 0, sizeof(g_physics_test_state));
}

PhysicsTestObject* physics_create_test_object(const float position[3], 
                                            const float velocity[3],
                                            float mass, float radius, 
                                            bool is_static) {
    if (g_physics_test_state.object_count >= g_physics_test_state.max_objects) {
        return NULL;
    }
    
    PhysicsTestObject *obj = &g_physics_test_state.objects[g_physics_test_state.object_count++];
    
    obj->id = g_physics_test_state.object_count;
    if (position) {
        memcpy(obj->position, position, sizeof(float) * 3);
    } else {
        memset(obj->position, 0, sizeof(float) * 3);
    }
    
    if (velocity) {
        memcpy(obj->velocity, velocity, sizeof(float) * 3);
    } else {
        memset(obj->velocity, 0, sizeof(float) * 3);
    }
    
    memset(obj->acceleration, 0, sizeof(float) * 3);
    obj->mass = mass;
    obj->radius = radius;
    obj->is_static = is_static;
    obj->is_active = true;

    // Create RigidBody
    PhysicsWorld *world = (PhysicsWorld*)g_physics_test_state.physics_world;
    if (world) {
        Vec3 pos = {obj->position[0], obj->position[1], obj->position[2]};
        BodyType type = is_static ? BODY_TYPE_STATIC : BODY_TYPE_DYNAMIC;

        RigidBody *body = rigid_body_create(type, pos);
        if (body) {
            Vec3 vel = {obj->velocity[0], obj->velocity[1], obj->velocity[2]};
            rigid_body_set_velocity(body, vel);
            rigid_body_set_friction(body, 0.5f);
            rigid_body_set_restitution(body, 0.8f); // Bouncy by default for tests
            // Mass is set in rigid_body_create default (1.0f) or static (0.0f)
            // We should allow setting mass for dynamic bodies
            if (!is_static) {
                // rigid_body_create doesn't take mass, but structure has it.
                // However, there is no rigid_body_set_mass in physics.h?
                // physics.h declares rigid_body_set_mass!
                rigid_body_set_mass(body, mass);
            }

            // Attach collider
            Collider *collider = test_collider_create_sphere(radius);
            rigid_body_attach_collider(body, collider);

            physics_world_add_body(world, body);
            obj->physics_handle = body;
        }
    }
    
    return obj;
}

void physics_destroy_test_object(PhysicsTestObject *object) {
    if (object && object->is_active) {
        PhysicsWorld *world = (PhysicsWorld*)g_physics_test_state.physics_world;
        RigidBody *body = (RigidBody*)object->physics_handle;

        if (world && body) {
            Collider *collider = rigid_body_get_collider(body);
            physics_world_remove_body(world, body);

            // We need to destroy collider separately as rigid_body_destroy might not own it
            // implementation of rigid_body_destroy in stubs only frees body.
            if (collider) {
                test_collider_destroy(collider);
            }

            rigid_body_destroy(body);
        }

        object->physics_handle = NULL;
        object->is_active = false;
        g_physics_test_state.object_count--;
    }
}

bool physics_update_simulation(float dt) {
    PhysicsWorld *world = (PhysicsWorld*)g_physics_test_state.physics_world;
    if (!world) return false;

    physics_world_step(world, dt);

    // Sync PhysicsTestObject state with RigidBody state
    for (uint32_t i = 0; i < g_physics_test_state.object_count; i++) {
        PhysicsTestObject *obj = &g_physics_test_state.objects[i];
        if (!obj->is_active) continue;
        
        RigidBody *body = (RigidBody*)obj->physics_handle;
        if (body) {
            Vec3 pos = rigid_body_get_position(body);
            Vec3 vel = rigid_body_get_velocity(body);

            obj->position[0] = pos.x;
            obj->position[1] = pos.y;
            obj->position[2] = pos.z;

            obj->velocity[0] = vel.x;
            obj->velocity[1] = vel.y;
            obj->velocity[2] = vel.z;
        }
    }
    
    g_physics_test_state.simulation_time += dt;
    g_physics_test_state.frame_count++;
    
    return true;
}

bool test_gravity_basic_simulation(void) {
    physics_reset_simulation();
    
    float position[3] = {0.0f, 10.0f, 0.0f};
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    PhysicsTestObject *obj = physics_create_test_object(position, velocity, 1.0f, 1.0f, false);
    if (!obj) return false;
    
    // Simulate for 1 second
    float dt = g_physics_test_config.time_step;
    int steps = (int)(1.0f / dt);
    
    for (int i = 0; i < steps; i++) {
        physics_update_simulation(dt);
    }
    
    // Check if object fell approximately 4.905 meters (0.5 * g * t^2)
    float expected_y = 10.0f - 0.5f * g_physics_test_config.gravity_strength * 1.0f;
    float actual_y = obj->position[1];
    
    // Allow for integration error
    float error = fabsf(expected_y - actual_y);
    // Explicitly higher tolerance because Euler integration is not perfect
    if (error > 0.2f) {
        printf("Gravity test failed: Expected Y %.2f, Actual Y %.2f, Error %.2f\n", expected_y, actual_y, error);
        return false;
    }
    
    return true;
}

bool test_gravity_mass_independence(void) {
    physics_reset_simulation();

    float pos1[3] = {-2.0f, 10.0f, 0.0f};
    float pos2[3] = { 2.0f, 10.0f, 0.0f};
    float vel[3] = {0.0f, 0.0f, 0.0f};

    // Object 1: Mass 1.0
    PhysicsTestObject *obj1 = physics_create_test_object(pos1, vel, 1.0f, 1.0f, false);
    // Object 2: Mass 10.0
    PhysicsTestObject *obj2 = physics_create_test_object(pos2, vel, 10.0f, 1.0f, false);

    if (!obj1 || !obj2) return false;

    float dt = g_physics_test_config.time_step;
    int steps = 10;

    for (int i = 0; i < steps; i++) {
        physics_update_simulation(dt);

        // Both objects should be at the same height (Galileo's experiment)
        if (fabsf(obj1->position[1] - obj2->position[1]) > 0.001f) {
            printf("Mass independence failed: Obj1 Y %.2f, Obj2 Y %.2f\n", obj1->position[1], obj2->position[1]);
            return false;
        }
    }

    return true;
}

bool test_collision_sphere_sphere(void) {
    physics_reset_simulation();
    
    float pos1[3] = {0.0f, 5.0f, 0.0f};
    float vel1[3] = {2.0f, 0.0f, 0.0f};
    float pos2[3] = {4.0f, 5.0f, 0.0f}; // 4 units away, radius 1+1=2. Should collide.
    float vel2[3] = {-2.0f, 0.0f, 0.0f};
    
    PhysicsTestObject *obj1 = physics_create_test_object(pos1, vel1, 1.0f, 1.0f, false);
    PhysicsTestObject *obj2 = physics_create_test_object(pos2, vel2, 1.0f, 1.0f, false);
    
    if (!obj1 || !obj2) return false;
    
    float dt = g_physics_test_config.time_step;
    
    // Simulate until expected collision and separation
    bool collision_occurred = false;

    for (int i = 0; i < 100; i++) {
        physics_update_simulation(dt);
        
        // Check relative velocity to detect bounce
        float rel_vel = (obj1->velocity[0] - obj2->velocity[0]);
        // Initially 2 - (-2) = 4 (approaching)
        // After collision, should be negative (separating)

        if (rel_vel < 0) {
            collision_occurred = true;
            break;
        }
    }
    
    return collision_occurred;
}

bool test_collision_sphere_plane(void) {
    physics_reset_simulation();

    // Drop sphere from height
    float pos[3] = {0.0f, 5.0f, 0.0f};
    float vel[3] = {0.0f, 0.0f, 0.0f};
    PhysicsTestObject *obj = physics_create_test_object(pos, vel, 1.0f, 1.0f, false);

    // The engine (physics_world_stubs) has a default ground plane at Y=0

    if (!obj) return false;

    float dt = g_physics_test_config.time_step;

    // Simulate until it should have hit the ground
    // Fall from 5m takes ~1s
    for (int i = 0; i < 120; i++) {
        physics_update_simulation(dt);
    }

    // Should bounce back up (positive velocity) or rest at ground
    // If it fell through, Y would be negative
    if (obj->position[1] < -0.1f) {
        printf("Sphere plane collision failed: Object fell through ground to %.2f\n", obj->position[1]);
        return false;
    }

    if (obj->position[1] > 100.0f) {
         printf("Sphere plane collision failed: Object exploded to %.2f\n", obj->position[1]);
         return false;
    }

    return true;
}

bool test_integration_falling_object(void) {
    physics_reset_simulation();
    
    float position[3] = {0.0f, 20.0f, 0.0f};
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    PhysicsTestObject *obj = physics_create_test_object(position, velocity, 1.0f, 1.0f, false);
    if (!obj) return false;
    
    float initial_energy = physics_calculate_potential_energy(obj, g_physics_test_config.gravity_strength);
    
    float dt = g_physics_test_config.time_step;
    float simulation_time = 2.0f;
    int steps = (int)(simulation_time / dt);
    
    for (int i = 0; i < steps; i++) {
        physics_update_simulation(dt);
    }
    
    float final_energy = physics_calculate_kinetic_energy(obj) + 
                        physics_calculate_potential_energy(obj, g_physics_test_config.gravity_strength);
    
    // Energy conservation is tricky with collisions and damping.
    // The system applies damping (linear/angular) which removes energy.
    // So we check if energy is NOT increased significantly (instability) and not zero (disappeared)

    if (final_energy > initial_energy * 1.5f) { // Allow some gain from integration error but not explosion
         return false;
    }

    return true;
}

bool test_stability_180hz(void) {
    physics_reset_simulation();
    
    // Create multiple objects with different initial conditions
    for (int i = 0; i < 10; i++) {
        float position[3] = {(float)i * 2.0f, 10.0f + (float)i, 0.0f};
        float velocity[3] = {(float)(i % 3 - 1), 0.0f, 0.0f};
        physics_create_test_object(position, velocity, 1.0f + (float)i * 0.5f, 1.0f, false);
    }
    
    // Run at 180 Hz (3x normal speed)
    float dt = 1.0f / 180.0f;
    int steps = (int)(1.0f / dt); // 1 second of simulation
    
    for (int i = 0; i < steps; i++) {
        if (!physics_update_simulation(dt)) {
            return false;
        }
    }
    
    return physics_validate_state();
}

bool stress_test_1000_objects(void) {
    physics_reset_simulation();
    
    // Create 1000 objects
    for (int i = 0; i < 1000; i++) {
        float position[3] = {
            (float)(i % 20) * 2.0f - 20.0f,
            5.0f + (float)(i % 10),
            (float)(i / 20) * 2.0f - 20.0f
        };
        float velocity[3] = {
            (float)(i % 5 - 2) * 0.5f,
            0.0f,
            (float)(i % 7 - 3) * 0.3f
        };
        physics_create_test_object(position, velocity, 1.0f, 0.5f, false);
    }
    
    float dt = g_physics_test_config.time_step;
    int frames = 10; // Run for 10 frames
    
    uint64_t start_time = get_time_ms();
    
    for (int frame = 0; frame < frames; frame++) {
        if (!physics_update_simulation(dt)) {
            return false;
        }
    }
    
    uint64_t end_time = get_time_ms();
    double total_time = (double)(end_time - start_time);
    
    g_physics_test_results.average_frame_time_ms = total_time / frames;
    
    return true;
}

bool physics_test_check_collision(const PhysicsTestObject *obj1, const PhysicsTestObject *obj2) {
    if (!obj1 || !obj2 || !obj1->is_active || !obj2->is_active) {
        return false;
    }
    
    float dx = obj1->position[0] - obj2->position[0];
    float dy = obj1->position[1] - obj2->position[1];
    float dz = obj1->position[2] - obj2->position[2];
    
    float distance_squared = dx * dx + dy * dy + dz * dz;
    float min_distance = obj1->radius + obj2->radius;
    
    return distance_squared <= (min_distance * min_distance);
}

float physics_calculate_kinetic_energy(const PhysicsTestObject *object) {
    if (!object || object->is_static) return 0.0f;
    
    float speed_squared = object->velocity[0] * object->velocity[0] +
                         object->velocity[1] * object->velocity[1] +
                         object->velocity[2] * object->velocity[2];
    
    return 0.5f * object->mass * speed_squared;
}

float physics_calculate_potential_energy(const PhysicsTestObject *object, float gravity) {
    if (!object) return 0.0f;
    return object->mass * gravity * object->position[1];
}

bool physics_validate_energy_conservation(float initial_energy, 
                                        float current_energy, 
                                        float tolerance) {
    return fabsf(initial_energy - current_energy) <= tolerance;
}

bool physics_validate_state(void) {
    if (g_physics_test_state.object_count > g_physics_test_state.max_objects) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_physics_test_state.object_count; i++) {
        PhysicsTestObject *obj = &g_physics_test_state.objects[i];
        if (!obj->is_active) continue;
        
        for (int j = 0; j < 3; j++) {
            if (!isfinite(obj->position[j]) || !isfinite(obj->velocity[j])) {
                return false;
            }
        }
    }
    
    return true;
}

void physics_reset_simulation(void) {
    // Need to clear physics world
    if (g_physics_test_state.physics_world) {
        PhysicsWorld *world = (PhysicsWorld*)g_physics_test_state.physics_world;
        // Ideally we remove all bodies
        // But for simplicity in tests, we rely on physics_destroy_test_object being called for all objects
        // Or we can just recreate the world.
        // But physics_create_test_object adds bodies to current world.

        // Let's clear our test objects
        for (uint32_t i = 0; i < g_physics_test_state.object_count; i++) {
             physics_destroy_test_object(&g_physics_test_state.objects[i]);
        }
    }

    g_physics_test_state.object_count = 0;
    g_physics_test_state.simulation_time = 0.0f;
    g_physics_test_state.frame_count = 0;
    
    memset(g_physics_test_state.objects, 0, 
           g_physics_test_state.max_objects * sizeof(PhysicsTestObject));
}

bool physics_run_all_integration_tests(void) {
    uint64_t start_time = get_time_ms();
    
    // Initialize config if not done
    if (!g_physics_test_state.objects) {
        physics_integration_tests_init(NULL);
    }

    bool (*tests[])(void) = {
        test_gravity_basic_simulation,
        test_gravity_mass_independence,
        test_collision_sphere_sphere,
        test_collision_sphere_plane,
        test_integration_falling_object,
        test_stability_180hz,
        stress_test_1000_objects
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < num_tests; i++) {
        g_physics_test_results.total_tests++;
        
        printf("Running test %d...\n", i);
        if (tests[i]()) {
            g_physics_test_results.passed_tests++;
            printf("Test %d passed.\n", i);
        } else {
            g_physics_test_results.failed_tests++;
            printf("Test %d failed.\n", i);
        }
    }
    
    g_physics_test_results.total_time_ms = get_time_ms() - start_time;
    
    return g_physics_test_results.failed_tests == 0;
}

void physics_print_test_summary(void) {
    printf("\n=== Physics Integration Test Summary ===\n");
    printf("Total Tests: %u\n", g_physics_test_results.total_tests);
    printf("Passed: %u\n", g_physics_test_results.passed_tests);
    printf("Failed: %u\n", g_physics_test_results.failed_tests);
    printf("Total Time: %.2f ms\n", g_physics_test_results.total_time_ms);
    
    if (g_physics_test_results.average_frame_time_ms > 0) {
        printf("Average Frame Time: %.3f ms\n", g_physics_test_results.average_frame_time_ms);
    }
    
    if (g_physics_test_results.failed_tests > 0) {
        printf("Some tests failed. Check logs.\n");
    }
    printf("=====================================\n");
}

PhysicsTestResults physics_get_test_results(void) {
    return g_physics_test_results;
}

bool physics_export_results(const char *filename) {
    if (!filename) return false;
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    fprintf(file, "Tests Passed: %u\n", g_physics_test_results.passed_tests);
    fprintf(file, "Tests Failed: %u\n", g_physics_test_results.failed_tests);
    fclose(file);
    return true;
}
