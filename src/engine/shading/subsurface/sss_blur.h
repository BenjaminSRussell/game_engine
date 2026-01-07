/*
 * sss_blur.h
 * Separable SSS blur
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_SSS_BLUR_H
#define SHADING_SSS_BLUR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "include/math/vec2.h"
#include "shading/subsurface/sss_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef uint32_t texture_handle_t;

typedef struct shading_sss_blur_handle {
    uint32_t id;
} shading_sss_blur_handle_t;

typedef struct shading_sss_blur_desc {
    uint32_t flags;
    float max_dd; // Maximum derivative of depth for edge stopping
} shading_sss_blur_desc_t;

typedef struct shading_sss_blur_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_sss_blur_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_sss_blur_init(void);
void shading_sss_blur_shutdown(void);

/* Lifecycle */
int shading_sss_blur_create(shading_sss_blur_handle_t* out_handle, const shading_sss_blur_desc_t* desc);
void shading_sss_blur_destroy(shading_sss_blur_handle_t handle);

/* Rendering Pass */
// Separable SSS blur pass (horizontal or vertical depending on direction)
void sss_blur_pass(texture_handle_t color, texture_handle_t depth,
                   shading_sss_profile_handle_t profile_handle, vec2_t direction);

/* Statistics */
uint32_t shading_sss_blur_get_count(void);
void shading_sss_blur_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_SSS_BLUR_H */
