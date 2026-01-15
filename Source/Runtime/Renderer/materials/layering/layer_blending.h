/*
 * layer_blending.h
 * Layer blend modes
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_LAYER_BLENDING_H
#define MATERIALS_LAYER_BLENDING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_layer_blending_handle {
    uint32_t id;
} materials_layer_blending_handle_t;

typedef struct materials_layer_blending_desc {
    uint32_t flags;
    void* user_data;
} materials_layer_blending_desc_t;

typedef struct materials_layer_blending_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_layer_blending_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_layer_blending_init(void);
void materials_layer_blending_shutdown(void);

/* Lifecycle */
int materials_layer_blending_create(materials_layer_blending_handle_t* out_handle, const materials_layer_blending_desc_t* desc);
void materials_layer_blending_destroy(materials_layer_blending_handle_t handle);

/* Operations */
int materials_layer_blending_update(materials_layer_blending_handle_t handle, const void* data, size_t size);
bool materials_layer_blending_is_valid(materials_layer_blending_handle_t handle);
int materials_layer_blending_get_info(materials_layer_blending_handle_t handle, materials_layer_blending_info_t* out_info);
void materials_layer_blending_mark_dirty(materials_layer_blending_handle_t handle);
int materials_layer_blending_process_pending(void);

/* Statistics */
uint32_t materials_layer_blending_get_count(void);
size_t materials_layer_blending_get_memory_usage(void);
void materials_layer_blending_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_LAYER_BLENDING_H */
