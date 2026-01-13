/*
 * hair_antialiasing.h
 * Hair edge AA
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_HAIR_ANTIALIASING_H
#define SHADING_HAIR_ANTIALIASING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_hair_antialiasing_handle {
    uint32_t id;
} shading_hair_antialiasing_handle_t;

typedef struct shading_hair_antialiasing_desc {
    uint32_t flags;
    void* user_data;
} shading_hair_antialiasing_desc_t;

typedef struct shading_hair_antialiasing_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_hair_antialiasing_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_hair_antialiasing_init(void);
void shading_hair_antialiasing_shutdown(void);

/* Lifecycle */
int shading_hair_antialiasing_create(shading_hair_antialiasing_handle_t* out_handle, const shading_hair_antialiasing_desc_t* desc);
void shading_hair_antialiasing_destroy(shading_hair_antialiasing_handle_t handle);

/* Operations */
int shading_hair_antialiasing_update(shading_hair_antialiasing_handle_t handle, const void* data, size_t size);
bool shading_hair_antialiasing_is_valid(shading_hair_antialiasing_handle_t handle);
int shading_hair_antialiasing_get_info(shading_hair_antialiasing_handle_t handle, shading_hair_antialiasing_info_t* out_info);
void shading_hair_antialiasing_mark_dirty(shading_hair_antialiasing_handle_t handle);
int shading_hair_antialiasing_process_pending(void);

/* Statistics */
uint32_t shading_hair_antialiasing_get_count(void);
size_t shading_hair_antialiasing_get_memory_usage(void);
void shading_hair_antialiasing_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_HAIR_ANTIALIASING_H */
