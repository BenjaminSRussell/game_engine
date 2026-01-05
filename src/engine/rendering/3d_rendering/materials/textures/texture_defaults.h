/*
 * texture_defaults.h
 * Default texture fallbacks
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_TEXTURE_DEFAULTS_H
#define MATERIALS_TEXTURE_DEFAULTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_texture_defaults_handle {
    uint32_t id;
} materials_texture_defaults_handle_t;

typedef struct materials_texture_defaults_desc {
    uint32_t flags;
    void* user_data;
} materials_texture_defaults_desc_t;

typedef struct materials_texture_defaults_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_texture_defaults_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_texture_defaults_init(void);
void materials_texture_defaults_shutdown(void);

/* Lifecycle */
int materials_texture_defaults_create(materials_texture_defaults_handle_t* out_handle, const materials_texture_defaults_desc_t* desc);
void materials_texture_defaults_destroy(materials_texture_defaults_handle_t handle);

/* Operations */
int materials_texture_defaults_update(materials_texture_defaults_handle_t handle, const void* data, size_t size);
bool materials_texture_defaults_is_valid(materials_texture_defaults_handle_t handle);
int materials_texture_defaults_get_info(materials_texture_defaults_handle_t handle, materials_texture_defaults_info_t* out_info);
void materials_texture_defaults_mark_dirty(materials_texture_defaults_handle_t handle);
int materials_texture_defaults_process_pending(void);

/* Statistics */
uint32_t materials_texture_defaults_get_count(void);
size_t materials_texture_defaults_get_memory_usage(void);
void materials_texture_defaults_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_TEXTURE_DEFAULTS_H */
