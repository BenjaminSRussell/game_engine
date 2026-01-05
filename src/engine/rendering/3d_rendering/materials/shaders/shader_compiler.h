/*
 * shader_compiler.h
 * HLSL/GLSL compilation
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_SHADER_COMPILER_H
#define MATERIALS_SHADER_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_shader_compiler_handle {
    uint32_t id;
} materials_shader_compiler_handle_t;

typedef struct materials_shader_compiler_desc {
    uint32_t flags;
    void* user_data;
} materials_shader_compiler_desc_t;

typedef struct materials_shader_compiler_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_shader_compiler_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_shader_compiler_init(void);
void materials_shader_compiler_shutdown(void);

/* Lifecycle */
int materials_shader_compiler_create(materials_shader_compiler_handle_t* out_handle, const materials_shader_compiler_desc_t* desc);
void materials_shader_compiler_destroy(materials_shader_compiler_handle_t handle);

/* Operations */
int materials_shader_compiler_update(materials_shader_compiler_handle_t handle, const void* data, size_t size);
bool materials_shader_compiler_is_valid(materials_shader_compiler_handle_t handle);
int materials_shader_compiler_get_info(materials_shader_compiler_handle_t handle, materials_shader_compiler_info_t* out_info);
void materials_shader_compiler_mark_dirty(materials_shader_compiler_handle_t handle);
int materials_shader_compiler_process_pending(void);

/* Statistics */
uint32_t materials_shader_compiler_get_count(void);
size_t materials_shader_compiler_get_memory_usage(void);
void materials_shader_compiler_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_SHADER_COMPILER_H */
