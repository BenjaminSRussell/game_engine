/*
 * detail_textures.h
 * Detail texture tiling
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_DETAIL_TEXTURES_H
#define MATERIALS_DETAIL_TEXTURES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_detail_textures_handle {
    uint32_t id;
} materials_detail_textures_handle_t;

typedef struct materials_detail_textures_desc {
    uint32_t flags;
    void* user_data;
} materials_detail_textures_desc_t;

typedef struct materials_detail_textures_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_detail_textures_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_detail_textures_init(void);
void materials_detail_textures_shutdown(void);

/* Lifecycle */
int materials_detail_textures_create(materials_detail_textures_handle_t* out_handle, const materials_detail_textures_desc_t* desc);
void materials_detail_textures_destroy(materials_detail_textures_handle_t handle);

/* Operations */
int materials_detail_textures_update(materials_detail_textures_handle_t handle, const void* data, size_t size);
bool materials_detail_textures_is_valid(materials_detail_textures_handle_t handle);
int materials_detail_textures_get_info(materials_detail_textures_handle_t handle, materials_detail_textures_info_t* out_info);
void materials_detail_textures_mark_dirty(materials_detail_textures_handle_t handle);
int materials_detail_textures_process_pending(void);

/* Statistics */
uint32_t materials_detail_textures_get_count(void);
size_t materials_detail_textures_get_memory_usage(void);
void materials_detail_textures_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_DETAIL_TEXTURES_H */
