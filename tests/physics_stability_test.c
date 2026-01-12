#include "physics_stability_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

PhysicsStabilityTestConfig g_physics_stability_test_config;
PhysicsStabilityTestResults g_physics_stability_test_results;

static PhysicsObject *g_physics_objects = NULL;
static uint32_t g_physics_object_capacity = 0;
static uint32_t g_next_object_id = 1;
static CollisionContact *g_collision_contacts = NULL;
static uint32_t g_max_contacts = 0;

static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

bool physics_stability_test_init(const PhysicsStabilityTestConfig *config) {
    if (config) {
        g_physics_stability_test_config = *config;
    } else {
        g_physics_stability_test_config = (PhysicsStabilityTestConfig){
            .enable_verbose_output = false,
            .enable_detailed_logging = false,
            .enable_energy_tracking = true,
            .enable_position_tracking = true,
            .target_frequency = 180.0f,
            .simulation_duration = 5.0f,
            .gravity_strength = 9.81f,
            .time_step = 1.0f / 180.0f,
            .max_substeps = 4,
            .position_tolerance = 0.01f,
            .velocity_tolerance = 0.1f,
            .energy_tolerance = 0.1f,
            .object_count = 100,
            .enable_collisions = true,
            .enable_constraints = false,
            .enable_sleeping = true,
            .output_file = NULL
        };
    }
    
    memset(&g_physics_stability_test_results, 0, sizeof(g_physics_stability_test_results));
    
    // Allocate physics objects array
    g_physics_object_capacity = g_physics_stability_test_config.object_count + 50;
    g_physics_objects = calloc(g_physics_object_capacity, sizeof(PhysicsObject));
    if (!g_physics_objects) {
        return false;
    }
    
    // Allocate collision contacts array
    g_max_contacts = g_physics_stability_test_config.object_count * 2;
    g_collision_contacts = malloc(g_max_contacts * sizeof(CollisionContact));
    if (!g_collision_contacts) {
        free(g_physics_objects);
        return false;
    }
    
    srand((unsigned int)time(NULL));
    return true;
}

void physics_stability_test_shutdown(bool generate_report) {
    // Destroy all physics objects
    for (uint32_t i = 0; i < g_physics_stability_test_results.total_frames; i++) {
        if (g_physics_objects[i].is_active) {
            physics_destroy_object(&g_physics_objects[i]);
        }
    }
    
    if (g_physics_objects) {
        free(g_physics_objects);
        g_physics_objects = NULL;
    }
    
    if (g_collision_contacts) {
        free(g_collision_contacts);
        g_collision_contacts = NULL;
    }
    
    if (generate_report) {
        physics_stability_test_print_summary();
        if (g_physics_stability_test_config.output_file) {
            physics_stability_test_export_results(g_physics_stability_test_config.output_file);
        }
    }
}

PhysicsObject* physics_create_object(const float position[3],
                                   const float velocity[3],
                                   float mass, float radius,
                                   bool is_static) {
    if (!g_physics_objects || g_next_object_id > g_physics_object_capacity) {
        return NULL;
    }
    
    PhysicsObject *object = &g_physics_objects[g_next_object_id - 1];
    
    memset(object, 0, sizeof(PhysicsObject));
    
    object->id = g_next_object_id++;
    snprintf(object->name, sizeof(object->name), "PhysicsObj_%u", object->id);
    
    if (position) {
        memcpy(object->position, position, sizeof(float) * 3);
    } else {
        memset(object->position, 0, sizeof(float) * 3);
    }
    
    if (velocity) {
        memcpy(object->velocity, velocity, sizeof(float) * 3);
    } else {
        memset(object->velocity, 0, sizeof(float) * 3);
    }
    
    object->mass = mass;
    object->radius = radius;
    object->restitution = 0.8f;
    object->friction = 0.3f;
    object->is_static = is_static;
    object->is_active = true;
    object->is_sleeping = false;
    
    // Initialize rotation to identity quaternion
    object->rotation[0] = object->rotation[1] = object->rotation[2] = 0.0f;
    object->rotation[3] = 1.0f;
    memset(object->angular_velocity, 0, sizeof(float) * 3);
    
    // Allocate history arrays
    object->history_size = 1000; // Store last 1000 frames
    object->position_history = calloc(object->history_size, sizeof(float) * 3);
    object->velocity_history = calloc(object->history_size, sizeof(float) * 3);
    
    if (!object->position_history || !object->velocity_history) {
        return NULL;
    }
    
    object->creation_time = get_time_ns();
    object->last_update_time = object->creation_time;
    
    // Calculate initial energy
    object->initial_energy = calculate_total_energy(object, g_physics_stability_test_config.gravity_strength);
    object->current_energy = object->initial_energy;
    object->max_energy = object->initial_energy;
    object->min_energy = object->initial_energy;
    
    return object;
}

void physics_destroy_object(PhysicsObject *object) {
    if (!object || !object->is_active) return;
    
    if (object->position_history) {
        free(object->position_history);
    }
    
    if (object->velocity_history) {
        free(object->velocity_history);
    }
    
    object->is_active = false;
}

void physics_update_object(PhysicsObject *object, float dt, const float gravity[3]) {
    if (!object || !object->is_active || object->is_static) return;
    
    // Apply gravity
    object->acceleration[0] = gravity[0];
    object->acceleration[1] = gravity[1];
    object->acceleration[2] = gravity[2];
    
    // Update velocity
    object->velocity[0] += object->acceleration[0] * dt;
    object->velocity[1] += object->acceleration[1] * dt;
    object->velocity[2] += object->acceleration[2] * dt;
    
    // Apply damping
    float damping = 0.999f;
    object->velocity[0] *= damping;
    object->velocity[1] *= damping;
    object->velocity[2] *= damping;
    
    // Update position
    object->position[0] += object->velocity[0] * dt;
    object->position[1] += object->velocity[1] * dt;
    object->position[2] += object->velocity[2] * dt;
    
    // Update history
    object->history_index = (object->history_index + 1) % object->history_size;
    uint32_t idx = object->history_index;
    object->position_history[idx * 3 + 0] = object->position[0];
    object->position_history[idx * 3 + 1] = object->position[1];
    object->position_history[idx * 3 + 2] = object->position[2];
    object->velocity_history[idx * 3 + 0] = object->velocity[0];
    object->velocity_history[idx * 3 + 1] = object->velocity[1];
    object->velocity_history[idx * 3 + 2] = object->velocity[2];
    
    // Update energy
    object->current_energy = calculate_total_energy(object, g_physics_stability_test_config.gravity_strength);
    if (object->current_energy > object->max_energy) {
        object->max_energy = object->current_energy;
    }
    if (object->current_energy < object->min_energy) {
        object->min_energy = object->current_energy;
    }
    
    object->last_update_time = get_time_ns();
    
    // Check for sleeping
    float speed = calculate_velocity_magnitude(object->velocity);
    if (speed < 0.1f && !object->is_static) {
        object->is_sleeping = true;
    } else {
        object->is_sleeping = false;
    }
}

bool physics_check_collision(const PhysicsObject *obj1,
                          const PhysicsObject *obj2,
                          CollisionContact *contact) {
    if (!obj1 || !obj2 || !contact || obj1->id == obj2->id) {
        return false;
    }
    
    // Calculate distance between centers
    float dx = obj2->position[0] - obj1->position[0];
    float dy = obj2->position[1] - obj1->position[1];
    float dz = obj2->position[2] - obj1->position[2];
    
    float distance_squared = dx * dx + dy * dy + dz * dz;
    float min_distance = obj1->radius + obj2->radius;
    float min_distance_squared = min_distance * min_distance;
    
    if (distance_squared >= min_distance_squared) {
        return false; // No collision
    }
    
    // Fill contact information
    float distance = sqrtf(distance_squared);
    
    contact->object1_id = obj1->id;
    contact->object2_id = obj2->id;
    contact->penetration_depth = min_distance - distance;
    
    // Contact point (midpoint between surfaces)
    float contact_x = obj1->position[0] + (dx / distance) * obj1->radius;
    float contact_y = obj1->position[1] + (dy / distance) * obj1->radius;
    float contact_z = obj1->position[2] + (dz / distance) * obj1->radius;
    
    contact->contact_point[0] = contact_x;
    contact->contact_point[1] = contact_y;
    contact->contact_point[2] = contact_z;
    
    // Contact normal (from obj1 to obj2)
    contact->contact_normal[0] = dx / distance;
    contact->contact_normal[1] = dy / distance;
    contact->contact_normal[2] = dz / distance;
    
    // Relative velocity
    contact->relative_velocity[0] = obj2->velocity[0] - obj1->velocity[0];
    contact->relative_velocity[1] = obj2->velocity[1] - obj1->velocity[1];
    contact->relative_velocity[2] = obj2->velocity[2] - obj1->velocity[2];
    
    contact->contact_time = get_time_ns();
    contact->is_valid = true;
    
    return true;
}

void physics_resolve_collision(PhysicsObject *obj1,
                            PhysicsObject *obj2,
                            const CollisionContact *contact) {
    if (!obj1 || !obj2 || !contact || !contact->is_valid) return;
    
    // Skip if both objects are static
    if (obj1->is_static && obj2->is_static) return;
    
    // Calculate relative velocity along normal
    float relative_velocity_normal = 
        contact->relative_velocity[0] * contact->contact_normal[0] +
        contact->relative_velocity[1] * contact->contact_normal[1] +
        contact->relative_velocity[2] * contact->contact_normal[2];
    
    // Don't resolve if objects are separating
    if (relative_velocity_normal > 0) return;
    
    // Calculate restitution
    float restitution = fminf(obj1->restitution, obj2->restitution);
    
    // Calculate impulse magnitude
    float impulse_magnitude = -(1.0f + restitution) * relative_velocity_normal;
    impulse_magnitude /= (1.0f / obj1->mass) + (1.0f / obj2->mass);
    
    // Apply impulse to objects
    float impulse_x = impulse_magnitude * contact->contact_normal[0];
    float impulse_y = impulse_magnitude * contact->contact_normal[1];
    float impulse_z = impulse_magnitude * contact->contact_normal[2];
    
    if (!obj1->is_static) {
        obj1->velocity[0] -= impulse_x / obj1->mass;
        obj1->velocity[1] -= impulse_y / obj1->mass;
        obj1->velocity[2] -= impulse_z / obj1->mass;
        obj1->collision_count++;
    }
    
    if (!obj2->is_static) {
        obj2->velocity[0] += impulse_x / obj2->mass;
        obj2->velocity[1] += impulse_y / obj2->mass;
        obj2->velocity[2] += impulse_z / obj2->mass;
        obj2->collision_count++;
    }
    
    // Separate objects to prevent overlap
    float separation_x = contact->contact_normal[0] * contact->penetration_depth * 0.5f;
    float separation_y = contact->contact_normal[1] * contact->penetration_depth * 0.5f;
    float separation_z = contact->contact_normal[2] * contact->penetration_depth * 0.5f;
    
    if (!obj1->is_static) {
        obj1->position[0] -= separation_x;
        obj1->position[1] -= separation_y;
        obj1->position[2] -= separation_z;
    }
    
    if (!obj2->is_static) {
        obj2->position[0] += separation_x;
        obj2->position[1] += separation_y;
        obj2->position[2] += separation_z;
    }
}

float calculate_kinetic_energy(const PhysicsObject *object) {
    if (!object || object->is_static) return 0.0f;
    
    float speed_squared = object->velocity[0] * object->velocity[0] +
                        object->velocity[1] * object->velocity[1] +
                        object->velocity[2] * object->velocity[2];
    
    return 0.5f * object->mass * speed_squared;
}

float calculate_potential_energy(const PhysicsObject *object, float gravity) {
    if (!object) return 0.0f;
    
    return object->mass * gravity * object->position[1];
}

float calculate_total_energy(const PhysicsObject *object, float gravity) {
    return calculate_kinetic_energy(object) + calculate_potential_energy(object, gravity);
}

float calculate_velocity_magnitude(const float velocity[3]) {
    return sqrtf(velocity[0] * velocity[0] + 
                  velocity[1] * velocity[1] + 
                  velocity[2] * velocity[2]);
}

bool is_invalid_float(float value) {
    return isnan(value) || isinf(value);
}

bool is_invalid_vector(const float vector[3]) {
    return is_invalid_float(vector[0]) || 
           is_invalid_float(vector[1]) || 
           is_invalid_float(vector[2]);
}

bool validate_physics_object(const PhysicsObject *object) {
    if (!object || !object->is_active) return false;
    
    // Check for invalid values
    if (is_invalid_vector(object->position) ||
        is_invalid_vector(object->velocity) ||
        is_invalid_vector(object->acceleration) ||
        is_invalid_vector(object->rotation) ||
        is_invalid_vector(object->angular_velocity)) {
        return false;
    }
    
    // Check physical constraints
    if (object->mass <= 0.0f || object->radius <= 0.0f) {
        return false;
    }
    
    // Check for reasonable values
    if (fabsf(object->position[1]) > 1000.0f) { // Object fell too far
        return false;
    }
    
    float speed = calculate_velocity_magnitude(object->velocity);
    if (speed > 1000.0f) { // Object moving too fast
        return false;
    }
    
    return true;
}

bool validate_energy_conservation(const PhysicsObject *objects,
                                uint32_t object_count,
                                float tolerance) {
    for (uint32_t i = 0; i < object_count; i++) {
        const PhysicsObject *object = &objects[i];
        if (!object->is_active || object->is_static) continue;
        
        float energy_error = fabsf(object->current_energy - object->initial_energy);
        float relative_error = energy_error / fabsf(object->initial_energy);
        
        if (relative_error > tolerance) {
            return false;
        }
    }
    
    return true;
}

void generate_random_position(float position[3], const float bounds[3]) {
    position[0] = (float)(rand() % (int)(bounds[0] * 2)) - bounds[0];
    position[1] = (float)(rand() % (int)(bounds[1] * 2)) - bounds[1];
    position[2] = (float)(rand() % (int)(bounds[2] * 2)) - bounds[2];
}

void generate_random_velocity(float velocity[3], float max_speed) {
    velocity[0] = ((float)rand() / (float)RAND_MAX) * max_speed * 2.0f - max_speed;
    velocity[1] = ((float)rand() / (float)RAND_MAX) * max_speed * 2.0f - max_speed;
    velocity[2] = ((float)rand() / (float)RAND_MAX) * max_speed * 2.0f - max_speed;
}

bool test_physics_stability_180hz(void) {
    const uint32_t object_count = g_physics_stability_test_config.object_count;
    const float simulation_duration = g_physics_stability_test_config.simulation_duration;
    const float dt = g_physics_stability_test_config.time_step;
    const float gravity[3] = {0.0f, -g_physics_stability_test_config.gravity_strength, 0.0f};
    
    float bounds[3] = {20.0f, 10.0f, 20.0f};
    
    // Create physics objects
    for (uint32_t i = 0; i < object_count; i++) {
        float position[3], velocity[3];
        generate_random_position(position, bounds);
        generate_random_velocity(velocity, 5.0f);
        
        float mass = 1.0f + (float)(rand() % 100) / 10.0f;
        float radius = 0.5f + (float)(rand() % 50) / 100.0f;
        bool is_static = (i % 10 == 0); // 10% static objects
        
        PhysicsObject *object = physics_create_object(position, velocity, mass, radius, is_static);
        if (!object) {
            return false;
        }
    }
    
    // Run simulation at 180 Hz
    uint32_t total_frames = (uint32_t)(simulation_duration / dt);
    uint64_t simulation_start = get_time_ms();
    
    for (uint32_t frame = 0; frame < total_frames; frame++) {
        uint64_t frame_start = get_time_ms();
        
        // Update physics
        for (uint32_t i = 0; i < object_count; i++) {
            PhysicsObject *object = &g_physics_objects[i];
            if (!object->is_active) continue;
            
            physics_update_object(object, dt, gravity);
        }
        
        // Handle collisions
        if (g_physics_stability_test_config.enable_collisions) {
            uint32_t contact_count = physics_find_all_collisions(
                g_physics_objects, object_count, 
                g_collision_contacts, g_max_contacts);
            
            for (uint32_t i = 0; i < contact_count; i++) {
                CollisionContact *contact = &g_collision_contacts[i];
                PhysicsObject *obj1 = &g_physics_objects[contact->object1_id - 1];
                PhysicsObject *obj2 = &g_physics_objects[contact->object2_id - 1];
                
                physics_resolve_collision(obj1, obj2, contact);
                g_physics_stability_test_results.collision_events++;
            }
        }
        
        // Validate all objects
        for (uint32_t i = 0; i < object_count; i++) {
            if (!validate_physics_object(&g_physics_objects[i])) {
                g_physics_stability_test_results.instability_events++;
                
                if (g_physics_stability_test_results.instability_events > 10) {
                    return false; // Too many instabilities
                }
            }
        }
        
        uint64_t frame_end = get_time_ms();
        double frame_time = (double)(frame_end - frame_start);
        
        // Update frame time statistics
        if (g_physics_stability_test_results.min_frame_time_ms == 0.0) {
            g_physics_stability_test_results.min_frame_time_ms = frame_time;
        }
        g_physics_stability_test_results.min_frame_time_ms = 
            fmin(g_physics_stability_test_results.min_frame_time_ms, frame_time);
        g_physics_stability_test_results.max_frame_time_ms = 
            fmax(g_physics_stability_test_results.max_frame_time_ms, frame_time);
        
        g_physics_stability_test_results.total_frames++;
        g_physics_stability_test_results.successful_frames++;
    }
    
    uint64_t simulation_end = get_time_ms();
    g_physics_stability_test_results.total_time_ms = simulation_end - simulation_start;
    
    // Calculate actual frequency
    g_physics_stability_test_results.actual_frequency = 
        (double)g_physics_stability_test_results.total_frames / 
        (g_physics_stability_test_results.total_time_ms / 1000.0);
    
    g_physics_stability_test_results.frequency_variance = 
        fabs(g_physics_stability_test_results.actual_frequency - 
              g_physics_stability_test_config.target_frequency);
    
    // Validate energy conservation
    bool energy_conserved = validate_energy_conservation(
        g_physics_objects, object_count, 
        g_physics_stability_test_config.energy_tolerance);
    
    if (!energy_conserved) {
        strcat(g_physics_stability_test_results.error_messages, 
               "Energy conservation validation failed\n");
        return false;
    }
    
    return g_physics_stability_test_results.instability_events == 0;
}

uint32_t physics_find_all_collisions(const PhysicsObject *objects,
                                    uint32_t object_count,
                                    CollisionContact *contacts,
                                    uint32_t max_contacts) {
    uint32_t contact_count = 0;
    
    for (uint32_t i = 0; i < object_count && contact_count < max_contacts; i++) {
        for (uint32_t j = i + 1; j < object_count && contact_count < max_contacts; j++) {
            const PhysicsObject *obj1 = &objects[i];
            const PhysicsObject *obj2 = &objects[j];
            
            if (!obj1->is_active || !obj2->is_active) continue;
            
            if (physics_check_collision(obj1, obj2, &contacts[contact_count])) {
                contact_count++;
            }
        }
    }
    
    return contact_count;
}

bool physics_stability_test_run(void) {
    uint64_t test_start_time = get_time_ms();
    
    bool test_passed = test_physics_stability_180hz();
    
    // Update results
    g_physics_stability_test_results.test_passed = test_passed;
    g_physics_stability_test_results.total_time_ms += get_time_ms() - test_start_time;
    g_physics_stability_test_results.average_frame_time_ms = 
        g_physics_stability_test_results.total_time_ms / 
        g_physics_stability_test_results.total_frames;
    
    return test_passed;
}

void physics_stability_test_print_summary(void) {
    printf("\n=== Physics Stability Test Summary ===\n");
    printf("Configuration:\n");
    printf("  Target Frequency: %.1f Hz\n", g_physics_stability_test_config.target_frequency);
    printf("  Simulation Duration: %.2f seconds\n", g_physics_stability_test_config.simulation_duration);
    printf("  Time Step: %.6f seconds\n", g_physics_stability_test_config.time_step);
    printf("  Object Count: %u\n", g_physics_stability_test_config.object_count);
    printf("  Collisions: %s\n", g_physics_stability_test_config.enable_collisions ? "Enabled" : "Disabled");
    
    printf("\nResults:\n");
    printf("  Total Frames: %u\n", g_physics_stability_test_results.total_frames);
    printf("  Successful Frames: %u\n", g_physics_stability_test_results.successful_frames);
    printf("  Failed Frames: %u\n", g_physics_stability_test_results.failed_frames);
    printf("  Actual Frequency: %.2f Hz\n", g_physics_stability_test_results.actual_frequency);
    printf("  Frequency Variance: %.2f Hz\n", g_physics_stability_test_results.frequency_variance);
    printf("  Instability Events: %u\n", g_physics_stability_test_results.instability_events);
    printf("  Collision Events: %u\n", g_physics_stability_test_results.collision_events);
    printf("  Constraint Violations: %u\n", g_physics_stability_test_results.constraint_violations);
    printf("  Test Passed: %s\n", g_physics_stability_test_results.test_passed ? "Yes" : "No");
    
    printf("\nPerformance:\n");
    printf("  Total Time: %.2f ms\n", g_physics_stability_test_results.total_time_ms);
    printf("  Average Frame Time: %.3f ms\n", g_physics_stability_test_results.average_frame_time_ms);
    printf("  Max Frame Time: %.3f ms\n", g_physics_stability_test_results.max_frame_time_ms);
    printf("  Min Frame Time: %.3f ms\n", g_physics_stability_test_results.min_frame_time_ms);
    
    if (strlen(g_physics_stability_test_results.error_messages) > 0) {
        printf("\nErrors:\n%s\n", g_physics_stability_test_results.error_messages);
    }
    
    printf("=====================================\n");
}

PhysicsStabilityTestResults physics_stability_test_get_results(void) {
    return g_physics_stability_test_results;
}

bool physics_stability_test_export_results(const char *filename) {
    if (!filename) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    fprintf(file, "Physics Stability Test Results\n");
    fprintf(file, "===============================\n\n");
    
    fprintf(file, "Configuration:\n");
    fprintf(file, "  Target Frequency: %.1f Hz\n", g_physics_stability_test_config.target_frequency);
    fprintf(file, "  Simulation Duration: %.2f seconds\n", g_physics_stability_test_config.simulation_duration);
    fprintf(file, "  Time Step: %.6f seconds\n", g_physics_stability_test_config.time_step);
    fprintf(file, "  Object Count: %u\n", g_physics_stability_test_config.object_count);
    fprintf(file, "  Gravity: %.2f m/s²\n", g_physics_stability_test_config.gravity_strength);
    fprintf(file, "  Collisions: %s\n", g_physics_stability_test_config.enable_collisions ? "Enabled" : "Disabled");
    
    fprintf(file, "\nResults:\n");
    fprintf(file, "  Total Frames: %u\n", g_physics_stability_test_results.total_frames);
    fprintf(file, "  Successful Frames: %u\n", g_physics_stability_test_results.successful_frames);
    fprintf(file, "  Failed Frames: %u\n", g_physics_stability_test_results.failed_frames);
    fprintf(file, "  Actual Frequency: %.2f Hz\n", g_physics_stability_test_results.actual_frequency);
    fprintf(file, "  Frequency Variance: %.2f Hz\n", g_physics_stability_test_results.frequency_variance);
    fprintf(file, "  Instability Events: %u\n", g_physics_stability_test_results.instability_events);
    fprintf(file, "  Collision Events: %u\n", g_physics_stability_test_results.collision_events);
    fprintf(file, "  Constraint Violations: %u\n", g_physics_stability_test_results.constraint_violations);
    fprintf(file, "  Test Passed: %s\n", g_physics_stability_test_results.test_passed ? "Yes" : "No");
    
    fprintf(file, "\nPerformance:\n");
    fprintf(file, "  Total Time: %.2f ms\n", g_physics_stability_test_results.total_time_ms);
    fprintf(file, "  Average Frame Time: %.3f ms\n", g_physics_stability_test_results.average_frame_time_ms);
    fprintf(file, "  Max Frame Time: %.3f ms\n", g_physics_stability_test_results.max_frame_time_ms);
    fprintf(file, "  Min Frame Time: %.3f ms\n", g_physics_stability_test_results.min_frame_time_ms);
    
    if (strlen(g_physics_stability_test_results.error_messages) > 0) {
        fprintf(file, "\nErrors:\n%s\n", g_physics_stability_test_results.error_messages);
    }
    
    fprintf(file, "\nStability Report:\n%s\n", g_physics_stability_test_results.stability_report);
    
    fclose(file);
    return true;
}
