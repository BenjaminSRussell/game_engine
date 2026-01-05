/*
 * shadow_atlas.h
 * Shadow map atlas management
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SHADOW_ATLAS_H
#define LIGHTING_SHADOW_ATLAS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_shadow_atlas_handle {
    uint32_t id;
} lighting_shadow_atlas_handle_t;

typedef struct lighting_shadow_atlas_desc {
    uint32_t flags;
    void* user_data;
} lighting_shadow_atlas_desc_t;

typedef struct lighting_shadow_atlas_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_shadow_atlas_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_shadow_atlas_init(void);
void lighting_shadow_atlas_shutdown(void);

/* Lifecycle */
int lighting_shadow_atlas_create(lighting_shadow_atlas_handle_t* out_handle, const lighting_shadow_atlas_desc_t* desc);
void lighting_shadow_atlas_destroy(lighting_shadow_atlas_handle_t handle);

/* Operations */
int lighting_shadow_atlas_update(lighting_shadow_atlas_handle_t handle, const void* data, size_t size);
bool lighting_shadow_atlas_is_valid(lighting_shadow_atlas_handle_t handle);
int lighting_shadow_atlas_get_info(lighting_shadow_atlas_handle_t handle, lighting_shadow_atlas_info_t* out_info);
void lighting_shadow_atlas_mark_dirty(lighting_shadow_atlas_handle_t handle);
int lighting_shadow_atlas_process_pending(void);

/* Statistics */
uint32_t lighting_shadow_atlas_get_count(void);
size_t lighting_shadow_atlas_get_memory_usage(void);
void lighting_shadow_atlas_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SHADOW_ATLAS_H */
