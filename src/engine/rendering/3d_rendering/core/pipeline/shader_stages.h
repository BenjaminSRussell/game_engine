/*
 * shader_stages.h
 * Shader stage configuration
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_SHADER_STAGES_H
#define CORE_SHADER_STAGES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_shader_stages_handle {
    uint32_t id;
} core_shader_stages_handle_t;

typedef struct core_shader_stages_desc {
    uint32_t flags;
    void* user_data;
} core_shader_stages_desc_t;

typedef struct core_shader_stages_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_shader_stages_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_shader_stages_init(void);
void core_shader_stages_shutdown(void);

/* Lifecycle */
int core_shader_stages_create(core_shader_stages_handle_t* out_handle, const core_shader_stages_desc_t* desc);
void core_shader_stages_destroy(core_shader_stages_handle_t handle);

/* Operations */
int core_shader_stages_update(core_shader_stages_handle_t handle, const void* data, size_t size);
bool core_shader_stages_is_valid(core_shader_stages_handle_t handle);
int core_shader_stages_get_info(core_shader_stages_handle_t handle, core_shader_stages_info_t* out_info);
void core_shader_stages_mark_dirty(core_shader_stages_handle_t handle);
int core_shader_stages_process_pending(void);

/* Statistics */
uint32_t core_shader_stages_get_count(void);
size_t core_shader_stages_get_memory_usage(void);
void core_shader_stages_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SHADER_STAGES_H */
