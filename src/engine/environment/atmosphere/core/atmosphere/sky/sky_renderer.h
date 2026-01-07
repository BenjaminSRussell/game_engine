/*
 * sky_renderer.h
 * Sky dome rendering
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_SKY_RENDERER_H
#define ATMOSPHERE_SKY_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_sky_renderer_handle {
    uint32_t id;
} atmosphere_sky_renderer_handle_t;

typedef struct atmosphere_sky_renderer_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_sky_renderer_desc_t;

typedef struct atmosphere_sky_renderer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_sky_renderer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_sky_renderer_init(void);
void atmosphere_sky_renderer_shutdown(void);

/* Lifecycle */
int atmosphere_sky_renderer_create(atmosphere_sky_renderer_handle_t* out_handle, const atmosphere_sky_renderer_desc_t* desc);
void atmosphere_sky_renderer_destroy(atmosphere_sky_renderer_handle_t handle);

/* Operations */
int atmosphere_sky_renderer_update(atmosphere_sky_renderer_handle_t handle, const void* data, size_t size);
bool atmosphere_sky_renderer_is_valid(atmosphere_sky_renderer_handle_t handle);
int atmosphere_sky_renderer_get_info(atmosphere_sky_renderer_handle_t handle, atmosphere_sky_renderer_info_t* out_info);
void atmosphere_sky_renderer_mark_dirty(atmosphere_sky_renderer_handle_t handle);
int atmosphere_sky_renderer_process_pending(void);

/* Statistics */
uint32_t atmosphere_sky_renderer_get_count(void);
size_t atmosphere_sky_renderer_get_memory_usage(void);
void atmosphere_sky_renderer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_SKY_RENDERER_H */
