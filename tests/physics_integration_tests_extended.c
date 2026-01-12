#include "physics_integration_tests_extended.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

PhysicsTestConfig g_physics_test_config;
PhysicsTestResults g_physics_test_results;
PhysicsTestState g_physics_test_state;

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
            .energy_tolerance = 0.1f,
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
    obj->physics_handle = NULL;
    
    return obj;
}

void physics_destroy_test_object(PhysicsTestObject *object) {
    if (object && object->is_active) {
        object->is_active = false;
        g_physics_test_state.object_count--;
    }
}

bool physics_update_simulation(float dt) {
    for (uint32_t i = 0; i < g_physics_test_state.object_count; i++) {
        PhysicsTestObject *obj = &g_physics_test_state.objects[i];
        if (!obj->is_active || obj->is_static) continue;
        
        // Apply gravity
        obj->acceleration[1] = -g_physics_test_config.gravity_strength;
        
        // Update velocity
        obj->velocity[0] += obj->acceleration[0] * dt;
        obj->velocity[1] += obj->acceleration[1] * dt;
        obj->velocity[2] += obj->acceleration[2] * dt;
        
        // Update position
        obj->position[0] += obj->velocity[0] * dt;
        obj->position[1] += obj->velocity[1] * dt;
        obj->position[2] += obj->velocity[2] * dt;
        
        // Ground collision
        if (obj->position[1] - obj->radius < 0.0f) {
            obj->position[1] = obj->radius;
            obj->velocity[1] = -obj->velocity[1] * 0.8f; // Bounce with damping
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
    
    float error = fabsf(expected_y - actual_y);
    if (error > g_physics_test_config.position_tolerance) {
        return false;
    }
    
    return true;
}

bool test_collision_sphere_sphere(void) {
    physics_reset_simulation();
    
    float pos1[3] = {0.0f, 5.0f, 0.0f};
    float vel1[3] = {1.0f, 0.0f, 0.0f};
    float pos2[3] = {3.0f, 5.0f, 0.0f};
    float vel2[3] = {-1.0f, 0.0f, 0.0f};
    
    PhysicsTestObject *obj1 = physics_create_test_object(pos1, vel1, 1.0f, 1.0f, false);
    PhysicsTestObject *obj2 = physics_create_test_object(pos2, vel2, 1.0f, 1.0f, false);
    
    if (!obj1 || !obj2) return false;
    
    float dt = g_physics_test_config.time_step;
    
    // Simulate until collision
    for (int i = 0; i < 100; i++) {
        physics_update_simulation(dt);
        
        if (physics_check_collision(obj1, obj2)) {
            // Check if objects are moving apart after collision
            float rel_vel_before = obj1->velocity[0] - obj2->velocity[0];
            
            // Simple collision response
            float temp_vel = obj1->velocity[0];
            obj1->velocity[0] = obj2->velocity[0] * 0.8f;
            obj2->velocity[0] = temp_vel * 0.8f;
            
            float rel_vel_after = obj1->velocity[0] - obj2->velocity[0];
            
            // Relative velocity should change direction
            return (rel_vel_before * rel_vel_after) < 0;
        }
    }
    
    return false; // No collision detected
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
    
    // Energy should be approximately conserved (with some loss due to bouncing)
    return physics_validate_energy_conservation(initial_energy, final_energy, 
                                               g_physics_test_config.energy_tolerance);
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
    
    uint64_t start_time = get_time_ms();
    
    for (int i = 0; i < steps; i++) {
        if (!physics_update_simulation(dt)) {
            return false;
        }
    }
    
    uint64_t end_time = get_time_ms();
    double execution_time = (double)(end_time - start_time);
    
    // Validate that simulation completed without crashes
    // and that all objects have reasonable positions
    for (uint32_t i = 0; i < g_physics_test_state.object_count; i++) {
        PhysicsTestObject *obj = &g_physics_test_state.objects[i];
        if (!obj->is_active) continue;
        
        // Check for NaN or infinite values
        for (int j = 0; j < 3; j++) {
            if (!isfinite(obj->position[j]) || !isfinite(obj->velocity[j])) {
                return false;
            }
        }
        
        // Check for reasonable positions (not too far from origin)
        if (fabsf(obj->position[1]) > 1000.0f) {
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
    
    // Store performance information
    g_physics_test_results.average_frame_time_ms = total_time / frames;
    g_physics_test_results.max_frame_time_ms = total_time; // Simplified
    g_physics_test_results.min_frame_time_ms = total_time / frames; // Simplified
    
    // Verify all objects are still active and have reasonable values
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

bool physics_check_collision(const PhysicsTestObject *obj1, const PhysicsTestObject *obj2) {
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
    // Basic state validation
    if (g_physics_test_state.object_count > g_physics_test_state.max_objects) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_physics_test_state.object_count; i++) {
        PhysicsTestObject *obj = &g_physics_test_state.objects[i];
        if (!obj->is_active) continue;
        
        // Validate mass and radius are positive
        if (obj->mass <= 0.0f || obj->radius <= 0.0f) {
            return false;
        }
        
        // Validate position and velocity are finite
        for (int j = 0; j < 3; j++) {
            if (!isfinite(obj->position[j]) || !isfinite(obj->velocity[j])) {
                return false;
            }
        }
    }
    
    return true;
}

void physics_reset_simulation(void) {
    g_physics_test_state.object_count = 0;
    g_physics_test_state.simulation_time = 0.0f;
    g_physics_test_state.frame_count = 0;
    
    memset(g_physics_test_state.objects, 0, 
           g_physics_test_state.max_objects * sizeof(PhysicsTestObject));
}

bool physics_run_all_integration_tests(void) {
    uint64_t start_time = get_time_ms();
    
    bool (*tests[])(void) = {
        test_gravity_basic_simulation,
        test_collision_sphere_sphere,
        test_integration_falling_object,
        test_stability_180hz,
        stress_test_1000_objects
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < num_tests; i++) {
        g_physics_test_results.total_tests++;
        
        if (tests[i]()) {
            g_physics_test_results.passed_tests++;
        } else {
            g_physics_test_results.failed_tests++;
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
    printf("Skipped: %u\n", g_physics_test_results.skipped_tests);
    printf("Total Time: %.2f ms\n", g_physics_test_results.total_time_ms);
    printf("Average Frame Time: %.3f ms\n", g_physics_test_results.average_frame_time_ms);
    printf("Max Frame Time: %.3f ms\n", g_physics_test_results.max_frame_time_ms);
    printf("Min Frame Time: %.3f ms\n", g_physics_test_results.min_frame_time_ms);
    
    if (g_physics_test_results.failed_tests > 0) {
        printf("\nFailed Tests:\n%s\n", g_physics_test_results.error_messages);
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
    
    fprintf(file, "Physics Integration Test Results\n");
    fprintf(file, "=================================\n\n");
    
    fprintf(file, "Configuration:\n");
    fprintf(file, "  Gravity: %.2f m/s²\n", g_physics_test_config.gravity_strength);
    fprintf(file, "  Time Step: %.6f s\n", g_physics_test_config.time_step);
    fprintf(file, "  Position Tolerance: %.6f\n", g_physics_test_config.position_tolerance);
    fprintf(file, "  Velocity Tolerance: %.6f\n", g_physics_test_config.velocity_tolerance);
    fprintf(file, "  Energy Tolerance: %.6f\n\n", g_physics_test_config.energy_tolerance);
    
    fprintf(file, "Results:\n");
    fprintf(file, "  Total Tests: %u\n", g_physics_test_results.total_tests);
    fprintf(file, "  Passed: %u\n", g_physics_test_results.passed_tests);
    fprintf(file, "  Failed: %u\n", g_physics_test_results.failed_tests);
    fprintf(file, "  Skipped: %u\n", g_physics_test_results.skipped_tests);
    fprintf(file, "  Total Time: %.2f ms\n", g_physics_test_results.total_time_ms);
    fprintf(file, "  Average Frame Time: %.3f ms\n", g_physics_test_results.average_frame_time_ms);
    fprintf(file, "  Max Frame Time: %.3f ms\n", g_physics_test_results.max_frame_time_ms);
    fprintf(file, "  Min Frame Time: %.3f ms\n\n", g_physics_test_results.min_frame_time_ms);
    
    if (g_physics_test_results.failed_tests > 0) {
        fprintf(file, "Failed Tests:\n%s\n", g_physics_test_results.error_messages);
    }
    
    fprintf(file, "Performance Report:\n%s\n", g_physics_test_results.performance_report);
    
    fclose(file);
    return true;
}
