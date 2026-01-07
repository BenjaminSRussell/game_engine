/*
 * vfx_compiler.h
 * VFX Graph Compiler
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_COMPILER_H
#define EFFECTS_VFX_COMPILER_H

#include "vfx_nodes.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

bool vfx_compiler_validate_graph(const vfx_node_t* nodes, uint32_t node_count);
void vfx_compiler_optimize_graph(vfx_node_t* nodes, uint32_t* node_count);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_COMPILER_H */
