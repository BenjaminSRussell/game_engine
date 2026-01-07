/*
 * relax_denoiser.h
 * ReLAX denoiser
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_RELAX_DENOISER_H
#define RAYTRACING_RELAX_DENOISER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_relax_denoiser_handle {
    uint32_t id;
} raytracing_relax_denoiser_handle_t;

typedef struct raytracing_relax_denoiser_desc {
    uint32_t flags;
    void* user_data;
} raytracing_relax_denoiser_desc_t;

typedef struct raytracing_relax_denoiser_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_relax_denoiser_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_relax_denoiser_init(void);
void raytracing_relax_denoiser_shutdown(void);

/* Lifecycle */
int raytracing_relax_denoiser_create(raytracing_relax_denoiser_handle_t* out_handle, const raytracing_relax_denoiser_desc_t* desc);
void raytracing_relax_denoiser_destroy(raytracing_relax_denoiser_handle_t handle);

/* Operations */
int raytracing_relax_denoiser_update(raytracing_relax_denoiser_handle_t handle, const void* data, size_t size);
bool raytracing_relax_denoiser_is_valid(raytracing_relax_denoiser_handle_t handle);
int raytracing_relax_denoiser_get_info(raytracing_relax_denoiser_handle_t handle, raytracing_relax_denoiser_info_t* out_info);
void raytracing_relax_denoiser_mark_dirty(raytracing_relax_denoiser_handle_t handle);
int raytracing_relax_denoiser_process_pending(void);

/* Statistics */
uint32_t raytracing_relax_denoiser_get_count(void);
size_t raytracing_relax_denoiser_get_memory_usage(void);
void raytracing_relax_denoiser_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_RELAX_DENOISER_H */
