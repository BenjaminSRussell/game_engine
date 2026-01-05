/*
 * spot_light.h
 * Spotlight rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SPOT_LIGHT_H
#define LIGHTING_SPOT_LIGHT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_spot_light_handle {
    uint32_t id;
} lighting_spot_light_handle_t;

typedef struct lighting_spot_light_desc {
    uint32_t flags;
    void* user_data;
} lighting_spot_light_desc_t;

typedef struct lighting_spot_light_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_spot_light_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_spot_light_init(void);
void lighting_spot_light_shutdown(void);

/* Lifecycle */
int lighting_spot_light_create(lighting_spot_light_handle_t* out_handle, const lighting_spot_light_desc_t* desc);
void lighting_spot_light_destroy(lighting_spot_light_handle_t handle);

/* Operations */
int lighting_spot_light_update(lighting_spot_light_handle_t handle, const void* data, size_t size);
bool lighting_spot_light_is_valid(lighting_spot_light_handle_t handle);
int lighting_spot_light_get_info(lighting_spot_light_handle_t handle, lighting_spot_light_info_t* out_info);
void lighting_spot_light_mark_dirty(lighting_spot_light_handle_t handle);
int lighting_spot_light_process_pending(void);

/* Statistics */
uint32_t lighting_spot_light_get_count(void);
size_t lighting_spot_light_get_memory_usage(void);
void lighting_spot_light_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SPOT_LIGHT_H */
