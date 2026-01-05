/*
 * shadow_caster.h
 * Shadow casting setup
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SHADOW_CASTER_H
#define LIGHTING_SHADOW_CASTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_shadow_caster_handle {
    uint32_t id;
} lighting_shadow_caster_handle_t;

typedef struct lighting_shadow_caster_desc {
    uint32_t flags;
    void* user_data;
} lighting_shadow_caster_desc_t;

typedef struct lighting_shadow_caster_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_shadow_caster_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_shadow_caster_init(void);
void lighting_shadow_caster_shutdown(void);

/* Lifecycle */
int lighting_shadow_caster_create(lighting_shadow_caster_handle_t* out_handle, const lighting_shadow_caster_desc_t* desc);
void lighting_shadow_caster_destroy(lighting_shadow_caster_handle_t handle);

/* Operations */
int lighting_shadow_caster_update(lighting_shadow_caster_handle_t handle, const void* data, size_t size);
bool lighting_shadow_caster_is_valid(lighting_shadow_caster_handle_t handle);
int lighting_shadow_caster_get_info(lighting_shadow_caster_handle_t handle, lighting_shadow_caster_info_t* out_info);
void lighting_shadow_caster_mark_dirty(lighting_shadow_caster_handle_t handle);
int lighting_shadow_caster_process_pending(void);

/* Statistics */
uint32_t lighting_shadow_caster_get_count(void);
size_t lighting_shadow_caster_get_memory_usage(void);
void lighting_shadow_caster_debug_print(void);

/* Shadow Pass Rendering */
int lighting_shadow_caster_begin_pass(uint32_t light_id, uint32_t atlas_x, uint32_t atlas_y, uint32_t resolution);
void lighting_shadow_caster_set_view_matrix(const float* view_matrix);
void lighting_shadow_caster_set_proj_matrix(const float* proj_matrix);
void lighting_shadow_caster_set_directional_light(const float* light_dir, const float* scene_center, float scene_radius);
void lighting_shadow_caster_set_spot_light(const float* light_pos, const float* light_dir, float fov, float near, float far);
void lighting_shadow_caster_set_bias(float depth_bias, float slope_bias);
void lighting_shadow_caster_end_pass(void);
void lighting_shadow_caster_render_all(void);
void lighting_shadow_caster_clear_passes(void);
uint32_t lighting_shadow_caster_get_pass_count(void);
int lighting_shadow_caster_get_pass_matrix(uint32_t pass_index, float* out_matrix);


#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SHADOW_CASTER_H */
