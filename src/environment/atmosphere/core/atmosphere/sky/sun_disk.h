/*
 * sun_disk.h
 * Sun disk rendering
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_SUN_DISK_H
#define ATMOSPHERE_SUN_DISK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_sun_disk_handle {
    uint32_t id;
} atmosphere_sun_disk_handle_t;

typedef struct atmosphere_sun_disk_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_sun_disk_desc_t;

typedef struct atmosphere_sun_disk_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_sun_disk_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_sun_disk_init(void);
void atmosphere_sun_disk_shutdown(void);

/* Lifecycle */
int atmosphere_sun_disk_create(atmosphere_sun_disk_handle_t* out_handle, const atmosphere_sun_disk_desc_t* desc);
void atmosphere_sun_disk_destroy(atmosphere_sun_disk_handle_t handle);

/* Operations */
int atmosphere_sun_disk_update(atmosphere_sun_disk_handle_t handle, const void* data, size_t size);
bool atmosphere_sun_disk_is_valid(atmosphere_sun_disk_handle_t handle);
int atmosphere_sun_disk_get_info(atmosphere_sun_disk_handle_t handle, atmosphere_sun_disk_info_t* out_info);
void atmosphere_sun_disk_mark_dirty(atmosphere_sun_disk_handle_t handle);
int atmosphere_sun_disk_process_pending(void);

/* Statistics */
uint32_t atmosphere_sun_disk_get_count(void);
size_t atmosphere_sun_disk_get_memory_usage(void);
void atmosphere_sun_disk_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_SUN_DISK_H */
