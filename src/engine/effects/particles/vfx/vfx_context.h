/*
 * vfx_context.h
 * VFX Graph Runtime Context
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_CONTEXT_H
#define EFFECTS_VFX_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct vfx_graph_handle {
    uint32_t id;
} vfx_graph_handle_t;

typedef struct vfx_context_desc {
    uint32_t max_graphs;
    uint32_t max_nodes_per_graph;
    void* user_data;
} vfx_context_desc_t;

/* ============================================================================
 * API
 * ============================================================================ */

int vfx_context_init(const vfx_context_desc_t* desc);
void vfx_context_shutdown(void);

int vfx_context_create_graph(vfx_graph_handle_t* out_handle);
void vfx_context_destroy_graph(vfx_graph_handle_t handle);

int vfx_context_update(float dt);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_CONTEXT_H */
