/*
 * svgf_denoiser.h
 * SVGF denoiser
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_SVGF_DENOISER_H
#define RAYTRACING_SVGF_DENOISER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_svgf_denoiser_handle {
    uint32_t id;
} raytracing_svgf_denoiser_handle_t;

typedef struct raytracing_svgf_denoiser_desc {
    uint32_t flags;
    void* user_data;
} raytracing_svgf_denoiser_desc_t;

typedef struct raytracing_svgf_denoiser_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_svgf_denoiser_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_svgf_denoiser_init(void);
void raytracing_svgf_denoiser_shutdown(void);

/* Lifecycle */
int raytracing_svgf_denoiser_create(raytracing_svgf_denoiser_handle_t* out_handle, const raytracing_svgf_denoiser_desc_t* desc);
void raytracing_svgf_denoiser_destroy(raytracing_svgf_denoiser_handle_t handle);

/* Operations */
int raytracing_svgf_denoiser_update(raytracing_svgf_denoiser_handle_t handle, const void* data, size_t size);
bool raytracing_svgf_denoiser_is_valid(raytracing_svgf_denoiser_handle_t handle);
int raytracing_svgf_denoiser_get_info(raytracing_svgf_denoiser_handle_t handle, raytracing_svgf_denoiser_info_t* out_info);
void raytracing_svgf_denoiser_mark_dirty(raytracing_svgf_denoiser_handle_t handle);
int raytracing_svgf_denoiser_process_pending(void);

/* Statistics */
uint32_t raytracing_svgf_denoiser_get_count(void);
size_t raytracing_svgf_denoiser_get_memory_usage(void);
void raytracing_svgf_denoiser_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_SVGF_DENOISER_H */
