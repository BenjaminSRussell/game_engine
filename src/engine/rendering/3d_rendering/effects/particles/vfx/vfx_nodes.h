/*
 * vfx_nodes.h
 * VFX Graph Nodes
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_NODES_H
#define EFFECTS_VFX_NODES_H

#include <stdint.h>
#include <stdbool.h>
#include "../../math/vec3.h"
#include "../../math/vec4.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum vfx_node_type {
    VFX_NODE_EMITTER,
    VFX_NODE_FORCE,
    VFX_NODE_EVENT,
    VFX_NODE_MODIFIER,
    VFX_NODE_OUTPUT,
    VFX_NODE_COUNT
} vfx_node_type_t;

typedef struct vfx_pin {
    uint32_t id;
    uint32_t type; // Float, Vec3, etc.
    bool computed;
    union {
        float f;
        int i;
        vec3_t v3;
        vec4_t v4;
    } value;
} vfx_pin_t;

typedef struct vfx_node {
    uint32_t id;
    vfx_node_type_t type;
    vfx_pin_t* inputs;
    uint32_t input_count;
    vfx_pin_t* outputs;
    uint32_t output_count;
    void* node_data;
} vfx_node_t;

/* ============================================================================
 * API
 * ============================================================================ */

vfx_node_t* vfx_node_create(vfx_node_type_t type);
void vfx_node_destroy(vfx_node_t* node);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_NODES_H */
