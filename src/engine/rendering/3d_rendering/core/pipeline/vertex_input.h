/*
 * vertex_input.h
 * Vertex input state
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_VERTEX_INPUT_H
#define CORE_VERTEX_INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_vertex_input_handle {
    uint32_t id;
} core_vertex_input_handle_t;

typedef struct core_vertex_input_desc {
    uint32_t flags;
    void* user_data;
} core_vertex_input_desc_t;

typedef struct core_vertex_input_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_vertex_input_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_vertex_input_init(void);
void core_vertex_input_shutdown(void);

/* Lifecycle */
int core_vertex_input_create(core_vertex_input_handle_t* out_handle, const core_vertex_input_desc_t* desc);
void core_vertex_input_destroy(core_vertex_input_handle_t handle);

/* Operations */
int core_vertex_input_update(core_vertex_input_handle_t handle, const void* data, size_t size);
bool core_vertex_input_is_valid(core_vertex_input_handle_t handle);
int core_vertex_input_get_info(core_vertex_input_handle_t handle, core_vertex_input_info_t* out_info);
void core_vertex_input_mark_dirty(core_vertex_input_handle_t handle);
int core_vertex_input_process_pending(void);

/* Statistics */
uint32_t core_vertex_input_get_count(void);
size_t core_vertex_input_get_memory_usage(void);
void core_vertex_input_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_VERTEX_INPUT_H */
