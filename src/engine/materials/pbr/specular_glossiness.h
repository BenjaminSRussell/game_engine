/*
 * specular_glossiness.h
 * Specular-glossiness workflow
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_SPECULAR_GLOSSINESS_H
#define MATERIALS_SPECULAR_GLOSSINESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_specular_glossiness_handle {
    uint32_t id;
} materials_specular_glossiness_handle_t;

typedef struct materials_specular_glossiness_desc {
    uint32_t flags;
    void* user_data;
} materials_specular_glossiness_desc_t;

typedef struct materials_specular_glossiness_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_specular_glossiness_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_specular_glossiness_init(void);
void materials_specular_glossiness_shutdown(void);

/* Lifecycle */
int materials_specular_glossiness_create(materials_specular_glossiness_handle_t* out_handle, const materials_specular_glossiness_desc_t* desc);
void materials_specular_glossiness_destroy(materials_specular_glossiness_handle_t handle);

/* Operations */
int materials_specular_glossiness_update(materials_specular_glossiness_handle_t handle, const void* data, size_t size);
bool materials_specular_glossiness_is_valid(materials_specular_glossiness_handle_t handle);
int materials_specular_glossiness_get_info(materials_specular_glossiness_handle_t handle, materials_specular_glossiness_info_t* out_info);
void materials_specular_glossiness_mark_dirty(materials_specular_glossiness_handle_t handle);
int materials_specular_glossiness_process_pending(void);

/* Statistics */
uint32_t materials_specular_glossiness_get_count(void);
size_t materials_specular_glossiness_get_memory_usage(void);
void materials_specular_glossiness_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_SPECULAR_GLOSSINESS_H */
