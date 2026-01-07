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
#include "cascade_splits.h"

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

/* Metal-compatible definitions (mirrored in C) */
typedef struct shadow_cascade {
    void* depth_texture; // id<MTLTexture>
    float view_proj[16]; // simd_float4x4
    float split_near;
    float split_far;
} shadow_cascade_t;

typedef struct shadow_map_system {
    shadow_cascade_t cascades[4];
    void* shadow_pipeline;      // id<MTLRenderPipelineState>
    void* shadow_depth_state;   // id<MTLDepthStencilState>
    uint32_t cascade_count;
    uint32_t resolution;
} shadow_map_system_t;

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

/* Cascaded Shadows API */
shadow_map_system_t* shadow_system_create(void* device, uint32_t resolution, uint32_t cascades);
void shadow_system_destroy(shadow_map_system_t* sys);
void shadow_system_update_cascades(shadow_map_system_t* sys, const cascade_camera_t* camera, const float* light_dir, float shadow_distance);
void shadow_system_render_cascade(shadow_map_system_t* sys, uint32_t cascade_index, void* cmd_buffer, void* shadow_casters);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SHADOW_CASTER_H */
