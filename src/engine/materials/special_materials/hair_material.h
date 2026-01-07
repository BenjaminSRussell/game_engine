/*
 * hair_material.h
 * Hair/fur material
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_HAIR_MATERIAL_H
#define MATERIALS_HAIR_MATERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_hair_material_handle {
    uint32_t id;
} materials_hair_material_handle_t;

typedef struct materials_hair_material_desc {
    uint32_t flags;
    void* user_data;
} materials_hair_material_desc_t;

typedef struct materials_hair_material_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_hair_material_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_hair_material_init(void);
void materials_hair_material_shutdown(void);

/* Lifecycle */
int materials_hair_material_create(materials_hair_material_handle_t* out_handle, const materials_hair_material_desc_t* desc);
void materials_hair_material_destroy(materials_hair_material_handle_t handle);

/* Operations */
int materials_hair_material_update(materials_hair_material_handle_t handle, const void* data, size_t size);
bool materials_hair_material_is_valid(materials_hair_material_handle_t handle);
int materials_hair_material_get_info(materials_hair_material_handle_t handle, materials_hair_material_info_t* out_info);
void materials_hair_material_mark_dirty(materials_hair_material_handle_t handle);
int materials_hair_material_process_pending(void);

/* Statistics */
uint32_t materials_hair_material_get_count(void);
size_t materials_hair_material_get_memory_usage(void);
void materials_hair_material_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_HAIR_MATERIAL_H */
