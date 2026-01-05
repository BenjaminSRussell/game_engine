/*
 * metallic_roughness.h
 * Metallic-roughness workflow
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_METALLIC_ROUGHNESS_H
#define MATERIALS_METALLIC_ROUGHNESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_metallic_roughness_handle {
    uint32_t id;
} materials_metallic_roughness_handle_t;

typedef struct materials_metallic_roughness_desc {
    uint32_t flags;
    void* user_data;
} materials_metallic_roughness_desc_t;

typedef struct materials_metallic_roughness_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_metallic_roughness_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_metallic_roughness_init(void);
void materials_metallic_roughness_shutdown(void);

/* Lifecycle */
int materials_metallic_roughness_create(materials_metallic_roughness_handle_t* out_handle, const materials_metallic_roughness_desc_t* desc);
void materials_metallic_roughness_destroy(materials_metallic_roughness_handle_t handle);

/* Operations */
int materials_metallic_roughness_update(materials_metallic_roughness_handle_t handle, const void* data, size_t size);
bool materials_metallic_roughness_is_valid(materials_metallic_roughness_handle_t handle);
int materials_metallic_roughness_get_info(materials_metallic_roughness_handle_t handle, materials_metallic_roughness_info_t* out_info);
void materials_metallic_roughness_mark_dirty(materials_metallic_roughness_handle_t handle);
int materials_metallic_roughness_process_pending(void);

/* Statistics */
uint32_t materials_metallic_roughness_get_count(void);
size_t materials_metallic_roughness_get_memory_usage(void);
void materials_metallic_roughness_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_METALLIC_ROUGHNESS_H */
