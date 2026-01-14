/*
 * graph_executor.h
 * Render graph execution
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_GRAPH_EXECUTOR_H
#define RENDERING_GRAPH_EXECUTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_graph_executor_handle {
    uint32_t id;
} rendering_graph_executor_handle_t;

typedef struct rendering_graph_executor_desc {
    uint32_t flags;
    void* user_data;
} rendering_graph_executor_desc_t;

typedef struct rendering_graph_executor_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_graph_executor_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

#include "rendering/render_graph/graph_compiler.h"

/* Graph Execution API */
void rg_execute(render_graph_t* graph, void* cmd);

int rendering_graph_executor_init(void);
void rendering_graph_executor_shutdown(void);
int rendering_graph_executor_process_pending(void);

/* Statistics */
uint32_t rendering_graph_executor_get_count(void);
size_t rendering_graph_executor_get_memory_usage(void);
void rendering_graph_executor_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GRAPH_EXECUTOR_H */
