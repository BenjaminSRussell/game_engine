#include "entity_stress_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

EntityStressTestConfig g_entity_stress_test_config;
EntityStressTestResults g_entity_stress_test_results;

static Entity *g_entities = NULL;
static uint32_t g_entity_capacity = 0;
static uint32_t g_next_entity_id = 1;
static bool g_performance_monitoring = false;
static uint64_t g_monitoring_start_time = 0;

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

bool entity_stress_test_init(const EntityStressTestConfig *config) {
    if (config) {
        g_entity_stress_test_config = *config;
    } else {
        g_entity_stress_test_config = (EntityStressTestConfig){
            .enable_verbose_output = false,
            .enable_memory_tracking = true,
            .enable_performance_monitoring = true,
            .entity_count = 1000,
            .frame_count = 10,
            .time_step = 1.0f / 60.0f,
            .components_per_entity = 4,
            .enable_physics = true,
            .enable_rendering = true,
            .enable_ai = true,
            .output_file = NULL
        };
    }
    
    memset(&g_entity_stress_test_results, 0, sizeof(g_entity_stress_test_results));
    
    // Allocate entity array
    g_entity_capacity = g_entity_stress_test_config.entity_count + 100; // Extra space
    g_entities = calloc(g_entity_capacity, sizeof(Entity));
    if (!g_entities) {
        return false;
    }
    
    srand((unsigned int)time(NULL));
    return true;
}

void entity_stress_test_shutdown(bool generate_report) {
    destroy_test_entities();
    
    if (g_entities) {
        free(g_entities);
        g_entities = NULL;
    }
    
    if (generate_report) {
        entity_stress_test_print_summary();
        if (g_entity_stress_test_config.output_file) {
            entity_stress_test_export_results(g_entity_stress_test_config.output_file);
        }
    }
}

Entity* entity_create(const char *name) {
    if (!g_entities || g_entity_stress_test_results.active_entities >= g_entity_capacity) {
        return NULL;
    }
    
    Entity *entity = &g_entities[g_entity_stress_test_results.active_entities];
    
    memset(entity, 0, sizeof(Entity));
    
    entity->id = g_next_entity_id++;
    if (name) {
        strncpy(entity->name, name, sizeof(entity->name) - 1);
    }
    entity->is_active = true;
    entity->scale[0] = entity->scale[1] = entity->scale[2] = 1.0f;
    entity->rotation[3] = 1.0f; // Identity quaternion
    
    entity->creation_time = get_time_ns();
    entity->last_update_time = entity->creation_time;
    
    g_entity_stress_test_results.active_entities++;
    g_entity_stress_test_results.total_entities++;
    
    return entity;
}

void entity_destroy(Entity *entity) {
    if (!entity || !entity->is_active) return;
    
    // Destroy components
    for (int i = 0; i < COMPONENT_COUNT; i++) {
        if (entity->components[i]) {
            free(entity->components[i]);
            entity->components[i] = NULL;
        }
    }
    
    entity->is_active = false;
    g_entity_stress_test_results.active_entities--;
}

bool entity_add_component(Entity *entity, ComponentType type, void *component) {
    if (!entity || !component || type >= COMPONENT_COUNT) {
        return false;
    }
    
    if (entity->components[type]) {
        free(entity->components[type]); // Replace existing component
    }
    
    entity->components[type] = component;
    entity->component_mask |= (1 << type);
    
    return true;
}

void* entity_get_component(const Entity *entity, ComponentType type) {
    if (!entity || type >= COMPONENT_COUNT) {
        return NULL;
    }
    
    return entity->components[type];
}

TransformComponent* create_transform_component(const float position[3],
                                            const float rotation[4],
                                            const float scale[3]) {
    TransformComponent *transform = malloc(sizeof(TransformComponent));
    if (!transform) return NULL;
    
    if (position) {
        memcpy(transform->position, position, sizeof(float) * 3);
    } else {
        memset(transform->position, 0, sizeof(float) * 3);
    }
    
    if (rotation) {
        memcpy(transform->rotation, rotation, sizeof(float) * 4);
    } else {
        transform->rotation[0] = transform->rotation[1] = transform->rotation[2] = 0.0f;
        transform->rotation[3] = 1.0f;
    }
    
    if (scale) {
        memcpy(transform->scale, scale, sizeof(float) * 3);
    } else {
        transform->scale[0] = transform->scale[1] = transform->scale[2] = 1.0f;
    }
    
    // Initialize to identity matrix
    memset(transform->matrix, 0, sizeof(float) * 16);
    transform->matrix[0] = transform->matrix[5] = transform->matrix[10] = transform->matrix[15] = 1.0f;
    transform->is_dirty = true;
    
    return transform;
}

PhysicsComponent* create_physics_component(float mass, float radius, bool is_static) {
    PhysicsComponent *physics = malloc(sizeof(PhysicsComponent));
    if (!physics) return NULL;
    
    memset(physics, 0, sizeof(PhysicsComponent));
    
    physics->mass = mass;
    physics->radius = radius;
    physics->is_static = is_static;
    physics->has_gravity = !is_static;
    
    return physics;
}

RenderingComponent* create_rendering_component(uint32_t mesh_id,
                                             uint32_t material_id,
                                             const float color[4]) {
    RenderingComponent *rendering = malloc(sizeof(RenderingComponent));
    if (!rendering) return NULL;
    
    rendering->mesh_id = mesh_id;
    rendering->material_id = material_id;
    rendering->is_visible = true;
    
    if (color) {
        memcpy(rendering->color, color, sizeof(float) * 4);
    } else {
        rendering->color[0] = rendering->color[1] = rendering->color[2] = 1.0f;
        rendering->color[3] = 1.0f;
    }
    
    rendering->vertex_count = 100 + (rand() % 900);
    rendering->triangle_count = rendering->vertex_count / 3;
    
    return rendering;
}

AIComponent* create_ai_component(uint32_t behavior_tree_id, float health) {
    AIComponent *ai = malloc(sizeof(AIComponent));
    if (!ai) return NULL;
    
    ai->behavior_tree_id = behavior_tree_id;
    ai->think_interval = 0.1f + (float)(rand() % 100) / 1000.0f;
    ai->last_think_time = get_time_ns();
    ai->current_state = 0;
    ai->health = health;
    ai->max_health = health;
    
    return ai;
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

void generate_random_color(float color[4]) {
    color[0] = (float)rand() / (float)RAND_MAX;
    color[1] = (float)rand() / (float)RAND_MAX;
    color[2] = (float)rand() / (float)RAND_MAX;
    color[3] = 1.0f;
}

bool create_test_entities(uint32_t count) {
    float bounds[3] = {50.0f, 20.0f, 50.0f};
    
    for (uint32_t i = 0; i < count; i++) {
        char name[64];
        snprintf(name, sizeof(name), "TestEntity_%u", i);
        
        Entity *entity = entity_create(name);
        if (!entity) {
            return false;
        }
        
        // Add transform component
        float position[3], scale[3] = {1.0f, 1.0f, 1.0f};
        generate_random_position(position, bounds);
        TransformComponent *transform = create_transform_component(position, NULL, scale);
        if (!transform) return false;
        entity_add_component(entity, COMPONENT_TRANSFORM, transform);
        
        // Add physics component
        float mass = 1.0f + (float)(rand() % 100) / 10.0f;
        float radius = 0.5f + (float)(rand() % 50) / 100.0f;
        bool is_static = (i % 10 == 0); // 10% static objects
        PhysicsComponent *physics = create_physics_component(mass, radius, is_static);
        if (!physics) return false;
        entity_add_component(entity, COMPONENT_PHYSICS, physics);
        
        // Add rendering component
        float color[4];
        generate_random_color(color);
        uint32_t mesh_id = rand() % 100;
        uint32_t material_id = rand() % 50;
        RenderingComponent *rendering = create_rendering_component(mesh_id, material_id, color);
        if (!rendering) return false;
        entity_add_component(entity, COMPONENT_RENDERING, rendering);
        
        // Add AI component (only to non-static entities)
        if (!is_static && g_entity_stress_test_config.enable_ai) {
            uint32_t behavior_tree_id = rand() % 20;
            float health = 50.0f + (float)(rand() % 100);
            AIComponent *ai = create_ai_component(behavior_tree_id, health);
            if (!ai) return false;
            entity_add_component(entity, COMPONENT_AI, ai);
        }
        
        // Copy transform data to entity for easy access
        memcpy(entity->position, transform->position, sizeof(float) * 3);
        memcpy(entity->scale, transform->scale, sizeof(float) * 3);
    }
    
    return true;
}

void destroy_test_entities(void) {
    for (uint32_t i = 0; i < g_entity_stress_test_results.total_entities; i++) {
        if (g_entities[i].is_active) {
            entity_destroy(&g_entities[i]);
        }
    }
    
    g_entity_stress_test_results.active_entities = 0;
}

void update_physics_system(Entity *entities, uint32_t entity_count, float dt) {
    for (uint32_t i = 0; i < entity_count; i++) {
        Entity *entity = &entities[i];
        if (!entity->is_active) continue;
        
        PhysicsComponent *physics = (PhysicsComponent*)entity_get_component(entity, COMPONENT_PHYSICS);
        TransformComponent *transform = (TransformComponent*)entity_get_component(entity, COMPONENT_TRANSFORM);
        
        if (!physics || !transform || physics->is_static) continue;
        
        // Apply gravity
        if (physics->has_gravity) {
            physics->acceleration[1] = -9.81f;
        }
        
        // Update velocity
        physics->velocity[0] += physics->acceleration[0] * dt;
        physics->velocity[1] += physics->acceleration[1] * dt;
        physics->velocity[2] += physics->acceleration[2] * dt;
        
        // Apply damping
        float damping = 0.99f;
        physics->velocity[0] *= damping;
        physics->velocity[1] *= damping;
        physics->velocity[2] *= damping;
        
        // Update position
        transform->position[0] += physics->velocity[0] * dt;
        transform->position[1] += physics->velocity[1] * dt;
        transform->position[2] += physics->velocity[2] * dt;
        
        // Ground collision
        if (transform->position[1] - physics->radius < 0.0f) {
            transform->position[1] = physics->radius;
            physics->velocity[1] = -physics->velocity[1] * 0.8f; // Bounce
        }
        
        transform->is_dirty = true;
    }
}

void update_rendering_system(Entity *entities, uint32_t entity_count, float dt) {
    for (uint32_t i = 0; i < entity_count; i++) {
        Entity *entity = &entities[i];
        if (!entity->is_active) continue;
        
        RenderingComponent *rendering = (RenderingComponent*)entity_get_component(entity, COMPONENT_RENDERING);
        TransformComponent *transform = (TransformComponent*)entity_get_component(entity, COMPONENT_TRANSFORM);
        
        if (!rendering || !transform) continue;
        
        // Simulate rendering work
        if (transform->is_dirty) {
            // Update transformation matrix (simplified)
            transform->matrix[0] = transform->scale[0];
            transform->matrix[5] = transform->scale[1];
            transform->matrix[10] = transform->scale[2];
            transform->matrix[12] = transform->position[0];
            transform->matrix[13] = transform->position[1];
            transform->matrix[14] = transform->position[2];
            transform->is_dirty = false;
        }
        
        // Simulate rendering processing
        volatile float dummy = rendering->color[0] + rendering->color[1] + rendering->color[2];
        (void)dummy; // Prevent optimization
    }
}

void update_ai_system(Entity *entities, uint32_t entity_count, float dt) {
    uint64_t current_time = get_time_ns();
    
    for (uint32_t i = 0; i < entity_count; i++) {
        Entity *entity = &entities[i];
        if (!entity->is_active) continue;
        
        AIComponent *ai = (AIComponent*)entity_get_component(entity, COMPONENT_AI);
        if (!ai) continue;
        
        // Check if it's time to think
        uint64_t time_since_last_think = current_time - ai->last_think_time;
        uint64_t think_interval_ns = (uint64_t)(ai->think_interval * 1000000000ULL);
        
        if (time_since_last_think >= think_interval_ns) {
            // Simulate AI thinking
            ai->current_state = (ai->current_state + 1) % 5;
            
            // Simple state-based behavior
            switch (ai->current_state) {
                case 0: // Idle
                    break;
                case 1: // Move
                    {
                        PhysicsComponent *physics = (PhysicsComponent*)entity_get_component(entity, COMPONENT_PHYSICS);
                        if (physics && !physics->is_static) {
                            physics->velocity[0] = (float)(rand() % 3 - 1) * 2.0f;
                            physics->velocity[2] = (float)(rand() % 3 - 1) * 2.0f;
                        }
                    }
                    break;
                case 2: // Stop
                    {
                        PhysicsComponent *physics = (PhysicsComponent*)entity_get_component(entity, COMPONENT_PHYSICS);
                        if (physics) {
                            physics->velocity[0] = 0.0f;
                            physics->velocity[2] = 0.0f;
                        }
                    }
                    break;
                default:
                    break;
            }
            
            ai->last_think_time = current_time;
        }
    }
}

void entity_update(Entity *entity, float dt) {
    if (!entity || !entity->is_active) return;
    
    entity->update_count++;
    entity->last_update_time = get_time_ns();
    
    // Update transform data from component
    TransformComponent *transform = (TransformComponent*)entity_get_component(entity, COMPONENT_TRANSFORM);
    if (transform) {
        memcpy(entity->position, transform->position, sizeof(float) * 3);
        memcpy(entity->scale, transform->scale, sizeof(float) * 3);
    }
    
    // Update velocity data from physics component
    PhysicsComponent *physics = (PhysicsComponent*)entity_get_component(entity, COMPONENT_PHYSICS);
    if (physics) {
        memcpy(entity->velocity, physics->velocity, sizeof(float) * 3);
    }
}

bool simulate_frames(uint32_t frame_count) {
    float dt = g_entity_stress_test_config.time_step;
    
    for (uint32_t frame = 0; frame < frame_count; frame++) {
        uint64_t frame_start_time = get_time_ms();
        
        // Update systems
        if (g_entity_stress_test_config.enable_physics) {
            update_physics_system(g_entities, g_entity_stress_test_results.total_entities, dt);
        }
        
        if (g_entity_stress_test_config.enable_rendering) {
            update_rendering_system(g_entities, g_entity_stress_test_results.total_entities, dt);
        }
        
        if (g_entity_stress_test_config.enable_ai) {
            update_ai_system(g_entities, g_entity_stress_test_results.total_entities, dt);
        }
        
        // Update all entities
        for (uint32_t i = 0; i < g_entity_stress_test_results.total_entities; i++) {
            entity_update(&g_entities[i], dt);
        }
        
        uint64_t frame_end_time = get_time_ms();
        double frame_time = (double)(frame_end_time - frame_start_time);
        
        // Update frame time statistics
        if (g_entity_stress_test_results.min_frame_time_ms == 0.0) {
            g_entity_stress_test_results.min_frame_time_ms = frame_time;
        }
        g_entity_stress_test_results.min_frame_time_ms = 
            fmin(g_entity_stress_test_results.min_frame_time_ms, frame_time);
        g_entity_stress_test_results.max_frame_time_ms = 
            fmax(g_entity_stress_test_results.max_frame_time_ms, frame_time);
        
        g_entity_stress_test_results.frames_simulated++;
        
        // Check for timeout
        if (frame_time > 1000.0) { // More than 1 second per frame
            snprintf(g_entity_stress_test_results.error_messages + strlen(g_entity_stress_test_results.error_messages),
                    sizeof(g_entity_stress_test_results.error_messages) - strlen(g_entity_stress_test_results.error_messages),
                    "Frame %u took too long: %.2f ms\n", frame, frame_time);
            return false;
        }
    }
    
    return true;
}

bool validate_entity_state(void) {
    for (uint32_t i = 0; i < g_entity_stress_test_results.total_entities; i++) {
        Entity *entity = &g_entities[i];
        if (!entity->is_active) continue;
        
        // Check for valid position
        for (int j = 0; j < 3; j++) {
            if (!isfinite(entity->position[j])) {
                return false;
            }
        }
        
        // Check for valid velocity
        for (int j = 0; j < 3; j++) {
            if (!isfinite(entity->velocity[j])) {
                return false;
            }
        }
        
        // Check for valid scale
        for (int j = 0; j < 3; j++) {
            if (!isfinite(entity->scale[j]) || entity->scale[j] <= 0.0f) {
                return false;
            }
        }
        
        // Check for valid rotation
        for (int j = 0; j < 4; j++) {
            if (!isfinite(entity->rotation[j])) {
                return false;
            }
        }
    }
    
    return true;
}

size_t measure_memory_usage(void) {
    // Simple memory usage estimation
    size_t usage = 0;
    
    // Entity memory
    usage += g_entity_stress_test_results.total_entities * sizeof(Entity);
    
    // Component memory
    for (uint32_t i = 0; i < g_entity_stress_test_results.total_entities; i++) {
        Entity *entity = &g_entities[i];
        if (!entity->is_active) continue;
        
        for (int j = 0; j < COMPONENT_COUNT; j++) {
            if (entity->components[j]) {
                switch ((ComponentType)j) {
                    case COMPONENT_TRANSFORM:
                        usage += sizeof(TransformComponent);
                        break;
                    case COMPONENT_PHYSICS:
                        usage += sizeof(PhysicsComponent);
                        break;
                    case COMPONENT_RENDERING:
                        usage += sizeof(RenderingComponent);
                        break;
                    case COMPONENT_AI:
                        usage += sizeof(AIComponent);
                        break;
                    default:
                        break;
                }
            }
        }
    }
    
    return usage;
}

bool entity_stress_test_run(void) {
    uint64_t test_start_time = get_time_ms();
    
    // Create test entities
    if (!create_test_entities(g_entity_stress_test_config.entity_count)) {
        snprintf(g_entity_stress_test_results.error_messages,
                sizeof(g_entity_stress_test_results.error_messages),
                "Failed to create %u test entities\n", g_entity_stress_test_config.entity_count);
        return false;
    }
    
    // Measure initial memory usage
    g_entity_stress_test_results.current_memory_usage = measure_memory_usage();
    g_entity_stress_test_results.peak_memory_usage = g_entity_stress_test_results.current_memory_usage;
    
    // Start performance monitoring
    start_performance_monitoring();
    
    // Simulate frames
    bool simulation_success = simulate_frames(g_entity_stress_test_config.frame_count);
    
    // Stop performance monitoring
    stop_performance_monitoring();
    
    // Validate final state
    bool state_valid = validate_entity_state();
    
    // Update final statistics
    g_entity_stress_test_results.total_time_ms = get_time_ms() - test_start_time;
    g_entity_stress_test_results.average_frame_time_ms = 
        g_entity_stress_test_results.total_time_ms / g_entity_stress_test_results.frames_simulated;
    g_entity_stress_test_results.entities_per_second = 
        (double)g_entity_stress_test_results.total_entities / (g_entity_stress_test_results.total_time_ms / 1000.0);
    g_entity_stress_test_results.frames_per_second = 
        (double)g_entity_stress_test_results.frames_simulated / (g_entity_stress_test_results.total_time_ms / 1000.0);
    
    // Update memory statistics
    g_entity_stress_test_results.current_memory_usage = measure_memory_usage();
    if (g_entity_stress_test_results.current_memory_usage > g_entity_stress_test_results.peak_memory_usage) {
        g_entity_stress_test_results.peak_memory_usage = g_entity_stress_test_results.current_memory_usage;
    }
    
    g_entity_stress_test_results.test_passed = simulation_success && state_valid;
    
    if (!simulation_success) {
        strcat(g_entity_stress_test_results.error_messages, "Simulation failed\n");
    }
    
    if (!state_valid) {
        strcat(g_entity_stress_test_results.error_messages, "Entity state validation failed\n");
    }
    
    return g_entity_stress_test_results.test_passed;
}

void start_performance_monitoring(void) {
    g_performance_monitoring = true;
    g_monitoring_start_time = get_time_ns();
}

void stop_performance_monitoring(void) {
    g_performance_monitoring = false;
}

void entity_stress_test_print_summary(void) {
    printf("\n=== Entity Stress Test Summary ===\n");
    printf("Test Configuration:\n");
    printf("  Entity Count: %u\n", g_entity_stress_test_config.entity_count);
    printf("  Frame Count: %u\n", g_entity_stress_test_config.frame_count);
    printf("  Time Step: %.4f s\n", g_entity_stress_test_config.time_step);
    printf("  Physics: %s\n", g_entity_stress_test_config.enable_physics ? "Enabled" : "Disabled");
    printf("  Rendering: %s\n", g_entity_stress_test_config.enable_rendering ? "Enabled" : "Disabled");
    printf("  AI: %s\n", g_entity_stress_test_config.enable_ai ? "Enabled" : "Disabled");
    
    printf("\nResults:\n");
    printf("  Total Entities: %u\n", g_entity_stress_test_results.total_entities);
    printf("  Active Entities: %u\n", g_entity_stress_test_results.active_entities);
    printf("  Frames Simulated: %u\n", g_entity_stress_test_results.frames_simulated);
    printf("  Test Passed: %s\n", g_entity_stress_test_results.test_passed ? "Yes" : "No");
    
    printf("\nPerformance:\n");
    printf("  Total Time: %.2f ms\n", g_entity_stress_test_results.total_time_ms);
    printf("  Average Frame Time: %.3f ms\n", g_entity_stress_test_results.average_frame_time_ms);
    printf("  Max Frame Time: %.3f ms\n", g_entity_stress_test_results.max_frame_time_ms);
    printf("  Min Frame Time: %.3f ms\n", g_entity_stress_test_results.min_frame_time_ms);
    printf("  Entities/Second: %.1f\n", g_entity_stress_test_results.entities_per_second);
    printf("  Frames/Second: %.1f\n", g_entity_stress_test_results.frames_per_second);
    
    printf("\nMemory:\n");
    printf("  Current Usage: %zu bytes\n", g_entity_stress_test_results.current_memory_usage);
    printf("  Peak Usage: %zu bytes\n", g_entity_stress_test_results.peak_memory_usage);
    printf("  Memory/Entity: %.1f bytes\n", 
           (double)g_entity_stress_test_results.current_memory_usage / g_entity_stress_test_results.total_entities);
    
    if (strlen(g_entity_stress_test_results.error_messages) > 0) {
        printf("\nErrors:\n%s\n", g_entity_stress_test_results.error_messages);
    }
    
    printf("===============================\n");
}

EntityStressTestResults entity_stress_test_get_results(void) {
    return g_entity_stress_test_results;
}

bool entity_stress_test_export_results(const char *filename) {
    if (!filename) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    fprintf(file, "Entity Stress Test Results\n");
    fprintf(file, "========================\n\n");
    
    fprintf(file, "Configuration:\n");
    fprintf(file, "  Entity Count: %u\n", g_entity_stress_test_config.entity_count);
    fprintf(file, "  Frame Count: %u\n", g_entity_stress_test_config.frame_count);
    fprintf(file, "  Time Step: %.4f s\n", g_entity_stress_test_config.time_step);
    fprintf(file, "  Components per Entity: %u\n", g_entity_stress_test_config.components_per_entity);
    fprintf(file, "  Physics: %s\n", g_entity_stress_test_config.enable_physics ? "Enabled" : "Disabled");
    fprintf(file, "  Rendering: %s\n", g_entity_stress_test_config.enable_rendering ? "Enabled" : "Disabled");
    fprintf(file, "  AI: %s\n\n", g_entity_stress_test_config.enable_ai ? "Enabled" : "Disabled");
    
    fprintf(file, "Results:\n");
    fprintf(file, "  Total Entities: %u\n", g_entity_stress_test_results.total_entities);
    fprintf(file, "  Active Entities: %u\n", g_entity_stress_test_results.active_entities);
    fprintf(file, "  Frames Simulated: %u\n", g_entity_stress_test_results.frames_simulated);
    fprintf(file, "  Test Passed: %s\n", g_entity_stress_test_results.test_passed ? "Yes" : "No");
    
    fprintf(file, "\nPerformance:\n");
    fprintf(file, "  Total Time: %.2f ms\n", g_entity_stress_test_results.total_time_ms);
    fprintf(file, "  Average Frame Time: %.3f ms\n", g_entity_stress_test_results.average_frame_time_ms);
    fprintf(file, "  Max Frame Time: %.3f ms\n", g_entity_stress_test_results.max_frame_time_ms);
    fprintf(file, "  Min Frame Time: %.3f ms\n", g_entity_stress_test_results.min_frame_time_ms);
    fprintf(file, "  Entities/Second: %.1f\n", g_entity_stress_test_results.entities_per_second);
    fprintf(file, "  Frames/Second: %.1f\n", g_entity_stress_test_results.frames_per_second);
    
    fprintf(file, "\nMemory:\n");
    fprintf(file, "  Current Usage: %zu bytes\n", g_entity_stress_test_results.current_memory_usage);
    fprintf(file, "  Peak Usage: %zu bytes\n", g_entity_stress_test_results.peak_memory_usage);
    fprintf(file, "  Memory/Entity: %.1f bytes\n", 
            (double)g_entity_stress_test_results.current_memory_usage / g_entity_stress_test_results.total_entities);
    
    if (strlen(g_entity_stress_test_results.error_messages) > 0) {
        fprintf(file, "\nErrors:\n%s\n", g_entity_stress_test_results.error_messages);
    }
    
    fprintf(file, "\nPerformance Report:\n%s\n", g_entity_stress_test_results.performance_report);
    
    fclose(file);
    return true;
}
