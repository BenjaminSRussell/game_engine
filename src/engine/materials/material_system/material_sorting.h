/*
 * material_sorting.h
 * Material render ordering
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_MATERIAL_SORTING_H
#define MATERIALS_MATERIAL_SORTING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_material_sorting_handle {
    uint32_t id;
} materials_material_sorting_handle_t;

typedef struct materials_material_sorting_desc {
    uint32_t flags;
    void* user_data;
} materials_material_sorting_desc_t;

typedef struct materials_material_sorting_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_material_sorting_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_material_sorting_init(void);
void materials_material_sorting_shutdown(void);

/* Lifecycle */
int materials_material_sorting_create(materials_material_sorting_handle_t* out_handle, const materials_material_sorting_desc_t* desc);
void materials_material_sorting_destroy(materials_material_sorting_handle_t handle);

/* Operations */
int materials_material_sorting_update(materials_material_sorting_handle_t handle, const void* data, size_t size);
bool materials_material_sorting_is_valid(materials_material_sorting_handle_t handle);
int materials_material_sorting_get_info(materials_material_sorting_handle_t handle, materials_material_sorting_info_t* out_info);
void materials_material_sorting_mark_dirty(materials_material_sorting_handle_t handle);
int materials_material_sorting_process_pending(void);

/* Statistics */
uint32_t materials_material_sorting_get_count(void);
size_t materials_material_sorting_get_memory_usage(void);
void materials_material_sorting_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_MATERIAL_SORTING_H */
