/*
 * lightmap_streaming.h
 * Lightmap streaming
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_LIGHTMAP_STREAMING_H
#define LIGHTING_LIGHTMAP_STREAMING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_lightmap_streaming_handle {
    uint32_t id;
} lighting_lightmap_streaming_handle_t;

typedef struct lighting_lightmap_streaming_desc {
    uint32_t flags;
    void* user_data;
} lighting_lightmap_streaming_desc_t;

typedef struct lighting_lightmap_streaming_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_lightmap_streaming_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_lightmap_streaming_init(void);
void lighting_lightmap_streaming_shutdown(void);

/* Lifecycle */
int lighting_lightmap_streaming_create(lighting_lightmap_streaming_handle_t* out_handle, const lighting_lightmap_streaming_desc_t* desc);
void lighting_lightmap_streaming_destroy(lighting_lightmap_streaming_handle_t handle);

/* Operations */
int lighting_lightmap_streaming_update(lighting_lightmap_streaming_handle_t handle, const void* data, size_t size);
bool lighting_lightmap_streaming_is_valid(lighting_lightmap_streaming_handle_t handle);
int lighting_lightmap_streaming_get_info(lighting_lightmap_streaming_handle_t handle, lighting_lightmap_streaming_info_t* out_info);
void lighting_lightmap_streaming_mark_dirty(lighting_lightmap_streaming_handle_t handle);
int lighting_lightmap_streaming_process_pending(void);

/* Statistics */
uint32_t lighting_lightmap_streaming_get_count(void);
size_t lighting_lightmap_streaming_get_memory_usage(void);
void lighting_lightmap_streaming_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_LIGHTMAP_STREAMING_H */
