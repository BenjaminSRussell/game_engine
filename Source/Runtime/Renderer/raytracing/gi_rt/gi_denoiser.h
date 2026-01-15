/*
 * gi_denoiser.h
 * GI temporal denoiser
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_GI_DENOISER_H
#define RAYTRACING_GI_DENOISER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_gi_denoiser_handle {
    uint32_t id;
} raytracing_gi_denoiser_handle_t;

typedef struct raytracing_gi_denoiser_desc {
    uint32_t flags;
    void* user_data;
} raytracing_gi_denoiser_desc_t;

typedef struct raytracing_gi_denoiser_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_gi_denoiser_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_gi_denoiser_init(void);
void raytracing_gi_denoiser_shutdown(void);

/* Lifecycle */
int raytracing_gi_denoiser_create(raytracing_gi_denoiser_handle_t* out_handle, const raytracing_gi_denoiser_desc_t* desc);
void raytracing_gi_denoiser_destroy(raytracing_gi_denoiser_handle_t handle);

/* Operations */
int raytracing_gi_denoiser_update(raytracing_gi_denoiser_handle_t handle, const void* data, size_t size);
bool raytracing_gi_denoiser_is_valid(raytracing_gi_denoiser_handle_t handle);
int raytracing_gi_denoiser_get_info(raytracing_gi_denoiser_handle_t handle, raytracing_gi_denoiser_info_t* out_info);
void raytracing_gi_denoiser_mark_dirty(raytracing_gi_denoiser_handle_t handle);
int raytracing_gi_denoiser_process_pending(void);

/* Statistics */
uint32_t raytracing_gi_denoiser_get_count(void);
size_t raytracing_gi_denoiser_get_memory_usage(void);
void raytracing_gi_denoiser_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_GI_DENOISER_H */
