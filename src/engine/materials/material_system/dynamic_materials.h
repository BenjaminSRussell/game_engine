/*
 * dynamic_materials.h
 * Runtime material creation
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_DYNAMIC_MATERIALS_H
#define MATERIALS_DYNAMIC_MATERIALS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_dynamic_materials_handle {
    uint32_t id;
} materials_dynamic_materials_handle_t;

typedef struct materials_dynamic_materials_desc {
    uint32_t flags;
    void* user_data;
} materials_dynamic_materials_desc_t;

typedef struct materials_dynamic_materials_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_dynamic_materials_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_dynamic_materials_init(void);
void materials_dynamic_materials_shutdown(void);

/* Lifecycle */
int materials_dynamic_materials_create(materials_dynamic_materials_handle_t* out_handle, const materials_dynamic_materials_desc_t* desc);
void materials_dynamic_materials_destroy(materials_dynamic_materials_handle_t handle);

/* Operations */
int materials_dynamic_materials_update(materials_dynamic_materials_handle_t handle, const void* data, size_t size);
bool materials_dynamic_materials_is_valid(materials_dynamic_materials_handle_t handle);
int materials_dynamic_materials_get_info(materials_dynamic_materials_handle_t handle, materials_dynamic_materials_info_t* out_info);
void materials_dynamic_materials_mark_dirty(materials_dynamic_materials_handle_t handle);
int materials_dynamic_materials_process_pending(void);

/* Statistics */
uint32_t materials_dynamic_materials_get_count(void);
size_t materials_dynamic_materials_get_memory_usage(void);
void materials_dynamic_materials_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_DYNAMIC_MATERIALS_H */
