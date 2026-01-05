/*
 * gpu_skinning.c
 * GPU bone matrix upload and compute shader skinning dispatch
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements compute shader vertex skinning with dual quaternion support
 */

#include "gpu_skinning.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GPU_SKINNING_MAX_COUNT 4096
#define GPU_SKINNING_DEFAULT_CAPACITY 256
#define GPU_SKINNING_MAX_BONES 256
#define GPU_SKINNING_WORKGROUP_SIZE 64
#define GPU_SKINNING_MAX_WEIGHTS_PER_VERTEX 4

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct mat4 {
    float m[16];
} mat4_t;

typedef struct dual_quat {
    float real[4];  // Rotation quaternion
    float dual[4];  // Translation quaternion
} dual_quat_t;

/* ============================================================================
 * SKINNING TYPES
 * ============================================================================ */

typedef enum skinning_method {
    SKINNING_METHOD_LINEAR_BLEND = 0,   // Linear blend skinning (LBS)
    SKINNING_METHOD_DUAL_QUATERNION     // Dual quaternion skinning (no volume loss)
} skinning_method_t;

typedef struct skinning_vertex {
    float position[3];
    float normal[3];
    float tangent[4];
} skinning_vertex_t;

typedef struct bone_weight {
    uint8_t indices[GPU_SKINNING_MAX_WEIGHTS_PER_VERTEX];  // Bone indices
    float weights[GPU_SKINNING_MAX_WEIGHTS_PER_VERTEX];     // Blend weights (sum to 1.0)
} bone_weight_t;

typedef struct skinning_buffers {
    // GPU buffer handles (using uint64_t as placeholder for actual GPU handles)
    uint64_t bone_matrices_buffer;      // mat4 per bone
    uint64_t src_vertices_buffer;       // Bind pose vertices
    uint64_t dst_vertices_buffer;       // Skinned output vertices
    uint64_t bone_weights_buffer;       // Weights per vertex
    uint64_t bone_indices_buffer;       // Indices per vertex
    
    // CPU staging data
    mat4_t* bone_matrices;
    dual_quat_t* bone_dual_quats;
    
    uint32_t bone_count;
    uint32_t vertex_count;
    bool buffers_valid;
} skinning_buffers_t;

typedef struct animation_gpu_skinning_internal {
    uint32_t id;
    uint32_t flags;
    
    skinning_method_t method;
    skinning_buffers_t buffers;
    
    // Compute dispatch info
    uint32_t dispatch_x;
    uint32_t dispatch_y;
    uint32_t dispatch_z;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} animation_gpu_skinning_internal_t;

typedef struct animation_gpu_skinning_context {
    animation_gpu_skinning_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    
    // Shared compute pipeline resources
    uint64_t lbs_compute_pipeline;      // Linear blend skinning pipeline
    uint64_t dq_compute_pipeline;       // Dual quaternion skinning pipeline
    
    bool initialized;
} animation_gpu_skinning_context_t;

static animation_gpu_skinning_context_t g_gpu_skinning_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static void mat4_identity(mat4_t* m) {
    memset(m->m, 0, sizeof(m->m));
    m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}

static void mat4_to_dual_quat(dual_quat_t* dq, const mat4_t* m) {
    // Extract rotation as quaternion from matrix
    float trace = m->m[0] + m->m[5] + m->m[10];
    float qw, qx, qy, qz;
    
    if (trace > 0) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        qw = 0.25f / s;
        qx = (m->m[6] - m->m[9]) * s;
        qy = (m->m[8] - m->m[2]) * s;
        qz = (m->m[1] - m->m[4]) * s;
    } else if (m->m[0] > m->m[5] && m->m[0] > m->m[10]) {
        float s = 2.0f * sqrtf(1.0f + m->m[0] - m->m[5] - m->m[10]);
        qw = (m->m[6] - m->m[9]) / s;
        qx = 0.25f * s;
        qy = (m->m[4] + m->m[1]) / s;
        qz = (m->m[8] + m->m[2]) / s;
    } else if (m->m[5] > m->m[10]) {
        float s = 2.0f * sqrtf(1.0f + m->m[5] - m->m[0] - m->m[10]);
        qw = (m->m[8] - m->m[2]) / s;
        qx = (m->m[4] + m->m[1]) / s;
        qy = 0.25f * s;
        qz = (m->m[9] + m->m[6]) / s;
    } else {
        float s = 2.0f * sqrtf(1.0f + m->m[10] - m->m[0] - m->m[5]);
        qw = (m->m[1] - m->m[4]) / s;
        qx = (m->m[8] + m->m[2]) / s;
        qy = (m->m[9] + m->m[6]) / s;
        qz = 0.25f * s;
    }
    
    // Real part (rotation)
    dq->real[0] = qx;
    dq->real[1] = qy;
    dq->real[2] = qz;
    dq->real[3] = qw;
    
    // Dual part (translation encoded)
    float tx = m->m[12], ty = m->m[13], tz = m->m[14];
    dq->dual[0] = 0.5f * ( tx * qw + ty * qz - tz * qy);
    dq->dual[1] = 0.5f * (-tx * qz + ty * qw + tz * qx);
    dq->dual[2] = 0.5f * ( tx * qy - ty * qx + tz * qw);
    dq->dual[3] = 0.5f * (-tx * qx - ty * qy - tz * qz);
}

static float sqrtf(float x) {
    // Simple sqrt implementation for platforms without math.h
    if (x <= 0) return 0;
    float guess = x / 2.0f;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) / 2.0f;
    }
    return guess;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_gpu_skinning_validate(const animation_gpu_skinning_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void skinning_buffers_cleanup(skinning_buffers_t* buffers) {
    if (!buffers) return;
    
    if (buffers->bone_matrices) {
        free(buffers->bone_matrices);
        buffers->bone_matrices = NULL;
    }
    if (buffers->bone_dual_quats) {
        free(buffers->bone_dual_quats);
        buffers->bone_dual_quats = NULL;
    }
    
    // GPU buffer cleanup would happen here
    buffers->bone_matrices_buffer = 0;
    buffers->src_vertices_buffer = 0;
    buffers->dst_vertices_buffer = 0;
    buffers->bone_weights_buffer = 0;
    buffers->bone_indices_buffer = 0;
    
    buffers->bone_count = 0;
    buffers->vertex_count = 0;
    buffers->buffers_valid = false;
}

static int skinning_buffers_allocate(skinning_buffers_t* buffers, 
                                      uint32_t bone_count, 
                                      uint32_t vertex_count) {
    if (!buffers) return -1;
    
    skinning_buffers_cleanup(buffers);
    
    if (bone_count > GPU_SKINNING_MAX_BONES) {
        return -2;
    }
    
    buffers->bone_matrices = calloc(bone_count, sizeof(mat4_t));
    buffers->bone_dual_quats = calloc(bone_count, sizeof(dual_quat_t));
    
    if (!buffers->bone_matrices || !buffers->bone_dual_quats) {
        skinning_buffers_cleanup(buffers);
        return -3;
    }
    
    // Initialize to identity
    for (uint32_t i = 0; i < bone_count; i++) {
        mat4_identity(&buffers->bone_matrices[i]);
    }
    
    buffers->bone_count = bone_count;
    buffers->vertex_count = vertex_count;
    buffers->buffers_valid = true;
    
    // GPU buffer creation would happen here
    // buffers->bone_matrices_buffer = create_storage_buffer(...)
    
    return 0;
}

static void animation_gpu_skinning_cleanup_internal(animation_gpu_skinning_internal_t* item) {
    if (!item) return;
    skinning_buffers_cleanup(&item->buffers);
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_gpu_skinning_init(void) {
    if (g_gpu_skinning_ctx.initialized) {
        return 0;
    }

    g_gpu_skinning_ctx.capacity = GPU_SKINNING_DEFAULT_CAPACITY;
    g_gpu_skinning_ctx.items = calloc(g_gpu_skinning_ctx.capacity, 
                                       sizeof(animation_gpu_skinning_internal_t));
    if (!g_gpu_skinning_ctx.items) {
        return -1;
    }

    // Create compute pipelines for skinning
    // Linear blend skinning pipeline
    // g_gpu_skinning_ctx.lbs_compute_pipeline = create_compute_pipeline("shaders/lbs_skinning.comp");
    // Dual quaternion skinning pipeline  
    // g_gpu_skinning_ctx.dq_compute_pipeline = create_compute_pipeline("shaders/dq_skinning.comp");

    g_gpu_skinning_ctx.count = 0;
    g_gpu_skinning_ctx.initialized = true;

    return 0;
}

void animation_gpu_skinning_shutdown(void) {
    if (!g_gpu_skinning_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gpu_skinning_ctx.count; i++) {
        animation_gpu_skinning_cleanup_internal(&g_gpu_skinning_ctx.items[i]);
    }

    // Destroy compute pipelines
    // destroy_pipeline(g_gpu_skinning_ctx.lbs_compute_pipeline);
    // destroy_pipeline(g_gpu_skinning_ctx.dq_compute_pipeline);

    free(g_gpu_skinning_ctx.items);
    g_gpu_skinning_ctx.items = NULL;
    g_gpu_skinning_ctx.count = 0;
    g_gpu_skinning_ctx.capacity = 0;
    g_gpu_skinning_ctx.initialized = false;
}

int animation_gpu_skinning_create(animation_gpu_skinning_handle_t* out_handle, 
                                   const animation_gpu_skinning_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gpu_skinning_ctx.initialized) {
        return -2;
    }

    if (g_gpu_skinning_ctx.count >= g_gpu_skinning_ctx.capacity) {
        uint32_t new_capacity = g_gpu_skinning_ctx.capacity * 2;
        if (new_capacity > GPU_SKINNING_MAX_COUNT) {
            new_capacity = GPU_SKINNING_MAX_COUNT;
        }
        if (g_gpu_skinning_ctx.count >= new_capacity) {
            return -3;
        }
        
        animation_gpu_skinning_internal_t* new_items = realloc(
            g_gpu_skinning_ctx.items,
            new_capacity * sizeof(animation_gpu_skinning_internal_t)
        );
        if (!new_items) {
            return -3;
        }
        
        memset(&new_items[g_gpu_skinning_ctx.capacity], 0,
               (new_capacity - g_gpu_skinning_ctx.capacity) * sizeof(animation_gpu_skinning_internal_t));
        
        g_gpu_skinning_ctx.items = new_items;
        g_gpu_skinning_ctx.capacity = new_capacity;
    }

    uint32_t index = g_gpu_skinning_ctx.count++;
    animation_gpu_skinning_internal_t* item = &g_gpu_skinning_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->method = SKINNING_METHOD_LINEAR_BLEND;
    memset(&item->buffers, 0, sizeof(item->buffers));
    item->dispatch_x = 1;
    item->dispatch_y = 1;
    item->dispatch_z = 1;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void animation_gpu_skinning_destroy(animation_gpu_skinning_handle_t handle) {
    if (handle.id >= g_gpu_skinning_ctx.count) {
        return;
    }

    animation_gpu_skinning_cleanup_internal(&g_gpu_skinning_ctx.items[handle.id]);
}

int animation_gpu_skinning_setup(animation_gpu_skinning_handle_t handle,
                                  uint32_t bone_count,
                                  uint32_t vertex_count,
                                  skinning_method_t method) {
    if (handle.id >= g_gpu_skinning_ctx.count) {
        return -1;
    }
    
    animation_gpu_skinning_internal_t* item = &g_gpu_skinning_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    int result = skinning_buffers_allocate(&item->buffers, bone_count, vertex_count);
    if (result != 0) {
        return result;
    }
    
    item->method = method;
    
    // Calculate compute dispatch dimensions
    item->dispatch_x = (vertex_count + GPU_SKINNING_WORKGROUP_SIZE - 1) / GPU_SKINNING_WORKGROUP_SIZE;
    item->dispatch_y = 1;
    item->dispatch_z = 1;
    
    item->dirty = true;
    return 0;
}

int animation_gpu_skinning_upload_matrices(animation_gpu_skinning_handle_t handle,
                                            const void* matrices,
                                            uint32_t count) {
    if (handle.id >= g_gpu_skinning_ctx.count || !matrices) {
        return -1;
    }
    
    animation_gpu_skinning_internal_t* item = &g_gpu_skinning_ctx.items[handle.id];
    if (!item->initialized || !item->buffers.buffers_valid) {
        return -2;
    }
    
    if (count > item->buffers.bone_count) {
        count = item->buffers.bone_count;
    }
    
    // Copy bone matrices to staging buffer
    memcpy(item->buffers.bone_matrices, matrices, count * sizeof(mat4_t));
    
    // Convert to dual quaternions if using DQ skinning
    if (item->method == SKINNING_METHOD_DUAL_QUATERNION) {
        for (uint32_t i = 0; i < count; i++) {
            mat4_to_dual_quat(&item->buffers.bone_dual_quats[i], 
                              &item->buffers.bone_matrices[i]);
        }
    }
    
    item->dirty = true;
    return 0;
}

int animation_gpu_skinning_dispatch(animation_gpu_skinning_handle_t handle) {
    if (handle.id >= g_gpu_skinning_ctx.count) {
        return -1;
    }
    
    animation_gpu_skinning_internal_t* item = &g_gpu_skinning_ctx.items[handle.id];
    if (!item->initialized || !item->buffers.buffers_valid) {
        return -2;
    }
    
    // Upload bone matrices to GPU if dirty
    if (item->dirty) {
        // gpu_buffer_upload(item->buffers.bone_matrices_buffer, 
        //                   item->buffers.bone_matrices,
        //                   item->buffers.bone_count * sizeof(mat4_t));
        item->dirty = false;
    }
    
    // Bind compute pipeline based on skinning method
    // if (item->method == SKINNING_METHOD_DUAL_QUATERNION) {
    //     bind_compute_pipeline(g_gpu_skinning_ctx.dq_compute_pipeline);
    // } else {
    //     bind_compute_pipeline(g_gpu_skinning_ctx.lbs_compute_pipeline);
    // }
    
    // Bind buffers
    // bind_storage_buffer(item->buffers.bone_matrices_buffer, 0);
    // bind_storage_buffer(item->buffers.src_vertices_buffer, 1);
    // bind_storage_buffer(item->buffers.bone_weights_buffer, 2);
    // bind_storage_buffer(item->buffers.bone_indices_buffer, 3);
    // bind_storage_buffer(item->buffers.dst_vertices_buffer, 4);  // Output
    
    // Dispatch compute shader
    // dispatch_compute(item->dispatch_x, item->dispatch_y, item->dispatch_z);
    
    item->frame_updated++;
    return 0;
}

int animation_gpu_skinning_update(animation_gpu_skinning_handle_t handle, 
                                   const void* data, size_t size) {
    if (handle.id >= g_gpu_skinning_ctx.count) {
        return -1;
    }

    animation_gpu_skinning_internal_t* item = &g_gpu_skinning_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Interpret data as bone matrices
    if (data && size >= item->buffers.bone_count * sizeof(mat4_t)) {
        return animation_gpu_skinning_upload_matrices(handle, data, item->buffers.bone_count);
    }

    item->dirty = true;
    return 0;
}

bool animation_gpu_skinning_is_valid(animation_gpu_skinning_handle_t handle) {
    if (handle.id >= g_gpu_skinning_ctx.count) {
        return false;
    }
    return g_gpu_skinning_ctx.items[handle.id].initialized;
}

int animation_gpu_skinning_get_info(animation_gpu_skinning_handle_t handle, 
                                     animation_gpu_skinning_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_gpu_skinning_ctx.count) {
        return -2;
    }

    const animation_gpu_skinning_internal_t* item = &g_gpu_skinning_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_gpu_skinning_mark_dirty(animation_gpu_skinning_handle_t handle) {
    if (handle.id < g_gpu_skinning_ctx.count) {
        g_gpu_skinning_ctx.items[handle.id].dirty = true;
    }
}

int animation_gpu_skinning_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_gpu_skinning_ctx.count; i++) {
        animation_gpu_skinning_internal_t* item = &g_gpu_skinning_ctx.items[i];
        if (item->initialized && item->dirty && item->buffers.buffers_valid) {
            animation_gpu_skinning_handle_t handle = {.id = i};
            animation_gpu_skinning_dispatch(handle);
            processed++;
        }
    }

    return processed;
}

uint32_t animation_gpu_skinning_get_count(void) {
    return g_gpu_skinning_ctx.count;
}

size_t animation_gpu_skinning_get_memory_usage(void) {
    size_t total = sizeof(g_gpu_skinning_ctx);
    total += g_gpu_skinning_ctx.capacity * sizeof(animation_gpu_skinning_internal_t);

    for (uint32_t i = 0; i < g_gpu_skinning_ctx.count; i++) {
        skinning_buffers_t* buf = &g_gpu_skinning_ctx.items[i].buffers;
        if (buf->bone_matrices) {
            total += buf->bone_count * sizeof(mat4_t);
        }
        if (buf->bone_dual_quats) {
            total += buf->bone_count * sizeof(dual_quat_t);
        }
    }

    return total;
}

void animation_gpu_skinning_debug_print(void) {
    // Debug output for GPU skinning state
}

/* End of gpu_skinning.c */
