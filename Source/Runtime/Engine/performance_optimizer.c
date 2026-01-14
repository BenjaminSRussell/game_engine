// performance_optimizer.c - Performance Optimization Implementation
#include "performance_optimizer.h"
#include <pthread.h>
#include <math.h>
#include <string.h>

static struct {
    bool initialized;
    pthread_mutex_t global_mutex;
} g_perf_state = {0};

// Core implementation functions
performance_optimizer_t* performance_optimizer_create(const performance_optimization_flags_t flags) {
    performance_optimizer_t* optimizer = UNIFIED_NEW(performance_optimizer_t);
    if (!optimizer) return NULL;
    
    memset(optimizer, 0, sizeof(performance_optimizer_t));
    optimizer->enabled_optimizations = flags;
    optimizer->target_frame_time_ms = 16.67f; // 60 FPS
    optimizer->auto_optimize = true;
    
    // Create subsystems based on flags
    if (flags & PERF_OPT_LOD) {
        optimizer->lod_system = lod_system_create(1000);
    }
    if (flags & PERF_OPT_OCCLUSION) {
        optimizer->occlusion_system = occlusion_culling_create(1000, 1024, 1024);
    }
    if (flags & PERF_OPT_FRUSTUM) {
        optimizer->frustum_system = frustum_culling_create();
    }
    if (flags & PERF_OPT_BATCHING) {
        optimizer->batch_system = batch_rendering_create(10000, 20000);
    }
    if (flags & PERF_OPT_INSTANCING) {
        optimizer->instancing_system = instancing_create(1000);
    }
    
    optimizer->monitor = performance_monitor_create();
    optimizer->initialized = true;
    
    return optimizer;
}

void performance_optimizer_destroy(performance_optimizer_t* optimizer) {
    if (!optimizer) return;
    
    if (optimizer->lod_system) lod_system_destroy(optimizer->lod_system);
    if (optimizer->occlusion_system) occlusion_culling_destroy(optimizer->occlusion_system);
    if (optimizer->frustum_system) frustum_culling_destroy(optimizer->frustum_system);
    if (optimizer->batch_system) batch_rendering_destroy(optimizer->batch_system);
    if (optimizer->instancing_system) instancing_destroy(optimizer->instancing_system);
    if (optimizer->monitor) performance_monitor_destroy(optimizer->monitor);
    
    UNIFIED_FREE(optimizer);
}

void performance_optimizer_update(performance_optimizer_t* optimizer, float dt) {
    if (!optimizer || !optimizer->initialized) return;
    
    PERF_BEGIN_FRAME(optimizer->monitor);
    
    // Update LOD system
    if (optimizer->lod_system) {
        lod_system_update(optimizer->lod_system, optimizer->camera_position,
                       optimizer->camera_direction, optimizer->fov, optimizer->aspect_ratio);
    }
    
    // Update culling systems
    if (optimizer->occlusion_system) {
        occlusion_culling_update(optimizer->occlusion_system);
    }
    if (optimizer->frustum_system) {
        frustum_culling_update(optimizer->frustum_system, NULL, NULL);
    }
    
    // Auto-optimization
    if (optimizer->auto_optimize) {
        performance_optimizer_auto_adjust_quality(optimizer);
    }
    
    optimizer->frame_number++;
    optimizer->last_frame_time = dt;
    
    PERF_END_FRAME(optimizer->monitor);
}

// LOD system implementation
lod_system_t* lod_system_create(uint32_t capacity) {
    lod_system_t* lod = UNIFIED_NEW(lod_system_t);
    if (!lod) return NULL;
    
    lod->capacity = capacity;
    lod->objects = UNIFIED_NEW_ARRAY(lod_object_t, capacity);
    lod->auto_lod = true;
    lod->lod_bias = 0.0f;
    lod->max_lod_levels = 4;
    
    return lod;
}

void lod_system_update(lod_system_t* lod, const float camera_pos[3], const float camera_dir[3],
                        float fov, float aspect_ratio) {
    if (!lod) return;
    
    memcpy(lod->camera_position, camera_pos, sizeof(float) * 3);
    memcpy(lod->camera_direction, camera_dir, sizeof(float) * 3);
    lod->fov = fov;
    lod->aspect_ratio = aspect_ratio;
    
    for (uint32_t i = 0; i < lod->object_count; i++) {
        lod_object_t* obj = &lod->objects[i];
        
        // Calculate distance to camera
        float dx = obj->position[0] - camera_pos[0];
        float dy = obj->position[1] - camera_pos[1];
        float dz = obj->position[2] - camera_pos[2];
        float distance = sqrtf(dx*dx + dy*dy + dz*dz);
        
        // Calculate screen size
        float screen_size = calculate_screen_size(obj->bounds_radius, distance, fov, 1080.0f);
        
        // Select LOD level
        obj->current_mesh = select_lod_level(obj->meshes, obj->mesh_count, 
                                           screen_size, lod->lod_bias);
    }
}

// Utility functions
float calculate_screen_size(const float object_radius, const float distance, 
                          float fov, float viewport_height) {
    if (distance <= 0.0f) return FLT_MAX;
    return (object_radius * viewport_height) / (distance * tanf(fov * 0.5f));
}

uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void performance_optimizer_init(void) {
    if (g_perf_state.initialized) return;
    
    pthread_mutex_init(&g_perf_state.global_mutex, NULL);
    g_perf_state.initialized = true;
    
    LOG_CORE_INFO("Performance optimizer initialized");
}
