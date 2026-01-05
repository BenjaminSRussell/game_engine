/**
 * Dynamic AABB Tree for Broadphase Collision Detection
 * 
 * Self-balancing binary tree using Axis-Aligned Bounding Boxes (AABBs).
 * Provides O(N log N) broadphase vs O(N²) naive sweep.
 * 
 * Key features:
 * - Surface Area Heuristic (SAH) for optimal tree construction
 * - AVL-style balancing to maintain O(log N) depth
 * - Fat AABBs to reduce tree churn from moving objects
 * - Incremental updates (no full tree rebuild per frame)
 */

#ifndef AABB_TREE_H
#define AABB_TREE_H

#include "../../core/simd/simd_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// AABB Structure
// ========================================

typedef struct AABB {
    v4f min;  // Minimum corner (x, y, z, 0)
    v4f max;  // Maximum corner (x, y, z, 0)
} AABB;

// ========================================
// Tree Node
// ========================================

typedef struct AABBTreeNode {
    AABB aabb;              // Bounding box (fat AABB for branches, tight for leaves)
    
    union {
        // Branch node (internal)
        struct {
            int32_t child1;  // Left child index
            int32_t child2;  // Right child index
        };
        
        // Leaf node
        struct {
            uint32_t body_id;  // Index into physics SoA container
            void *user_data;   // Optional user pointer
        };
    };
    
    int32_t parent;         // Parent node index (-1 for root)
    int32_t height;         // Height of subtree (0 for leaves)
    
    // Node state
    bool is_leaf;
    
} AABBTreeNode;

// Null node sentinel
#define AABB_NULL_NODE -1

// ========================================
// Dynamic AABB Tree
// ========================================

typedef struct AABBTree {
    AABBTreeNode *nodes;    // Node pool
    int32_t root;           // Root node index
    
    uint32_t node_capacity; // Total allocated nodes
    uint32_t node_count;    // Active nodes
    int32_t free_list;      // Head of free node list
    
    // Fat AABB parameters
    float fat_margin;       // Expansion margin (e.g., 0.1m)
    
} AABBTree;

// ========================================
// Tree Management
// ========================================

/**
 * Create a new AABB tree with specified capacity
 */
AABBTree* aabb_tree_create(uint32_t capacity, float fat_margin);

/**
 * Destroy tree and free memory
 */
void aabb_tree_destroy(AABBTree *tree);

/**
 * Insert a new leaf node
 * 
 * @param tree The AABB tree
 * @param aabb Tight AABB for the object
 * @param body_id Physics body index
 * @return Proxy ID (node index) for future updates
 */
int32_t aabb_tree_insert(AABBTree *tree, const AABB *aabb, uint32_t body_id);

/**
 * Remove a leaf node
 * 
 * @param tree The AABB tree
 * @param proxy_id Node index returned from insert
 */
void aabb_tree_remove(AABBTree *tree, int32_t proxy_id);

/**
 * Update a leaf's AABB (with motion prediction)
 * 
 * @param tree The AABB tree
 * @param proxy_id Node index
 * @param new_aabb New tight AABB
 * @param displacement Velocity * dt for predictive fattening
 * @return true if tree structure changed (rare due to fat AABBs)
 */
bool aabb_tree_update(AABBTree *tree, int32_t proxy_id, const AABB *new_aabb, const v4f *displacement);

// ========================================
// AABB Utilities
// ========================================

/**
 * Test if two AABBs overlap
 */
bool aabb_overlap(const AABB *a, const AABB *b);

/**
 * Compute union of two AABBs
 */
void aabb_union(AABB *result, const AABB *a, const AABB *b);

/**
 * Compute surface area of AABB (for SAH)
 */
float aabb_surface_area(const AABB *aabb);

/**
 * Fatten AABB by margin and velocity prediction
 */
void aabb_fatten(AABB *fat, const AABB *tight, float margin, const v4f *displacement);

/**
 * Check if tight AABB is still contained in fat AABB
 */
bool aabb_contains(const AABB *fat, const AABB *tight);

// ========================================
// Query Operations
// ========================================

/**
 * Callback for query results
 * Return true to continue query, false to stop
 */
typedef bool (*AABBQueryCallback)(int32_t proxy_id, uint32_t body_id, void *user_data);

/**
 * Query all overlaps with given AABB
 */
void aabb_tree_query(const AABBTree *tree, const AABB *aabb, AABBQueryCallback callback, void *user_data);

/**
 * Find all potential collision pairs
 * 
 * This is the main broadphase function.
 * Callback is invoked for each overlapping pair.
 */
void aabb_tree_find_pairs(const AABBTree *tree, AABBQueryCallback callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif // AABB_TREE_H
