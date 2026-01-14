/**
 * @file physics_consolidated.c
 * @brief Consolidated Physics System Implementation
 * @description Unified physics system consolidating all physics subsystems
 * @date 2026-01-13
 */

#include "physics/physics_consolidated.h"
#include "physics/physics_internal.h"
#include "core/error_handling.h"
#include "core/unified_memory_allocator.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "ecs/ecs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct {
    PhysicsWorld* world;
    PhysicsConfig config;
    bool initialized;
    pthread_mutex_t mutex;
} PhysicsSystem;

static PhysicsSystem g_physics = {0};

// ============================================================================
// CORE PHYSICS WORLD IMPLEMENTATION
// ============================================================================

static PhysicsWorld* create_physics_world(const PhysicsConfig* config) {
    PhysicsWorld* world = UNIFIED_ALLOC(sizeof(PhysicsWorld), MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_ZERO);
    if (!world) {
        REPORT_ERROR(ERROR_OUT_OF_MEMORY, "Failed to allocate physics world");
        return NULL;
    }
    
    // Copy configuration
    world->gravity = config->gravity;
    world->time_step = config->time_step;
    world->max_substeps = config->max_substeps;
    world->enable_gpu_acceleration = config->enable_gpu_acceleration;
    world->enable_sleeping = config->enable_sleeping;
    world->worker_threads = config->worker_threads;
    
    // Initialize subsystems
    world->rigid_body_system = rigid_body_system_create(config);
    world->soft_body_system = soft_body_system_create(config);
    world->fluid_system = fluid_system_create(config);
    world->cloth_system = cloth_system_create(config);
    world->vehicle_system = vehicle_system_create(config);
    world->character_system = character_system_create(config);
    
    // Initialize collision detection
    world->collision_system = collision_detection_create(config);
    
    // Initialize constraint solver
    world->constraint_solver = constraint_solver_create(config);
    
    // Initialize performance monitoring
    world->performance_monitor = performance_monitor_create();
    
    return world;
}

static void destroy_physics_world(PhysicsWorld* world) {
    if (!world) return;
    
    // Shutdown subsystems in reverse order
    if (world->performance_monitor) {
        performance_monitor_destroy(world->performance_monitor);
    }
    
    if (world->constraint_solver) {
        constraint_solver_destroy(world->constraint_solver);
    }
    
    if (world->collision_system) {
        collision_detection_destroy(world->collision_system);
    }
    
    if (world->character_system) {
        character_system_destroy(world->character_system);
    }
    
    if (world->vehicle_system) {
        vehicle_system_destroy(world->vehicle_system);
    }
    
    if (world->cloth_system) {
        cloth_system_destroy(world->cloth_system);
    }
    
    if (world->fluid_system) {
        fluid_system_destroy(world->fluid_system);
    }
    
    if (world->soft_body_system) {
        soft_body_system_destroy(world->soft_body_system);
    }
    
    if (world->rigid_body_system) {
        rigid_body_system_destroy(world->rigid_body_system);
    }
    
    UNIFIED_FREE(world);
}

// ============================================================================
// MAIN PHYSICS SYSTEM API
// ============================================================================

PhysicsWorld* physics_world_create(const PhysicsConfig* config) {
    if (g_physics.initialized) {
        REPORT_ERROR(ERROR_ALREADY_EXISTS, "Physics system already initialized");
        return NULL;
    }
    
    pthread_mutex_lock(&g_physics.mutex);
    
    g_physics.world = create_physics_world(config);
    if (!g_physics.world) {
        pthread_mutex_unlock(&g_physics.mutex);
        return NULL;
    }
    
    if (config) {
        g_physics.config = *config;
    } else {
        g_physics.config = physics_get_default_config();
    }
    
    g_physics.initialized = true;
    
    pthread_mutex_unlock(&g_physics.mutex);
    
    LOG_CORE_INFO("Physics system initialized with %d worker threads", 
                g_physics.config.worker_threads);
    
    return g_physics.world;
}

void physics_world_destroy(PhysicsWorld* world) {
    if (!g_physics.initialized || g_physics.world != world) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid physics world");
        return;
    }
    
    pthread_mutex_lock(&g_physics.mutex);
    
    destroy_physics_world(g_physics.world);
    g_physics.world = NULL;
    g_physics.initialized = false;
    
    pthread_mutex_unlock(&g_physics.mutex);
    
    LOG_CORE_INFO("Physics system shutdown");
}

void physics_world_update(PhysicsWorld* world, float delta_time) {
    if (!g_physics.initialized || g_physics.world != world) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid physics world");
        return;
    }
    
    pthread_mutex_lock(&g_physics.mutex);
    
    // Begin performance monitoring
    performance_monitor_begin_frame(world->performance_monitor);
    
    // Update physics simulation
    float accumulated_time = 0.0f;
    uint32_t substeps = 0;
    
    while (accumulated_time < delta_time && substeps < world->max_substeps) {
        float step_time = world->time_step;
        if (accumulated_time + step_time > delta_time) {
            step_time = delta_time - accumulated_time;
        }
        
        // Update all subsystems
        rigid_body_system_update(world->rigid_body_system, step_time);
        soft_body_system_update(world->soft_body_system, step_time);
        fluid_system_update(world->fluid_system, step_time);
        cloth_system_update(world->cloth_system, step_time);
        vehicle_system_update(world->vehicle_system, step_time);
        character_system_update(world->character_system, step_time);
        
        accumulated_time += step_time;
        substeps++;
    }
    
    // Process collisions
    collision_detection_update(world->collision_system);
    
    // Solve constraints
    constraint_solver_solve(world->constraint_solver, world->time_step);
    
    // End performance monitoring
    performance_monitor_end_frame(world->performance_monitor);
    
    pthread_mutex_unlock(&g_physics.mutex);
}

void physics_world_step(PhysicsWorld* world, float time_step) {
    if (!g_physics.initialized || g_physics.world != world) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid physics world");
        return;
    }
    
    pthread_mutex_lock(&g_physics.mutex);
    
    // Single physics step
    rigid_body_system_update(world->rigid_body_system, time_step);
    soft_body_system_update(world->soft_body_system, time_step);
    fluid_system_update(world->fluid_system, time_step);
    cloth_system_update(world->cloth_system, time_step);
    vehicle_system_update(world->vehicle_system, time_step);
    character_system_update(world->character_system, time_step);
    
    collision_detection_update(world->collision_system);
    constraint_solver_solve(world->constraint_solver, time_step);
    
    pthread_mutex_unlock(&g_physics.mutex);
}

// ============================================================================
// CONFIGURATION API
// ============================================================================

PhysicsConfig physics_get_default_config(void) {
    return (PhysicsConfig){
        .gravity = {0.0f, -9.81f, 0.0f},
        .time_step = 1.0f / 60.0f,
        .max_substeps = 4,
        .enable_gpu_acceleration = true,
        .enable_sleeping = true,
        .worker_threads = 4
    };
}

void physics_set_gravity(PhysicsWorld* world, Vec3 gravity) {
    if (!g_physics.initialized || g_physics.world != world) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid physics world");
        return;
    }
    
    pthread_mutex_lock(&g_physics.mutex);
    world->gravity = gravity;
    pthread_mutex_unlock(&g_physics.mutex);
}

void physics_set_time_step(PhysicsWorld* world, float time_step) {
    if (!g_physics.initialized || g_physics.world != world) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid physics world");
        return;
    }
    
    pthread_mutex_lock(&g_physics.mutex);
    world->time_step = time_step;
    pthread_mutex_unlock(&g_physics.mutex);
}

// ============================================================================
// RIGID BODY API
// ============================================================================

RigidBody* physics_create_rigid_body(PhysicsWorld* world, Entity entity, BodyType type) {
    if (!g_physics.initialized || g_physics.world != world) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid physics world");
        return NULL;
    }
    
    return rigid_body_system_create_body(world->rigid_body_system, entity, type);
}

void physics_destroy_rigid_body(PhysicsWorld* world, RigidBody* body) {
    if (!g_physics.initialized || g_physics.world != world) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid physics world");
        return;
    }
    
    rigid_body_system_destroy_body(world->rigid_body_system, body);
}

void physics_set_body_shape(RigidBody* body, CollisionShapeType type, void* shape_data) {
    if (!body) return;
    
    rigid_body_system_set_shape(body, type, shape_data);
}

void physics_set_body_position(RigidBody* body, Vec3 position) {
    if (!body) return;
    
    rigid_body_system_set_position(body, position);
}

void physics_set_body_velocity(RigidBody* body, Vec3 velocity) {
    if (!body) return;
    
    rigid_body_system_set_velocity(body, velocity);
}

void physics_apply_force(RigidBody* body, Vec3 force) {
    if (!body) return;
    
    rigid_body_system_apply_force(body, force);
}

void physics_apply_impulse(RigidBody* body, Vec3 impulse) {
    if (!body) return;
    
    rigid_body_system_apply_impulse(body, impulse);
}

// ============================================================================
// RAYCASTING API
// ============================================================================

bool physics_raycast(PhysicsWorld* world, Vec3 origin, Vec3 direction, 
                   float max_distance, RaycastHit* hit) {
    if (!g_physics.initialized || g_physics.world != world || !hit) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid parameters for raycast");
        return false;
    }
    
    return collision_detection_raycast(world->collision_system, origin, direction, 
                                     max_distance, hit);
}

// ============================================================================
// BLOCK PHYSICS API
// ============================================================================

bool physics_block_raycast(PhysicsWorld* world, Vec3 origin, Vec3 direction,
                        float max_distance, BlockRaycastResult* result) {
    if (!g_physics.initialized || g_physics.world != world || !result) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid parameters for block raycast");
        return false;
    }
    
    return collision_detection_block_raycast(world->collision_system, origin, direction,
                                       max_distance, result);
}

// ============================================================================
// PERFORMANCE MONITORING API
// ============================================================================

PhysicsStats physics_get_stats(PhysicsWorld* world) {
    if (!g_physics.initialized || g_physics.world != world) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid physics world");
        PhysicsStats empty_stats = {0};
        return empty_stats;
    }
    
    return performance_monitor_get_stats(world->performance_monitor);
}

// ============================================================================
// SYSTEM INITIALIZATION
// ============================================================================

void physics_system_init(void) {
    pthread_mutex_init(&g_physics.mutex, NULL);
    g_physics.initialized = false;
    g_physics.world = NULL;
}

void physics_system_shutdown(void) {
    pthread_mutex_lock(&g_physics.mutex);
    
    if (g_physics.world) {
        destroy_physics_world(g_physics.world);
        g_physics.world = NULL;
    }
    
    g_physics.initialized = false;
    
    pthread_mutex_unlock(&g_physics.mutex);
    pthread_mutex_destroy(&g_physics.mutex);
}
