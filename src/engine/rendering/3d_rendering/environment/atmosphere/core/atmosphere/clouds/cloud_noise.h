/*
 * cloud_noise.h
 * Cloud noise generation
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_CLOUD_NOISE_H
#define ATMOSPHERE_CLOUD_NOISE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_cloud_noise_handle {
    uint32_t id;
} atmosphere_cloud_noise_handle_t;

typedef struct atmosphere_cloud_noise_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_cloud_noise_desc_t;

typedef struct atmosphere_cloud_noise_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_cloud_noise_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_cloud_noise_init(void);
void atmosphere_cloud_noise_shutdown(void);

/* Lifecycle */
int atmosphere_cloud_noise_create(atmosphere_cloud_noise_handle_t* out_handle, const atmosphere_cloud_noise_desc_t* desc);
void atmosphere_cloud_noise_destroy(atmosphere_cloud_noise_handle_t handle);

/* Operations */
int atmosphere_cloud_noise_update(atmosphere_cloud_noise_handle_t handle, const void* data, size_t size);
bool atmosphere_cloud_noise_is_valid(atmosphere_cloud_noise_handle_t handle);
int atmosphere_cloud_noise_get_info(atmosphere_cloud_noise_handle_t handle, atmosphere_cloud_noise_info_t* out_info);
void atmosphere_cloud_noise_mark_dirty(atmosphere_cloud_noise_handle_t handle);
int atmosphere_cloud_noise_process_pending(void);

/* Statistics */
uint32_t atmosphere_cloud_noise_get_count(void);
size_t atmosphere_cloud_noise_get_memory_usage(void);
void atmosphere_cloud_noise_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_CLOUD_NOISE_H */
