/*
 * material_classify.h
 * Material classification
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_MATERIAL_CLASSIFY_H
#define RENDERING_MATERIAL_CLASSIFY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_material_classify_handle {
    uint32_t id;
} rendering_material_classify_handle_t;

typedef struct rendering_material_classify_desc {
    uint32_t flags;
    void* user_data;
} rendering_material_classify_desc_t;

typedef struct rendering_material_classify_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_material_classify_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_material_classify_init(void);
void rendering_material_classify_shutdown(void);

/* Lifecycle */
int rendering_material_classify_create(rendering_material_classify_handle_t* out_handle, const rendering_material_classify_desc_t* desc);
void rendering_material_classify_destroy(rendering_material_classify_handle_t handle);

/* Operations */
int rendering_material_classify_update(rendering_material_classify_handle_t handle, const void* data, size_t size);
bool rendering_material_classify_is_valid(rendering_material_classify_handle_t handle);
int rendering_material_classify_get_info(rendering_material_classify_handle_t handle, rendering_material_classify_info_t* out_info);
void rendering_material_classify_mark_dirty(rendering_material_classify_handle_t handle);
int rendering_material_classify_process_pending(void);

/* Statistics */
uint32_t rendering_material_classify_get_count(void);
size_t rendering_material_classify_get_memory_usage(void);
void rendering_material_classify_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_MATERIAL_CLASSIFY_H */
