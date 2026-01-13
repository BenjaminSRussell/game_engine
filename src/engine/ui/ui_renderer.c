#include "ui_renderer.h"
#include "core/memory.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_VERTICES 100000
#define MAX_INDICES 150000
#define MAX_CLIP_STACK 64

typedef struct {
    UIVertex* vertices;
    uint32_t vertex_count;

    uint32_t* indices;
    uint32_t index_count;

    uint32_t current_texture_id;

    Rect clip_stack[MAX_CLIP_STACK];
    uint32_t clip_stack_top;
    Rect current_clip;

} UIRendererInternal;

bool ui_renderer_init(UIRenderer* renderer, float width, float height,
                      UIAntialiasingMode aa_mode, UIGPUBackend gpu_backend, UIZSortMode z_sort_mode) {
    if (!renderer) return false;

    renderer->id = 0; // TODO: Generate ID
    renderer->initialized = true;
    renderer->aa_mode = aa_mode;
    renderer->gpu_backend = gpu_backend;
    renderer->z_sort_mode = z_sort_mode;
    renderer->viewport_width = width;
    renderer->viewport_height = height;
    renderer->dpi_scale = 1.0f;
    renderer->profiling_enabled = false;
    renderer->needs_sort = false;
    renderer->needs_aa_resolve = false;

    // Allocate internal buffers
    UIRendererInternal* internal = (UIRendererInternal*)memory_alloc(sizeof(UIRendererInternal));
    if (!internal) {
        LOG_ERROR(LOG_CAT_UI, "Failed to allocate UIRenderer internal state");
        return false;
    }

    internal->vertices = (UIVertex*)memory_alloc(sizeof(UIVertex) * MAX_VERTICES);
    internal->indices = (uint32_t*)memory_alloc(sizeof(uint32_t) * MAX_INDICES);

    if (!internal->vertices || !internal->indices) {
        LOG_ERROR(LOG_CAT_UI, "Failed to allocate UIRenderer geometry buffers");
        if (internal->vertices) memory_free(internal->vertices);
        if (internal->indices) memory_free(internal->indices);
        memory_free(internal);
        return false;
    }

    internal->vertex_count = 0;
    internal->index_count = 0;
    internal->current_texture_id = 0;

    // Init clip stack with full viewport
    internal->clip_stack_top = 0;
    internal->current_clip = (Rect){0, 0, width, height};
    internal->clip_stack[0] = internal->current_clip;

    renderer->gpu_buffers = internal;
    renderer->aa_context = NULL; // TODO: AA context
    renderer->z_context = NULL; // TODO: Z context

    LOG_INFO(LOG_CAT_UI, "UI Renderer initialized (AA: %d, Backend: %d)", aa_mode, gpu_backend);
    return true;
}

void ui_renderer_shutdown(UIRenderer* renderer) {
    if (!renderer || !renderer->initialized) return;

    UIRendererInternal* internal = (UIRendererInternal*)renderer->gpu_buffers;
    if (internal) {
        if (internal->vertices) memory_free(internal->vertices);
        if (internal->indices) memory_free(internal->indices);
        memory_free(internal);
    }

    renderer->gpu_buffers = NULL;
    renderer->initialized = false;
    LOG_INFO(LOG_CAT_UI, "UI Renderer shutdown");
}

void ui_renderer_set_antialiasing_mode(UIRenderer* renderer, UIAntialiasingMode mode) {
    if (!renderer) return;
    renderer->aa_mode = mode;
    // Trigger internal state update if needed
}

void ui_renderer_set_gpu_backend(UIRenderer* renderer, UIGPUBackend backend) {
    if (!renderer) return;
    renderer->gpu_backend = backend;
}

void ui_renderer_set_z_sort_mode(UIRenderer* renderer, UIZSortMode mode) {
    if (!renderer) return;
    renderer->z_sort_mode = mode;
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

void ui_renderer_flush(UIRenderer* renderer) {
    if (!renderer || !renderer->initialized) return;

    UIRendererInternal* internal = (UIRendererInternal*)renderer->gpu_buffers;
    if (internal->index_count == 0) return;

    // Here we would issue the draw call to the backend
    // For now, we just reset the buffers
    // e.g. backend_draw_indexed(internal->vertices, internal->indices, internal->index_count, internal->current_texture_id, internal->current_clip);

    internal->vertex_count = 0;
    internal->index_count = 0;
}

void ui_renderer_push_clip_rect(UIRenderer* renderer, Rect rect) {
    if (!renderer || !renderer->initialized) return;

    UIRendererInternal* internal = (UIRendererInternal*)renderer->gpu_buffers;

    // Flush current batch because clip state changes
    ui_renderer_flush(renderer);

    if (internal->clip_stack_top < MAX_CLIP_STACK - 1) {
        internal->clip_stack_top++;

        // Intersect with current clip
        Rect prev = internal->clip_stack[internal->clip_stack_top - 1];
        Rect new_clip;

        float x1 = MAX(prev.x, rect.x);
        float y1 = MAX(prev.y, rect.y);
        float x2 = MIN(prev.x + prev.width, rect.x + rect.width);
        float y2 = MIN(prev.y + prev.height, rect.y + rect.height);

        new_clip.x = x1;
        new_clip.y = y1;
        new_clip.width = MAX(0.0f, x2 - x1);
        new_clip.height = MAX(0.0f, y2 - y1);

        internal->clip_stack[internal->clip_stack_top] = new_clip;
        internal->current_clip = new_clip;
    } else {
        LOG_WARN(LOG_CAT_UI, "Clip stack overflow");
    }
}

void ui_renderer_pop_clip_rect(UIRenderer* renderer) {
    if (!renderer || !renderer->initialized) return;

    UIRendererInternal* internal = (UIRendererInternal*)renderer->gpu_buffers;

    // Flush current batch
    ui_renderer_flush(renderer);

    if (internal->clip_stack_top > 0) {
        internal->clip_stack_top--;
        internal->current_clip = internal->clip_stack[internal->clip_stack_top];
    } else {
        LOG_WARN(LOG_CAT_UI, "Clip stack underflow");
    }
}

void ui_renderer_submit_element(UIRenderer* renderer,
                                const LayoutNode* element,
                                const UIVertex* vertices,
                                uint32_t vertex_count,
                                const uint32_t* indices,
                                uint32_t index_count,
                                uint32_t texture_id) {
    if (!renderer || !renderer->initialized || !vertices || !indices) return;

    UIRendererInternal* internal = (UIRendererInternal*)renderer->gpu_buffers;

    // Check if flush needed
    bool texture_changed = (texture_id != internal->current_texture_id && internal->index_count > 0);
    bool buffer_full = (internal->vertex_count + vertex_count > MAX_VERTICES) ||
                       (internal->index_count + index_count > MAX_INDICES);

    if (texture_changed || buffer_full) {
        ui_renderer_flush(renderer);
    }

    internal->current_texture_id = texture_id;

    // Append geometry
    uint32_t base_vertex = internal->vertex_count;

    memcpy(internal->vertices + base_vertex, vertices, vertex_count * sizeof(UIVertex));

    for (uint32_t i = 0; i < index_count; i++) {
        internal->indices[internal->index_count + i] = base_vertex + indices[i];
    }

    internal->vertex_count += vertex_count;
    internal->index_count += index_count;
}
