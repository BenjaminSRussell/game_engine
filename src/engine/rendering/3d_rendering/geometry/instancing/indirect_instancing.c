/*
 * indirect_instancing.c
 * Multi-draw indirect command buffer generation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "indirect_instancing.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define INDIRECT_COMMAND_DEFAULT_CAPACITY 512
#define MAX_INDIRECT_DRAWS 4096

/* ============================================================================
 * TYPES
 * ============================================================================ */

// VK_STRUCTURE_TYPE_DRAW_INDEXED_INDIRECT_COMMAND / glDrawElementsIndirect
typedef struct draw_indexed_indirect_command {
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t vertex_offset;
    uint32_t first_instance;
} draw_indexed_indirect_command_t;

typedef struct indirect_draw_batch {
    draw_indexed_indirect_command_t* commands;
    uint32_t* draw_ids;  // Parallel array for draw IDs
    uint32_t count;
    uint32_t capacity;
    void* gpu_buffer;
    bool dirty;
} indirect_draw_batch_t;

typedef struct geometry_indirect_instancing_internal {
    uint32_t id;
    uint32_t flags;
    indirect_draw_batch_t batch;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} geometry_indirect_instancing_internal_t;

typedef struct geometry_indirect_instancing_context {
    geometry_indirect_instancing_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} geometry_indirect_instancing_context_t;

static geometry_indirect_instancing_context_t g_indirect_instancing_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static bool allocate_command_buffer(indirect_draw_batch_t* batch, uint32_t capacity) {
    batch->commands = calloc(capacity, sizeof(draw_indexed_indirect_command_t));
    batch->draw_ids = calloc(capacity, sizeof(uint32_t));
    
    if (!batch->commands || !batch->draw_ids) {
        free(batch->commands);
        free(batch->draw_ids);
        return false;
    }
    
    batch->capacity = capacity;
    batch->count = 0;
    batch->dirty = true;
    batch->gpu_buffer = NULL;  // TODO: Allocate GPU buffer
    
    return true;
}

static void free_command_buffer(indirect_draw_batch_t* batch) {
    if (batch->commands) {
        free(batch->commands);
        batch->commands = NULL;
    }
    if (batch->draw_ids) {
        free(batch->draw_ids);
        batch->draw_ids = NULL;
    }
    
    // TODO: Free GPU buffer
    batch->gpu_buffer = NULL;
    batch->capacity = 0;
    batch->count = 0;
}

static bool resize_command_buffer(indirect_draw_batch_t* batch, uint32_t new_capacity) {
    if (new_capacity <= batch->capacity) {
        return true;
    }
    
    draw_indexed_indirect_command_t* new_commands = 
        realloc(batch->commands, new_capacity * sizeof(draw_indexed_indirect_command_t));
    uint32_t* new_ids = realloc(batch->draw_ids, new_capacity * sizeof(uint32_t));
    
    if (!new_commands || !new_ids) {
        return false;
    }
    
    batch->commands = new_commands;
    batch->draw_ids = new_ids;
    batch->capacity = new_capacity;
    batch->dirty = true;
    
    return true;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool geometry_indirect_instancing_validate(const geometry_indirect_instancing_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void geometry_indirect_instancing_cleanup_internal(geometry_indirect_instancing_internal_t* item) {
    if (!item) return;
    
    free_command_buffer(&item->batch);
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_indirect_instancing_init(void) {
    if (g_indirect_instancing_ctx.initialized) {
        return 0;
    }
    
    g_indirect_instancing_ctx.capacity = INDIRECT_COMMAND_DEFAULT_CAPACITY;
    g_indirect_instancing_ctx.items = calloc(g_indirect_instancing_ctx.capacity,
                                             sizeof(geometry_indirect_instancing_internal_t));
    if (!g_indirect_instancing_ctx.items) {
        return -1;
    }
    
    g_indirect_instancing_ctx.count = 0;
    g_indirect_instancing_ctx.initialized = true;
    
    return 0;
}

void geometry_indirect_instancing_shutdown(void) {
    if (!g_indirect_instancing_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_indirect_instancing_ctx.count; i++) {
        geometry_indirect_instancing_cleanup_internal(&g_indirect_instancing_ctx.items[i]);
    }
    
    free(g_indirect_instancing_ctx.items);
    g_indirect_instancing_ctx.items = NULL;
    g_indirect_instancing_ctx.count = 0;
    g_indirect_instancing_ctx.capacity = 0;
    g_indirect_instancing_ctx.initialized = false;
}

int geometry_indirect_instancing_create(geometry_indirect_instancing_handle_t* out_handle,
                                         const geometry_indirect_instancing_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_indirect_instancing_ctx.initialized) {
        return -2;
    }
    
    if (g_indirect_instancing_ctx.count >= g_indirect_instancing_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_indirect_instancing_ctx.count++;
    geometry_indirect_instancing_internal_t* item = &g_indirect_instancing_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    uint32_t initial_capacity = desc->initial_capacity > 0 ? desc->initial_capacity : 128;
    if (!allocate_command_buffer(&item->batch, initial_capacity)) {
        return -4;
    }
    
    out_handle->id = index;
    return 0;
}

void geometry_indirect_instancing_destroy(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= g_indirect_instancing_ctx.count) {
        return;
    }
    
    geometry_indirect_instancing_cleanup_internal(&g_indirect_instancing_ctx.items[handle.id]);
}

int geometry_indirect_instancing_add_draw(geometry_indirect_instancing_handle_t handle,
                                           uint32_t index_count,
                                           uint32_t instance_count,
                                           uint32_t first_index,
                                           int32_t vertex_offset,
                                           uint32_t first_instance,
                                           uint32_t draw_id) {
    if (handle.id >= g_indirect_instancing_ctx.count) {
        return -1;
    }
    
    geometry_indirect_instancing_internal_t* item = &g_indirect_instancing_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    indirect_draw_batch_t* batch = &item->batch;
    
    // Resize if needed
    if (batch->count >= batch->capacity) {
        if (!resize_command_buffer(batch, batch->capacity * 2)) {
            return -3;
        }
    }
    
    // Add command
    draw_indexed_indirect_command_t* cmd = &batch->commands[batch->count];
    cmd->index_count = index_count;
    cmd->instance_count = instance_count;
    cmd->first_index = first_index;
    cmd->vertex_offset = vertex_offset;
    cmd->first_instance = first_instance;
    
    batch->draw_ids[batch->count] = draw_id;
    batch->count++;
    batch->dirty = true;
    item->dirty = true;
    
    return 0;
}

int geometry_indirect_instancing_clear_draws(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= g_indirect_instancing_ctx.count) {
        return -1;
    }
    
    geometry_indirect_instancing_internal_t* item = &g_indirect_instancing_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    item->batch.count = 0;
    item->batch.dirty = true;
    item->dirty = true;
    
    return 0;
}

int geometry_indirect_instancing_update(geometry_indirect_instancing_handle_t handle,
                                         const void* data, size_t size) {
    if (handle.id >= g_indirect_instancing_ctx.count) {
        return -1;
    }
    
    geometry_indirect_instancing_internal_t* item = &g_indirect_instancing_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    item->dirty = true;
    return 0;
}

bool geometry_indirect_instancing_is_valid(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= g_indirect_instancing_ctx.count) {
        return false;
    }
    return g_indirect_instancing_ctx.items[handle.id].initialized;
}

int geometry_indirect_instancing_get_info(geometry_indirect_instancing_handle_t handle,
                                           geometry_indirect_instancing_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (handle.id >= g_indirect_instancing_ctx.count) {
        return -2;
    }
    
    const geometry_indirect_instancing_internal_t* item = &g_indirect_instancing_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->draw_count = item->batch.count;
    
    return 0;
}

void geometry_indirect_instancing_mark_dirty(geometry_indirect_instancing_handle_t handle) {
    if (handle.id < g_indirect_instancing_ctx.count) {
        g_indirect_instancing_ctx.items[handle.id].dirty = true;
        g_indirect_instancing_ctx.items[handle.id].batch.dirty = true;
    }
}

int geometry_indirect_instancing_process_pending(void) {
    int processed = 0;
    
    for (uint32_t i = 0; i < g_indirect_instancing_ctx.count; i++) {
        geometry_indirect_instancing_internal_t* item = &g_indirect_instancing_ctx.items[i];
        if (item->initialized && item->batch.dirty) {
            // TODO: Upload commands to GPU
            item->batch.dirty = false;
            item->dirty = false;
            processed++;
        }
    }
    
    return processed;
}

uint32_t geometry_indirect_instancing_get_count(void) {
    return g_indirect_instancing_ctx.count;
}

size_t geometry_indirect_instancing_get_memory_usage(void) {
    size_t total = sizeof(g_indirect_instancing_ctx);
    total += g_indirect_instancing_ctx.capacity * sizeof(geometry_indirect_instancing_internal_t);
    
    for (uint32_t i = 0; i < g_indirect_instancing_ctx.count; i++) {
        const geometry_indirect_instancing_internal_t* item = &g_indirect_instancing_ctx.items[i];
        total += item->batch.capacity * (sizeof(draw_indexed_indirect_command_t) + sizeof(uint32_t));
    }
    
    return total;
}

void geometry_indirect_instancing_debug_print(void) {
    printf("[Indirect Instancing] Total batches: %u\n", g_indirect_instancing_ctx.count);
    
    for (uint32_t i = 0; i < g_indirect_instancing_ctx.count; i++) {
        const geometry_indirect_instancing_internal_t* item = &g_indirect_instancing_ctx.items[i];
        if (item->initialized) {
            printf("  Batch %u: %u draws\n", i, item->batch.count);
        }
    }
}

/* End of indirect_instancing.c */
