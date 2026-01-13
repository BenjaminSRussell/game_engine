/*
 * ui_batch.c
 * UI draw batching and geometry rendering
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements batch collection, sorting, and optimization of UI geometry.
 * Supports vertex/index buffering, material merging, and GPU integration.
 */

#include "editor/ui/canvas/ui_batch.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define UI_RENDERING_UI_BATCH_MAX_COUNT 4096
#define UI_RENDERING_UI_BATCH_DEFAULT_CAPACITY 256
#define UI_RENDERING_UI_BATCH_DEFAULT_VERTICES 4096
#define UI_RENDERING_UI_BATCH_DEFAULT_INDICES 16384
#define UI_RENDERING_UI_BATCH_ALIGNMENT 16
#define UI_RENDERING_DRAW_COMMAND_CAPACITY 256

/* Batch flags */
#define UI_BATCH_FLAG_GPU_DIRTY 0x01
#define UI_BATCH_FLAG_GEOMETRY_DIRTY 0x02
#define UI_BATCH_FLAG_SORTED 0x04

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_rendering_batch_geometry {
    ui_rendering_vertex_t* vertices;
    uint32_t vertex_count;
    uint32_t vertex_capacity;

    uint32_t* indices;
    uint32_t index_count;
    uint32_t index_capacity;

    ui_rendering_draw_command_t* draw_commands;
    uint32_t draw_command_count;
    uint32_t draw_command_capacity;
} ui_rendering_batch_geometry_t;

typedef struct ui_rendering_ui_batch_internal {
    uint32_t id;
    uint32_t flags;

    ui_rendering_batch_geometry_t geometry;

    bool initialized;
    bool dirty;
    bool gpu_uploaded;
    uint64_t frame_updated;
    uint64_t frame_last_sorted;

    uint32_t merged_draw_calls;
    size_t estimated_memory;
} ui_rendering_ui_batch_internal_t;

typedef struct ui_rendering_ui_batch_context {
    ui_rendering_ui_batch_internal_t* items;
    uint32_t count;
    uint32_t capacity;

    ui_rendering_batch_stats_t stats;

    bool initialized;
} ui_rendering_ui_batch_context_t;

static ui_rendering_ui_batch_context_t g_ui_batch_ctx = {0};
static uint64_t g_frame_counter = 0;

/* ============================================================================
 * PRIVATE FUNCTIONS - MEMORY MANAGEMENT
 * ============================================================================ */

static void* ui_batch_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

static void ui_batch_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

static int ui_batch_geometry_init(ui_rendering_batch_geometry_t* geom, uint32_t max_verts, uint32_t max_indices) {
    if (!geom) return -1;

    geom->vertex_capacity = max_verts > 0 ? max_verts : UI_RENDERING_UI_BATCH_DEFAULT_VERTICES;
    geom->index_capacity = max_indices > 0 ? max_indices : UI_RENDERING_UI_BATCH_DEFAULT_INDICES;
    geom->draw_command_capacity = UI_RENDERING_DRAW_COMMAND_CAPACITY;

    geom->vertices = ui_batch_malloc(geom->vertex_capacity * sizeof(ui_rendering_vertex_t));
    geom->indices = ui_batch_malloc(geom->index_capacity * sizeof(uint32_t));
    geom->draw_commands = ui_batch_malloc(geom->draw_command_capacity * sizeof(ui_rendering_draw_command_t));

    if (!geom->vertices || !geom->indices || !geom->draw_commands) {
        ui_batch_free(geom->vertices);
        ui_batch_free(geom->indices);
        ui_batch_free(geom->draw_commands);
        return -1;
    }

    geom->vertex_count = 0;
    geom->index_count = 0;
    geom->draw_command_count = 0;

    return 0;
}

static void ui_batch_geometry_shutdown(ui_rendering_batch_geometry_t* geom) {
    if (!geom) return;

    ui_batch_free(geom->vertices);
    ui_batch_free(geom->indices);
    ui_batch_free(geom->draw_commands);

    memset(geom, 0, sizeof(*geom));
}

static int ui_batch_geometry_expand_vertices(ui_rendering_batch_geometry_t* geom, uint32_t needed) {
    if (geom->vertex_count + needed <= geom->vertex_capacity) {
        return 0;
    }

    uint32_t new_capacity = geom->vertex_capacity;
    while (new_capacity < geom->vertex_count + needed) {
        new_capacity *= 2;
    }

    ui_rendering_vertex_t* new_vertices = ui_batch_malloc(new_capacity * sizeof(ui_rendering_vertex_t));
    if (!new_vertices) return -1;

    if (geom->vertex_count > 0) {
        memcpy(new_vertices, geom->vertices, geom->vertex_count * sizeof(ui_rendering_vertex_t));
    }

    ui_batch_free(geom->vertices);
    geom->vertices = new_vertices;
    geom->vertex_capacity = new_capacity;

    return 0;
}

static int ui_batch_geometry_expand_indices(ui_rendering_batch_geometry_t* geom, uint32_t needed) {
    if (geom->index_count + needed <= geom->index_capacity) {
        return 0;
    }

    uint32_t new_capacity = geom->index_capacity;
    while (new_capacity < geom->index_count + needed) {
        new_capacity *= 2;
    }

    uint32_t* new_indices = ui_batch_malloc(new_capacity * sizeof(uint32_t));
    if (!new_indices) return -1;

    if (geom->index_count > 0) {
        memcpy(new_indices, geom->indices, geom->index_count * sizeof(uint32_t));
    }

    ui_batch_free(geom->indices);
    geom->indices = new_indices;
    geom->index_capacity = new_capacity;

    return 0;
}

/* ============================================================================
 * PRIVATE FUNCTIONS - BATCHING & SORTING
 * ============================================================================ */

static int ui_batch_compare_draw_commands(const void* a, const void* b) {
    const ui_rendering_draw_command_t* cmd_a = (const ui_rendering_draw_command_t*)a;
    const ui_rendering_draw_command_t* cmd_b = (const ui_rendering_draw_command_t*)b;

    /* Sort by z-order first (back to front for transparency) */
    if (fabsf(cmd_a->z_order - cmd_b->z_order) > 0.001f) {
        return (cmd_a->z_order > cmd_b->z_order) ? -1 : 1;
    }

    /* Then by material to enable batching */
    if (cmd_a->material_id != cmd_b->material_id) {
        return (int)cmd_a->material_id - (int)cmd_b->material_id;
    }

    /* Then by texture */
    if (cmd_a->texture_id != cmd_b->texture_id) {
        return (int)cmd_a->texture_id - (int)cmd_b->texture_id;
    }

    /* Then by blend mode */
    return (int)cmd_a->blend_mode - (int)cmd_b->blend_mode;
}

static int ui_batch_can_merge_commands(const ui_rendering_draw_command_t* a, const ui_rendering_draw_command_t* b) {
    /* Commands can be merged if they have same material, texture, and blend mode */
    if (a->material_id != b->material_id) return 0;
    if (a->texture_id != b->texture_id) return 0;
    if (a->blend_mode != b->blend_mode) return 0;

    /* Check if indices are sequential or can be merged */
    if (a->index_offset + a->index_count == b->index_offset) {
        return 1; /* Contiguous, can merge */
    }

    return 0;
}

static int ui_batch_geometry_optimize_draw_calls(ui_rendering_batch_geometry_t* geom) {
    if (geom->draw_command_count <= 1) {
        return 0;
    }

    /* Sort commands for optimal batching */
    qsort(geom->draw_commands, geom->draw_command_count,
          sizeof(ui_rendering_draw_command_t), ui_batch_compare_draw_commands);

    /* Merge compatible consecutive commands */
    uint32_t write_idx = 0;
    for (uint32_t i = 1; i < geom->draw_command_count; i++) {
        if (ui_batch_can_merge_commands(&geom->draw_commands[write_idx], &geom->draw_commands[i])) {
            /* Merge by extending vertex/index count */
            geom->draw_commands[write_idx].index_count += geom->draw_commands[i].index_count;
            geom->draw_commands[write_idx].vertex_count += geom->draw_commands[i].vertex_count;
        } else {
            /* Keep as separate command */
            write_idx++;
            if (write_idx != i) {
                geom->draw_commands[write_idx] = geom->draw_commands[i];
            }
        }
    }

    int merged = (int)geom->draw_command_count - (int)(write_idx + 1);
    geom->draw_command_count = write_idx + 1;

    return merged;
}

static bool ui_rendering_ui_batch_validate(const ui_rendering_ui_batch_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->geometry.vertices || !item->geometry.indices) return false;
    return true;
}

static void ui_rendering_ui_batch_cleanup_internal(ui_rendering_ui_batch_internal_t* item) {
    if (!item) return;

    ui_batch_geometry_shutdown(&item->geometry);
    item->initialized = false;
}

static size_t ui_batch_calculate_memory_usage(const ui_rendering_ui_batch_internal_t* item) {
    size_t total = sizeof(*item);
    total += item->geometry.vertex_capacity * sizeof(ui_rendering_vertex_t);
    total += item->geometry.index_capacity * sizeof(uint32_t);
    total += item->geometry.draw_command_capacity * sizeof(ui_rendering_draw_command_t);
    return total;
}

/* ============================================================================
 * PUBLIC API - INITIALIZATION
 * ============================================================================ */

int ui_rendering_ui_batch_init(void) {
    if (g_ui_batch_ctx.initialized) {
        return 0;
    }

    g_ui_batch_ctx.capacity = UI_RENDERING_UI_BATCH_DEFAULT_CAPACITY;
    g_ui_batch_ctx.items = ui_batch_malloc(g_ui_batch_ctx.capacity * sizeof(ui_rendering_ui_batch_internal_t));
    if (!g_ui_batch_ctx.items) {
        return -1;
    }

    g_ui_batch_ctx.count = 0;
    g_ui_batch_ctx.initialized = true;
    memset(&g_ui_batch_ctx.stats, 0, sizeof(g_ui_batch_ctx.stats));
    g_frame_counter = 0;

    return 0;
}

void ui_rendering_ui_batch_shutdown(void) {
    if (!g_ui_batch_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_ui_batch_ctx.count; i++) {
        ui_rendering_ui_batch_cleanup_internal(&g_ui_batch_ctx.items[i]);
    }

    ui_batch_free(g_ui_batch_ctx.items);
    g_ui_batch_ctx.items = NULL;
    g_ui_batch_ctx.count = 0;
    g_ui_batch_ctx.capacity = 0;
    g_ui_batch_ctx.initialized = false;
}

/* ============================================================================
 * PUBLIC API - LIFECYCLE
 * ============================================================================ */

int ui_rendering_ui_batch_create(ui_rendering_ui_batch_handle_t* out_handle,
                                  const ui_rendering_ui_batch_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ui_batch_ctx.initialized) {
        return -2;
    }

    if (g_ui_batch_ctx.count >= g_ui_batch_ctx.capacity) {
        return -3; /* At capacity */
    }

    uint32_t index = g_ui_batch_ctx.count++;
    ui_rendering_ui_batch_internal_t* item = &g_ui_batch_ctx.items[index];

    if (ui_batch_geometry_init(&item->geometry, desc->max_vertices, desc->max_indices) != 0) {
        g_ui_batch_ctx.count--;
        return -4;
    }

    item->id = index;
    item->flags = desc->flags | UI_BATCH_FLAG_GPU_DIRTY | UI_BATCH_FLAG_GEOMETRY_DIRTY;
    item->initialized = true;
    item->dirty = true;
    item->gpu_uploaded = false;
    item->frame_updated = g_frame_counter;
    item->frame_last_sorted = 0;
    item->merged_draw_calls = 0;
    item->estimated_memory = ui_batch_calculate_memory_usage(item);

    out_handle->id = index;
    return 0;
}

void ui_rendering_ui_batch_destroy(ui_rendering_ui_batch_handle_t handle) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return;
    }

    ui_rendering_ui_batch_cleanup_internal(&g_ui_batch_ctx.items[handle.id]);
}

/* ============================================================================
 * PUBLIC API - GEOMETRY OPERATIONS
 * ============================================================================ */

int ui_rendering_ui_batch_add_geometry(ui_rendering_ui_batch_handle_t handle,
                                        const ui_rendering_vertex_t* vertices,
                                        uint32_t vertex_count,
                                        const uint32_t* indices,
                                        uint32_t index_count) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return -1;
    }

    ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -2;
    }

    if (!vertices || vertex_count == 0 || !indices || index_count == 0) {
        return -3;
    }

    /* Expand buffers if needed */
    if (ui_batch_geometry_expand_vertices(&batch->geometry, vertex_count) != 0) {
        return -4;
    }
    if (ui_batch_geometry_expand_indices(&batch->geometry, index_count) != 0) {
        return -5;
    }

    /* Copy geometry data */
    ui_rendering_vertex_t* verts = batch->geometry.vertices + batch->geometry.vertex_count;
    memcpy(verts, vertices, vertex_count * sizeof(ui_rendering_vertex_t));

    uint32_t vertex_offset = batch->geometry.vertex_count;
    batch->geometry.vertex_count += vertex_count;

    /* Update index offsets and copy */
    uint32_t* idx = batch->geometry.indices + batch->geometry.index_count;
    for (uint32_t i = 0; i < index_count; i++) {
        idx[i] = indices[i] + vertex_offset;
    }
    batch->geometry.index_count += index_count;

    batch->dirty = true;
    batch->flags |= UI_BATCH_FLAG_GEOMETRY_DIRTY;
    batch->flags |= UI_BATCH_FLAG_GPU_DIRTY;

    return 0;
}

int ui_rendering_ui_batch_add_draw_command(ui_rendering_ui_batch_handle_t handle,
                                            const ui_rendering_draw_command_t* cmd) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return -1;
    }

    ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -2;
    }

    if (!cmd) {
        return -3;
    }

    /* Expand command buffer if needed */
    if (batch->geometry.draw_command_count >= batch->geometry.draw_command_capacity) {
        uint32_t new_capacity = batch->geometry.draw_command_capacity * 2;
        ui_rendering_draw_command_t* new_commands = ui_batch_malloc(new_capacity * sizeof(ui_rendering_draw_command_t));
        if (!new_commands) {
            return -4;
        }

        if (batch->geometry.draw_command_count > 0) {
            memcpy(new_commands, batch->geometry.draw_commands,
                   batch->geometry.draw_command_count * sizeof(ui_rendering_draw_command_t));
        }

        ui_batch_free(batch->geometry.draw_commands);
        batch->geometry.draw_commands = new_commands;
        batch->geometry.draw_command_capacity = new_capacity;
    }

    memcpy(&batch->geometry.draw_commands[batch->geometry.draw_command_count], cmd,
           sizeof(ui_rendering_draw_command_t));
    batch->geometry.draw_command_count++;

    batch->dirty = true;
    batch->flags |= UI_BATCH_FLAG_GEOMETRY_DIRTY;

    return 0;
}

int ui_rendering_ui_batch_clear(ui_rendering_ui_batch_handle_t handle) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return -1;
    }

    ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -2;
    }

    batch->geometry.vertex_count = 0;
    batch->geometry.index_count = 0;
    batch->geometry.draw_command_count = 0;
    batch->dirty = true;
    batch->gpu_uploaded = false;
    batch->flags |= UI_BATCH_FLAG_GPU_DIRTY;

    return 0;
}

/* ============================================================================
 * PUBLIC API - BATCH OPERATIONS
 * ============================================================================ */

int ui_rendering_ui_batch_update(ui_rendering_ui_batch_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return -1;
    }

    ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -2;
    }

    batch->dirty = true;
    batch->flags |= UI_BATCH_FLAG_GPU_DIRTY;
    batch->frame_updated = g_frame_counter;

    return 0;
}

bool ui_rendering_ui_batch_is_valid(ui_rendering_ui_batch_handle_t handle) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return false;
    }
    return g_ui_batch_ctx.items[handle.id].initialized;
}

int ui_rendering_ui_batch_get_info(ui_rendering_ui_batch_handle_t handle,
                                    ui_rendering_ui_batch_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_ui_batch_ctx.count) {
        return -2;
    }

    const ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    out_info->id = batch->id;
    out_info->flags = batch->flags;
    out_info->initialized = batch->initialized;
    out_info->vertex_count = batch->geometry.vertex_count;
    out_info->index_count = batch->geometry.index_count;
    out_info->memory_usage = ui_batch_calculate_memory_usage(batch);

    return 0;
}

void ui_rendering_ui_batch_mark_dirty(ui_rendering_ui_batch_handle_t handle) {
    if (handle.id < g_ui_batch_ctx.count) {
        g_ui_batch_ctx.items[handle.id].dirty = true;
        g_ui_batch_ctx.items[handle.id].flags |= UI_BATCH_FLAG_GPU_DIRTY;
    }
}

/* ============================================================================
 * PUBLIC API - BATCH PROCESSING & RENDERING
 * ============================================================================ */

int ui_rendering_ui_batch_process_pending(void) {
    int processed = 0;

    for (uint32_t i = 0; i < g_ui_batch_ctx.count; i++) {
        ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[i];
        if (!batch->initialized || !batch->dirty) {
            continue;
        }

        /* Mark as processed */
        batch->dirty = false;
        batch->frame_updated = g_frame_counter;
        processed++;
    }

    g_frame_counter++;
    return processed;
}

int ui_rendering_ui_batch_get_draw_commands(ui_rendering_ui_batch_handle_t handle,
                                             ui_rendering_draw_command_t** out_commands,
                                             uint32_t* out_count) {
    if (!out_commands || !out_count) {
        return -1;
    }

    if (handle.id >= g_ui_batch_ctx.count) {
        return -2;
    }

    ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -3;
    }

    *out_commands = batch->geometry.draw_commands;
    *out_count = batch->geometry.draw_command_count;

    return 0;
}

int ui_rendering_ui_batch_optimize_draw_calls(ui_rendering_ui_batch_handle_t handle) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return -1;
    }

    ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -2;
    }

    int merged = ui_batch_geometry_optimize_draw_calls(&batch->geometry);
    batch->merged_draw_calls = merged;

    return merged;
}

int ui_rendering_ui_batch_sort(ui_rendering_ui_batch_handle_t handle) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return -1;
    }

    ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -2;
    }

    if (batch->geometry.draw_command_count <= 1) {
        return 0;
    }

    qsort(batch->geometry.draw_commands, batch->geometry.draw_command_count,
          sizeof(ui_rendering_draw_command_t), ui_batch_compare_draw_commands);

    batch->frame_last_sorted = g_frame_counter;
    batch->flags |= UI_BATCH_FLAG_SORTED;

    return 0;
}

/* ============================================================================
 * PUBLIC API - GPU UPLOAD
 * ============================================================================ */

int ui_rendering_ui_batch_get_vertex_buffer(ui_rendering_ui_batch_handle_t handle,
                                             const ui_rendering_vertex_t** out_vertices,
                                             uint32_t* out_count) {
    if (!out_vertices || !out_count) {
        return -1;
    }

    if (handle.id >= g_ui_batch_ctx.count) {
        return -2;
    }

    const ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -3;
    }

    *out_vertices = batch->geometry.vertices;
    *out_count = batch->geometry.vertex_count;

    return 0;
}

int ui_rendering_ui_batch_get_index_buffer(ui_rendering_ui_batch_handle_t handle,
                                            const uint32_t** out_indices,
                                            uint32_t* out_count) {
    if (!out_indices || !out_count) {
        return -1;
    }

    if (handle.id >= g_ui_batch_ctx.count) {
        return -2;
    }

    const ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -3;
    }

    *out_indices = batch->geometry.indices;
    *out_count = batch->geometry.index_count;

    return 0;
}

int ui_rendering_ui_batch_mark_gpu_uploaded(ui_rendering_ui_batch_handle_t handle) {
    if (handle.id >= g_ui_batch_ctx.count) {
        return -1;
    }

    ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[handle.id];
    if (!ui_rendering_ui_batch_validate(batch)) {
        return -2;
    }

    batch->gpu_uploaded = true;
    batch->flags &= ~UI_BATCH_FLAG_GPU_DIRTY;

    return 0;
}

/* ============================================================================
 * PUBLIC API - STATISTICS & DEBUG
 * ============================================================================ */

uint32_t ui_rendering_ui_batch_get_count(void) {
    return g_ui_batch_ctx.count;
}

size_t ui_rendering_ui_batch_get_memory_usage(void) {
    size_t total = sizeof(g_ui_batch_ctx);
    total += g_ui_batch_ctx.capacity * sizeof(ui_rendering_ui_batch_internal_t);

    for (uint32_t i = 0; i < g_ui_batch_ctx.count; i++) {
        total += g_ui_batch_ctx.items[i].estimated_memory;
    }

    return total;
}

int ui_rendering_ui_batch_get_stats(ui_rendering_batch_stats_t* out_stats) {
    if (!out_stats) {
        return -1;
    }

    memset(out_stats, 0, sizeof(*out_stats));
    out_stats->total_batches = g_ui_batch_ctx.count;

    for (uint32_t i = 0; i < g_ui_batch_ctx.count; i++) {
        const ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[i];
        out_stats->total_vertices += batch->geometry.vertex_count;
        out_stats->total_indices += batch->geometry.index_count;
        out_stats->total_draw_calls += batch->geometry.draw_command_count;
        out_stats->merged_draw_calls += batch->merged_draw_calls;
    }

    out_stats->total_memory = ui_rendering_ui_batch_get_memory_usage();
    out_stats->frames_since_update = g_frame_counter;

    return 0;
}

void ui_rendering_ui_batch_debug_print(void) {
    if (!g_ui_batch_ctx.initialized) {
        return;
    }

    ui_rendering_batch_stats_t stats;
    ui_rendering_ui_batch_get_stats(&stats);

    /* Debug output */
    for (uint32_t i = 0; i < g_ui_batch_ctx.count; i++) {
        const ui_rendering_ui_batch_internal_t* batch = &g_ui_batch_ctx.items[i];
        if (!batch->initialized) continue;
    }
}

/* End of ui_batch.c */
