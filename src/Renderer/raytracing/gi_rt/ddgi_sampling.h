/*
 * ddgi_sampling.h
 * DDGI irradiance sampling
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_DDGI_SAMPLING_H
#define RAYTRACING_DDGI_SAMPLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_ddgi_sampling_handle {
    uint32_t id;
} raytracing_ddgi_sampling_handle_t;

typedef struct raytracing_ddgi_sampling_desc {
    uint32_t flags;
    void* user_data;
} raytracing_ddgi_sampling_desc_t;

typedef struct raytracing_ddgi_sampling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_ddgi_sampling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_ddgi_sampling_init(void);
void raytracing_ddgi_sampling_shutdown(void);

/* Lifecycle */
int raytracing_ddgi_sampling_create(raytracing_ddgi_sampling_handle_t* out_handle, const raytracing_ddgi_sampling_desc_t* desc);
void raytracing_ddgi_sampling_destroy(raytracing_ddgi_sampling_handle_t handle);

/* Operations */
int raytracing_ddgi_sampling_update(raytracing_ddgi_sampling_handle_t handle, const void* data, size_t size);
bool raytracing_ddgi_sampling_is_valid(raytracing_ddgi_sampling_handle_t handle);
int raytracing_ddgi_sampling_get_info(raytracing_ddgi_sampling_handle_t handle, raytracing_ddgi_sampling_info_t* out_info);
void raytracing_ddgi_sampling_mark_dirty(raytracing_ddgi_sampling_handle_t handle);
int raytracing_ddgi_sampling_process_pending(void);

/* Statistics */
uint32_t raytracing_ddgi_sampling_get_count(void);
size_t raytracing_ddgi_sampling_get_memory_usage(void);
void raytracing_ddgi_sampling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_DDGI_SAMPLING_H */
