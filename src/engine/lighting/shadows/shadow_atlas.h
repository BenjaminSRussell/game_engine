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

typedef enum lighting_shadow_format {
    LIGHTING_SHADOW_FORMAT_DEPTH32,     /* Standard depth-only shadow map */
    LIGHTING_SHADOW_FORMAT_RGBA16F,     /* VSM/EVSM/Moment shadow map */
    LIGHTING_SHADOW_FORMAT_KIND_COUNT
} lighting_shadow_format_t;

typedef enum lighting_atlas_update_flags {
    LIGHTING_ATLAS_UPDATE_NONE = 0,
    LIGHTING_ATLAS_UPDATE_RESIZE = 1 << 0,
    LIGHTING_ATLAS_UPDATE_DEFRAG = 1 << 1,
    LIGHTING_ATLAS_UPDATE_CLEAR = 1 << 2
} lighting_atlas_update_flags_t;

typedef struct lighting_shadow_atlas_desc {
    uint32_t width;
    uint32_t height;
    lighting_shadow_format_t format;
    uint32_t cascade_count;     /* Max cascades to support (typically 4) */
    bool enable_mipmaps;        /* For VSM/Min-Max generation */
    uint32_t flags;
    void* user_data;
} lighting_shadow_atlas_desc_t;

typedef struct lighting_shadow_atlas_info {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    lighting_shadow_format_t format;
    uint32_t active_allocations;
    uint32_t used_memory_bytes;
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
int lighting_shadow_atlas_resize(lighting_shadow_atlas_handle_t handle, uint32_t new_width, uint32_t new_height);
int lighting_shadow_atlas_defragment(lighting_shadow_atlas_handle_t handle);
int lighting_shadow_atlas_clear_region(lighting_shadow_atlas_handle_t handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height, float clear_value);

bool lighting_shadow_atlas_is_valid(lighting_shadow_atlas_handle_t handle);
int lighting_shadow_atlas_get_info(lighting_shadow_atlas_handle_t handle, lighting_shadow_atlas_info_t* out_info);
void lighting_shadow_atlas_mark_dirty(lighting_shadow_atlas_handle_t handle);
int lighting_shadow_atlas_process_pending(void);

/* Statistics */
uint32_t lighting_shadow_atlas_get_count(void);
size_t lighting_shadow_atlas_get_memory_usage(void);
void lighting_shadow_atlas_debug_print(void);

/* Shadow slot allocation */
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t layer;         /* For array textures */
    uint32_t mip_level;     /* Base mip level */
    float uv_min[2];
    float uv_max[2];
} atlas_allocation_t;

int lighting_shadow_atlas_allocate_slot(lighting_shadow_atlas_handle_t handle, 
                                        uint32_t light_id, 
                                        uint32_t shadow_map_size,
                                        atlas_allocation_t* out_allocation);
void lighting_shadow_atlas_free_slot(lighting_shadow_atlas_handle_t handle, uint32_t light_id);

/* Atlas access */
typedef struct { struct { uint32_t handle; } id; } texture_handle_t;
texture_handle_t lighting_shadow_atlas_get_texture(void);
uint32_t lighting_shadow_atlas_get_size(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SHADOW_ATLAS_H */
