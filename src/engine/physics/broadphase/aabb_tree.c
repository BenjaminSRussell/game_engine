/**
 * Dynamic AABB Tree for Broadphase Collision Detection
 * 
 * Implementation of self-balancing binary tree using Axis-Aligned Bounding Boxes (AABBs).
 * Provides O(N log N) broadphase vs O(N) naive sweep.
 */

#include "aabb_tree.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// ========================================
// Internal Constants
// ========================================

#define AABB_NULL_NODE -1
#define DEFAULT_FAT_MARGIN 0.1f

// ========================================
// AABB Utilities Implementation
// ========================================

bool aabb_overlap(const AABB *a, const AABB *b) {
    return a->max.x > b->min.x && a->min.x < b->max.x &&
           a->max.y > b->min.y && a->min.y < b->max.y &&
           a->max.z > b->min.z && a->min.z < b->max.z;
}

void aabb_union(AABB *result, const AABB *a, const AABB *b) {
    result->min.x = fminf(a->min.x, b->min.x);
    result->min.y = fminf(a->min.y, b->min.y);
    result->min.z = fminf(a->min.z, b->min.z);
    
    result->max.x = fmaxf(a->max.x, b->max.x);
    result->max.y = fmaxf(a->max.y, b->max.y);
    result->max.z = fmaxf(a->max.z, b->max.z);
}

float aabb_surface_area(const AABB *aabb) {
    v4f extent = {
        aabb->max.x - aabb->min.x,
        aabb->max.y - aabb->min.y,
        aabb->max.z - aabb->min.z,
        0.0f
    };
    
    return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
}

void aabb_fatten(AABB *fat, const AABB *tight, float margin, const v4f *displacement) {
    fat->min.x = tight->min.x - margin - fabsf(displacement->x);
    fat->min.y = tight->min.y - margin - fabsf(displacement->y);
    fat->min.z = tight->min.z - margin - fabsf(displacement->z);
    
    fat->max.x = tight->max.x + margin + fabsf(displacement->x);
    fat->max.y = tight->max.y + margin + fabsf(displacement->y);
    fat->max.z = tight->max.z + margin + fabsf(displacement->z);
}

bool aabb_contains(const AABB *fat, const AABB *tight) {
    return fat->min.x <= tight->min.x && fat->min.y <= tight->min.y && fat->min.z <= tight->min.z &&
           fat->max.x >= tight->max.x && fat->max.y >= tight->max.y && fat->max.z >= tight->max.z;
}

// ========================================
// Tree Node Management
// ========================================

static int32_t aabb_tree_allocate_node(AABBTree *tree) {
    if (tree->free_list != AABB_NULL_NODE) {
        int32_t node_id = tree->free_list;
        tree->free_list = tree->nodes[node_id].parent;
        return node_id;
    }
    
    if (tree->node_count >= tree->node_capacity) {
        // Double capacity
        uint32_t new_capacity = tree->node_capacity * 2;
        AABBTreeNode *new_nodes = (AABBTreeNode *)realloc(tree->nodes, new_capacity * sizeof(AABBTreeNode));
        if (!new_nodes) return AABB_NULL_NODE;
        
        tree->nodes = new_nodes;
        tree->node_capacity = new_capacity;
    }
    
    return tree->node_count++;
}

static void aabb_tree_free_node(AABBTree *tree, int32_t node_id) {
    tree->nodes[node_id].parent = tree->free_list;
    tree->free_list = node_id;
}

// ========================================
// Tree Operations
// ========================================

static void aabb_tree_update_height(AABBTree *tree, int32_t node_id) {
    if (node_id == AABB_NULL_NODE) return;
    
    AABBTreeNode *node = &tree->nodes[node_id];
    if (node->is_leaf) {
        node->height = 0;
        return;
    }
    
    int32_t child1 = node->child1;
    int32_t child2 = node->child2;
    
    int32_t height1 = tree->nodes[child1].height;
    int32_t height2 = tree->nodes[child2].height;
    node->height = 1 + (height1 > height2 ? height1 : height2);
}

static int32_t aabb_tree_balance(AABBTree *tree, int32_t node_id) {
    if (node_id == AABB_NULL_NODE) return AABB_NULL_NODE;
    
    AABBTreeNode *node = &tree->nodes[node_id];
    if (node->is_leaf || node->height < 2) {
        return node_id;
    }
    
    int32_t child1 = node->child1;
    int32_t child2 = node->child2;
    
    int32_t balance = tree->nodes[child2].height - tree->nodes[child1].height;
    
    // Right subtree is much larger
    if (balance > 1) {
        int32_t grandchild1 = tree->nodes[child2].child1;
        int32_t grandchild2 = tree->nodes[child2].child2;
        
        // Rotate right
        tree->nodes[child2].child1 = node_id;
        tree->nodes[child2].parent = node->parent;
        node->parent = child2;
        
        if (tree->nodes[child2].parent != AABB_NULL_NODE) {
            if (tree->nodes[tree->nodes[child2].parent].child1 == node_id) {
                tree->nodes[tree->nodes[child2].parent].child1 = child2;
            } else {
                tree->nodes[tree->nodes[child2].parent].child2 = child2;
            }
        } else {
            tree->root = child2;
        }
        
        node->child2 = grandchild1;
        tree->nodes[grandchild1].parent = node_id;
        
        aabb_tree_update_height(tree, node_id);
        aabb_tree_update_height(tree, child2);
        
        return child2;
    }
    
    // Left subtree is much larger
    if (balance < -1) {
        int32_t grandchild1 = tree->nodes[child1].child1;
        int32_t grandchild2 = tree->nodes[child1].child2;
        
        // Rotate left
        tree->nodes[child1].child1 = node_id;
        tree->nodes[child1].parent = node->parent;
        node->parent = child1;
        
        if (tree->nodes[child1].parent != AABB_NULL_NODE) {
            if (tree->nodes[tree->nodes[child1].parent].child1 == node_id) {
                tree->nodes[tree->nodes[child1].parent].child1 = child1;
            } else {
                tree->nodes[tree->nodes[child1].parent].child2 = child1;
            }
        } else {
            tree->root = child1;
        }
        
        node->child1 = grandchild2;
        tree->nodes[grandchild2].parent = node_id;
        
        aabb_tree_update_height(tree, node_id);
        aabb_tree_update_height(tree, child1);
        
        return child1;
    }
    
    return node_id;
}

// ========================================
// Public API Implementation
// ========================================

AABBTree* aabb_tree_create(uint32_t capacity, float fat_margin) {
    if (capacity == 0) capacity = 16;
    if (fat_margin <= 0.0f) fat_margin = DEFAULT_FAT_MARGIN;
    
    AABBTree *tree = (AABBTree *)calloc(1, sizeof(AABBTree));
    if (!tree) return NULL;
    
    tree->nodes = (AABBTreeNode *)calloc(capacity, sizeof(AABBTreeNode));
    if (!tree->nodes) {
        free(tree);
        return NULL;
    }
    
    tree->node_capacity = capacity;
    tree->root = AABB_NULL_NODE;
    tree->free_list = AABB_NULL_NODE;
    tree->fat_margin = fat_margin;
    
    return tree;
}

void aabb_tree_destroy(AABBTree *tree) {
    if (tree) {
        free(tree->nodes);
        free(tree);
    }
}

int32_t aabb_tree_insert(AABBTree *tree, const AABB *aabb, uint32_t body_id) {
    if (!tree || !aabb) return AABB_NULL_NODE;
    
    // Create fat AABB
    AABB fat_aabb;
    v4f zero_displacement = {0, 0, 0, 0};
    aabb_fatten(&fat_aabb, aabb, tree->fat_margin, &zero_displacement);
    
    // Allocate new leaf node
    int32_t leaf_id = aabb_tree_allocate_node(tree);
    if (leaf_id == AABB_NULL_NODE) return AABB_NULL_NODE;
    
    AABBTreeNode *leaf = &tree->nodes[leaf_id];
    leaf->aabb = fat_aabb;
    leaf->body_id = body_id;
    leaf->user_data = NULL;
    leaf->parent = AABB_NULL_NODE;
    leaf->height = 0;
    leaf->is_leaf = true;
    
    // Insert into tree
    if (tree->root == AABB_NULL_NODE) {
        tree->root = leaf_id;
    } else {
        // Find best sibling for the new leaf
        int32_t sibling = tree->root;
        AABB leaf_aabb = fat_aabb;
        
        while (!tree->nodes[sibling].is_leaf) {
            int32_t child1 = tree->nodes[sibling].child1;
            int32_t child2 = tree->nodes[sibling].child2;
            
            AABB combined1, combined2;
            aabb_union(&combined1, &tree->nodes[child1].aabb, &leaf_aabb);
            aabb_union(&combined2, &tree->nodes[child2].aabb, &leaf_aabb);
            
            float cost1 = aabb_surface_area(&combined1) - aabb_surface_area(&tree->nodes[child1].aabb);
            float cost2 = aabb_surface_area(&combined2) - aabb_surface_area(&tree->nodes[child2].aabb);
            
            if (cost1 < cost2) {
                sibling = child1;
            } else {
                sibling = child2;
            }
        }
        
        // Create new parent node
        int32_t parent_id = aabb_tree_allocate_node(tree);
        int32_t old_parent = tree->nodes[sibling].parent;
        
        AABBTreeNode *parent = &tree->nodes[parent_id];
        parent->parent = old_parent;
        parent->is_leaf = false;
        parent->height = tree->nodes[sibling].height + 1;
        
        if (old_parent != AABB_NULL_NODE) {
            if (tree->nodes[old_parent].child1 == sibling) {
                tree->nodes[old_parent].child1 = parent_id;
            } else {
                tree->nodes[old_parent].child2 = parent_id;
            }
        } else {
            tree->root = parent_id;
        }
        
        parent->child1 = sibling;
        parent->child2 = leaf_id;
        tree->nodes[sibling].parent = parent_id;
        leaf->parent = parent_id;
        
        // Update parent AABB
        aabb_union(&parent->aabb, &tree->nodes[sibling].aabb, &leaf->aabb);
        
        // Balance tree
        int32_t current = parent_id;
        while (current != AABB_NULL_NODE) {
            current = aabb_tree_balance(tree, current);
            
            if (tree->nodes[current].parent != AABB_NULL_NODE) {
                int32_t parent_of_current = tree->nodes[current].parent;
                int32_t sibling = (tree->nodes[parent_of_current].child1 == current) ? 
                                 tree->nodes[parent_of_current].child2 : 
                                 tree->nodes[parent_of_current].child1;
                aabb_union(&tree->nodes[parent_of_current].aabb, &tree->nodes[current].aabb, &tree->nodes[sibling].aabb);
            }
            
            current = tree->nodes[current].parent;
        }
    }
    
    return leaf_id;
}

void aabb_tree_remove(AABBTree *tree, int32_t proxy_id) {
    if (!tree || proxy_id == AABB_NULL_NODE) return;
    
    AABBTreeNode *leaf = &tree->nodes[proxy_id];
    if (!leaf->is_leaf) return;
    
    if (proxy_id == tree->root) {
        tree->root = AABB_NULL_NODE;
    } else {
        int32_t parent_id = leaf->parent;
        int32_t grandparent_id = tree->nodes[parent_id].parent;
        int32_t sibling_id = (tree->nodes[parent_id].child1 == proxy_id) ? 
                            tree->nodes[parent_id].child2 : 
                            tree->nodes[parent_id].child1;
        
        if (grandparent_id != AABB_NULL_NODE) {
            if (tree->nodes[grandparent_id].child1 == parent_id) {
                tree->nodes[grandparent_id].child1 = sibling_id;
            } else {
                tree->nodes[grandparent_id].child2 = sibling_id;
            }
            tree->nodes[sibling_id].parent = grandparent_id;
            
            // Balance tree
            int32_t current = grandparent_id;
            while (current != AABB_NULL_NODE) {
                current = aabb_tree_balance(tree, current);
                
                if (tree->nodes[current].parent != AABB_NULL_NODE) {
                    int32_t parent_of_current = tree->nodes[current].parent;
                    int32_t sibling = (tree->nodes[parent_of_current].child1 == current) ? 
                                     tree->nodes[parent_of_current].child2 : 
                                     tree->nodes[parent_of_current].child1;
                    aabb_union(&tree->nodes[parent_of_current].aabb, &tree->nodes[current].aabb, &tree->nodes[sibling].aabb);
                }
                
                current = tree->nodes[current].parent;
            }
        } else {
            tree->root = sibling_id;
            tree->nodes[sibling_id].parent = AABB_NULL_NODE;
        }
        
        aabb_tree_free_node(tree, parent_id);
    }
    
    aabb_tree_free_node(tree, proxy_id);
}

bool aabb_tree_update(AABBTree *tree, int32_t proxy_id, const AABB *new_aabb, const v4f *displacement) {
    if (!tree || proxy_id == AABB_NULL_NODE || !new_aabb) return false;
    
    AABBTreeNode *leaf = &tree->nodes[proxy_id];
    if (!leaf->is_leaf) return false;
    
    // Check if object is still contained in fat AABB
    if (aabb_contains(&leaf->aabb, new_aabb)) {
        return false; // No update needed
    }
    
    // Remove and re-insert with new fat AABB
    uint32_t body_id = leaf->body_id;
    void *user_data = leaf->user_data;
    
    aabb_tree_remove(tree, proxy_id);
    
    AABB fat_aabb;
    aabb_fatten(&fat_aabb, new_aabb, tree->fat_margin, displacement);
    
    int32_t new_proxy_id = aabb_tree_insert(tree, &fat_aabb, body_id);
    if (new_proxy_id != AABB_NULL_NODE) {
        tree->nodes[new_proxy_id].user_data = user_data;
    }
    
    return true;
}

void aabb_tree_query(const AABBTree *tree, const AABB *aabb, AABBQueryCallback callback, void *user_data) {
    if (!tree || !aabb || !callback || tree->root == AABB_NULL_NODE) return;
    
    // Simple stack-based traversal
    int32_t stack[64];
    int32_t stack_top = 0;
    stack[stack_top++] = tree->root;
    
    while (stack_top > 0) {
        int32_t node_id = stack[--stack_top];
        AABBTreeNode *node = &tree->nodes[node_id];
        
        if (!aabb_overlap(aabb, &node->aabb)) continue;
        
        if (node->is_leaf) {
            if (!callback(node_id, node->body_id, user_data)) break;
        } else {
            if (stack_top < 63) {
                stack[stack_top++] = node->child1;
                stack[stack_top++] = node->child2;
            }
        }
    }
}

void aabb_tree_find_pairs(const AABBTree *tree, AABBQueryCallback callback, void *user_data) {
    if (!tree || !callback || tree->root == AABB_NULL_NODE) return;
    
    // This is a simplified implementation - a more efficient version would
    // traverse the tree once and generate pairs
    typedef struct {
        int32_t proxy_id;
        uint32_t body_id;
    } QueryResult;
    
    QueryResult results[1024];
    uint32_t result_count = 0;
    
    // Collect all leaf nodes
    int32_t stack[64];
    int32_t stack_top = 0;
    stack[stack_top++] = tree->root;
    
    while (stack_top > 0 && result_count < 1024) {
        int32_t node_id = stack[--stack_top];
        AABBTreeNode *node = &tree->nodes[node_id];
        
        if (node->is_leaf) {
            results[result_count].proxy_id = node_id;
            results[result_count].body_id = node->body_id;
            result_count++;
        } else {
            if (stack_top < 62) {
                stack[stack_top++] = node->child1;
                stack[stack_top++] = node->child2;
            }
        }
    }
    
    // Generate pairs (O(N) for simplicity - could be optimized)
    for (uint32_t i = 0; i < result_count; i++) {
        for (uint32_t j = i + 1; j < result_count; j++) {
            AABBTreeNode *node1 = &tree->nodes[results[i].proxy_id];
            AABBTreeNode *node2 = &tree->nodes[results[j].proxy_id];
            
            if (aabb_overlap(&node1->aabb, &node2->aabb)) {
                if (!callback(results[i].proxy_id, results[j].body_id, user_data)) return;
            }
        }
    }
}
