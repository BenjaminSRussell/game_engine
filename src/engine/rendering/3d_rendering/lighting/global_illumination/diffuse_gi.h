/*
 * diffuse_gi.h
 * Diffuse global illumination
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_DIFFUSE_GI_H
#define LIGHTING_DIFFUSE_GI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_diffuse_gi_handle {
    uint32_t id;
} lighting_diffuse_gi_handle_t;

typedef struct lighting_diffuse_gi_desc {
    uint32_t flags;
    void* user_data;
} lighting_diffuse_gi_desc_t;

typedef struct lighting_diffuse_gi_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_diffuse_gi_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_diffuse_gi_init(void);
void lighting_diffuse_gi_shutdown(void);

/* Lifecycle */
int lighting_diffuse_gi_create(lighting_diffuse_gi_handle_t* out_handle, const lighting_diffuse_gi_desc_t* desc);
void lighting_diffuse_gi_destroy(lighting_diffuse_gi_handle_t handle);

/* Operations */
int lighting_diffuse_gi_update(lighting_diffuse_gi_handle_t handle, const void* data, size_t size);
bool lighting_diffuse_gi_is_valid(lighting_diffuse_gi_handle_t handle);
int lighting_diffuse_gi_get_info(lighting_diffuse_gi_handle_t handle, lighting_diffuse_gi_info_t* out_info);
void lighting_diffuse_gi_mark_dirty(lighting_diffuse_gi_handle_t handle);
int lighting_diffuse_gi_process_pending(void);

/* Statistics */
uint32_t lighting_diffuse_gi_get_count(void);
size_t lighting_diffuse_gi_get_memory_usage(void);
void lighting_diffuse_gi_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_DIFFUSE_GI_H */
