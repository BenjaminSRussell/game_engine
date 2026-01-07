/*
 * anisotropy.h
 * Anisotropic highlights
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_ANISOTROPY_H
#define SHADING_ANISOTROPY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_anisotropy_handle {
    uint32_t id;
} shading_anisotropy_handle_t;

typedef struct shading_anisotropy_desc {
    uint32_t flags;
    void* user_data;
} shading_anisotropy_desc_t;

typedef struct shading_anisotropy_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_anisotropy_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_anisotropy_init(void);
void shading_anisotropy_shutdown(void);

/* Lifecycle */
int shading_anisotropy_create(shading_anisotropy_handle_t* out_handle, const shading_anisotropy_desc_t* desc);
void shading_anisotropy_destroy(shading_anisotropy_handle_t handle);

/* Operations */
int shading_anisotropy_update(shading_anisotropy_handle_t handle, const void* data, size_t size);
bool shading_anisotropy_is_valid(shading_anisotropy_handle_t handle);
int shading_anisotropy_get_info(shading_anisotropy_handle_t handle, shading_anisotropy_info_t* out_info);
void shading_anisotropy_mark_dirty(shading_anisotropy_handle_t handle);
int shading_anisotropy_process_pending(void);

/* Statistics */
uint32_t shading_anisotropy_get_count(void);
size_t shading_anisotropy_get_memory_usage(void);
void shading_anisotropy_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_ANISOTROPY_H */
