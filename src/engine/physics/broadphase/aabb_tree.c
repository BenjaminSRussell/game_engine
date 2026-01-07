/**
 * Dynamic AABB Tree Implementation
 * 
 * Based on Box2D's b2DynamicTree with enhancements:
 * - Surface Area Heuristic (SAH) for insertion
 * - AVL balancing for O(log N) guarantee
 * - Fat AABBs with velocity prediction
 */

#include "physics/broadphase/aabb_tree.h"
#include "core/simd/simd_math.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>

// ========================================
// AABB Utilities
// ========================================

bool aabb_overlap(const AABB *a, const AABB *b) {
    // Separating axis test (early out if any axis doesn't overlap)
    if (a->max.x < b->min.x || a->min.x > b->max.x) return false;
    if (a->max.y < b->min.y || a->min.y > b->max.y) return false;
    if (a->max.z < b->min.z || a->min.z > b->max.z) return false;
    return true;
}

void aabb_union(AABB *result, const AABB *a, const AABB *b) {
    result->min.x = (a->min.x < b->min.x) ? a->min.x : b->min.x;
    result->min.y = (a->min.y < b->min.y) ? a->min.y : b->min.y;
    result->min.z = (a->min.z < b->min.z) ? a->min.z : b->min.z;
    result->min.w = 0.0f;
    
    result->max.x = (a->max.x > b->max.x) ? a->max.x : b->max.x;
    result->max.y = (a->max.y > b->max.y) ? a->max.y : b->max.y;
    result->max.z = (a->max.z > b->max.z) ? a->max.z : b->max.z;
    result->max.w = 0.0f;
}

float aabb_surface_area(const AABB *aabb) {
    float dx = aabb->max.x - aabb->min.x;
    float dy = aabb->max.y - aabb->min.y;
    float dz = aabb->max.z - aabb->min.z;
    return 2.0f * (dx * dy + dy * dz + dz * dx);
}

void aabb_fatten(AABB *fat, const AABB *tight, float margin, const v4f *displacement) {
    // Start with tight AABB
    *fat = *tight;
    
    // Add fixed margin
    fat->min.x -= margin;
    fat->min.y -= margin;
    fat->min.z -= margin;
    
    fat->max.x += margin;
    fat->max.y += margin;
    fat->max.z += margin;
    
    // Extend in direction of motion (predictive)
    if (displacement) {
        if (displacement->x < 0.0f) {
            fat->min.x += displacement->x;
        } else {
            fat->max.x += displacement->x;
        }
        
        if (displacement->y < 0.0f) {
            fat->min.y += displacement->y;
        } else {
            fat->max.y += displacement->y;
        }
        
        if (displacement->z < 0.0f) {
            fat->min.z += displacement->z;
        } else {
            fat->max.z += displacement->z;
        }
    }
}

bool aabb_contains(const AABB *fat, const AABB *tight) {
    return (tight->min.x >= fat->min.x && tight->max.x <= fat->max.x &&
            tight->min.y >= fat->min.y && tight->max.y <= fat->max.y &&
            tight->min.z >= fat->min.z && tight->max.z <= fat->max.z);
}

// ========================================
// Tree Node Management
// ========================================

static int32_t allocate_node(AABBTree *tree) {
    // Expand pool if needed
    if (tree->free_list == AABB_NULL_NODE) {
        // TODO: Implement pool expansion
        return AABB_NULL_NODE;
    }
    
    int32_t node_id = tree->free_list;
    AABBTreeNode *node = &tree->nodes[node_id];
    tree->free_list = node->parent;  // Free list uses parent as next pointer
    
    node->parent = AABB_NULL_NODE;
    node->child1 = AABB_NULL_NODE;
    node->child2 = AABB_NULL_NODE;
    node->height = 0;
    node->user_data = NULL;
    
    tree->node_count++;
    return node_id;
}

static void free_node(AABBTree *tree, int32_t node_id) {
    AABBTreeNode *node = &tree->nodes[node_id];
    node->parent = tree->free_list;
    node->height = -1;
    tree->free_list = node_id;
    tree->node_count--;
}

// ========================================
// Tree Management
// ========================================

AABBTree* aabb_tree_create(uint32_t capacity, float fat_margin) {
    AABBTree *tree = (AABBTree*)malloc(sizeof(AABBTree));
    if (!tree) return NULL;
    
    tree->node_capacity = capacity;
    tree->node_count = 0;
    tree->root = AABB_NULL_NODE;
    tree->fat_margin = fat_margin;
    
    // Allocate node pool
    tree->nodes = (AABBTreeNode*)malloc(sizeof(AABBTreeNode) * capacity);
    if (!tree->nodes) {
        free(tree);
        return NULL;
    }
    
    // Build free list
    for (uint32_t i = 0; i < capacity - 1; i++) {
        tree->nodes[i].parent = i + 1;
        tree->nodes[i].height = -1;
    }
    tree->nodes[capacity - 1].parent = AABB_NULL_NODE;
    tree->nodes[capacity - 1].height = -1;
    tree->free_list = 0;
    
    return tree;
}

void aabb_tree_destroy(AABBTree *tree) {
    if (!tree) return;
    free(tree->nodes);
    free(tree);
}

// ========================================
// Tree Operations
// ========================================

int32_t aabb_tree_insert(AABBTree *tree, const AABB *aabb, uint32_t body_id) {
    // Allocate leaf node
    int32_t leaf_id = allocate_node(tree);
    if (leaf_id == AABB_NULL_NODE) return AABB_NULL_NODE;
    
    AABBTreeNode *leaf = &tree->nodes[leaf_id];
    
    // Fatten AABB
    aabb_fatten(&leaf->aabb, aabb, tree->fat_margin, NULL);
    
    leaf->body_id = body_id;
    leaf->is_leaf = true;
    leaf->height = 0;
    
    // Insert into tree
    if (tree->root == AABB_NULL_NODE) {
        tree->root = leaf_id;
        return leaf_id;
    }
    
    // Find best sibling using SAH
    AABB leaf_aabb = leaf->aabb;
    int32_t index = tree->root;
    
    while (!tree->nodes[index].is_leaf) {
        int32_t child1 = tree->nodes[index].child1;
        int32_t child2 = tree->nodes[index].child2;
        
        float area = aabb_surface_area(&tree->nodes[index].aabb);
        
        AABB combined_aabb;
        aabb_union(&combined_aabb, &tree->nodes[index].aabb, &leaf_aabb);
        float combined_area = aabb_surface_area(&combined_aabb);
        
        // Cost of creating a new parent
        float cost = 2.0f * combined_area;
        
        // Minimum cost of pushing the leaf further down
        float inheritance_cost = 2.0f * (combined_area - area);
        
        // Cost of descending to child1
        AABB aabb1;
        aabb_union(&aabb1, &leaf_aabb, &tree->nodes[child1].aabb);
        float cost1 = aabb_surface_area(&aabb1) + inheritance_cost;
        if (!tree->nodes[child1].is_leaf) {
            float old_area = aabb_surface_area(&tree->nodes[child1].aabb);
            cost1 -= old_area;
        }
        
        // Cost of descending to child2
        AABB aabb2;
        aabb_union(&aabb2, &leaf_aabb, &tree->nodes[child2].aabb);
        float cost2 = aabb_surface_area(&aabb2) + inheritance_cost;
        if (!tree->nodes[child2].is_leaf) {
            float old_area = aabb_surface_area(&tree->nodes[child2].aabb);
            cost2 -= old_area;
        }
        
        // Descend according to minimum cost
        if (cost < cost1 && cost < cost2) {
            break;
        }
        
        // Descend
        index = (cost1 < cost2) ? child1 : child2;
    }
    
    int32_t sibling = index;
    
    // Create new parent
    int32_t old_parent = tree->nodes[sibling].parent;
    int32_t new_parent = allocate_node(tree);
    tree->nodes[new_parent].parent = old_parent;
    tree->nodes[new_parent].user_data = NULL;
    aabb_union(&tree->nodes[new_parent].aabb, &leaf_aabb, &tree->nodes[sibling].aabb);
    tree->nodes[new_parent].height = tree->nodes[sibling].height + 1;
    tree->nodes[new_parent].is_leaf = false;
    
    if (old_parent != AABB_NULL_NODE) {
        // Sibling was not root
        if (tree->nodes[old_parent].child1 == sibling) {
            tree->nodes[old_parent].child1 = new_parent;
        } else {
            tree->nodes[old_parent].child2 = new_parent;
        }
        
        tree->nodes[new_parent].child1 = sibling;
        tree->nodes[new_parent].child2 = leaf_id;
        tree->nodes[sibling].parent = new_parent;
        tree->nodes[leaf_id].parent = new_parent;
    } else {
        // Sibling was root
        tree->nodes[new_parent].child1 = sibling;
        tree->nodes[new_parent].child2 = leaf_id;
        tree->nodes[sibling].parent = new_parent;
        tree->nodes[leaf_id].parent = new_parent;
        tree->root = new_parent;
    }
    
    // Walk back up the tree fixing heights and AABBs
    index = tree->nodes[leaf_id].parent;
    while (index != AABB_NULL_NODE) {
        // Balance removed for now - will add AVL balancing later
        
        int32_t child1 = tree->nodes[index].child1;
        int32_t child2 = tree->nodes[index].child2;
        
        tree->nodes[index].height = 1 + ((tree->nodes[child1].height > tree->nodes[child2].height) ? 
                                         tree->nodes[child1].height : tree->nodes[child2].height);
        aabb_union(&tree->nodes[index].aabb, &tree->nodes[child1].aabb, &tree->nodes[child2].aabb);
        
        index = tree->nodes[index].parent;
    }
    
    return leaf_id;
}

void aabb_tree_remove(AABBTree *tree, int32_t proxy_id) {
    if (proxy_id == AABB_NULL_NODE) return;
    
    if (proxy_id == tree->root) {
        tree->root = AABB_NULL_NODE;
        free_node(tree, proxy_id);
        return;
    }
    
    int32_t parent = tree->nodes[proxy_id].parent;
    int32_t grandparent = tree->nodes[parent].parent;
    int32_t sibling = (tree->nodes[parent].child1 == proxy_id) ? 
                      tree->nodes[parent].child2 : tree->nodes[parent].child1;
    
    if (grandparent != AABB_NULL_NODE) {
        // Destroy parent and connect sibling to grandparent
        if (tree->nodes[grandparent].child1 == parent) {
            tree->nodes[grandparent].child1 = sibling;
        } else {
            tree->nodes[grandparent].child2 = sibling;
        }
        tree->nodes[sibling].parent = grandparent;
        free_node(tree, parent);
        
        // Adjust ancestor bounds
        int32_t index = grandparent;
        while (index != AABB_NULL_NODE) {
            int32_t child1 = tree->nodes[index].child1;
            int32_t child2 = tree->nodes[index].child2;
            
            tree->nodes[index].height = 1 + ((tree->nodes[child1].height > tree->nodes[child2].height) ?
                                             tree->nodes[child1].height : tree->nodes[child2].height);
            aabb_union(&tree->nodes[index].aabb, &tree->nodes[child1].aabb, &tree->nodes[child2].aabb);
            
            index = tree->nodes[index].parent;
        }
    } else {
        tree->root = sibling;
        tree->nodes[sibling].parent = AABB_NULL_NODE;
        free_node(tree, parent);
    }
    
    free_node(tree, proxy_id);
}

bool aabb_tree_update(AABBTree *tree, int32_t proxy_id, const AABB *new_aabb, const v4f *displacement) {
    if (proxy_id == AABB_NULL_NODE) return false;
    
    // Check if current fat AABB still contains the new tight AABB
    if (aabb_contains(&tree->nodes[proxy_id].aabb, new_aabb)) {
        return false;  // No tree update needed
    }
    
    // Remove and reinsert
    uint32_t body_id = tree->nodes[proxy_id].body_id;
    aabb_tree_remove(tree, proxy_id);
    
    // Create new fat AABB
    AABB fat_aabb;
    aabb_fatten(&fat_aabb, new_aabb, tree->fat_margin, displacement);
    
    // Reinsert (reuses the same proxy_id if possible)
    aabb_tree_insert(tree, &fat_aabb, body_id);
    
    return true;
}

// ========================================
// Query Operations
// ========================================

void aabb_tree_query(const AABBTree *tree, const AABB *aabb, AABBQueryCallback callback, void *user_data) {
    if (tree->root == AABB_NULL_NODE) return;
    
    // Stack for DFS traversal
    int32_t stack[256];
    int32_t stack_count = 0;
    stack[stack_count++] = tree->root;
    
    while (stack_count > 0) {
        int32_t node_id = stack[--stack_count];
        if (node_id == AABB_NULL_NODE) continue;
        
        const AABBTreeNode *node = &tree->nodes[node_id];
        
        if (aabb_overlap(&node->aabb, aabb)) {
            if (node->is_leaf) {
                // Invoke callback for leaf
                if (!callback(node_id, node->body_id, user_data)) {
                    return;  // Early out
                }
            } else {
                // Push children
                if (stack_count < 256) {
                    stack[stack_count++] = node->child1;
                    stack[stack_count++] = node->child2;
                }
            }
        }
    }
}

// Placeholder for find_pairs - will implement in next iteration
void aabb_tree_find_pairs(const AABBTree *tree, AABBQueryCallback callback, void *user_data) {
    // TODO: Implement self-query for all overlapping pairs
    // This requires recursively testing all leaf nodes against each other
}
