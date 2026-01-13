/*
 * anisotropic_cloth.h
 * Anisotropic cloth
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_ANISOTROPIC_CLOTH_H
#define SHADING_ANISOTROPIC_CLOTH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_anisotropic_cloth_handle {
    uint32_t id;
} shading_anisotropic_cloth_handle_t;

typedef struct shading_anisotropic_cloth_desc {
    uint32_t flags;
    void* user_data;
} shading_anisotropic_cloth_desc_t;

typedef struct shading_anisotropic_cloth_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_anisotropic_cloth_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_anisotropic_cloth_init(void);
void shading_anisotropic_cloth_shutdown(void);

/* Lifecycle */
int shading_anisotropic_cloth_create(shading_anisotropic_cloth_handle_t* out_handle, const shading_anisotropic_cloth_desc_t* desc);
void shading_anisotropic_cloth_destroy(shading_anisotropic_cloth_handle_t handle);

/* Operations */
int shading_anisotropic_cloth_update(shading_anisotropic_cloth_handle_t handle, const void* data, size_t size);
bool shading_anisotropic_cloth_is_valid(shading_anisotropic_cloth_handle_t handle);
int shading_anisotropic_cloth_get_info(shading_anisotropic_cloth_handle_t handle, shading_anisotropic_cloth_info_t* out_info);
void shading_anisotropic_cloth_mark_dirty(shading_anisotropic_cloth_handle_t handle);
int shading_anisotropic_cloth_process_pending(void);

/* Statistics */
uint32_t shading_anisotropic_cloth_get_count(void);
size_t shading_anisotropic_cloth_get_memory_usage(void);
void shading_anisotropic_cloth_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_ANISOTROPIC_CLOTH_H */
