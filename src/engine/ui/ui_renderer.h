/*
 * ui_renderer.h
 * UI Rendering System
 * Advanced UI rendering with antialiasing, GPU acceleration, and Z-ordering
 */

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <stdbool.h>
#include <stdint.h>
#include "ui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Antialiasing modes
typedef enum {
    UI_AA_NONE,
    UI_AA_MSAA_2X,
    UI_AA_MSAA_4X,
    UI_AA_MSAA_8X,
    UI_AA_FXAA,
    UI_AA_TAA,
    UI_AA_SMAA
} UIAntialiasingMode;

// GPU backends
typedef enum {
    UI_GPU_BACKEND_NONE,
    UI_GPU_BACKEND_OPENGL,
    UI_GPU_BACKEND_VULKAN,
    UI_GPU_BACKEND_METAL,
    UI_GPU_BACKEND_D3D11,
    UI_GPU_BACKEND_D3D12
} UIGPUBackend;

// Z-order sorting modes
typedef enum {
    UI_Z_SORT_NONE,
    UI_Z_SORT_PAINTERS,
    UI_Z_SORT_DEPTH_BUFFER,
    UI_Z_SORT_HYBRID,
    UI_Z_SORT_OPTIMIZED
} UIZSortMode;

// Main renderer structure
typedef struct UIRenderer {
    uint32_t id;
    bool initialized;
    
    UIAntialiasingMode aa_mode;
    UIGPUBackend gpu_backend;
    UIZSortMode z_sort_mode;
    
    float viewport_width;
    float viewport_height;
    float dpi_scale;
    
    void* gpu_buffers;
    void* aa_context;
    void* z_context;
    
    bool profiling_enabled;
    bool needs_sort;
    bool needs_aa_resolve;
} UIRenderer;

// UI vertex structure for submission
typedef struct {
    float position[3];        // x, y, z
    float texcoord[2];        // u, v
    float color[4];           // r, g, b, a
    uint32_t element_id;      // Element ID
    float z_depth;            // Depth for sorting
} UIVertex;

// API functions
bool ui_renderer_init(UIRenderer* renderer, float width, float height, 
                      UIAntialiasingMode aa_mode, UIGPUBackend gpu_backend, UIZSortMode z_sort_mode);
void ui_renderer_shutdown(UIRenderer* renderer);
void ui_renderer_set_antialiasing_mode(UIRenderer* renderer, UIAntialiasingMode mode);
void ui_renderer_set_gpu_backend(UIRenderer* renderer, UIGPUBackend backend);
void ui_renderer_set_z_sort_mode(UIRenderer* renderer, UIZSortMode mode);

// Clipping
void ui_renderer_push_clip_rect(UIRenderer* renderer, Rect rect);
void ui_renderer_pop_clip_rect(UIRenderer* renderer);

// Submission and rendering
void ui_renderer_submit_element(UIRenderer* renderer,
                                const LayoutNode* element,
                                const UIVertex* vertices,
                                uint32_t vertex_count,
                                const uint32_t* indices,
                                uint32_t index_count,
                                uint32_t texture_id);
void ui_renderer_flush(UIRenderer* renderer);

// Getters
UIAntialiasingMode ui_renderer_get_antialiasing_mode(const UIRenderer* renderer);
UIGPUBackend ui_renderer_get_gpu_backend(const UIRenderer* renderer);
UIZSortMode ui_renderer_get_z_sort_mode(const UIRenderer* renderer);

#ifdef __cplusplus
}
#endif

#endif // UI_RENDERER_H
