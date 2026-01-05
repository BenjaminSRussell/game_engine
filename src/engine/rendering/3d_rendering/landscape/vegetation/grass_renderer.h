#ifndef LANDSCAPE_GRASS_RENDERER_H
#define LANDSCAPE_GRASS_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h> // For position/color

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_grass_renderer_handle {
    uint32_t id;
} landscape_grass_renderer_handle_t;

typedef struct grass_blade_config {
    float width;
    float min_height;
    float max_height;
    float bend_amount;
    float wind_stiffness;
    Vec3 base_color;
    Vec3 tip_color;
} grass_blade_config_t;

typedef struct landscape_grass_renderer_desc {
    uint32_t flags;
    void* user_data;
    grass_blade_config_t blade_config;
    float density;           // blades per square meter
    float cull_distance;     // max render distance
    uint32_t chunk_size;     // size of grass chunks in meters
} landscape_grass_renderer_desc_t;

typedef struct landscape_grass_renderer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t instance_count;
    uint32_t visible_instances;
    size_t buffer_memory;
} landscape_grass_renderer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_grass_renderer_init(void);
void landscape_grass_renderer_shutdown(void);

/* Lifecycle */
int landscape_grass_renderer_create(landscape_grass_renderer_handle_t* out_handle, const landscape_grass_renderer_desc_t* desc);
void landscape_grass_renderer_destroy(landscape_grass_renderer_handle_t handle);

/* Operations */
// Update with camera position for culling/LOD
int landscape_grass_renderer_update(landscape_grass_renderer_handle_t handle, Vec3 camera_pos, float delta_time);
void landscape_grass_renderer_render(landscape_grass_renderer_handle_t handle, const void* render_context); // Opaque render context

bool landscape_grass_renderer_is_valid(landscape_grass_renderer_handle_t handle);
int landscape_grass_renderer_get_info(landscape_grass_renderer_handle_t handle, landscape_grass_renderer_info_t* out_info);
void landscape_grass_renderer_mark_dirty(landscape_grass_renderer_handle_t handle);
int landscape_grass_renderer_process_pending(void);

/* Statistics */
uint32_t landscape_grass_renderer_get_count(void);
size_t landscape_grass_renderer_get_memory_usage(void);
void landscape_grass_renderer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_GRASS_RENDERER_H */
