/*
 * multi_draw_indirect.h
 * Multi-draw indirect
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_MULTI_DRAW_INDIRECT_H
#define RENDERING_MULTI_DRAW_INDIRECT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_multi_draw_indirect_handle {
    uint32_t id;
} rendering_multi_draw_indirect_handle_t;

typedef struct rendering_multi_draw_indirect_desc {
    uint32_t flags;
    void* user_data;
} rendering_multi_draw_indirect_desc_t;

typedef struct rendering_multi_draw_indirect_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_multi_draw_indirect_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_multi_draw_indirect_init(void);
void rendering_multi_draw_indirect_shutdown(void);

/* Lifecycle */
int rendering_multi_draw_indirect_create(rendering_multi_draw_indirect_handle_t* out_handle, const rendering_multi_draw_indirect_desc_t* desc);
void rendering_multi_draw_indirect_destroy(rendering_multi_draw_indirect_handle_t handle);

/* Operations */
int rendering_multi_draw_indirect_update(rendering_multi_draw_indirect_handle_t handle, const void* data, size_t size);
bool rendering_multi_draw_indirect_is_valid(rendering_multi_draw_indirect_handle_t handle);
int rendering_multi_draw_indirect_get_info(rendering_multi_draw_indirect_handle_t handle, rendering_multi_draw_indirect_info_t* out_info);
void rendering_multi_draw_indirect_mark_dirty(rendering_multi_draw_indirect_handle_t handle);
int rendering_multi_draw_indirect_process_pending(void);

/* Statistics */
uint32_t rendering_multi_draw_indirect_get_count(void);
size_t rendering_multi_draw_indirect_get_memory_usage(void);
void rendering_multi_draw_indirect_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_MULTI_DRAW_INDIRECT_H */
