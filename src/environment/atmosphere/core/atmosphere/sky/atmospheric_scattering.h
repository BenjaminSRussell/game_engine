/*
 * atmospheric_scattering.h
 * Atmospheric scattering
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_ATMOSPHERIC_SCATTERING_H
#define ATMOSPHERE_ATMOSPHERIC_SCATTERING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_atmospheric_scattering_handle {
    uint32_t id;
} atmosphere_atmospheric_scattering_handle_t;

typedef struct atmosphere_atmospheric_scattering_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_atmospheric_scattering_desc_t;

typedef struct atmosphere_atmospheric_scattering_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_atmospheric_scattering_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_atmospheric_scattering_init(void);
void atmosphere_atmospheric_scattering_shutdown(void);

/* Lifecycle */
int atmosphere_atmospheric_scattering_create(atmosphere_atmospheric_scattering_handle_t* out_handle, const atmosphere_atmospheric_scattering_desc_t* desc);
void atmosphere_atmospheric_scattering_destroy(atmosphere_atmospheric_scattering_handle_t handle);

/* Operations */
int atmosphere_atmospheric_scattering_update(atmosphere_atmospheric_scattering_handle_t handle, const void* data, size_t size);
bool atmosphere_atmospheric_scattering_is_valid(atmosphere_atmospheric_scattering_handle_t handle);
int atmosphere_atmospheric_scattering_get_info(atmosphere_atmospheric_scattering_handle_t handle, atmosphere_atmospheric_scattering_info_t* out_info);
void atmosphere_atmospheric_scattering_mark_dirty(atmosphere_atmospheric_scattering_handle_t handle);
int atmosphere_atmospheric_scattering_process_pending(void);

/* Statistics */
uint32_t atmosphere_atmospheric_scattering_get_count(void);
size_t atmosphere_atmospheric_scattering_get_memory_usage(void);
void atmosphere_atmospheric_scattering_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_ATMOSPHERIC_SCATTERING_H */
