/*
 * reference_renderer.h
 * Reference path tracer
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_REFERENCE_RENDERER_H
#define RAYTRACING_REFERENCE_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_reference_renderer_handle {
    uint32_t id;
} raytracing_reference_renderer_handle_t;

typedef struct raytracing_reference_renderer_desc {
    uint32_t flags;
    void* user_data;
} raytracing_reference_renderer_desc_t;

typedef struct raytracing_reference_renderer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_reference_renderer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_reference_renderer_init(void);
void raytracing_reference_renderer_shutdown(void);

/* Lifecycle */
int raytracing_reference_renderer_create(raytracing_reference_renderer_handle_t* out_handle, const raytracing_reference_renderer_desc_t* desc);
void raytracing_reference_renderer_destroy(raytracing_reference_renderer_handle_t handle);

/* Operations */
int raytracing_reference_renderer_update(raytracing_reference_renderer_handle_t handle, const void* data, size_t size);
bool raytracing_reference_renderer_is_valid(raytracing_reference_renderer_handle_t handle);
int raytracing_reference_renderer_get_info(raytracing_reference_renderer_handle_t handle, raytracing_reference_renderer_info_t* out_info);
void raytracing_reference_renderer_mark_dirty(raytracing_reference_renderer_handle_t handle);
int raytracing_reference_renderer_process_pending(void);

/* Statistics */
uint32_t raytracing_reference_renderer_get_count(void);
size_t raytracing_reference_renderer_get_memory_usage(void);
void raytracing_reference_renderer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_REFERENCE_RENDERER_H */
