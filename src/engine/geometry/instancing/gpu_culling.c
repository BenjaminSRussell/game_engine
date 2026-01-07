/*
 * gpu_culling.c
 * GPU-driven instance culling implementation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "gpu_culling.h"
#include "instance_data.h"
#include "../../rendering/3d_rendering/backend/metal/mtl_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

typedef struct gpu_culling_state {
    bool initialized;
    uint32_t context_count;
} gpu_culling_state_t;

static gpu_culling_state_t g_culling_state = {0};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int gpu_culling_init(void) {
    if (g_culling_state.initialized) {
        return 0;
    }
    
    memset(&g_culling_state, 0, sizeof(gpu_culling_state_t));
    g_culling_state.initialized = true;
    
    printf("[GPU Culling] System initialized\n");
    return 0;
}

void gpu_culling_shutdown(void) {
    if (!g_culling_state.initialized) {
        return;
    }
    
    printf("[GPU Culling] System shutdown - %u contexts created\n",
           g_culling_state.context_count);
    
    memset(&g_culling_state, 0, sizeof(gpu_culling_state_t));
}

/* ============================================================================
 * CONTEXT MANAGEMENT
 * ============================================================================ */

gpu_culling_context_t* gpu_culling_context_create(
    metal_device_t* device,
    uint32_t max_instances,
    const gpu_culling_config_t* config) {
    
    if (!g_culling_state.initialized) {
        fprintf(stderr, "[GPU Culling] System not initialized\n");
        return NULL;
    }
    
    if (!device || max_instances == 0) {
        fprintf(stderr, "[GPU Culling] Invalid parameters\n");
        return NULL;
    }
    
    // Allocate context
    gpu_culling_context_t* context = (gpu_culling_context_t*)calloc(
        1, sizeof(gpu_culling_context_t));
    if (!context) {
        return NULL;
    }
    
    context->device = device;
    context->max_instances = max_instances;
    
    // Copy configuration
    if (config) {
        memcpy(&context->config, config, sizeof(gpu_culling_config_t));
    } else {
        // Default configuration
        context->config.enable_frustum_culling = true;
        context->config.enable_occlusion_culling = false;
        context->config.enable_distance_culling = true;
        context->config.enable_backface_culling = false;
        context->config.near_distance = 0.1f;
        context->config.far_distance = 1000.0f;
        context->config.lod0_distance = 50.0f;
        context->config.lod1_distance = 100.0f;
        context->config.lod2_distance = 250.0f;
        context->config.max_visible_instances = max_instances;
    }
    
    // Create buffers
    metal_buffer_desc_t buffer_desc = {0};
    
    // Frustum buffer (6 planes * 16 bytes)
    buffer_desc.size = sizeof(camera_frustum_t);
    buffer_desc.storage_mode = METAL_STORAGE_SHARED;
    buffer_desc.usage = METAL_BUFFER_USAGE_UNIFORM;
    buffer_desc.label = "FrustumBuffer";
    context->frustum_buffer = metal_buffer_create(device, &buffer_desc);
    
    // Config buffer
    buffer_desc.size = sizeof(gpu_culling_config_t);
    buffer_desc.label = "CullingConfigBuffer";
    context->config_buffer = metal_buffer_create(device, &buffer_desc);
    
    // Visible IDs buffer (stores instance IDs that passed culling)
    buffer_desc.size = max_instances * sizeof(uint32_t);
    buffer_desc.storage_mode = METAL_STORAGE_PRIVATE;
    buffer_desc.usage = METAL_BUFFER_USAGE_STORAGE;
    buffer_desc.label = "VisibleIDsBuffer";
    context->visible_ids_buffer = metal_buffer_create(device, &buffer_desc);
    
    // Atomic counter buffer (single uint32_t for visible count)
    buffer_desc.size = sizeof(uint32_t);
    buffer_desc.storage_mode = METAL_STORAGE_SHARED;
    buffer_desc.label = "AtomicCounterBuffer";
    context->atomic_counter_buffer = metal_buffer_create(device, &buffer_desc);
    
    // Statistics buffer
    buffer_desc.size = sizeof(gpu_culling_stats_t);
    buffer_desc.label = "CullingStatsBuffer";
    context->stats_buffer = metal_buffer_create(device, &buffer_desc);
    
    // Verify buffer creation
    if (!context->frustum_buffer || !context->config_buffer ||
        !context->visible_ids_buffer || !context->atomic_counter_buffer ||
        !context->stats_buffer) {
        fprintf(stderr, "[GPU Culling] Failed to create buffers\n");
        gpu_culling_context_destroy(context);
        return NULL;
    }
    
#ifdef __OBJC__
    // Load compute shader (TODO: Load from compiled metallib)
    // For now, we'll note that the shader needs to be loaded from the metallib
    // This would typically be done via MTLLibrary and newFunctionWithName:
    context->culling_pipeline = nil; // TODO: Create compute pipeline state
#endif
    
    snprintf(context->label, sizeof(context->label), "CullingContext_%u", 
             g_culling_state.context_count);
    
    context->initialized = true;
    g_culling_state.context_count++;
    
    printf("[GPU Culling] Created context '%s': %u max instances\n",
           context->label, max_instances);
    
    return context;
}

void gpu_culling_context_destroy(gpu_culling_context_t* context) {
    if (!context) {
        return;
    }
    
    // Destroy buffers
    if (context->frustum_buffer) {
        metal_buffer_destroy(context->frustum_buffer);
    }
    if (context->config_buffer) {
        metal_buffer_destroy(context->config_buffer);
    }
    if (context->visible_ids_buffer) {
        metal_buffer_destroy(context->visible_ids_buffer);
    }
    if (context->atomic_counter_buffer) {
        metal_buffer_destroy(context->atomic_counter_buffer);
    }
    if (context->stats_buffer) {
        metal_buffer_destroy(context->stats_buffer);
    }
    
#ifdef __OBJC__
    if (context->culling_pipeline) {
        // Pipeline state is autoreleased in Objective-C
        context->culling_pipeline = nil;
    }
#endif
    
    printf("[GPU Culling] Destroyed context '%s'\n", context->label);
    
    free(context);
}

/* ============================================================================
 * CULLING OPERATIONS
 * ============================================================================ */

int gpu_culling_execute(
    gpu_culling_context_t* context,
    MTLCommandBuffer* command_buffer,
    instance_buffer_t* instance_buffer,
    const float camera_position[3],
    const camera_frustum_t* frustum,
    void* hiz_texture) {
    
    if (!context || !context->initialized) {
        return -1;
    }
    
    if (!command_buffer || !instance_buffer || !frustum) {
        return -1;
    }
    
#ifdef __OBJC__
    // Update frustum buffer
    metal_buffer_update(context->frustum_buffer, frustum, sizeof(camera_frustum_t), 0);
    
    // Update config buffer with camera position
    gpu_culling_config_t config = context->config;
    memcpy(config.camera_position, camera_position, sizeof(float) * 3);
    metal_buffer_update(context->config_buffer, &config, sizeof(gpu_culling_config_t), 0);
    
    // Reset atomic counter
    uint32_t zero = 0;
    metal_buffer_update(context->atomic_counter_buffer, &zero, sizeof(uint32_t), 0);
    
    // Create compute encoder
    id<MTLComputeCommandEncoder> encoder = [command_buffer computeCommandEncoder];
    if (!encoder) {
        return -1;
    }
    
    // Set pipeline (TODO: Load actual pipeline)
    if (context->culling_pipeline) {
        [encoder setComputePipelineState:context->culling_pipeline];
        
        // Bind buffers
        [encoder setBuffer:(__bridge id<MTLBuffer>)instance_buffer_get_metal_buffer(instance_buffer)->buffer
                    offset:0
                   atIndex:0]; // instances
        [encoder setBuffer:(__bridge id<MTLBuffer>)context->visible_ids_buffer->buffer
                    offset:0
                   atIndex:1]; // visible_instance_ids
        [encoder setBuffer:(__bridge id<MTLBuffer>)context->atomic_counter_buffer->buffer
                    offset:0
                   atIndex:2]; // visible_count
        [encoder setBuffer:(__bridge id<MTLBuffer>)context->frustum_buffer->buffer
                    offset:0
                   atIndex:3]; // frustum
        [encoder setBuffer:(__bridge id<MTLBuffer>)context->config_buffer->buffer
                    offset:0
                   atIndex:4]; // config
        [encoder setBuffer:(__bridge id<MTLBuffer>)context->stats_buffer->buffer
                    offset:0
                   atIndex:5]; // stats
        
        // Calculate dispatch size
        uint32_t instance_count = instance_buffer_get_count(instance_buffer);
        uint32_t threadgroup_size = gpu_culling_calculate_threadgroup_size(instance_count);
        uint32_t threadgroup_count = (instance_count + threadgroup_size - 1) / threadgroup_size;
        
        MTLSize threadgroups = MTLSizeMake(threadgroup_count, 1, 1);
        MTLSize threads_per_threadgroup = MTLSizeMake(threadgroup_size, 1, 1);
        
        [encoder dispatchThreadgroups:threadgroups
                threadsPerThreadgroup:threads_per_threadgroup];
    }
    
    [encoder endEncoding];
    
    context->frame_count++;
#endif
    
    return 0;
}

int gpu_culling_get_result(
    gpu_culling_context_t* context,
    gpu_culling_result_t* out_result) {
    
    if (!context || !out_result) {
        return -1;
    }
    
    // TODO: Read back results from GPU
    // This would typically wait for command buffer completion and map buffers
    
    // For now, stub implementation
    out_result->visible_instance_ids = NULL;
    out_result->visible_count = 0;
    memset(&out_result->stats, 0, sizeof(gpu_culling_stats_t));
    
    return 0;
}

void gpu_culling_update_config(
    gpu_culling_context_t* context,
    const gpu_culling_config_t* config) {
    
    if (context && config) {
        memcpy(&context->config, config, sizeof(gpu_culling_config_t));
    }
}

void gpu_culling_reset(gpu_culling_context_t* context) {
    if (context) {
        // Reset could clear statistics or reset buffers
        memset(&context->last_stats, 0, sizeof(gpu_culling_stats_t));
    }
}

/* ============================================================================
 * UTILITIES
 * ============================================================================ */

void gpu_culling_extract_frustum(
    const float view_proj_matrix[16],
    camera_frustum_t* out_frustum) {
    
    if (!view_proj_matrix || !out_frustum) {
        return;
    }
    
    // Extract frustum planes from view-projection matrix
    // Left: row4 + row1
    out_frustum->planes[0].a = view_proj_matrix[3] + view_proj_matrix[0];
    out_frustum->planes[0].b = view_proj_matrix[7] + view_proj_matrix[4];
    out_frustum->planes[0].c = view_proj_matrix[11] + view_proj_matrix[8];
    out_frustum->planes[0].d = view_proj_matrix[15] + view_proj_matrix[12];
    
    // Right: row4 - row1
    out_frustum->planes[1].a = view_proj_matrix[3] - view_proj_matrix[0];
    out_frustum->planes[1].b = view_proj_matrix[7] - view_proj_matrix[4];
    out_frustum->planes[1].c = view_proj_matrix[11] - view_proj_matrix[8];
    out_frustum->planes[1].d = view_proj_matrix[15] - view_proj_matrix[12];
    
    // Bottom: row4 + row2
    out_frustum->planes[2].a = view_proj_matrix[3] + view_proj_matrix[1];
    out_frustum->planes[2].b = view_proj_matrix[7] + view_proj_matrix[5];
    out_frustum->planes[2].c = view_proj_matrix[11] + view_proj_matrix[9];
    out_frustum->planes[2].d = view_proj_matrix[15] + view_proj_matrix[13];
    
    // Top: row4 - row2
    out_frustum->planes[3].a = view_proj_matrix[3] - view_proj_matrix[1];
    out_frustum->planes[3].b = view_proj_matrix[7] - view_proj_matrix[5];
    out_frustum->planes[3].c = view_proj_matrix[11] - view_proj_matrix[9];
    out_frustum->planes[3].d = view_proj_matrix[15] - view_proj_matrix[13];
    
    // Near: row4 + row3
    out_frustum->planes[4].a = view_proj_matrix[3] + view_proj_matrix[2];
    out_frustum->planes[4].b = view_proj_matrix[7] + view_proj_matrix[6];
    out_frustum->planes[4].c = view_proj_matrix[11] + view_proj_matrix[10];
    out_frustum->planes[4].d = view_proj_matrix[15] + view_proj_matrix[14];
    
    // Far: row4 - row3
    out_frustum->planes[5].a = view_proj_matrix[3] - view_proj_matrix[2];
    out_frustum->planes[5].b = view_proj_matrix[7] - view_proj_matrix[6];
    out_frustum->planes[5].c = view_proj_matrix[11] - view_proj_matrix[10];
    out_frustum->planes[5].d = view_proj_matrix[15] - view_proj_matrix[14];
    
    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float length = sqrtf(
            out_frustum->planes[i].a * out_frustum->planes[i].a +
            out_frustum->planes[i].b * out_frustum->planes[i].b +
            out_frustum->planes[i].c * out_frustum->planes[i].c
        );
        
        if (length > 0.0001f) {
            out_frustum->planes[i].a /= length;
            out_frustum->planes[i].b /= length;
            out_frustum->planes[i].c /= length;
            out_frustum->planes[i].d /= length;
        }
    }
}

bool gpu_culling_test_point_frustum(
    const float point[3],
    const camera_frustum_t* frustum) {
    
    if (!point || !frustum) {
        return false;
    }
    
    for (int i = 0; i < 6; i++) {
        const frustum_plane_t* plane = &frustum->planes[i];
        float distance = 
            plane->a * point[0] +
            plane->b * point[1] +
            plane->c * point[2] +
            plane->d;
        
        if (distance < 0.0f) {
            return false; // Outside this plane
        }
    }
    
    return true;
}

bool gpu_culling_test_sphere_frustum(
    const float center[3],
    float radius,
    const camera_frustum_t* frustum) {
    
    if (!center || !frustum) {
        return false;
    }
    
    for (int i = 0; i < 6; i++) {
        const frustum_plane_t* plane = &frustum->planes[i];
        float distance = 
            plane->a * center[0] +
            plane->b * center[1] +
            plane->c * center[2] +
            plane->d;
        
        if (distance < -radius) {
            return false; // Sphere completely outside this plane
        }
    }
    
    return true;
}

gpu_culling_stats_t gpu_culling_get_stats(const gpu_culling_context_t* context) {
    gpu_culling_stats_t stats = {0};
    
    if (context) {
        stats = context->last_stats;
    }
    
    return stats;
}

uint32_t gpu_culling_calculate_threadgroup_size(uint32_t instance_count) {
    // Metal typically has optimal threadgroup sizes of 32, 64, 128, or 256
    // Choose based on instance count for good occupancy
    
    if (instance_count < 128) {
        return 32;
    } else if (instance_count < 512) {
        return 64;
    } else if (instance_count < 2048) {
        return 128;
    } else {
        return 256;
    }
}

void gpu_culling_debug_print(const gpu_culling_context_t* context) {
    if (!context) {
        return;
    }
    
    printf("=== GPU Culling Context: %s ===\n", context->label);
    printf("  Max Instances: %u\n", context->max_instances);
    printf("  Frame Count: %u\n", context->frame_count);
    printf("  Configuration:\n");
    printf("    Frustum Culling: %s\n", context->config.enable_frustum_culling ? "ON" : "OFF");
    printf("    Occlusion Culling: %s\n", context->config.enable_occlusion_culling ? "ON" : "OFF");
    printf("    Distance Culling: %s\n", context->config.enable_distance_culling ? "ON" : "OFF");
    printf("    Backface Culling: %s\n", context->config.enable_backface_culling ? "ON" : "OFF");
    printf("    LOD Distances: %.1f / %.1f / %.1f\n",
           context->config.lod0_distance,
           context->config.lod1_distance,
           context->config.lod2_distance);
    printf("  Last Stats:\n");
    printf("    Total: %u\n", context->last_stats.total_instances);
    printf("    Visible: %u\n", context->last_stats.visible_instances);
    printf("    Frustum Culled: %u\n", context->last_stats.frustum_culled);
    printf("    Distance Culled: %u\n", context->last_stats.distance_culled);
    printf("    Efficiency: %.1f%%\n", context->last_stats.culling_efficiency * 100.0f);
}
