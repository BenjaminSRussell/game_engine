#include "simd_physics_manager.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>

// Worker thread data
typedef struct {
    simd_physics_manager* manager;
    u32 thread_id;
    u32 start_index;
    u32 end_index;
    physics_system_type system_type;
    f32 dt;
} worker_thread_data;

// Worker thread function
void* physics_worker_thread(void* arg) {
    worker_thread_data* data = (worker_thread_data*)arg;
    simd_physics_manager* manager = data->manager;
    
    switch (data->system_type) {
        case PHYSICS_SYSTEM_RIGID_BODY:
            for (u32 i = data->start_index; i < data->end_index; i++) {
                simd_integrate_rigid_bodies(&manager->rigid_bodies[i], 8, data->dt);
            }
            break;
            
        case PHYSICS_SYSTEM_CLOTH:
            for (u32 i = data->start_index; i < data->end_index; i++) {
                simd_cloth_update(&manager->cloth_systems[i], data->dt);
            }
            break;
            
        case PHYSICS_SYSTEM_FLUID:
            for (u32 i = data->start_index; i < data->end_index; i++) {
                simd_fluid_update(&manager->fluid_systems[i], data->dt);
            }
            break;
            
        default:
            break;
    }
    
    return NULL;
}

void simd_physics_manager_create(simd_physics_manager* manager, u32 rigid_body_capacity, 
                                u32 cloth_capacity, u32 fluid_capacity) {
    memset(manager, 0, sizeof(simd_physics_manager));
    
    // Check SIMD support
    manager->simd_supported = simd_is_supported();
    manager->simd_width = manager->simd_supported ? 8 : 1; // AVX = 8 floats
    
    // Allocate arrays
    if (rigid_body_capacity > 0) {
        manager->rigid_bodies = malloc(rigid_body_capacity * sizeof(simd_rigid_body));
        manager->rigid_body_capacity = rigid_body_capacity;
    }
    
    if (cloth_capacity > 0) {
        manager->cloth_systems = malloc(cloth_capacity * sizeof(simd_cloth_system));
        manager->cloth_capacity = cloth_capacity;
    }
    
    if (fluid_capacity > 0) {
        manager->fluid_systems = malloc(fluid_capacity * sizeof(simd_fluid_system));
        manager->fluid_capacity = fluid_capacity;
    }
    
    // Initialize default parameters
    manager->gravity = (vec3){0.0f, -9.81f, 0.0f};
    manager->global_damping = 0.99f;
    manager->time_step = 1.0f / 60.0f; // 60 FPS
    manager->max_substeps = 4;
    
    // Initialize threading
    pthread_mutex_init(&manager->update_mutex, NULL);
    manager->worker_count = 4;
    manager->running = true;
    
    // Create worker threads
    for (u32 i = 0; i < manager->worker_count; i++) {
        pthread_create(&manager->worker_threads[i], NULL, physics_worker_thread, NULL);
    }
}

void simd_physics_manager_destroy(simd_physics_manager* manager) {
    // Stop worker threads
    manager->running = false;
    
    for (u32 i = 0; i < manager->worker_count; i++) {
        pthread_join(manager->worker_threads[i], NULL);
    }
    
    // Destroy resources
    if (manager->rigid_bodies) {
        free(manager->rigid_bodies);
        manager->rigid_bodies = NULL;
    }
    
    if (manager->cloth_systems) {
        for (u32 i = 0; i < manager->cloth_count; i++) {
            simd_cloth_destroy(&manager->cloth_systems[i]);
        }
        free(manager->cloth_systems);
        manager->cloth_systems = NULL;
    }
    
    if (manager->fluid_systems) {
        for (u32 i = 0; i < manager->fluid_count; i++) {
            simd_fluid_destroy(&manager->fluid_systems[i]);
        }
        free(manager->fluid_systems);
        manager->fluid_systems = NULL;
    }
    
    pthread_mutex_destroy(&manager->update_mutex);
    
    memset(manager, 0, sizeof(simd_physics_manager));
}

u32 simd_physics_add_rigid_body(simd_physics_manager* manager, const simd_rigid_body* body) {
    if (manager->rigid_body_count >= manager->rigid_body_capacity) {
        return UINT32_MAX;
    }
    
    u32 id = manager->rigid_body_count++;
    manager->rigid_bodies[id] = *body;
    return id;
}

u32 simd_physics_add_cloth_system(simd_physics_manager* manager, const simd_cloth_system* cloth) {
    if (manager->cloth_count >= manager->cloth_capacity) {
        return UINT32_MAX;
    }
    
    u32 id = manager->cloth_count++;
    manager->cloth_systems[id] = *cloth;
    return id;
}

u32 simd_physics_add_fluid_system(simd_physics_manager* manager, const simd_fluid_system* fluid) {
    if (manager->fluid_count >= manager->fluid_capacity) {
        return UINT32_MAX;
    }
    
    u32 id = manager->fluid_count++;
    manager->fluid_systems[id] = *fluid;
    return id;
}

void simd_physics_remove_rigid_body(simd_physics_manager* manager, u32 id) {
    if (id >= manager->rigid_body_count) return;
    
    // Move last element to removed position
    if (id < manager->rigid_body_count - 1) {
        manager->rigid_bodies[id] = manager->rigid_bodies[manager->rigid_body_count - 1];
    }
    
    manager->rigid_body_count--;
}

void simd_physics_remove_cloth_system(simd_physics_manager* manager, u32 id) {
    if (id >= manager->cloth_count) return;
    
    simd_cloth_destroy(&manager->cloth_systems[id]);
    
    // Move last element to removed position
    if (id < manager->cloth_count - 1) {
        manager->cloth_systems[id] = manager->cloth_systems[manager->cloth_count - 1];
    }
    
    manager->cloth_count--;
}

void simd_physics_remove_fluid_system(simd_physics_manager* manager, u32 id) {
    if (id >= manager->fluid_count) return;
    
    simd_fluid_destroy(&manager->fluid_systems[id]);
    
    // Move last element to removed position
    if (id < manager->fluid_count - 1) {
        manager->fluid_systems[id] = manager->fluid_systems[manager->fluid_count - 1];
    }
    
    manager->fluid_count--;
}

void simd_physics_update_parallel(simd_physics_manager* manager, f32 dt) {
    pthread_mutex_lock(&manager->update_mutex);
    
    clock_t start_time = clock();
    
    // Subdivide time step if needed
    f32 sub_dt = dt / (f32)manager->max_substeps;
    
    for (u32 substep = 0; substep < manager->max_substeps; substep++) {
        // Update rigid bodies in parallel
        if (manager->rigid_body_count > 0) {
            u32 bodies_per_thread = (manager->rigid_body_count + manager->worker_count - 1) / manager->worker_count;
            
            worker_thread_data* thread_data = malloc(manager->worker_count * sizeof(worker_thread_data));
            
            for (u32 i = 0; i < manager->worker_count; i++) {
                thread_data[i].manager = manager;
                thread_data[i].thread_id = i;
                thread_data[i].start_index = i * bodies_per_thread;
                thread_data[i].end_index = min((i + 1) * bodies_per_thread, manager->rigid_body_count);
                thread_data[i].system_type = PHYSICS_SYSTEM_RIGID_BODY;
                thread_data[i].dt = sub_dt;
                
                if (thread_data[i].start_index < thread_data[i].end_index) {
                    pthread_create(&manager->worker_threads[i], NULL, physics_worker_thread, &thread_data[i]);
                }
            }
            
            for (u32 i = 0; i < manager->worker_count; i++) {
                if (thread_data[i].start_index < thread_data[i].end_index) {
                    pthread_join(manager->worker_threads[i], NULL);
                }
            }
            
            free(thread_data);
        }
        
        // Update cloth systems in parallel
        if (manager->cloth_count > 0) {
            u32 cloths_per_thread = (manager->cloth_count + manager->worker_count - 1) / manager->worker_count;
            
            worker_thread_data* thread_data = malloc(manager->worker_count * sizeof(worker_thread_data));
            
            for (u32 i = 0; i < manager->worker_count; i++) {
                thread_data[i].manager = manager;
                thread_data[i].thread_id = i;
                thread_data[i].start_index = i * cloths_per_thread;
                thread_data[i].end_index = min((i + 1) * cloths_per_thread, manager->cloth_count);
                thread_data[i].system_type = PHYSICS_SYSTEM_CLOTH;
                thread_data[i].dt = sub_dt;
                
                if (thread_data[i].start_index < thread_data[i].end_index) {
                    pthread_create(&manager->worker_threads[i], NULL, physics_worker_thread, &thread_data[i]);
                }
            }
            
            for (u32 i = 0; i < manager->worker_count; i++) {
                if (thread_data[i].start_index < thread_data[i].end_index) {
                    pthread_join(manager->worker_threads[i], NULL);
                }
            }
            
            free(thread_data);
        }
        
        // Update fluid systems in parallel
        if (manager->fluid_count > 0) {
            u32 fluids_per_thread = (manager->fluid_count + manager->worker_count - 1) / manager->worker_count;
            
            worker_thread_data* thread_data = malloc(manager->worker_count * sizeof(worker_thread_data));
            
            for (u32 i = 0; i < manager->worker_count; i++) {
                thread_data[i].manager = manager;
                thread_data[i].thread_id = i;
                thread_data[i].start_index = i * fluids_per_thread;
                thread_data[i].end_index = min((i + 1) * fluids_per_thread, manager->fluid_count);
                thread_data[i].system_type = PHYSICS_SYSTEM_FLUID;
                thread_data[i].dt = sub_dt;
                
                if (thread_data[i].start_index < thread_data[i].end_index) {
                    pthread_create(&manager->worker_threads[i], NULL, physics_worker_thread, &thread_data[i]);
                }
            }
            
            for (u32 i = 0; i < manager->worker_count; i++) {
                if (thread_data[i].start_index < thread_data[i].end_index) {
                    pthread_join(manager->worker_threads[i], NULL);
                }
            }
            
            free(thread_data);
        }
        
        // Collision detection and resolution
        simd_physics_detect_collisions(manager);
        simd_physics_resolve_collisions(manager);
    }
    
    clock_t end_time = clock();
    manager->total_update_time += (end_time - start_time);
    manager->frame_count++;
    
    pthread_mutex_unlock(&manager->update_mutex);
}

void simd_physics_update(simd_physics_manager* manager, f32 dt) {
    if (manager->simd_supported && manager->worker_count > 1) {
        simd_physics_update_parallel(manager, dt);
    } else {
        // Fallback to single-threaded update
        pthread_mutex_lock(&manager->update_mutex);
        
        clock_t start_time = clock();
        
        f32 sub_dt = dt / (f32)manager->max_substeps;
        
        for (u32 substep = 0; substep < manager->max_substeps; substep++) {
            // Update rigid bodies
            for (u32 i = 0; i < manager->rigid_body_count; i++) {
                simd_integrate_rigid_bodies(&manager->rigid_bodies[i], 8, sub_dt);
            }
            
            // Update cloth systems
            for (u32 i = 0; i < manager->cloth_count; i++) {
                simd_cloth_update(&manager->cloth_systems[i], sub_dt);
            }
            
            // Update fluid systems
            for (u32 i = 0; i < manager->fluid_count; i++) {
                simd_fluid_update(&manager->fluid_systems[i], sub_dt);
            }
            
            // Collision detection and resolution
            simd_physics_detect_collisions(manager);
            simd_physics_resolve_collisions(manager);
        }
        
        clock_t end_time = clock();
        manager->total_update_time += (end_time - start_time);
        manager->frame_count++;
        
        pthread_mutex_unlock(&manager->update_mutex);
    }
}

void simd_physics_set_gravity(simd_physics_manager* manager, const vec3* gravity) {
    manager->gravity = *gravity;
}

void simd_physics_set_time_step(simd_physics_manager* manager, f32 time_step) {
    manager->time_step = time_step;
}

void simd_physics_set_max_substeps(simd_physics_manager* manager, u32 substeps) {
    manager->max_substeps = substeps;
}

physics_performance_stats simd_physics_get_performance_stats(simd_physics_manager* manager) {
    physics_performance_stats stats = {0};
    
    stats.rigid_body_count = manager->rigid_body_count;
    stats.cloth_particle_count = 0;
    stats.fluid_particle_count = 0;
    stats.constraint_count = 0;
    
    for (u32 i = 0; i < manager->cloth_count; i++) {
        stats.cloth_particle_count += manager->cloth_systems[i].particle_count;
        stats.constraint_count += manager->cloth_systems[i].constraint_count;
    }
    
    for (u32 i = 0; i < manager->fluid_count; i++) {
        stats.fluid_particle_count += manager->fluid_systems[i].particle_count;
    }
    
    if (manager->frame_count > 0) {
        stats.average_update_time = (f32)manager->total_update_time / (f32)manager->frame_count / CLOCKS_PER_SEC * 1000.0f;
    }
    
    stats.peak_update_time = stats.average_update_time; // Simplified
    stats.total_simulated_time = manager->frame_count * manager->time_step;
    
    return stats;
}

void simd_physics_reset_performance_stats(simd_physics_manager* manager) {
    manager->total_update_time = 0;
    manager->rigid_body_time = 0;
    manager->cloth_time = 0;
    manager->fluid_time = 0;
    manager->frame_count = 0;
}

bool simd_is_supported(void) {
    int cpuinfo[4];
    __cpuid(cpuinfo, 1);
    return (cpuinfo[2] & (1 << 28)) != 0; // Check AVX bit
}

u32 simd_physics_get_simd_width(void) {
    return simd_is_supported() ? 8 : 1;
}

void simd_physics_detect_collisions(simd_physics_manager* manager) {
    // Simplified collision detection
    // In a full implementation, this would use spatial partitioning and SIMD-optimized tests
    
    // Rigid body vs rigid body collisions
    for (u32 i = 0; i < manager->rigid_body_count; i++) {
        for (u32 j = i + 1; j < manager->rigid_body_count; j++) {
            // Simple sphere-sphere collision test
            simd_rigid_body* body_a = &manager->rigid_bodies[i];
            simd_rigid_body* body_b = &manager->rigid_bodies[j];
            
            // Calculate distance (simplified)
            __m256 dx = _mm256_sub_ps(body_a->position.x, body_b->position.x);
            __m256 dy = _mm256_sub_ps(body_a->position.y, body_b->position.y);
            __m256 dz = _mm256_sub_ps(body_a->position.z, body_b->position.z);
            
            __m256 dist_sq = _mm256_add_ps(_mm256_add_ps(
                _mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
            
            // Check collision (simplified threshold)
            __m256 collision_threshold = _mm256_set1_ps(4.0f); // 2m radius
            __m256 collision = _mm256_cmp_ps(dist_sq, collision_threshold, _CMP_LT_OQ);
            
            // Apply simple collision response
            if (_mm256_movemask_ps(collision) != 0) {
                simd_resolve_collisions_batch(manager->rigid_bodies, manager->rigid_body_count);
            }
        }
    }
}

void simd_physics_resolve_collisions(simd_physics_manager* manager) {
    // Apply collision resolution to all systems
    simd_resolve_collisions_batch(manager->rigid_bodies, manager->rigid_body_count);
    
    // Cloth and fluid collision resolution would be handled in their respective update functions
}

void simd_physics_update_broadphase(simd_physics_manager* manager) {
    // Update broadphase for all systems
    simd_update_broadphase_batch(manager->rigid_bodies, manager->rigid_body_count);
    
    // Cloth and fluid broadphase would be handled in their respective systems
}

void simd_physics_integrate_systems(simd_physics_manager* manager, f32 dt) {
    // Integrate all physics systems
    for (u32 i = 0; i < manager->rigid_body_count; i++) {
        simd_integrate_rigid_bodies(&manager->rigid_bodies[i], 8, dt);
    }
    
    for (u32 i = 0; i < manager->cloth_count; i++) {
        simd_cloth_update(&manager->cloth_systems[i], dt);
    }
    
    for (u32 i = 0; i < manager->fluid_count; i++) {
        simd_fluid_update(&manager->fluid_systems[i], dt);
    }
}
