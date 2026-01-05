/*
 * point_light.h
 * Point light rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_POINT_LIGHT_H
#define LIGHTING_POINT_LIGHT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_point_light_handle {
    uint32_t id;
} lighting_point_light_handle_t;

typedef struct lighting_point_light_desc {
    uint32_t flags;
    void* user_data;
} lighting_point_light_desc_t;

typedef struct lighting_point_light_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_point_light_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_point_light_init(void);
void lighting_point_light_shutdown(void);

/* Lifecycle */
int lighting_point_light_create(lighting_point_light_handle_t* out_handle, const lighting_point_light_desc_t* desc);
void lighting_point_light_destroy(lighting_point_light_handle_t handle);

/* Operations */
int lighting_point_light_update(lighting_point_light_handle_t handle, const void* data, size_t size);
bool lighting_point_light_is_valid(lighting_point_light_handle_t handle);
int lighting_point_light_get_info(lighting_point_light_handle_t handle, lighting_point_light_info_t* out_info);
void lighting_point_light_mark_dirty(lighting_point_light_handle_t handle);
int lighting_point_light_process_pending(void);

/* Statistics */
uint32_t lighting_point_light_get_count(void);
size_t lighting_point_light_get_memory_usage(void);
void lighting_point_light_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_POINT_LIGHT_H */
