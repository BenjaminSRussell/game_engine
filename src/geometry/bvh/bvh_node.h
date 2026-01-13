/*
 * bvh_node.h
 * BVH node and primitive structure definitions
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_BVH_NODE_H
#define GEOMETRY_BVH_NODE_H

#include "include/math/vec3.h"
#include "include/math/aabb.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * BVH STRUCTURES
 * ============================================================================ */

/**
 * BVH primitive - represents an object in the BVH with bounds and centroid
 */
typedef struct bvh_primitive {
    AABB bounds;        // Bounding box of the primitive
    Vec3 centroid;      // Center point for spatial partitioning
    uint32_t id;        // User-defined primitive ID
} bvh_primitive_t;

/**
 * BVH node - represents both interior and leaf nodes in the tree
 * Interior nodes: prim_count = 0, left_child and right_child are valid
 * Leaf nodes: prim_count > 0, first_prim points to primitive range
 */
typedef struct bvh_node {
    AABB bounds;            // Bounding box for this node
    uint32_t first_prim;    // For leaves: first primitive index
    uint32_t prim_count;    // For leaves: primitive count, 0 for interior nodes
    uint32_t left_child;    // For interior: left child node index
    uint32_t right_child;   // For interior: right child node index
    uint8_t split_axis;     // Splitting axis: 0=X, 1=Y, 2=Z
    uint8_t padding[3];     // Padding for alignment
} bvh_node_t;

/**
 * Flattened BVH node optimized for GPU traversal
 * Uses the skip-pointer layout for efficient GPU SIMD processing
 */
typedef struct bvh_node_gpu {
    float bounds_min[3];    // Min bounds (SIMD aligned)
    uint32_t left_first;    // Left child index (interior) or first prim (leaf)
    float bounds_max[3];    // Max bounds (SIMD aligned)
    uint32_t prim_count;    // 0 for interior, >0 for leaf
} bvh_node_gpu_t;

/**
 * BVH build context - internal structure for tree construction
 */


/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Check if a BVH node is a leaf node
 */
static inline bool bvh_node_is_leaf(const bvh_node_t* node) {
    return node->prim_count > 0;
}

/**
 * Flatten a BVH tree for GPU consumption
 * Converts the standard node layout to a GPU-friendly skip-pointer layout
 */
int bvh_flatten_for_gpu(const bvh_node_t* nodes, uint32_t node_count,
                        bvh_node_gpu_t** out_gpu_nodes, uint32_t* out_count);

/**
 * Free a flattened GPU BVH
 */
void bvh_free_gpu_nodes(bvh_node_gpu_t* gpu_nodes);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_BVH_NODE_H */
