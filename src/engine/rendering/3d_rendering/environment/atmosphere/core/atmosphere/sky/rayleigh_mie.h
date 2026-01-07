/*
 * rayleigh_mie.h
 * Rayleigh/Mie scattering
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_RAYLEIGH_MIE_H
#define ATMOSPHERE_RAYLEIGH_MIE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_rayleigh_mie_handle {
    uint32_t id;
} atmosphere_rayleigh_mie_handle_t;

typedef struct atmosphere_rayleigh_mie_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_rayleigh_mie_desc_t;

typedef struct atmosphere_rayleigh_mie_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_rayleigh_mie_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_rayleigh_mie_init(void);
void atmosphere_rayleigh_mie_shutdown(void);

/* Lifecycle */
int atmosphere_rayleigh_mie_create(atmosphere_rayleigh_mie_handle_t* out_handle, const atmosphere_rayleigh_mie_desc_t* desc);
void atmosphere_rayleigh_mie_destroy(atmosphere_rayleigh_mie_handle_t handle);

/* Operations */
int atmosphere_rayleigh_mie_update(atmosphere_rayleigh_mie_handle_t handle, const void* data, size_t size);
bool atmosphere_rayleigh_mie_is_valid(atmosphere_rayleigh_mie_handle_t handle);
int atmosphere_rayleigh_mie_get_info(atmosphere_rayleigh_mie_handle_t handle, atmosphere_rayleigh_mie_info_t* out_info);
void atmosphere_rayleigh_mie_mark_dirty(atmosphere_rayleigh_mie_handle_t handle);
int atmosphere_rayleigh_mie_process_pending(void);

/* Statistics */
uint32_t atmosphere_rayleigh_mie_get_count(void);
size_t atmosphere_rayleigh_mie_get_memory_usage(void);
void atmosphere_rayleigh_mie_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_RAYLEIGH_MIE_H */
