#ifndef RENDERING_FORWARD_LIGHTING_H
#define RENDERING_FORWARD_LIGHTING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Metal Forward Renderer State */
#ifdef __OBJC__
typedef struct forward_renderer {
    id<MTLRenderPipelineState> opaque_pipeline;
    id<MTLRenderPipelineState> transparent_pipeline;
    id<MTLDepthStencilState> depth_state_opaque;
    id<MTLDepthStencilState> depth_state_transparent;
    id<MTLBuffer> light_grid_buffer;  // Clustered light indices
    id<MTLBuffer> light_data_buffer;
} forward_renderer_t;
#else
typedef struct forward_renderer forward_renderer_t;
#endif

typedef struct rendering_forward_lighting_handle {
    uint32_t id;
} rendering_forward_lighting_handle_t;

typedef struct rendering_forward_lighting_desc {
    uint32_t flags;
    void* user_data;
} rendering_forward_lighting_desc_t;

typedef struct rendering_forward_lighting_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_forward_lighting_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_forward_lighting_init(void);
void rendering_forward_lighting_shutdown(void);

/* Get the global forward renderer instance */
forward_renderer_t* rendering_forward_lighting_get_renderer(void);

/* Lifecycle */
int rendering_forward_lighting_create(rendering_forward_lighting_handle_t* out_handle, const rendering_forward_lighting_desc_t* desc);
void rendering_forward_lighting_destroy(rendering_forward_lighting_handle_t handle);

/* Operations */
int rendering_forward_lighting_update(rendering_forward_lighting_handle_t handle, const void* data, size_t size);
bool rendering_forward_lighting_is_valid(rendering_forward_lighting_handle_t handle);
int rendering_forward_lighting_get_info(rendering_forward_lighting_handle_t handle, rendering_forward_lighting_info_t* out_info);
void rendering_forward_lighting_mark_dirty(rendering_forward_lighting_handle_t handle);
int rendering_forward_lighting_process_pending(void);

/* Statistics */
uint32_t rendering_forward_lighting_get_count(void);
size_t rendering_forward_lighting_get_memory_usage(void);
void rendering_forward_lighting_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_FORWARD_LIGHTING_H */
