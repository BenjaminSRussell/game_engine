/*
 * texture_transform.h
 * UV transform parameters
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_TEXTURE_TRANSFORM_H
#define MATERIALS_TEXTURE_TRANSFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_texture_transform_handle {
    uint32_t id;
} materials_texture_transform_handle_t;

typedef struct materials_texture_transform_desc {
    uint32_t flags;
    void* user_data;
} materials_texture_transform_desc_t;

typedef struct materials_texture_transform_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_texture_transform_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_texture_transform_init(void);
void materials_texture_transform_shutdown(void);

/* Lifecycle */
int materials_texture_transform_create(materials_texture_transform_handle_t* out_handle, const materials_texture_transform_desc_t* desc);
void materials_texture_transform_destroy(materials_texture_transform_handle_t handle);

/* Operations */
int materials_texture_transform_update(materials_texture_transform_handle_t handle, const void* data, size_t size);
bool materials_texture_transform_is_valid(materials_texture_transform_handle_t handle);
int materials_texture_transform_get_info(materials_texture_transform_handle_t handle, materials_texture_transform_info_t* out_info);
void materials_texture_transform_mark_dirty(materials_texture_transform_handle_t handle);
int materials_texture_transform_process_pending(void);

/* Statistics */
uint32_t materials_texture_transform_get_count(void);
size_t materials_texture_transform_get_memory_usage(void);
void materials_texture_transform_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_TEXTURE_TRANSFORM_H */
