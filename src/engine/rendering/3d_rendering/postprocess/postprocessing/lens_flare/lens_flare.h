/*
 * lens_flare.h
 * Lens flare effect
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_LENS_FLARE_H
#define POSTPROCESSING_LENS_FLARE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "math/vec3.h"
#include "renderer/core/texture.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_lens_flare_handle {
    uint32_t id;
} postprocessing_lens_flare_handle_t;

typedef struct postprocessing_lens_flare_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_lens_flare_desc_t;

typedef struct postprocessing_lens_flare_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_lens_flare_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_lens_flare_init(void);
void postprocessing_lens_flare_shutdown(void);

/* Lifecycle */
int postprocessing_lens_flare_create(postprocessing_lens_flare_handle_t* out_handle, const postprocessing_lens_flare_desc_t* desc);
void postprocessing_lens_flare_destroy(postprocessing_lens_flare_handle_t handle);

/* Operations */
int postprocessing_lens_flare_update(postprocessing_lens_flare_handle_t handle, const void* data, size_t size);
bool postprocessing_lens_flare_is_valid(postprocessing_lens_flare_handle_t handle);
int postprocessing_lens_flare_get_info(postprocessing_lens_flare_handle_t handle, postprocessing_lens_flare_info_t* out_info);
void postprocessing_lens_flare_mark_dirty(postprocessing_lens_flare_handle_t handle);
int postprocessing_lens_flare_process_pending(void);

/* Rendering */
void render_lens_flare(postprocessing_lens_flare_handle_t handle, Vec3 light_pos_world, TextureID output);
void postprocessing_lens_flare_add_element(postprocessing_lens_flare_handle_t handle, float offset, float scale, Vec3 color, TextureID texture);

/* Statistics */
uint32_t postprocessing_lens_flare_get_count(void);
size_t postprocessing_lens_flare_get_memory_usage(void);
void postprocessing_lens_flare_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_LENS_FLARE_H */
