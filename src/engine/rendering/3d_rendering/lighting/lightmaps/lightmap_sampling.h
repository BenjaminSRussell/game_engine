/*
 * lightmap_sampling.h
 * Lightmap texture sampling
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_LIGHTMAP_SAMPLING_H
#define LIGHTING_LIGHTMAP_SAMPLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_lightmap_sampling_handle {
    uint32_t id;
} lighting_lightmap_sampling_handle_t;

typedef struct lighting_lightmap_sampling_desc {
    uint32_t flags;
    void* user_data;
} lighting_lightmap_sampling_desc_t;

typedef struct lighting_lightmap_sampling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_lightmap_sampling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_lightmap_sampling_init(void);
void lighting_lightmap_sampling_shutdown(void);

/* Lifecycle */
int lighting_lightmap_sampling_create(lighting_lightmap_sampling_handle_t* out_handle, const lighting_lightmap_sampling_desc_t* desc);
void lighting_lightmap_sampling_destroy(lighting_lightmap_sampling_handle_t handle);

/* Operations */
int lighting_lightmap_sampling_update(lighting_lightmap_sampling_handle_t handle, const void* data, size_t size);
bool lighting_lightmap_sampling_is_valid(lighting_lightmap_sampling_handle_t handle);
int lighting_lightmap_sampling_get_info(lighting_lightmap_sampling_handle_t handle, lighting_lightmap_sampling_info_t* out_info);
void lighting_lightmap_sampling_mark_dirty(lighting_lightmap_sampling_handle_t handle);
int lighting_lightmap_sampling_process_pending(void);

/* Statistics */
uint32_t lighting_lightmap_sampling_get_count(void);
size_t lighting_lightmap_sampling_get_memory_usage(void);
void lighting_lightmap_sampling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_LIGHTMAP_SAMPLING_H */
