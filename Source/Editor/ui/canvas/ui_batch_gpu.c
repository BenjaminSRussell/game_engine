/*
 * ui_batch_gpu.c
 * GPU buffer management and rendering integration
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "editor/ui/canvas/ui_batch_gpu.h"
#include "editor/ui/canvas/ui_batch.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define UI_BATCH_GPU_MAX_BUFFERS 2048
#define UI_BATCH_GPU_DEFAULT_POOL_SIZE (256 * 1024 * 1024) /* 256MB */

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_batch_gpu_buffer {
    uint32_t id;
    uint32_t buffer_type;
    uint32_t usage_flags;

    void* gpu_memory;
    size_t size;
    size_t allocated_size;

    bool uploaded;
    bool valid;
} ui_batch_gpu_buffer_t;

typedef struct ui_batch_gpu_context {
    ui_batch_gpu_buffer_t* buffers;
    uint32_t buffer_count;
    uint32_t buffer_capacity;

    size_t total_allocated;
    size_t total_available;

    bool initialized;
} ui_batch_gpu_context_t;

static ui_batch_gpu_context_t g_gpu_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void* ui_batch_gpu_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

static void ui_batch_gpu_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

static int ui_batch_gpu_validate_buffer(ui_batch_gpu_buffer_t* buffer) {
    if (!buffer) return 0;
    if (!buffer->valid) return 0;
    if (!buffer->gpu_memory) return 0;
    return 1;
}

/* ============================================================================
 * PUBLIC API - INITIALIZATION
 * ============================================================================ */

int ui_batch_gpu_init(void) {
    if (g_gpu_ctx.initialized) {
        return 0;
    }

    g_gpu_ctx.buffer_capacity = UI_BATCH_GPU_MAX_BUFFERS;
    g_gpu_ctx.buffers = ui_batch_gpu_malloc(g_gpu_ctx.buffer_capacity * sizeof(ui_batch_gpu_buffer_t));
    if (!g_gpu_ctx.buffers) {
        return -1;
    }

    g_gpu_ctx.buffer_count = 0;
    g_gpu_ctx.total_allocated = 0;
    g_gpu_ctx.total_available = UI_BATCH_GPU_DEFAULT_POOL_SIZE;
    g_gpu_ctx.initialized = true;

    return 0;
}

void ui_batch_gpu_shutdown(void) {
    if (!g_gpu_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gpu_ctx.buffer_count; i++) {
        ui_batch_gpu_buffer_t* buf = &g_gpu_ctx.buffers[i];
        if (buf->valid && buf->gpu_memory) {
            ui_batch_gpu_free(buf->gpu_memory);
        }
    }

    ui_batch_gpu_free(g_gpu_ctx.buffers);
    g_gpu_ctx.buffers = NULL;
    g_gpu_ctx.buffer_count = 0;
    g_gpu_ctx.buffer_capacity = 0;
    g_gpu_ctx.initialized = false;
}

/* ============================================================================
 * PUBLIC API - BUFFER MANAGEMENT
 * ============================================================================ */

int ui_batch_gpu_create_buffer(ui_batch_gpu_buffer_handle_t* out_handle,
                                const ui_batch_gpu_buffer_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gpu_ctx.initialized) {
        return -2;
    }

    if (g_gpu_ctx.buffer_count >= g_gpu_ctx.buffer_capacity) {
        return -3;
    }

    if (desc->size > g_gpu_ctx.total_available) {
        return -4;
    }

    uint32_t index = g_gpu_ctx.buffer_count++;
    ui_batch_gpu_buffer_t* buf = &g_gpu_ctx.buffers[index];

    buf->gpu_memory = ui_batch_gpu_malloc(desc->size);
    if (!buf->gpu_memory) {
        g_gpu_ctx.buffer_count--;
        return -5;
    }

    if (desc->initial_data) {
        memcpy(buf->gpu_memory, desc->initial_data, desc->size);
    }

    buf->id = index;
    buf->buffer_type = desc->buffer_type;
    buf->usage_flags = desc->usage_flags;
    buf->size = desc->size;
    buf->allocated_size = desc->size;
    buf->uploaded = true;
    buf->valid = true;

    g_gpu_ctx.total_allocated += desc->size;
    g_gpu_ctx.total_available -= desc->size;

    out_handle->id = index;
    return 0;
}

void ui_batch_gpu_destroy_buffer(ui_batch_gpu_buffer_handle_t handle) {
    if (handle.id >= g_gpu_ctx.buffer_count) {
        return;
    }

    ui_batch_gpu_buffer_t* buf = &g_gpu_ctx.buffers[handle.id];
    if (!buf->valid) {
        return;
    }

    if (buf->gpu_memory) {
        g_gpu_ctx.total_allocated -= buf->allocated_size;
        g_gpu_ctx.total_available += buf->allocated_size;
        ui_batch_gpu_free(buf->gpu_memory);
        buf->gpu_memory = NULL;
    }

    buf->valid = false;
}

int ui_batch_gpu_upload_buffer(ui_batch_gpu_buffer_handle_t handle,
                                const void* data, size_t size, size_t offset) {
    if (handle.id >= g_gpu_ctx.buffer_count) {
        return -1;
    }

    ui_batch_gpu_buffer_t* buf = &g_gpu_ctx.buffers[handle.id];
    if (!ui_batch_gpu_validate_buffer(buf)) {
        return -2;
    }

    if (!data || size == 0) {
        return -3;
    }

    if (offset + size > buf->allocated_size) {
        return -4;
    }

    memcpy((uint8_t*)buf->gpu_memory + offset, data, size);
    buf->uploaded = true;

    return 0;
}

int ui_batch_gpu_update_buffer(ui_batch_gpu_buffer_handle_t handle,
                                const void* data, size_t size, size_t offset) {
    /* For now, update is same as upload */
    return ui_batch_gpu_upload_buffer(handle, data, size, offset);
}

/* ============================================================================
 * PUBLIC API - BATCH GPU OPERATIONS
 * ============================================================================ */

int ui_batch_gpu_upload_batch(ui_rendering_ui_batch_handle_t batch_handle) {
    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -1;
    }

    ui_rendering_ui_batch_mark_gpu_uploaded(batch_handle);

    return 0;
}

int ui_batch_gpu_bind_batch(ui_rendering_ui_batch_handle_t batch_handle) {
    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -1;
    }

    /* Binding logic would go here - implementation depends on graphics API */

    return 0;
}

int ui_batch_gpu_draw_batch(ui_rendering_ui_batch_handle_t batch_handle,
                             const ui_batch_gpu_render_context_t* ctx) {
    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -1;
    }

    if (!ctx) {
        return -2;
    }

    ui_rendering_draw_command_t* commands = NULL;
    uint32_t command_count = 0;

    if (ui_rendering_ui_batch_get_draw_commands(batch_handle, &commands, &command_count) != 0) {
        return -3;
    }

    if (command_count == 0) {
        return 0; /* Nothing to draw */
    }

    /* Draw command execution would go here */

    return command_count;
}

/* ============================================================================
 * PUBLIC API - RESOURCE POOLING
 * ============================================================================ */

size_t ui_batch_gpu_get_available_memory(void) {
    if (!g_gpu_ctx.initialized) {
        return 0;
    }
    return g_gpu_ctx.total_available;
}

int ui_batch_gpu_allocate_memory(size_t size, void** out_gpu_memory) {
    if (!out_gpu_memory || size == 0) {
        return -1;
    }

    if (!g_gpu_ctx.initialized) {
        return -2;
    }

    if (size > g_gpu_ctx.total_available) {
        return -3;
    }

    void* memory = ui_batch_gpu_malloc(size);
    if (!memory) {
        return -4;
    }

    *out_gpu_memory = memory;
    g_gpu_ctx.total_allocated += size;
    g_gpu_ctx.total_available -= size;

    return 0;
}

void ui_batch_gpu_free_memory(void* gpu_memory, size_t size) {
    if (gpu_memory && size > 0) {
        ui_batch_gpu_free(gpu_memory);
        g_gpu_ctx.total_allocated -= size;
        g_gpu_ctx.total_available += size;
    }
}

/* ============================================================================
 * PUBLIC API - STATISTICS
 * ============================================================================ */

size_t ui_batch_gpu_get_memory_usage(void) {
    if (!g_gpu_ctx.initialized) {
        return 0;
    }
    return g_gpu_ctx.total_allocated;
}

uint32_t ui_batch_gpu_get_buffer_count(void) {
    if (!g_gpu_ctx.initialized) {
        return 0;
    }
    return g_gpu_ctx.buffer_count;
}

/* End of ui_batch_gpu.c */
