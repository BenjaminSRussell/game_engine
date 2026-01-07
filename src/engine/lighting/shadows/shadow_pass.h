/*
 * shadow_pass.h
 * Shadow rendering pass management
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SHADOW_PASS_H
#define LIGHTING_SHADOW_PASS_H

#include "lighting/shadows/shadow_atlas.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct shadow_pass_config {
    lighting_shadow_atlas_handle_t atlas;
    uint32_t cascade_index;
    float depth_bias_const;
    float depth_bias_slope;
    bool front_face_culling;
} shadow_pass_config_t;

typedef struct shadow_pass_handle {
    uint32_t id;
} shadow_pass_handle_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Pass Setup */
int shadow_pass_create(shadow_pass_handle_t* out_handle, const shadow_pass_config_t* config);
void shadow_pass_destroy(shadow_pass_handle_t handle);

/* Rendering */
void shadow_pass_begin(shadow_pass_handle_t handle, void* command_buffer, const float* view_proj);
void shadow_pass_draw_casters(shadow_pass_handle_t handle, uint32_t* visible_indices, uint32_t count);
void shadow_pass_end(shadow_pass_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SHADOW_PASS_H */
