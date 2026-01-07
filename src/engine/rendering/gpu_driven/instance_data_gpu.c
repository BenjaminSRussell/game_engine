/*
 * instance_data_gpu.c
 * GPU instance buffer
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/gpu_driven/instance_data_gpu.h"
#include "backend/metal/mtl_buffer.h"
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

#define INITIAL_INSTANCE_CAPACITY 256
#define MAX_INSTANCES 65536
#define INSTANCE_SIZE sizeof(gpu_instance_data_t)

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_instance_data_gpu_internal {
    uint32_t id;
    uint32_t flags;
    metal_device_t* device;
    metal_buffer_t* gpu_buffer;                  // GPU-resident instance data
    gpu_instance_state_t* cpu_instances;         // CPU-side instance array
    uint32_t instance_capacity;
    uint32_t instance_count;
    uint32_t* dirty_instances;                   // Indices of dirty instances
    uint32_t dirty_count;
    bool initialized;

    /* Statistics */
    size_t peak_memory_used;
    uint32_t frame_number;
} rendering_instance_data_gpu_internal_t;

typedef struct rendering_instance_data_gpu_context {
    rendering_instance_data_gpu_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_instance_data_gpu_context_t;

static rendering_instance_data_gpu_context_t g_instance_data_gpu_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_instance_data_gpu_cleanup_internal(rendering_instance_data_gpu_internal_t* item) {
    if (!item) return;

    // Destroy GPU buffer
    if (item->gpu_buffer) {
        metal_buffer_destroy(item->gpu_buffer);
        item->gpu_buffer = NULL;
    }

    // Free CPU instance array
    if (item->cpu_instances) {
        free(item->cpu_instances);
        item->cpu_instances = NULL;
    }

    // Free dirty instances tracking
    if (item->dirty_instances) {
        free(item->dirty_instances);
        item->dirty_instances = NULL;
    }

    item->instance_count = 0;
    item->instance_capacity = 0;
    item->dirty_count = 0;
    item->initialized = false;
}

static int rendering_instance_data_gpu_grow_capacity(rendering_instance_data_gpu_internal_t* item,
                                                     uint32_t new_capacity) {
    if (new_capacity > MAX_INSTANCES) {
        return -1;  // Exceeded maximum
    }

    // Grow CPU instance array
    gpu_instance_state_t* new_instances = realloc(item->cpu_instances,
                                                   new_capacity * sizeof(gpu_instance_state_t));
    if (!new_instances) {
        return -2;
    }

    // Initialize new instances
    for (uint32_t i = item->instance_capacity; i < new_capacity; i++) {
        memset(&new_instances[i], 0, sizeof(gpu_instance_state_t));
    }

    item->cpu_instances = new_instances;
    item->instance_capacity = new_capacity;

    // Reallocate GPU buffer with new size
    if (item->gpu_buffer) {
        metal_buffer_destroy(item->gpu_buffer);
    }

    metal_buffer_desc_t buffer_desc = {
        .size = new_capacity * INSTANCE_SIZE,
        .storage_mode = METAL_STORAGE_SHARED,  // CPU-writable
        .usage = METAL_BUFFER_USAGE_STORAGE,
        .label = "Instance Data Buffer"
    };

    item->gpu_buffer = metal_buffer_create(item->device, &buffer_desc);
    if (!item->gpu_buffer) {
        return -3;
    }

    // Regrow dirty instances array
    uint32_t* new_dirty = realloc(item->dirty_instances,
                                  new_capacity * sizeof(uint32_t));
    if (!new_dirty) {
        return -4;
    }

    item->dirty_instances = new_dirty;

    // Update statistics
    size_t new_memory = (new_capacity * INSTANCE_SIZE) +
                       (new_capacity * sizeof(gpu_instance_state_t)) +
                       (new_capacity * sizeof(uint32_t));
    if (new_memory > item->peak_memory_used) {
        item->peak_memory_used = new_memory;
    }

    return 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_instance_data_gpu_init(void) {
    if (g_instance_data_gpu_ctx.initialized) {
        return 0;
    }

    g_instance_data_gpu_ctx.capacity = 256;
    g_instance_data_gpu_ctx.items = calloc(g_instance_data_gpu_ctx.capacity, sizeof(rendering_instance_data_gpu_internal_t));
    if (!g_instance_data_gpu_ctx.items) {
        return -1;
    }

    g_instance_data_gpu_ctx.count = 0;
    g_instance_data_gpu_ctx.initialized = true;

    return 0;
}

void rendering_instance_data_gpu_shutdown(void) {
    if (!g_instance_data_gpu_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_instance_data_gpu_ctx.count; i++) {
        rendering_instance_data_gpu_cleanup_internal(&g_instance_data_gpu_ctx.items[i]);
    }

    free(g_instance_data_gpu_ctx.items);
    g_instance_data_gpu_ctx.items = NULL;
    g_instance_data_gpu_ctx.count = 0;
    g_instance_data_gpu_ctx.capacity = 0;
    g_instance_data_gpu_ctx.initialized = false;
}

int rendering_instance_data_gpu_create(rendering_instance_data_gpu_handle_t* out_handle, const rendering_instance_data_gpu_desc_t* desc) {
    if (!out_handle || !desc || !desc->device) {
        return -1;
    }

    if (!g_instance_data_gpu_ctx.initialized) {
        return -2;
    }

    if (g_instance_data_gpu_ctx.count >= g_instance_data_gpu_ctx.capacity) {
        uint32_t new_capacity = g_instance_data_gpu_ctx.capacity * 2;
        rendering_instance_data_gpu_internal_t* new_items = realloc(g_instance_data_gpu_ctx.items,
                                                                    new_capacity * sizeof(rendering_instance_data_gpu_internal_t));
        if (!new_items) return -3;
        g_instance_data_gpu_ctx.items = new_items;
        g_instance_data_gpu_ctx.capacity = new_capacity;
    }

    uint32_t index = g_instance_data_gpu_ctx.count++;
    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->device = desc->device;
    item->instance_count = 0;
    item->dirty_count = 0;
    item->frame_number = 0;
    item->peak_memory_used = 0;

    // Initialize with requested capacity or default
    uint32_t initial_capacity = desc->max_instances > 0 ? desc->max_instances : INITIAL_INSTANCE_CAPACITY;
    if (initial_capacity > MAX_INSTANCES) {
        initial_capacity = MAX_INSTANCES;
    }

    // Allocate CPU instance array
    item->cpu_instances = calloc(initial_capacity, sizeof(gpu_instance_state_t));
    if (!item->cpu_instances) {
        return -4;
    }

    // Allocate dirty instances tracking
    item->dirty_instances = calloc(initial_capacity, sizeof(uint32_t));
    if (!item->dirty_instances) {
        free(item->cpu_instances);
        return -5;
    }

    item->instance_capacity = initial_capacity;

    // Allocate GPU buffer
    metal_buffer_desc_t buffer_desc = {
        .size = initial_capacity * INSTANCE_SIZE,
        .storage_mode = METAL_STORAGE_SHARED,  // CPU-writable for updates
        .usage = METAL_BUFFER_USAGE_STORAGE,
        .label = "Instance Data Buffer"
    };

    item->gpu_buffer = metal_buffer_create(desc->device, &buffer_desc);
    if (!item->gpu_buffer) {
        free(item->cpu_instances);
        free(item->dirty_instances);
        return -6;
    }

    item->initialized = true;

    out_handle->id = index;
    return 0;
}

void rendering_instance_data_gpu_destroy(rendering_instance_data_gpu_handle_t handle) {
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return;
    }

    rendering_instance_data_gpu_cleanup_internal(&g_instance_data_gpu_ctx.items[handle.id]);
}

int rendering_instance_data_gpu_add_instance(rendering_instance_data_gpu_handle_t handle,
                                            const gpu_instance_data_t* instance_data,
                                            uint32_t* out_instance_id) {
    if (handle.id >= g_instance_data_gpu_ctx.count || !instance_data || !out_instance_id) {
        return -1;
    }

    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Grow capacity if needed
    if (item->instance_count >= item->instance_capacity) {
        uint32_t new_capacity = item->instance_capacity * 2;
        if (new_capacity > MAX_INSTANCES) {
            new_capacity = MAX_INSTANCES;
        }
        if (new_capacity == item->instance_capacity) {
            return -3;  // At max capacity
        }

        int result = rendering_instance_data_gpu_grow_capacity(item, new_capacity);
        if (result != 0) {
            return result - 10;  // Adjust error codes
        }
    }

    // Add instance
    uint32_t instance_id = item->instance_count++;
    gpu_instance_state_t* state = &item->cpu_instances[instance_id];
    memcpy(&state->data, instance_data, sizeof(gpu_instance_data_t));
    state->dirty = true;
    state->frame_updated = item->frame_number;

    // Add to dirty list
    if (item->dirty_count < item->instance_capacity) {
        item->dirty_instances[item->dirty_count++] = instance_id;
    }

    *out_instance_id = instance_id;
    return 0;
}

int rendering_instance_data_gpu_update_instance(rendering_instance_data_gpu_handle_t handle,
                                               uint32_t instance_id,
                                               const gpu_instance_data_t* instance_data) {
    if (handle.id >= g_instance_data_gpu_ctx.count || !instance_data) {
        return -1;
    }

    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    if (!item->initialized || instance_id >= item->instance_count) {
        return -2;
    }

    gpu_instance_state_t* state = &item->cpu_instances[instance_id];
    memcpy(&state->data, instance_data, sizeof(gpu_instance_data_t));
    state->dirty = true;
    state->frame_updated = item->frame_number;

    // Add to dirty list if not already there
    bool found = false;
    for (uint32_t i = 0; i < item->dirty_count; i++) {
        if (item->dirty_instances[i] == instance_id) {
            found = true;
            break;
        }
    }

    if (!found && item->dirty_count < item->instance_capacity) {
        item->dirty_instances[item->dirty_count++] = instance_id;
    }

    return 0;
}

int rendering_instance_data_gpu_update_instance_transform(rendering_instance_data_gpu_handle_t handle,
                                                         uint32_t instance_id,
                                                         const float* transform_matrix) {
    if (handle.id >= g_instance_data_gpu_ctx.count || !transform_matrix) {
        return -1;
    }

    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    if (!item->initialized || instance_id >= item->instance_count) {
        return -2;
    }

    gpu_instance_state_t* state = &item->cpu_instances[instance_id];
    memcpy(state->data.transform, transform_matrix, 16 * sizeof(float));
    state->dirty = true;
    state->frame_updated = item->frame_number;

    // Add to dirty list if not already there
    bool found = false;
    for (uint32_t i = 0; i < item->dirty_count; i++) {
        if (item->dirty_instances[i] == instance_id) {
            found = true;
            break;
        }
    }

    if (!found && item->dirty_count < item->instance_capacity) {
        item->dirty_instances[item->dirty_count++] = instance_id;
    }

    return 0;
}

int rendering_instance_data_gpu_update_instance_bounds(rendering_instance_data_gpu_handle_t handle,
                                                      uint32_t instance_id,
                                                      const float* center,
                                                      float radius) {
    if (handle.id >= g_instance_data_gpu_ctx.count || !center) {
        return -1;
    }

    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    if (!item->initialized || instance_id >= item->instance_count) {
        return -2;
    }

    gpu_instance_state_t* state = &item->cpu_instances[instance_id];
    memcpy(state->data.bounds_center, center, 3 * sizeof(float));
    state->data.bounds_radius = radius;
    state->dirty = true;
    state->frame_updated = item->frame_number;

    // Add to dirty list if not already there
    bool found = false;
    for (uint32_t i = 0; i < item->dirty_count; i++) {
        if (item->dirty_instances[i] == instance_id) {
            found = true;
            break;
        }
    }

    if (!found && item->dirty_count < item->instance_capacity) {
        item->dirty_instances[item->dirty_count++] = instance_id;
    }

    return 0;
}

int rendering_instance_data_gpu_remove_instance(rendering_instance_data_gpu_handle_t handle,
                                               uint32_t instance_id) {
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return -1;
    }

    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    if (!item->initialized || instance_id >= item->instance_count) {
        return -2;
    }

    // Mark as deleted (swap with last if not last)
    if (instance_id != item->instance_count - 1) {
        memcpy(&item->cpu_instances[instance_id],
               &item->cpu_instances[item->instance_count - 1],
               sizeof(gpu_instance_state_t));
    }

    item->instance_count--;
    return 0;
}

int rendering_instance_data_gpu_update(rendering_instance_data_gpu_handle_t handle, const void* data, size_t size) {
    // Legacy function - just marks as dirty
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return -1;
    }

    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Mark all instances dirty
    item->dirty_count = item->instance_count;
    for (uint32_t i = 0; i < item->instance_count; i++) {
        item->dirty_instances[i] = i;
        item->cpu_instances[i].dirty = true;
    }

    return 0;
}

bool rendering_instance_data_gpu_is_valid(rendering_instance_data_gpu_handle_t handle) {
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return false;
    }
    return g_instance_data_gpu_ctx.items[handle.id].initialized;
}

int rendering_instance_data_gpu_get_info(rendering_instance_data_gpu_handle_t handle, rendering_instance_data_gpu_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return -2;
    }

    const rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->instance_count = item->instance_count;
    out_info->max_capacity = item->instance_capacity;
    out_info->memory_used = (item->instance_count * INSTANCE_SIZE) +
                           (item->instance_capacity * sizeof(gpu_instance_state_t)) +
                           (item->instance_capacity * sizeof(uint32_t));

    return 0;
}

void rendering_instance_data_gpu_mark_dirty(rendering_instance_data_gpu_handle_t handle) {
    if (handle.id < g_instance_data_gpu_ctx.count) {
        rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
        // Mark all instances dirty
        item->dirty_count = item->instance_count;
        for (uint32_t i = 0; i < item->instance_count; i++) {
            item->dirty_instances[i] = i;
            item->cpu_instances[i].dirty = true;
        }
    }
}

int rendering_instance_data_gpu_process_pending(metal_device_t* device) {
    if (!g_instance_data_gpu_ctx.initialized || !device) return 0;

    int total_uploaded = 0;

    for (uint32_t i = 0; i < g_instance_data_gpu_ctx.count; i++) {
        rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[i];
        if (!item->initialized || item->dirty_count == 0) {
            continue;
        }

        // Upload dirty instances to GPU buffer
        if (item->gpu_buffer && item->gpu_buffer->mapped_ptr) {
            gpu_instance_data_t* gpu_data = (gpu_instance_data_t*)item->gpu_buffer->mapped_ptr;

            for (uint32_t j = 0; j < item->dirty_count; j++) {
                uint32_t instance_id = item->dirty_instances[j];
                if (instance_id < item->instance_count) {
                    memcpy(&gpu_data[instance_id],
                           &item->cpu_instances[instance_id].data,
                           sizeof(gpu_instance_data_t));
                    item->cpu_instances[instance_id].dirty = false;
                    total_uploaded++;
                }
            }
        }

        item->dirty_count = 0;
        item->frame_number++;
    }

    return total_uploaded;
}

metal_buffer_t* rendering_instance_data_gpu_get_buffer(rendering_instance_data_gpu_handle_t handle) {
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return NULL;
    }

    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    if (!item->initialized) {
        return NULL;
    }

    return item->gpu_buffer;
}

uint32_t rendering_instance_data_gpu_get_count(void) {
    return g_instance_data_gpu_ctx.count;
}

uint32_t rendering_instance_data_gpu_get_instance_count(rendering_instance_data_gpu_handle_t handle) {
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return 0;
    }

    return g_instance_data_gpu_ctx.items[handle.id].instance_count;
}

size_t rendering_instance_data_gpu_get_memory_usage(void) {
    size_t total = sizeof(rendering_instance_data_gpu_context_t);
    total += g_instance_data_gpu_ctx.capacity * sizeof(rendering_instance_data_gpu_internal_t);

    for (uint32_t i = 0; i < g_instance_data_gpu_ctx.count; i++) {
        if (g_instance_data_gpu_ctx.items[i].initialized) {
            rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[i];
            total += (item->instance_capacity * INSTANCE_SIZE) +
                    (item->instance_capacity * sizeof(gpu_instance_state_t)) +
                    (item->instance_capacity * sizeof(uint32_t));
        }
    }

    return total;
}

void rendering_instance_data_gpu_debug_print(void) {
    if (!g_instance_data_gpu_ctx.initialized) return;

    printf("Instance Data GPU Status:\n");
    printf("  Total Buffers: %u / %u\n", g_instance_data_gpu_ctx.count, g_instance_data_gpu_ctx.capacity);
    printf("  Memory Usage: %.2f MB\n", rendering_instance_data_gpu_get_memory_usage() / (1024.0f * 1024.0f));

    for (uint32_t i = 0; i < g_instance_data_gpu_ctx.count; i++) {
        rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[i];
        if (item->initialized) {
            printf("\n  Buffer %u:\n", i);
            printf("    Instances: %u / %u\n", item->instance_count, item->instance_capacity);
            printf("    Dirty: %u\n", item->dirty_count);
            printf("    Memory: %.2f KB\n", (item->instance_capacity * INSTANCE_SIZE) / 1024.0f);
        }
    }
}

/* End of instance_data_gpu.c */
