#ifndef LANDSCAPE_SPLAT_RENDERER_H
#define LANDSCAPE_SPLAT_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h> // For tiling vector

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_splat_renderer_handle {
    uint32_t id;
} landscape_splat_renderer_handle_t;

typedef struct splat_layer {
    uint32_t diffuse_texture_id;
    uint32_t normal_texture_id;
    uint32_t roughness_texture_id;
    float tiling_x;
    float tiling_y;
    float global_opacity;
    bool active;
} splat_layer_t;

typedef struct landscape_splat_renderer_desc {
    uint32_t flags;
    void* user_data;
    uint32_t max_layers;
    uint32_t splatmap_resolution;
    const char* splatmap_path;
} landscape_splat_renderer_desc_t;

typedef struct landscape_splat_renderer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t active_layers;
    uint32_t texture_memory_usage;
} landscape_splat_renderer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_splat_renderer_init(void);
void landscape_splat_renderer_shutdown(void);

/* Lifecycle */
int landscape_splat_renderer_create(landscape_splat_renderer_handle_t* out_handle, const landscape_splat_renderer_desc_t* desc);
void landscape_splat_renderer_destroy(landscape_splat_renderer_handle_t handle);

/* Layer Management */
int landscape_splat_renderer_add_layer(landscape_splat_renderer_handle_t handle, uint32_t index, const splat_layer_t* layer);
int landscape_splat_renderer_remove_layer(landscape_splat_renderer_handle_t handle, uint32_t index);
int landscape_splat_renderer_update_splatmap(landscape_splat_renderer_handle_t handle, const uint8_t* data, uint32_t width, uint32_t height);

/* Operations */
int landscape_splat_renderer_update(landscape_splat_renderer_handle_t handle, float delta_time);
bool landscape_splat_renderer_is_valid(landscape_splat_renderer_handle_t handle);
int landscape_splat_renderer_get_info(landscape_splat_renderer_handle_t handle, landscape_splat_renderer_info_t* out_info);
void landscape_splat_renderer_mark_dirty(landscape_splat_renderer_handle_t handle);
int landscape_splat_renderer_process_pending(void);

/* Statistics */
uint32_t landscape_splat_renderer_get_count(void);
size_t landscape_splat_renderer_get_memory_usage(void);
void landscape_splat_renderer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_SPLAT_RENDERER_H */
