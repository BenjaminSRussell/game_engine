/*
 * graph_compiler.h
 * Render graph compilation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_GRAPH_COMPILER_H
#define RENDERING_GRAPH_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_graph_compiler_handle {
    uint32_t id;
} rendering_graph_compiler_handle_t;

typedef struct rendering_graph_compiler_desc {
    uint32_t flags;
    void* user_data;
} rendering_graph_compiler_desc_t;

typedef struct rendering_graph_compiler_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_graph_compiler_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_graph_compiler_init(void);
void rendering_graph_compiler_shutdown(void);

/* Lifecycle */
int rendering_graph_compiler_create(rendering_graph_compiler_handle_t* out_handle, const rendering_graph_compiler_desc_t* desc);
void rendering_graph_compiler_destroy(rendering_graph_compiler_handle_t handle);

/* Operations */
int rendering_graph_compiler_update(rendering_graph_compiler_handle_t handle, const void* data, size_t size);
bool rendering_graph_compiler_is_valid(rendering_graph_compiler_handle_t handle);
int rendering_graph_compiler_get_info(rendering_graph_compiler_handle_t handle, rendering_graph_compiler_info_t* out_info);
void rendering_graph_compiler_mark_dirty(rendering_graph_compiler_handle_t handle);
int rendering_graph_compiler_process_pending(void);

/* Statistics */
uint32_t rendering_graph_compiler_get_count(void);
size_t rendering_graph_compiler_get_memory_usage(void);
void rendering_graph_compiler_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GRAPH_COMPILER_H */
