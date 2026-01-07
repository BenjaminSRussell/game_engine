/*
 * draw_command_gen.c
 * Draw command generation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/gpu_driven/draw_command_gen.h"
#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_device.h"
#include "../core/gpu_types.h"
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

#define MAX_DRAW_COMMANDS 8192
#define MAX_BATCHES 4096
#define INITIAL_BATCH_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_draw_command_gen_internal {
    uint32_t id;
    uint32_t flags;
    metal_device_t* device;

    // Batch management
    draw_batch_t* batches;
    uint32_t batch_capacity;
    uint32_t batch_count;

    // GPU buffers
    metal_buffer_t* command_buffer;      // Indirect draw commands
    metal_buffer_t* visible_buffer;      // Visible instance IDs
    metal_buffer_t* counter_buffer;      // Atomic counters

    // CPU-side data
    void* command_data;                  // Mapped command buffer
    void* visible_data;                  // Mapped visible buffer
    void* counter_data;                  // Mapped counter buffer

    // Statistics
    uint32_t total_commands_generated;
    uint32_t peak_batch_count;

    bool initialized;
    bool dirty;
} rendering_draw_command_gen_internal_t;

typedef struct rendering_draw_command_gen_context {
    rendering_draw_command_gen_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_draw_command_gen_context_t;

static rendering_draw_command_gen_context_t g_draw_command_gen_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_draw_command_gen_cleanup_internal(rendering_draw_command_gen_internal_t* item) {
    if (!item) return;

    // Destroy GPU buffers
    if (item->command_buffer) {
        metal_buffer_destroy(item->command_buffer);
        item->command_buffer = NULL;
        item->command_data = NULL;
    }

    if (item->visible_buffer) {
        metal_buffer_destroy(item->visible_buffer);
        item->visible_buffer = NULL;
        item->visible_data = NULL;
    }

    if (item->counter_buffer) {
        metal_buffer_destroy(item->counter_buffer);
        item->counter_buffer = NULL;
        item->counter_data = NULL;
    }

    // Free batch array
    if (item->batches) {
        free(item->batches);
        item->batches = NULL;
    }

    item->batch_capacity = 0;
    item->batch_count = 0;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_draw_command_gen_init(void) {
    if (g_draw_command_gen_ctx.initialized) {
        return 0;
    }

    g_draw_command_gen_ctx.capacity = 256;
    g_draw_command_gen_ctx.items = calloc(g_draw_command_gen_ctx.capacity, sizeof(rendering_draw_command_gen_internal_t));
    if (!g_draw_command_gen_ctx.items) {
        return -1;
    }

    g_draw_command_gen_ctx.count = 0;
    g_draw_command_gen_ctx.initialized = true;

    return 0;
}

void rendering_draw_command_gen_shutdown(void) {
    if (!g_draw_command_gen_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_draw_command_gen_ctx.count; i++) {
        rendering_draw_command_gen_cleanup_internal(&g_draw_command_gen_ctx.items[i]);
    }

    free(g_draw_command_gen_ctx.items);
    g_draw_command_gen_ctx.items = NULL;
    g_draw_command_gen_ctx.count = 0;
    g_draw_command_gen_ctx.capacity = 0;
    g_draw_command_gen_ctx.initialized = false;
}

int rendering_draw_command_gen_create(rendering_draw_command_gen_handle_t* out_handle, const rendering_draw_command_gen_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_draw_command_gen_ctx.initialized) {
        return -2;
    }

    if (g_draw_command_gen_ctx.count >= g_draw_command_gen_ctx.capacity) {
        uint32_t new_capacity = g_draw_command_gen_ctx.capacity * 2;
        rendering_draw_command_gen_internal_t* new_items = realloc(g_draw_command_gen_ctx.items, new_capacity * sizeof(rendering_draw_command_gen_internal_t));
        if (!new_items) return -3;
        g_draw_command_gen_ctx.items = new_items;
        g_draw_command_gen_ctx.capacity = new_capacity;
    }

    uint32_t index = g_draw_command_gen_ctx.count++;
    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->device = desc->device;

    // Allocate batch management array
    item->batch_capacity = INITIAL_BATCH_CAPACITY;
    item->batches = malloc(item->batch_capacity * sizeof(draw_batch_t));
    if (!item->batches) {
        return -4;
    }
    item->batch_count = 0;

    // Allocate GPU buffers for indirect rendering
    // Command buffer: stores indirect draw commands
    uint32_t max_commands = desc->max_draw_commands > 0 ? desc->max_draw_commands : MAX_DRAW_COMMANDS;
    size_t command_buffer_size = max_commands * sizeof(IndirectDrawArgs);

    metal_buffer_desc_t cmd_desc = {
        .size = command_buffer_size,
        .storage_mode = METAL_STORAGE_SHARED,  // CPU-writable for batch generation
        .usage = METAL_BUFFER_USAGE_STORAGE,
        .label = "Draw Command Buffer"
    };
    item->command_buffer = metal_buffer_create(desc->device, &cmd_desc);
    if (!item->command_buffer) {
        free(item->batches);
        return -5;
    }
    item->command_data = metal_buffer_get_cpu_ptr(item->command_buffer);

    // Visible IDs buffer: stores visible instance IDs from culling
    size_t visible_buffer_size = max_commands * MAX_BATCHES * sizeof(uint32_t);  // Conservative estimate
    metal_buffer_desc_t vis_desc = {
        .size = visible_buffer_size,
        .storage_mode = METAL_STORAGE_SHARED,
        .usage = METAL_BUFFER_USAGE_STORAGE,
        .label = "Visible Instance IDs Buffer"
    };
    item->visible_buffer = metal_buffer_create(desc->device, &vis_desc);
    if (!item->visible_buffer) {
        metal_buffer_destroy(item->command_buffer);
        item->command_buffer = NULL;
        free(item->batches);
        return -6;
    }
    item->visible_data = metal_buffer_get_cpu_ptr(item->visible_buffer);

    // Counter buffer: atomic counters for GPU-side counting
    size_t counter_buffer_size = 4 * sizeof(uint32_t);  // 4 counters (visible count, batch count, etc)
    metal_buffer_desc_t ctr_desc = {
        .size = counter_buffer_size,
        .storage_mode = METAL_STORAGE_SHARED,
        .usage = METAL_BUFFER_USAGE_STORAGE,
        .label = "Draw Command Counter Buffer"
    };
    item->counter_buffer = metal_buffer_create(desc->device, &ctr_desc);
    if (!item->counter_buffer) {
        metal_buffer_destroy(item->visible_buffer);
        item->visible_buffer = NULL;
        metal_buffer_destroy(item->command_buffer);
        item->command_buffer = NULL;
        free(item->batches);
        return -7;
    }
    item->counter_data = metal_buffer_get_cpu_ptr(item->counter_buffer);

    // Initialize counters to zero
    if (item->counter_data) {
        memset(item->counter_data, 0, counter_buffer_size);
    }

    item->total_commands_generated = 0;
    item->peak_batch_count = 0;
    item->initialized = true;
    item->dirty = true;

    out_handle->id = index;
    return 0;
}

void rendering_draw_command_gen_destroy(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return;
    }

    rendering_draw_command_gen_cleanup_internal(&g_draw_command_gen_ctx.items[handle.id]);
}

int rendering_draw_command_gen_update(rendering_draw_command_gen_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return -1;
    }

    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool rendering_draw_command_gen_is_valid(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return false;
    }
    return g_draw_command_gen_ctx.items[handle.id].initialized;
}

int rendering_draw_command_gen_get_info(rendering_draw_command_gen_handle_t handle, rendering_draw_command_gen_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_draw_command_gen_ctx.count) {
        return -2;
    }

    const rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->pending_commands = item->batch_count;
    out_info->max_capacity = MAX_DRAW_COMMANDS;
    out_info->memory_used = (item->command_buffer ? metal_buffer_get_size(item->command_buffer) : 0) +
                            (item->visible_buffer ? metal_buffer_get_size(item->visible_buffer) : 0) +
                            (item->counter_buffer ? metal_buffer_get_size(item->counter_buffer) : 0) +
                            (item->batch_capacity * sizeof(draw_batch_t));

    return 0;
}

void rendering_draw_command_gen_mark_dirty(rendering_draw_command_gen_handle_t handle) {
    if (handle.id < g_draw_command_gen_ctx.count) {
        g_draw_command_gen_ctx.items[handle.id].dirty = true;
    }
}

int rendering_draw_command_gen_process_pending(void) {
    if (!g_draw_command_gen_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_draw_command_gen_ctx.count; i++) {
        rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Dispatch compute shader to generate indirect draw commands
            // Compaction of visible clusters into draw streams
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t rendering_draw_command_gen_get_count(void) {
    return g_draw_command_gen_ctx.count;
}

size_t rendering_draw_command_gen_get_memory_usage(void) {
    size_t total = sizeof(rendering_draw_command_gen_context_t);
    total += g_draw_command_gen_ctx.capacity * sizeof(rendering_draw_command_gen_internal_t);
    return total;
}

void rendering_draw_command_gen_debug_print(void) {
    if (!g_draw_command_gen_ctx.initialized) return;

    printf("Draw Command Generation Status:\n");
    printf("  Count: %u / %u\n", g_draw_command_gen_ctx.count, g_draw_command_gen_ctx.capacity);

    for (uint32_t i = 0; i < g_draw_command_gen_ctx.count; i++) {
        const rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[i];
        if (item->initialized) {
            printf("  [%u] Batches: %u / %u, Commands Generated: %u, Peak: %u\n",
                   item->id, item->batch_count, item->batch_capacity,
                   item->total_commands_generated, item->peak_batch_count);
        }
    }
}

int rendering_draw_command_gen_add_batch(rendering_draw_command_gen_handle_t handle,
                                        const draw_batch_t* batch) {
    if (!batch || handle.id >= g_draw_command_gen_ctx.count) {
        return -1;
    }

    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Grow batch array if needed
    if (item->batch_count >= item->batch_capacity) {
        uint32_t new_capacity = item->batch_capacity * 2;
        if (new_capacity > MAX_BATCHES) new_capacity = MAX_BATCHES;
        if (new_capacity == item->batch_capacity) {
            return -3;  // At capacity limit
        }

        draw_batch_t* new_batches = realloc(item->batches, new_capacity * sizeof(draw_batch_t));
        if (!new_batches) {
            return -4;
        }
        item->batches = new_batches;
        item->batch_capacity = new_capacity;
    }

    // Add batch
    item->batches[item->batch_count] = *batch;
    item->dirty = true;

    if (item->batch_count >= item->peak_batch_count) {
        item->peak_batch_count = item->batch_count + 1;
    }

    return item->batch_count++;
}

void rendering_draw_command_gen_clear_batches(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return;
    }

    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    if (item->initialized) {
        item->batch_count = 0;
        item->dirty = true;
    }
}

uint32_t rendering_draw_command_gen_get_batch_count(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return 0;
    }

    const rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    return item->initialized ? item->batch_count : 0;
}

metal_buffer_t* rendering_draw_command_gen_get_command_buffer(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return NULL;
    }

    const rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    return item->initialized ? item->command_buffer : NULL;
}

metal_buffer_t* rendering_draw_command_gen_get_visible_buffer(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return NULL;
    }

    const rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    return item->initialized ? item->visible_buffer : NULL;
}

metal_buffer_t* rendering_draw_command_gen_get_counter_buffer(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return NULL;
    }

    const rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    return item->initialized ? item->counter_buffer : NULL;
}

uint32_t rendering_draw_command_gen_get_total_draw_commands(void) {
    uint32_t total = 0;
    if (g_draw_command_gen_ctx.initialized) {
        for (uint32_t i = 0; i < g_draw_command_gen_ctx.count; i++) {
            total += g_draw_command_gen_ctx.items[i].total_commands_generated;
        }
    }
    return total;
}

/* ============================================================================
 * GPU-DRIVEN RENDERING FUNCTIONS
 * ============================================================================ */

int rendering_draw_command_gen_generate_from_culling(
    rendering_draw_command_gen_handle_t handle,
    const CullingResult* culling_results,
    uint32_t result_count,
    IndirectDrawArgs* out_args,
    uint32_t* out_arg_count) {

    if (!culling_results || !out_args || !out_arg_count || handle.id >= g_draw_command_gen_ctx.count) {
        return -1;
    }

    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Implement culling result processing to generate indirect draw args
    // This would:
    // 1. Group visible instances by mesh/material
    // 2. Generate one IndirectDrawArgs per group
    // 3. Set indexCount, instanceCount, indexStart, baseVertex, baseInstance
    // 4. Track in item->indirect_args

    *out_arg_count = 0;
    return 0;
}

int rendering_draw_command_gen_populate_icb(
    rendering_draw_command_gen_handle_t handle,
    const IndirectDrawArgs* args,
    uint32_t arg_count,
    void* icb_buffer) {

    if (!args || arg_count == 0 || !icb_buffer || handle.id >= g_draw_command_gen_ctx.count) {
        return -1;
    }

    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Implement Metal ICB population
    // This would encode indirect draw commands into the ICB buffer
    // from the IndirectDrawArgs array

    return 0;
}

IndirectDrawArgs* rendering_draw_command_gen_get_indirect_buffer(
    rendering_draw_command_gen_handle_t handle,
    uint32_t* out_count) {

    if (!out_count || handle.id >= g_draw_command_gen_ctx.count) {
        return NULL;
    }

    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    if (!item->initialized) {
        return NULL;
    }

    *out_count = item->indirect_arg_count;
    return item->indirect_args;
}

uint32_t rendering_draw_command_gen_get_draw_count(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return 0;
    }

    const rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    if (!item->initialized) {
        return 0;
    }

    return item->indirect_arg_count;
}

/* End of draw_command_gen.c */
