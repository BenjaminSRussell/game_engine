/*
 * multi_layer_material.h
 * Multi-layer materials
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_MULTI_LAYER_MATERIAL_H
#define SHADING_MULTI_LAYER_MATERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_multi_layer_material_handle {
    uint32_t id;
} shading_multi_layer_material_handle_t;

typedef struct shading_multi_layer_material_desc {
    uint32_t flags;
    void* user_data;
} shading_multi_layer_material_desc_t;

typedef struct shading_multi_layer_material_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_multi_layer_material_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_multi_layer_material_init(void);
void shading_multi_layer_material_shutdown(void);

/* Lifecycle */
int shading_multi_layer_material_create(shading_multi_layer_material_handle_t* out_handle, const shading_multi_layer_material_desc_t* desc);
void shading_multi_layer_material_destroy(shading_multi_layer_material_handle_t handle);

/* Operations */
int shading_multi_layer_material_update(shading_multi_layer_material_handle_t handle, const void* data, size_t size);
bool shading_multi_layer_material_is_valid(shading_multi_layer_material_handle_t handle);
int shading_multi_layer_material_get_info(shading_multi_layer_material_handle_t handle, shading_multi_layer_material_info_t* out_info);
void shading_multi_layer_material_mark_dirty(shading_multi_layer_material_handle_t handle);
int shading_multi_layer_material_process_pending(void);

/* Statistics */
uint32_t shading_multi_layer_material_get_count(void);
size_t shading_multi_layer_material_get_memory_usage(void);
void shading_multi_layer_material_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_MULTI_LAYER_MATERIAL_H */
