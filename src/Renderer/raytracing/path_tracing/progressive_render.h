/*
 * progressive_render.h
 * Progressive refinement
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_PROGRESSIVE_RENDER_H
#define RAYTRACING_PROGRESSIVE_RENDER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_progressive_render_handle {
    uint32_t id;
} raytracing_progressive_render_handle_t;

typedef struct raytracing_progressive_render_desc {
    uint32_t flags;
    void* user_data;
} raytracing_progressive_render_desc_t;

typedef struct raytracing_progressive_render_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_progressive_render_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_progressive_render_init(void);
void raytracing_progressive_render_shutdown(void);

/* Lifecycle */
int raytracing_progressive_render_create(raytracing_progressive_render_handle_t* out_handle, const raytracing_progressive_render_desc_t* desc);
void raytracing_progressive_render_destroy(raytracing_progressive_render_handle_t handle);

/* Operations */
int raytracing_progressive_render_update(raytracing_progressive_render_handle_t handle, const void* data, size_t size);
bool raytracing_progressive_render_is_valid(raytracing_progressive_render_handle_t handle);
int raytracing_progressive_render_get_info(raytracing_progressive_render_handle_t handle, raytracing_progressive_render_info_t* out_info);
void raytracing_progressive_render_mark_dirty(raytracing_progressive_render_handle_t handle);
int raytracing_progressive_render_process_pending(void);

/* Statistics */
uint32_t raytracing_progressive_render_get_count(void);
size_t raytracing_progressive_render_get_memory_usage(void);
void raytracing_progressive_render_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_PROGRESSIVE_RENDER_H */
