/*
 * ocean_renderer.h
 * Ocean surface rendering
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_OCEAN_RENDERER_H
#define WATER_OCEAN_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_ocean_renderer_handle {
    uint32_t id;
} water_ocean_renderer_handle_t;

typedef struct water_ocean_renderer_desc {
    uint32_t flags;
    void* user_data;
} water_ocean_renderer_desc_t;

typedef struct water_ocean_renderer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_ocean_renderer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_ocean_renderer_init(void);
void water_ocean_renderer_shutdown(void);

/* Lifecycle */
int water_ocean_renderer_create(water_ocean_renderer_handle_t* out_handle, const water_ocean_renderer_desc_t* desc);
void water_ocean_renderer_destroy(water_ocean_renderer_handle_t handle);

/* Operations */
int water_ocean_renderer_update(water_ocean_renderer_handle_t handle, const void* data, size_t size);
bool water_ocean_renderer_is_valid(water_ocean_renderer_handle_t handle);
int water_ocean_renderer_get_info(water_ocean_renderer_handle_t handle, water_ocean_renderer_info_t* out_info);
void water_ocean_renderer_mark_dirty(water_ocean_renderer_handle_t handle);
int water_ocean_renderer_process_pending(void);

/* Statistics */
uint32_t water_ocean_renderer_get_count(void);
size_t water_ocean_renderer_get_memory_usage(void);
void water_ocean_renderer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_OCEAN_RENDERER_H */
