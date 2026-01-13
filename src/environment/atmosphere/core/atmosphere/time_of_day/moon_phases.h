/*
 * moon_phases.h
 * Moon phase rendering
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_MOON_PHASES_H
#define ATMOSPHERE_MOON_PHASES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_moon_phases_handle {
    uint32_t id;
} atmosphere_moon_phases_handle_t;

typedef struct atmosphere_moon_phases_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_moon_phases_desc_t;

typedef struct atmosphere_moon_phases_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_moon_phases_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_moon_phases_init(void);
void atmosphere_moon_phases_shutdown(void);

/* Lifecycle */
int atmosphere_moon_phases_create(atmosphere_moon_phases_handle_t* out_handle, const atmosphere_moon_phases_desc_t* desc);
void atmosphere_moon_phases_destroy(atmosphere_moon_phases_handle_t handle);

/* Operations */
int atmosphere_moon_phases_update(atmosphere_moon_phases_handle_t handle, const void* data, size_t size);
bool atmosphere_moon_phases_is_valid(atmosphere_moon_phases_handle_t handle);
int atmosphere_moon_phases_get_info(atmosphere_moon_phases_handle_t handle, atmosphere_moon_phases_info_t* out_info);
void atmosphere_moon_phases_mark_dirty(atmosphere_moon_phases_handle_t handle);
int atmosphere_moon_phases_process_pending(void);

/* Statistics */
uint32_t atmosphere_moon_phases_get_count(void);
size_t atmosphere_moon_phases_get_memory_usage(void);
void atmosphere_moon_phases_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_MOON_PHASES_H */
