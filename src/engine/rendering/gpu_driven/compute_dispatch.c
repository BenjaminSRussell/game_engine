/*
 * compute_dispatch.c
 * Centralized GPU compute shader dispatch framework
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/gpu_driven/compute_dispatch.h"
#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_encoder.h"
#include "backend/metal/mtl_pipeline.h"
#include "../../3d_rendering.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_DISPATCHERS 256
#define MAX_BOUND_BUFFERS 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct compute_dispatcher_internal {
    uint32_t id;
    metal_compute_pipeline_t* pipeline;
    metal_device_t* device;

    /* Buffer bindings */
    metal_buffer_t* bound_buffers[MAX_BOUND_BUFFERS];
    size_t buffer_offsets[MAX_BOUND_BUFFERS];
    uint32_t bound_buffer_count;

    /* Shader info */
    char shader_path[256];
    char kernel_name[128];

    /* Threadgroup size */
    uint32_t threadgroup_size_x;
    uint32_t threadgroup_size_y;
    uint32_t threadgroup_size_z;

    /* Statistics */
    compute_dispatch_stats_t stats;

    bool initialized;
} compute_dispatcher_internal_t;

typedef struct compute_dispatch_context {
    compute_dispatcher_internal_t* dispatchers;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} compute_dispatch_context_t;

static compute_dispatch_context_t g_compute_dispatch_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void compute_dispatcher_cleanup_internal(compute_dispatcher_internal_t* dispatcher) {
    if (!dispatcher) return;

    // Pipeline is managed by Metal device, don't destroy it here
    dispatcher->pipeline = NULL;
    dispatcher->device = NULL;

    // Clear buffer bindings
    for (uint32_t i = 0; i < MAX_BOUND_BUFFERS; i++) {
        dispatcher->bound_buffers[i] = NULL;
        dispatcher->buffer_offsets[i] = 0;
    }
    dispatcher->bound_buffer_count = 0;

    memset(&dispatcher->stats, 0, sizeof(compute_dispatch_stats_t));
    dispatcher->initialized = false;
}

/* ============================================================================
 * PUBLIC API - INITIALIZATION
 * ============================================================================ */

int compute_dispatch_init(void) {
    if (g_compute_dispatch_ctx.initialized) {
        return 0;
    }

    g_compute_dispatch_ctx.capacity = 64;
    g_compute_dispatch_ctx.dispatchers = calloc(g_compute_dispatch_ctx.capacity,
                                               sizeof(compute_dispatcher_internal_t));
    if (!g_compute_dispatch_ctx.dispatchers) {
        return -1;
    }

    g_compute_dispatch_ctx.count = 0;
    g_compute_dispatch_ctx.initialized = true;

    return 0;
}

void compute_dispatch_shutdown(void) {
    if (!g_compute_dispatch_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_compute_dispatch_ctx.count; i++) {
        compute_dispatcher_cleanup_internal(&g_compute_dispatch_ctx.dispatchers[i]);
    }

    free(g_compute_dispatch_ctx.dispatchers);
    g_compute_dispatch_ctx.dispatchers = NULL;
    g_compute_dispatch_ctx.count = 0;
    g_compute_dispatch_ctx.capacity = 0;
    g_compute_dispatch_ctx.initialized = false;
}

/* ============================================================================
 * PUBLIC API - DISPATCHER LIFECYCLE
 * ============================================================================ */

int compute_dispatch_create(compute_dispatcher_handle_t* out_handle,
                           const compute_dispatch_desc_t* desc) {
    if (!out_handle || !desc || !desc->device || !desc->shader_path || !desc->kernel_name) {
        return -1;
    }

    if (!g_compute_dispatch_ctx.initialized) {
        return -2;
    }

    // Grow capacity if needed
    if (g_compute_dispatch_ctx.count >= g_compute_dispatch_ctx.capacity) {
        uint32_t new_capacity = g_compute_dispatch_ctx.capacity * 2;
        if (new_capacity > MAX_DISPATCHERS) {
            new_capacity = MAX_DISPATCHERS;
        }
        if (new_capacity == g_compute_dispatch_ctx.capacity) {
            return -3;  // At max capacity
        }

        compute_dispatcher_internal_t* new_dispatchers = realloc(g_compute_dispatch_ctx.dispatchers,
                                                               new_capacity * sizeof(compute_dispatcher_internal_t));
        if (!new_dispatchers) {
            return -4;
        }

        g_compute_dispatch_ctx.dispatchers = new_dispatchers;
        g_compute_dispatch_ctx.capacity = new_capacity;
    }

    uint32_t index = g_compute_dispatch_ctx.count++;
    compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[index];

    dispatcher->id = index;
    dispatcher->device = desc->device;

    strncpy(dispatcher->shader_path, desc->shader_path, sizeof(dispatcher->shader_path) - 1);
    strncpy(dispatcher->kernel_name, desc->kernel_name, sizeof(dispatcher->kernel_name) - 1);

    dispatcher->threadgroup_size_x = desc->threadgroup_size_x > 0 ? desc->threadgroup_size_x : 256;
    dispatcher->threadgroup_size_y = desc->threadgroup_size_y > 0 ? desc->threadgroup_size_y : 1;
    dispatcher->threadgroup_size_z = desc->threadgroup_size_z > 0 ? desc->threadgroup_size_z : 1;

    dispatcher->bound_buffer_count = 0;
    memset(&dispatcher->stats, 0, sizeof(compute_dispatch_stats_t));

    // Load shader and create pipeline
    // TODO: Actually load the shader and create compute pipeline
    // For now, this is a placeholder that needs Metal integration

    dispatcher->initialized = true;

    out_handle->id = index;
    return 0;
}

void compute_dispatch_destroy(compute_dispatcher_handle_t handle) {
    if (handle.id >= g_compute_dispatch_ctx.count) {
        return;
    }

    compute_dispatcher_cleanup_internal(&g_compute_dispatch_ctx.dispatchers[handle.id]);
}

bool compute_dispatch_is_valid(compute_dispatcher_handle_t handle) {
    if (handle.id >= g_compute_dispatch_ctx.count) {
        return false;
    }
    return g_compute_dispatch_ctx.dispatchers[handle.id].initialized;
}

/* ============================================================================
 * PUBLIC API - BUFFER BINDING
 * ============================================================================ */

int compute_dispatch_bind_buffer(compute_dispatcher_handle_t handle,
                                uint32_t buffer_index,
                                metal_buffer_t* buffer,
                                size_t offset) {
    if (handle.id >= g_compute_dispatch_ctx.count || buffer_index >= MAX_BOUND_BUFFERS) {
        return -1;
    }

    compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[handle.id];
    if (!dispatcher->initialized) {
        return -2;
    }

    dispatcher->bound_buffers[buffer_index] = buffer;
    dispatcher->buffer_offsets[buffer_index] = offset;

    // Update bound buffer count
    if (buffer_index >= dispatcher->bound_buffer_count) {
        dispatcher->bound_buffer_count = buffer_index + 1;
    }

    return 0;
}

int compute_dispatch_bind_buffers(compute_dispatcher_handle_t handle,
                                 metal_buffer_t** buffers,
                                 uint32_t buffer_count) {
    if (handle.id >= g_compute_dispatch_ctx.count || !buffers || buffer_count > MAX_BOUND_BUFFERS) {
        return -1;
    }

    compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[handle.id];
    if (!dispatcher->initialized) {
        return -2;
    }

    for (uint32_t i = 0; i < buffer_count; i++) {
        dispatcher->bound_buffers[i] = buffers[i];
        dispatcher->buffer_offsets[i] = 0;
    }

    dispatcher->bound_buffer_count = buffer_count;
    return 0;
}

void compute_dispatch_clear_buffers(compute_dispatcher_handle_t handle) {
    if (handle.id >= g_compute_dispatch_ctx.count) {
        return;
    }

    compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[handle.id];
    if (!dispatcher->initialized) {
        return;
    }

    for (uint32_t i = 0; i < MAX_BOUND_BUFFERS; i++) {
        dispatcher->bound_buffers[i] = NULL;
        dispatcher->buffer_offsets[i] = 0;
    }
    dispatcher->bound_buffer_count = 0;
}

/* ============================================================================
 * PUBLIC API - DISPATCH
 * ============================================================================ */

int compute_dispatch_execute(compute_dispatcher_handle_t handle,
                            MTLCommandBuffer* command_buffer,
                            uint32_t thread_count) {
    if (handle.id >= g_compute_dispatch_ctx.count || !command_buffer) {
        return -1;
    }

    compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[handle.id];
    if (!dispatcher->initialized || !dispatcher->pipeline) {
        return -2;
    }

    // Calculate threadgroup count
    uint32_t threadgroup_size = dispatcher->threadgroup_size_x *
                               dispatcher->threadgroup_size_y *
                               dispatcher->threadgroup_size_z;
    uint32_t threadgroup_count = (thread_count + threadgroup_size - 1) / threadgroup_size;

    return compute_dispatch_execute_threadgroups(handle, command_buffer, threadgroup_count);
}

int compute_dispatch_execute_threadgroups(compute_dispatcher_handle_t handle,
                                         MTLCommandBuffer* command_buffer,
                                         uint32_t threadgroup_count) {
    if (handle.id >= g_compute_dispatch_ctx.count || !command_buffer) {
        return -1;
    }

    compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[handle.id];
    if (!dispatcher->initialized || !dispatcher->pipeline) {
        return -2;
    }

    // TODO: Implement actual Metal compute dispatch
    // This requires:
    // 1. Get compute encoder from command buffer
    // 2. Set compute pipeline state
    // 3. Bind buffers
    // 4. Dispatch threadgroups
    // 5. End encoding

    // Update statistics
    dispatcher->stats.total_dispatches++;
    dispatcher->stats.threads_per_call = threadgroup_count * dispatcher->threadgroup_size_x;
    if (threadgroup_count > dispatcher->stats.peak_threadgroups) {
        dispatcher->stats.peak_threadgroups = threadgroup_count;
    }

    return 0;
}

int compute_dispatch_execute_timed(compute_dispatcher_handle_t handle,
                                  MTLCommandBuffer* command_buffer,
                                  uint32_t thread_count,
                                  uint64_t* out_gpu_time_ns) {
    if (handle.id >= g_compute_dispatch_ctx.count) {
        return -1;
    }

    int result = compute_dispatch_execute(handle, command_buffer, thread_count);

    // TODO: Get actual GPU timing
    // For now, just return a placeholder
    if (out_gpu_time_ns) {
        *out_gpu_time_ns = 0;
    }

    return result;
}

/* ============================================================================
 * PUBLIC API - STATISTICS
 * ============================================================================ */

int compute_dispatch_get_stats(compute_dispatcher_handle_t handle,
                              compute_dispatch_stats_t* out_stats) {
    if (handle.id >= g_compute_dispatch_ctx.count || !out_stats) {
        return -1;
    }

    compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[handle.id];
    if (!dispatcher->initialized) {
        return -2;
    }

    memcpy(out_stats, &dispatcher->stats, sizeof(compute_dispatch_stats_t));
    return 0;
}

void compute_dispatch_reset_stats(compute_dispatcher_handle_t handle) {
    if (handle.id >= g_compute_dispatch_ctx.count) {
        return;
    }

    compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[handle.id];
    if (!dispatcher->initialized) {
        return;
    }

    memset(&dispatcher->stats, 0, sizeof(compute_dispatch_stats_t));
}

size_t compute_dispatch_get_total_memory(void) {
    size_t total = sizeof(compute_dispatch_context_t);
    total += g_compute_dispatch_ctx.capacity * sizeof(compute_dispatcher_internal_t);
    return total;
}

void compute_dispatch_debug_print(void) {
    if (!g_compute_dispatch_ctx.initialized) {
        return;
    }

    printf("Compute Dispatch Status:\n");
    printf("  Total Dispatchers: %u / %u\n", g_compute_dispatch_ctx.count, g_compute_dispatch_ctx.capacity);
    printf("  Memory Usage: %.2f KB\n", compute_dispatch_get_total_memory() / 1024.0f);

    for (uint32_t i = 0; i < g_compute_dispatch_ctx.count; i++) {
        compute_dispatcher_internal_t* dispatcher = &g_compute_dispatch_ctx.dispatchers[i];
        if (dispatcher->initialized) {
            printf("\n  Dispatcher %u: %s::%s\n", i, dispatcher->shader_path, dispatcher->kernel_name);
            printf("    Threadgroup: (%u, %u, %u)\n",
                   dispatcher->threadgroup_size_x,
                   dispatcher->threadgroup_size_y,
                   dispatcher->threadgroup_size_z);
            printf("    Bound Buffers: %u\n", dispatcher->bound_buffer_count);
            printf("    Dispatches: %u\n", dispatcher->stats.total_dispatches);
            printf("    Peak Threadgroups: %u\n", dispatcher->stats.peak_threadgroups);
        }
    }
}

/* End of compute_dispatch.c */
