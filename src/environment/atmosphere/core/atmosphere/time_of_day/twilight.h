/*
 * twilight.h
 * Dawn/dusk transitions
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_TWILIGHT_H
#define ATMOSPHERE_TWILIGHT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_twilight_handle {
    uint32_t id;
} atmosphere_twilight_handle_t;

typedef struct atmosphere_twilight_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_twilight_desc_t;

typedef struct atmosphere_twilight_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_twilight_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_twilight_init(void);
void atmosphere_twilight_shutdown(void);

/* Lifecycle */
int atmosphere_twilight_create(atmosphere_twilight_handle_t* out_handle, const atmosphere_twilight_desc_t* desc);
void atmosphere_twilight_destroy(atmosphere_twilight_handle_t handle);

/* Operations */
int atmosphere_twilight_update(atmosphere_twilight_handle_t handle, const void* data, size_t size);
bool atmosphere_twilight_is_valid(atmosphere_twilight_handle_t handle);
int atmosphere_twilight_get_info(atmosphere_twilight_handle_t handle, atmosphere_twilight_info_t* out_info);
void atmosphere_twilight_mark_dirty(atmosphere_twilight_handle_t handle);
int atmosphere_twilight_process_pending(void);

/* Statistics */
uint32_t atmosphere_twilight_get_count(void);
size_t atmosphere_twilight_get_memory_usage(void);
void atmosphere_twilight_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_TWILIGHT_H */
