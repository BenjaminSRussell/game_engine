/*
 * cloth_ao.h
 * Cloth ambient occlusion
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_CLOTH_AO_H
#define SHADING_CLOTH_AO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_cloth_ao_handle {
    uint32_t id;
} shading_cloth_ao_handle_t;

typedef struct shading_cloth_ao_desc {
    uint32_t flags;
    void* user_data;
} shading_cloth_ao_desc_t;

typedef struct shading_cloth_ao_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_cloth_ao_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_cloth_ao_init(void);
void shading_cloth_ao_shutdown(void);

/* Lifecycle */
int shading_cloth_ao_create(shading_cloth_ao_handle_t* out_handle, const shading_cloth_ao_desc_t* desc);
void shading_cloth_ao_destroy(shading_cloth_ao_handle_t handle);

/* Operations */
int shading_cloth_ao_update(shading_cloth_ao_handle_t handle, const void* data, size_t size);
bool shading_cloth_ao_is_valid(shading_cloth_ao_handle_t handle);
int shading_cloth_ao_get_info(shading_cloth_ao_handle_t handle, shading_cloth_ao_info_t* out_info);
void shading_cloth_ao_mark_dirty(shading_cloth_ao_handle_t handle);
int shading_cloth_ao_process_pending(void);

/* Statistics */
uint32_t shading_cloth_ao_get_count(void);
size_t shading_cloth_ao_get_memory_usage(void);
void shading_cloth_ao_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_CLOTH_AO_H */
