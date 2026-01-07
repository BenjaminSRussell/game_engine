/*
 * directional_lightmaps.h
 * Directional lightmap data
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_DIRECTIONAL_LIGHTMAPS_H
#define LIGHTING_DIRECTIONAL_LIGHTMAPS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "include/math/vec3.h"
#include "include/math/vec2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {
    Vec3 basis_irradiance[3];
} directional_texel_t;

typedef struct lighting_directional_lightmaps_handle {
    uint32_t id;
} lighting_directional_lightmaps_handle_t;

typedef struct lighting_directional_lightmaps_desc {
    uint32_t flags;
    void* user_data;
} lighting_directional_lightmaps_desc_t;

typedef struct lighting_directional_lightmaps_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_directional_lightmaps_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_directional_lightmaps_init(void);
void lighting_directional_lightmaps_shutdown(void);

/* Lifecycle */
int lighting_directional_lightmaps_create(lighting_directional_lightmaps_handle_t* out_handle, const lighting_directional_lightmaps_desc_t* desc);
void lighting_directional_lightmaps_destroy(lighting_directional_lightmaps_handle_t handle);

/* Operations */
int lighting_directional_lightmap_encode(Vec3 color, Vec3 direction, Vec3 normal, directional_texel_t* out_texel);
Vec3 lighting_directional_lightmap_sample(const directional_texel_t* texel, Vec3 normal, Vec3 tangent, Vec3 bitangent, Vec3 surface_normal);
int lighting_directional_lightmaps_update(lighting_directional_lightmaps_handle_t handle, const void* data, size_t size);
bool lighting_directional_lightmaps_is_valid(lighting_directional_lightmaps_handle_t handle);
int lighting_directional_lightmaps_get_info(lighting_directional_lightmaps_handle_t handle, lighting_directional_lightmaps_info_t* out_info);
void lighting_directional_lightmaps_mark_dirty(lighting_directional_lightmaps_handle_t handle);
int lighting_directional_lightmaps_process_pending(void);

/* Statistics */
uint32_t lighting_directional_lightmaps_get_count(void);
size_t lighting_directional_lightmaps_get_memory_usage(void);
void lighting_directional_lightmaps_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_DIRECTIONAL_LIGHTMAPS_H */
