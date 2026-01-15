/*
 * decal_streaming.h
 * Decal texture streaming
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_DECAL_STREAMING_H
#define EFFECTS_DECAL_STREAMING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_decal_streaming_handle {
    uint32_t id;
} effects_decal_streaming_handle_t;

typedef struct effects_decal_streaming_desc {
    uint32_t flags;
    void* user_data;
} effects_decal_streaming_desc_t;

typedef struct effects_decal_streaming_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_decal_streaming_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_decal_streaming_init(void);
void effects_decal_streaming_shutdown(void);

/* Lifecycle */
int effects_decal_streaming_create(effects_decal_streaming_handle_t* out_handle, const effects_decal_streaming_desc_t* desc);
void effects_decal_streaming_destroy(effects_decal_streaming_handle_t handle);

/* Operations */
int effects_decal_streaming_update(effects_decal_streaming_handle_t handle, const void* data, size_t size);
bool effects_decal_streaming_is_valid(effects_decal_streaming_handle_t handle);
int effects_decal_streaming_get_info(effects_decal_streaming_handle_t handle, effects_decal_streaming_info_t* out_info);
void effects_decal_streaming_mark_dirty(effects_decal_streaming_handle_t handle);
int effects_decal_streaming_process_pending(void);

/* Statistics */
uint32_t effects_decal_streaming_get_count(void);
size_t effects_decal_streaming_get_memory_usage(void);
void effects_decal_streaming_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_DECAL_STREAMING_H */
