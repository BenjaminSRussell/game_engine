/*
 * water_material.h
 * Water surface material
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_WATER_MATERIAL_H
#define MATERIALS_WATER_MATERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_water_material_handle {
    uint32_t id;
} materials_water_material_handle_t;

typedef struct materials_water_material_desc {
    uint32_t flags;
    void* user_data;
} materials_water_material_desc_t;

typedef struct materials_water_material_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_water_material_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_water_material_init(void);
void materials_water_material_shutdown(void);

/* Lifecycle */
int materials_water_material_create(materials_water_material_handle_t* out_handle, const materials_water_material_desc_t* desc);
void materials_water_material_destroy(materials_water_material_handle_t handle);

/* Operations */
int materials_water_material_update(materials_water_material_handle_t handle, const void* data, size_t size);
bool materials_water_material_is_valid(materials_water_material_handle_t handle);
int materials_water_material_get_info(materials_water_material_handle_t handle, materials_water_material_info_t* out_info);
void materials_water_material_mark_dirty(materials_water_material_handle_t handle);
int materials_water_material_process_pending(void);

/* Statistics */
uint32_t materials_water_material_get_count(void);
size_t materials_water_material_get_memory_usage(void);
void materials_water_material_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_WATER_MATERIAL_H */
