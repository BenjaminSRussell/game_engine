/*
 * volumetric_fog.h
 * Volumetric fog rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_VOLUMETRIC_FOG_H
#define LIGHTING_VOLUMETRIC_FOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_volumetric_fog_handle {
    uint32_t id;
} lighting_volumetric_fog_handle_t;

typedef struct lighting_volumetric_fog_desc {
    uint32_t flags;
    void* user_data;
} lighting_volumetric_fog_desc_t;

typedef struct lighting_volumetric_fog_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_volumetric_fog_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_volumetric_fog_init(void);
void lighting_volumetric_fog_shutdown(void);

/* Lifecycle */
int lighting_volumetric_fog_create(lighting_volumetric_fog_handle_t* out_handle, const lighting_volumetric_fog_desc_t* desc);
void lighting_volumetric_fog_destroy(lighting_volumetric_fog_handle_t handle);

/* Operations */
int lighting_volumetric_fog_update(lighting_volumetric_fog_handle_t handle, const void* data, size_t size);
bool lighting_volumetric_fog_is_valid(lighting_volumetric_fog_handle_t handle);
int lighting_volumetric_fog_get_info(lighting_volumetric_fog_handle_t handle, lighting_volumetric_fog_info_t* out_info);
void lighting_volumetric_fog_mark_dirty(lighting_volumetric_fog_handle_t handle);
int lighting_volumetric_fog_process_pending(void);

/* Statistics */
uint32_t lighting_volumetric_fog_get_count(void);
size_t lighting_volumetric_fog_get_memory_usage(void);
void lighting_volumetric_fog_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_VOLUMETRIC_FOG_H */
