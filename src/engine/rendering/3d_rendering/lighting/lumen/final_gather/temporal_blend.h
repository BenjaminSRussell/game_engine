/*
 * temporal_blend.h
 * Temporal blending
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_TEMPORAL_BLEND_H
#define LUMEN_TEMPORAL_BLEND_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_temporal_blend_handle {
    uint32_t id;
} lumen_temporal_blend_handle_t;

typedef struct lumen_temporal_blend_desc {
    uint32_t flags;
    void* user_data;
} lumen_temporal_blend_desc_t;

typedef struct lumen_temporal_blend_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_temporal_blend_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_temporal_blend_init(void);
void lumen_temporal_blend_shutdown(void);

/* Lifecycle */
int lumen_temporal_blend_create(lumen_temporal_blend_handle_t* out_handle, const lumen_temporal_blend_desc_t* desc);
void lumen_temporal_blend_destroy(lumen_temporal_blend_handle_t handle);

/* Operations */
int lumen_temporal_blend_update(lumen_temporal_blend_handle_t handle, const void* data, size_t size);
bool lumen_temporal_blend_is_valid(lumen_temporal_blend_handle_t handle);
int lumen_temporal_blend_get_info(lumen_temporal_blend_handle_t handle, lumen_temporal_blend_info_t* out_info);
void lumen_temporal_blend_mark_dirty(lumen_temporal_blend_handle_t handle);
int lumen_temporal_blend_process_pending(void);

/* Statistics */
uint32_t lumen_temporal_blend_get_count(void);
size_t lumen_temporal_blend_get_memory_usage(void);
void lumen_temporal_blend_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_TEMPORAL_BLEND_H */
