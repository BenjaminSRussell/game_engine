/*
 * foliage_material.h
 * Foliage two-sided material
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_FOLIAGE_MATERIAL_H
#define MATERIALS_FOLIAGE_MATERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_foliage_material_handle {
    uint32_t id;
} materials_foliage_material_handle_t;

typedef struct materials_foliage_material_desc {
    uint32_t flags;
    void* user_data;
} materials_foliage_material_desc_t;

typedef struct materials_foliage_material_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_foliage_material_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_foliage_material_init(void);
void materials_foliage_material_shutdown(void);

/* Lifecycle */
int materials_foliage_material_create(materials_foliage_material_handle_t* out_handle, const materials_foliage_material_desc_t* desc);
void materials_foliage_material_destroy(materials_foliage_material_handle_t handle);

/* Operations */
int materials_foliage_material_update(materials_foliage_material_handle_t handle, const void* data, size_t size);
bool materials_foliage_material_is_valid(materials_foliage_material_handle_t handle);
int materials_foliage_material_get_info(materials_foliage_material_handle_t handle, materials_foliage_material_info_t* out_info);
void materials_foliage_material_mark_dirty(materials_foliage_material_handle_t handle);
int materials_foliage_material_process_pending(void);

/* Statistics */
uint32_t materials_foliage_material_get_count(void);
size_t materials_foliage_material_get_memory_usage(void);
void materials_foliage_material_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_FOLIAGE_MATERIAL_H */
