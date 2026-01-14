#pragma once

#include "simd_physics.h"
#include "simd_cloth.h"
#include "simd_fluid.h"
#include "../math/types.h"
#include <pthread.h>

// SIMD-Optimized Physics Manager
// High-performance physics system with vectorized operations

// Physics System Types
typedef enum {
    PHYSICS_SYSTEM_RIGID_BODY,
    PHYSICS_SYSTEM_CLOTH,
    PHYSICS_SYSTEM_FLUID,
    PHYSICS_SYSTEM_SOFT_BODY,
    PHYSICS_SYSTEM_COUNT
} physics_system_type;

// SIMD Physics Manager
typedef struct {
    // Rigid body system
    simd_rigid_body* rigid_bodies;
    u32 rigid_body_count;
    u32 rigid_body_capacity;
    
    // Cloth system
    simd_cloth_system* cloth_systems;
    u32 cloth_count;
    u32 cloth_capacity;
    
    // Fluid system
    simd_fluid_system* fluid_systems;
    u32 fluid_count;
    u32 fluid_capacity;
    
    // Global parameters
    vec3 gravity;
    f32 global_damping;
    f32 time_step;
    u32 max_substeps;
    
    // Performance tracking
    u64 total_update_time;
    u64 rigid_body_time;
    u64 cloth_time;
    u64 fluid_time;
    u32 frame_count;
    
    // Threading
    pthread_mutex_t update_mutex;
    pthread_t worker_threads[4];
    bool running;
    u32 worker_count;
    
    // SIMD support
    bool simd_supported;
    u32 simd_width; // 8 for AVX, 4 for SSE
    
} simd_physics_manager;

// Physics Performance Stats
typedef struct {
    u32 rigid_body_count;
    u32 cloth_particle_count;
    u32 fluid_particle_count;
    u32 constraint_count;
    f32 average_update_time;
    f32 peak_update_time;
    u64 total_simulated_time;
} physics_performance_stats;

// SIMD Physics Manager API
void simd_physics_manager_create(simd_physics_manager* manager, u32 rigid_body_capacity, 
                                u32 cloth_capacity, u32 fluid_capacity);
void simd_physics_manager_destroy(simd_physics_manager* manager);

// System Management
u32 simd_physics_add_rigid_body(simd_physics_manager* manager, const simd_rigid_body* body);
u32 simd_physics_add_cloth_system(simd_physics_manager* manager, const simd_cloth_system* cloth);
u32 simd_physics_add_fluid_system(simd_physics_manager* manager, const simd_fluid_system* fluid);
void simd_physics_remove_rigid_body(simd_physics_manager* manager, u32 id);
void simd_physics_remove_cloth_system(simd_physics_manager* manager, u32 id);
void simd_physics_remove_fluid_system(simd_physics_manager* manager, u32 id);

// Main Update Loop
void simd_physics_update(simd_physics_manager* manager, f32 dt);
void simd_physics_update_parallel(simd_physics_manager* manager, f32 dt);

// Configuration
void simd_physics_set_gravity(simd_physics_manager* manager, const vec3* gravity);
void simd_physics_set_time_step(simd_physics_manager* manager, f32 time_step);
void simd_physics_set_max_substeps(simd_physics_manager* manager, u32 substeps);

// Performance and Debugging
physics_performance_stats simd_physics_get_performance_stats(simd_physics_manager* manager);
void simd_physics_reset_performance_stats(simd_physics_manager* manager);
bool simd_physics_is_simd_supported(void);
u32 simd_physics_get_simd_width(void);

// Collision Detection (SIMD-optimized)
void simd_physics_detect_collisions(simd_physics_manager* manager);
void simd_physics_resolve_collisions(simd_physics_manager* manager);

// Broadphase (SIMD-optimized)
void simd_physics_update_broadphase(simd_physics_manager* manager);

// Integration (SIMD-optimized)
void simd_physics_integrate_systems(simd_physics_manager* manager, f32 dt);

#endif // SIMD_PHYSICS_MANAGER_H
