/*
 * vfx_execution.h
 * VFX Graph Execution
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_EXECUTION_H
#define EFFECTS_VFX_EXECUTION_H

#include "vfx_context.h"
#include "vfx_nodes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

void vfx_execution_process_graph(vfx_graph_handle_t graph, float dt);
void vfx_execution_evaluate_node(vfx_node_t* node, float dt);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_EXECUTION_H */
