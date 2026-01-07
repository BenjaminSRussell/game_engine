/*
 * hair_shading_render.h
 * Hair shading
 *
 * Part of the Hair System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef HAIR_SYSTEM_HAIR_SHADING_RENDER_H
#define HAIR_SYSTEM_HAIR_SHADING_RENDER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct hair_system_hair_shading_render_handle {
    uint32_t id;
} hair_system_hair_shading_render_handle_t;

typedef struct hair_system_hair_shading_render_desc {
    uint32_t flags;
    void* user_data;
} hair_system_hair_shading_render_desc_t;

typedef struct hair_system_hair_shading_render_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} hair_system_hair_shading_render_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int hair_system_hair_shading_render_init(void);
void hair_system_hair_shading_render_shutdown(void);

/* Lifecycle */
int hair_system_hair_shading_render_create(hair_system_hair_shading_render_handle_t* out_handle, const hair_system_hair_shading_render_desc_t* desc);
void hair_system_hair_shading_render_destroy(hair_system_hair_shading_render_handle_t handle);

/* Operations */
int hair_system_hair_shading_render_update(hair_system_hair_shading_render_handle_t handle, const void* data, size_t size);
bool hair_system_hair_shading_render_is_valid(hair_system_hair_shading_render_handle_t handle);
int hair_system_hair_shading_render_get_info(hair_system_hair_shading_render_handle_t handle, hair_system_hair_shading_render_info_t* out_info);
void hair_system_hair_shading_render_mark_dirty(hair_system_hair_shading_render_handle_t handle);
int hair_system_hair_shading_render_process_pending(void);

/* Statistics */
uint32_t hair_system_hair_shading_render_get_count(void);
size_t hair_system_hair_shading_render_get_memory_usage(void);
void hair_system_hair_shading_render_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* HAIR_SYSTEM_HAIR_SHADING_RENDER_H */
