/*
 * clearcoat_layer.h
 * Clearcoat layer
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_CLEARCOAT_LAYER_H
#define SHADING_CLEARCOAT_LAYER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_clearcoat_layer_handle {
    uint32_t id;
} shading_clearcoat_layer_handle_t;

typedef struct shading_clearcoat_layer_desc {
    uint32_t flags;
    void* user_data;
} shading_clearcoat_layer_desc_t;

typedef struct shading_clearcoat_layer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_clearcoat_layer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_clearcoat_layer_init(void);
void shading_clearcoat_layer_shutdown(void);

/* Lifecycle */
int shading_clearcoat_layer_create(shading_clearcoat_layer_handle_t* out_handle, const shading_clearcoat_layer_desc_t* desc);
void shading_clearcoat_layer_destroy(shading_clearcoat_layer_handle_t handle);

/* Operations */
int shading_clearcoat_layer_update(shading_clearcoat_layer_handle_t handle, const void* data, size_t size);
bool shading_clearcoat_layer_is_valid(shading_clearcoat_layer_handle_t handle);
int shading_clearcoat_layer_get_info(shading_clearcoat_layer_handle_t handle, shading_clearcoat_layer_info_t* out_info);
void shading_clearcoat_layer_mark_dirty(shading_clearcoat_layer_handle_t handle);
int shading_clearcoat_layer_process_pending(void);

/* Statistics */
uint32_t shading_clearcoat_layer_get_count(void);
size_t shading_clearcoat_layer_get_memory_usage(void);
void shading_clearcoat_layer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_CLEARCOAT_LAYER_H */
