/*
 * hit_lighting.h
 * Hit point lighting
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_HIT_LIGHTING_H
#define LUMEN_HIT_LIGHTING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_hit_lighting_handle {
    uint32_t id;
} lumen_hit_lighting_handle_t;

typedef struct lumen_hit_lighting_desc {
    uint32_t flags;
    void* user_data;
} lumen_hit_lighting_desc_t;

typedef struct lumen_hit_lighting_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_hit_lighting_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_hit_lighting_init(void);
void lumen_hit_lighting_shutdown(void);

/* Lifecycle */
int lumen_hit_lighting_create(lumen_hit_lighting_handle_t* out_handle, const lumen_hit_lighting_desc_t* desc);
void lumen_hit_lighting_destroy(lumen_hit_lighting_handle_t handle);

/* Operations */
int lumen_hit_lighting_update(lumen_hit_lighting_handle_t handle, const void* data, size_t size);
bool lumen_hit_lighting_is_valid(lumen_hit_lighting_handle_t handle);
int lumen_hit_lighting_get_info(lumen_hit_lighting_handle_t handle, lumen_hit_lighting_info_t* out_info);
void lumen_hit_lighting_mark_dirty(lumen_hit_lighting_handle_t handle);
int lumen_hit_lighting_process_pending(void);

/* Statistics */
uint32_t lumen_hit_lighting_get_count(void);
size_t lumen_hit_lighting_get_memory_usage(void);
void lumen_hit_lighting_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_HIT_LIGHTING_H */
