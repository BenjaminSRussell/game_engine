/*
 * sun_moon_position.h
 * Sun/moon positioning
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_SUN_MOON_POSITION_H
#define ATMOSPHERE_SUN_MOON_POSITION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_sun_moon_position_handle {
    uint32_t id;
} atmosphere_sun_moon_position_handle_t;

typedef struct atmosphere_sun_moon_position_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_sun_moon_position_desc_t;

typedef struct atmosphere_sun_moon_position_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_sun_moon_position_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_sun_moon_position_init(void);
void atmosphere_sun_moon_position_shutdown(void);

/* Lifecycle */
int atmosphere_sun_moon_position_create(atmosphere_sun_moon_position_handle_t* out_handle, const atmosphere_sun_moon_position_desc_t* desc);
void atmosphere_sun_moon_position_destroy(atmosphere_sun_moon_position_handle_t handle);

/* Operations */
int atmosphere_sun_moon_position_update(atmosphere_sun_moon_position_handle_t handle, const void* data, size_t size);
bool atmosphere_sun_moon_position_is_valid(atmosphere_sun_moon_position_handle_t handle);
int atmosphere_sun_moon_position_get_info(atmosphere_sun_moon_position_handle_t handle, atmosphere_sun_moon_position_info_t* out_info);
void atmosphere_sun_moon_position_mark_dirty(atmosphere_sun_moon_position_handle_t handle);
int atmosphere_sun_moon_position_process_pending(void);

/* Statistics */
uint32_t atmosphere_sun_moon_position_get_count(void);
size_t atmosphere_sun_moon_position_get_memory_usage(void);
void atmosphere_sun_moon_position_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_SUN_MOON_POSITION_H */
