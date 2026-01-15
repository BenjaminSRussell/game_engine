#ifndef LANDSCAPE_SPLAT_BLENDING_H
#define LANDSCAPE_SPLAT_BLENDING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_splat_blending_handle {
    uint32_t id;
} landscape_splat_blending_handle_t;

typedef struct landscape_splat_blending_desc {
    uint32_t flags;
    void* user_data;
    float height_blend_falloff; // Smoothness of height blending (0.01 - 1.0)
} landscape_splat_blending_desc_t;

typedef struct landscape_splat_blending_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_splat_blending_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_splat_blending_init(void);
void landscape_splat_blending_shutdown(void);

/* Lifecycle */
int landscape_splat_blending_create(landscape_splat_blending_handle_t* out_handle, const landscape_splat_blending_desc_t* desc);
void landscape_splat_blending_destroy(landscape_splat_blending_handle_t handle);

/* Operations */
int landscape_splat_blending_update(landscape_splat_blending_handle_t handle, const void* data, size_t size);
bool landscape_splat_blending_is_valid(landscape_splat_blending_handle_t handle);
int landscape_splat_blending_get_info(landscape_splat_blending_handle_t handle, landscape_splat_blending_info_t* out_info);
void landscape_splat_blending_mark_dirty(landscape_splat_blending_handle_t handle);
int landscape_splat_blending_process_pending(void);

/* Blend Logic */
// Calculates blend weights for up to 4 layers based on heightmap values and layer properties
// Weights are normalized sum to 1.0
void landscape_splat_calculate_weights(
    float* out_weights,         // Array of 4 floats
    const float* layer_heights, // Array of 4 floats (height values from textures)
    const float* layer_alphas,  // Array of 4 floats (base alphas from splatmap)
    float height_blend_falloff  // Contrast factor
);

/* Statistics */
uint32_t landscape_splat_blending_get_count(void);
size_t landscape_splat_blending_get_memory_usage(void);
void landscape_splat_blending_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_SPLAT_BLENDING_H */
