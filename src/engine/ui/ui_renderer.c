#include "ui_renderer.h"
#include "core/logger.h"
#include "core/memory/unified_memory_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Internal structures
typedef struct {
    const LayoutNode* element;
    uint32_t vertex_offset;
    uint32_t vertex_count;
    uint32_t index_offset;
    uint32_t index_count;
    uint32_t texture_id;
    float z_depth;
} RenderCommand;

typedef struct {
    RenderCommand* commands;
    uint32_t cmd_count;
    uint32_t cmd_capacity;

    UIVertex* vertex_buffer;
    uint32_t vert_count;
    uint32_t vert_capacity;

    uint32_t* index_buffer;
    uint32_t idx_count;
    uint32_t idx_capacity;
} CommandBuffer;

typedef struct {
    // Mock GPU buffer handles
    uint32_t vbo;
    uint32_t ibo;
    uint32_t vao;
} GPUBuffers;

// Internal helper to resize buffers
static bool ensure_capacity(void** ptr, uint32_t* capacity, uint32_t required, size_t element_size) {
    if (*capacity < required) {
        uint32_t new_capacity = *capacity == 0 ? 128 : *capacity * 2;
        while (new_capacity < required) new_capacity *= 2;

        void* new_ptr = REALLOC(*ptr, new_capacity * element_size);
        if (!new_ptr) {
            LOG_ERROR(LOG_CAT_RENDERER, "Failed to resize UI buffer");
            return false;
        }
        *ptr = new_ptr;
        *capacity = new_capacity;
    }
    return true;
}

// Comparison functions for qsort
static int compare_z_painters(const void* a, const void* b) {
    const RenderCommand* cmdA = (const RenderCommand*)a;
    const RenderCommand* cmdB = (const RenderCommand*)b;
    // Lower Z first (back to front)
    if (cmdA->z_depth < cmdB->z_depth) return -1;
    if (cmdA->z_depth > cmdB->z_depth) return 1;
    return 0;
}

bool ui_renderer_init(UIRenderer* renderer, float width, float height,
                      UIAntialiasingMode aa_mode, UIGPUBackend gpu_backend, UIZSortMode z_sort_mode) {
    if (!renderer) return false;

    renderer->id = 1;
    renderer->viewport_width = width;
    renderer->viewport_height = height;
    renderer->dpi_scale = 1.0f;
    renderer->aa_mode = aa_mode;
    renderer->gpu_backend = gpu_backend;
    renderer->z_sort_mode = z_sort_mode;
    renderer->profiling_enabled = false;
    renderer->needs_sort = false;
    renderer->needs_aa_resolve = false;

    CommandBuffer* cmd_buffer = MALLOC(sizeof(CommandBuffer));
    if (!cmd_buffer) return false;

    memset(cmd_buffer, 0, sizeof(CommandBuffer));
    renderer->z_context = cmd_buffer;

    GPUBuffers* gpu = MALLOC(sizeof(GPUBuffers));
    if (!gpu) {
        FREE(cmd_buffer);
        return false;
    }
    gpu->vbo = 1;
    gpu->ibo = 2;
    gpu->vao = 3;
    renderer->gpu_buffers = gpu;

    renderer->initialized = true;
    return true;
}

void ui_renderer_shutdown(UIRenderer* renderer) {
    if (!renderer || !renderer->initialized) return;

    if (renderer->z_context) {
        CommandBuffer* cmd_buffer = (CommandBuffer*)renderer->z_context;
        if (cmd_buffer->commands) FREE(cmd_buffer->commands);
        if (cmd_buffer->vertex_buffer) FREE(cmd_buffer->vertex_buffer);
        if (cmd_buffer->index_buffer) FREE(cmd_buffer->index_buffer);
        FREE(cmd_buffer);
        renderer->z_context = NULL;
    }

    if (renderer->gpu_buffers) {
        FREE(renderer->gpu_buffers);
        renderer->gpu_buffers = NULL;
    }

    renderer->initialized = false;
}

void ui_renderer_set_antialiasing_mode(UIRenderer* renderer, UIAntialiasingMode mode) {
    if (renderer) renderer->aa_mode = mode;
}

void ui_renderer_set_gpu_backend(UIRenderer* renderer, UIGPUBackend backend) {
    if (renderer) renderer->gpu_backend = backend;
}

void ui_renderer_set_z_sort_mode(UIRenderer* renderer, UIZSortMode mode) {
    if (renderer) renderer->z_sort_mode = mode;
}

void ui_renderer_submit_element(UIRenderer* renderer,
                                const LayoutNode* element,
                                const UIVertex* vertices,
                                uint32_t vertex_count,
                                const uint32_t* indices,
                                uint32_t index_count,
                                uint32_t texture_id) {
    if (!renderer || !renderer->initialized) return;

    CommandBuffer* cb = (CommandBuffer*)renderer->z_context;

    // Resize buffers if needed
    if (!ensure_capacity((void**)&cb->commands, &cb->cmd_capacity, cb->cmd_count + 1, sizeof(RenderCommand))) return;
    if (!ensure_capacity((void**)&cb->vertex_buffer, &cb->vert_capacity, cb->vert_count + vertex_count, sizeof(UIVertex))) return;
    if (!ensure_capacity((void**)&cb->index_buffer, &cb->idx_capacity, cb->idx_count + index_count, sizeof(uint32_t))) return;

    // Add command
    RenderCommand* cmd = &cb->commands[cb->cmd_count];
    cmd->element = element;
    cmd->texture_id = texture_id;
    cmd->vertex_offset = cb->vert_count;
    cmd->vertex_count = vertex_count;
    cmd->index_offset = cb->idx_count;
    cmd->index_count = index_count;

    // Copy vertices
    if (vertices) {
        memcpy(&cb->vertex_buffer[cb->vert_count], vertices, vertex_count * sizeof(UIVertex));
    }

    // Copy indices
    if (indices) {
        memcpy(&cb->index_buffer[cb->idx_count], indices, index_count * sizeof(uint32_t));
    }

    // Determine Z-depth
    if (vertex_count > 0 && vertices) {
        cmd->z_depth = vertices[0].z_depth; // Use from UIVertex defined in header?
        // Note: header UIVertex struct in my plan was:
        // typedef struct { float position[3]; ... float z_depth; } UIVertex;
        // BUT ui_renderer.h defines it. Let's check.
    } else if (element) {
        cmd->z_depth = (float)element->z_index;
    } else {
        cmd->z_depth = 0.0f;
    }

    cb->cmd_count++;
    cb->vert_count += vertex_count;
    cb->idx_count += index_count;
    renderer->needs_sort = true;
}

// Mock GPU draw call
static void gpu_draw_elements(UIRenderer* renderer, const RenderCommand* cmd, const UIVertex* vertices, const uint32_t* indices) {
    // Using buffer + offset
    // In real GL/VK: bind buffers, drawIndexed(count, 1, index_offset, vertex_offset, 0)
    // (void)vertices; (void)indices;
    // printf("GPU Draw: %d indices (offset %d), Verts (offset %d), Z: %.2f\n",
    //        cmd->index_count, cmd->index_offset, cmd->vertex_offset, cmd->z_depth);
}

void ui_renderer_flush(UIRenderer* renderer) {
    if (!renderer || !renderer->initialized) return;

    CommandBuffer* cb = (CommandBuffer*)renderer->z_context;
    if (cb->cmd_count == 0) return;

    // Sorting
    if (renderer->needs_sort && renderer->z_sort_mode != UI_Z_SORT_NONE) {
        // Only Painter's supported for now
        qsort(cb->commands, cb->cmd_count, sizeof(RenderCommand), compare_z_painters);
        renderer->needs_sort = false;
    }

    // Execute commands
    for (uint32_t i = 0; i < cb->cmd_count; ++i) {
        gpu_draw_elements(renderer, &cb->commands[i], cb->vertex_buffer, cb->index_buffer);
    }

    // Reset counters (reuse memory capacity)
    cb->cmd_count = 0;
    cb->vert_count = 0;
    cb->idx_count = 0;
}

UIAntialiasingMode ui_renderer_get_antialiasing_mode(const UIRenderer* renderer) {
    return renderer ? renderer->aa_mode : UI_AA_NONE;
}

UIGPUBackend ui_renderer_get_gpu_backend(const UIRenderer* renderer) {
    return renderer ? renderer->gpu_backend : UI_GPU_BACKEND_NONE;
}

UIZSortMode ui_renderer_get_z_sort_mode(const UIRenderer* renderer) {
    return renderer ? renderer->z_sort_mode : UI_Z_SORT_NONE;
}
