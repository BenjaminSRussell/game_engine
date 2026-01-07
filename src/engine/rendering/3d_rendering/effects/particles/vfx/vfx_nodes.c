/*
 * vfx_nodes.c
 * VFX Graph Nodes
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#include "vfx_nodes.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * API
 * ============================================================================ */

vfx_node_t* vfx_node_create(vfx_node_type_t type) {
    vfx_node_t* node = calloc(1, sizeof(vfx_node_t));
    if (!node) return NULL;
    
    node->type = type;
    // Helper to setup pins based on type would go here
    return node;
}

void vfx_node_destroy(vfx_node_t* node) {
    if (!node) return;
    
    if (node->inputs) free(node->inputs);
    if (node->outputs) free(node->outputs);
    if (node->node_data) free(node->node_data);
    
    free(node);
}
