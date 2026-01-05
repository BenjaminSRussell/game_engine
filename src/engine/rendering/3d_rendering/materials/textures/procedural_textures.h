/*
 * procedural_textures.h
 * Procedural texture generation
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_PROCEDURAL_TEXTURES_H
#define MATERIALS_PROCEDURAL_TEXTURES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_procedural_textures_handle {
    uint32_t id;
} materials_procedural_textures_handle_t;

typedef struct materials_procedural_textures_desc {
    uint32_t flags;
    void* user_data;
} materials_procedural_textures_desc_t;

typedef struct materials_procedural_textures_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_procedural_textures_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_procedural_textures_init(void);
void materials_procedural_textures_shutdown(void);

/* Lifecycle */
int materials_procedural_textures_create(materials_procedural_textures_handle_t* out_handle, const materials_procedural_textures_desc_t* desc);
void materials_procedural_textures_destroy(materials_procedural_textures_handle_t handle);

/* Operations */
int materials_procedural_textures_update(materials_procedural_textures_handle_t handle, const void* data, size_t size);
bool materials_procedural_textures_is_valid(materials_procedural_textures_handle_t handle);
int materials_procedural_textures_get_info(materials_procedural_textures_handle_t handle, materials_procedural_textures_info_t* out_info);
void materials_procedural_textures_mark_dirty(materials_procedural_textures_handle_t handle);
int materials_procedural_textures_process_pending(void);

/* Statistics */
uint32_t materials_procedural_textures_get_count(void);
size_t materials_procedural_textures_get_memory_usage(void);
void materials_procedural_textures_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_PROCEDURAL_TEXTURES_H */
