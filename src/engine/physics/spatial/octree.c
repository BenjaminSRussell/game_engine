// Spatial octree implementation for queries.
#include <spatial/octree.h>
#include <stdlib.h>
#include <string.h>

static void octree_node_free(OctreeNode *node) {
    if (!node) return;
    
    if (!node->is_leaf) {
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                octree_node_free(node->children[i]);
                free(node->children[i]);
            }
        }
    }
    
    if (node->data) {
        free(node->data);
    }
}

void octree_init(Octree *tree, AABB bounds, u32 max_depth, u32 max_items_per_node) {
    tree->max_depth = max_depth;
    tree->max_items_per_node = max_items_per_node;
    tree->root = (OctreeNode *)calloc(1, sizeof(OctreeNode));
    tree->root->bounds = bounds;
    tree->root->is_leaf = true;
    tree->root->data = NULL;
    tree->root->data_count = 0;
    tree->root->data_capacity = 0;
}

void octree_free(Octree *tree) {
    if (tree->root) {
        octree_node_free(tree->root);
        free(tree->root);
        tree->root = NULL;
    }
}

static void octree_subdivide(OctreeNode *node, u32 current_depth, u32 max_depth, u32 max_items_per_node) {
    if (current_depth >= max_depth || node->is_leaf == false) {
        return;
    }
    
    Vec3 center = aabb_center(node->bounds);
    Vec3 size = aabb_size(node->bounds);
    Vec3 half_size = vec3_mul(size, 0.5f);
    Vec3 quarter_size = vec3_mul(size, 0.25f);
    
    // Create 8 children
    for (int i = 0; i < 8; i++) {
        node->children[i] = (OctreeNode *)calloc(1, sizeof(OctreeNode));
        OctreeNode *child = node->children[i];
        
        // Calculate child bounds based on octant
        Vec3 offset = vec3(
            (i & 1) ? quarter_size.x : -quarter_size.x,
            (i & 2) ? quarter_size.y : -quarter_size.y,
            (i & 4) ? quarter_size.z : -quarter_size.z
        );
        
        child->bounds = aabb_from_center_size(vec3_add(center, offset), half_size);
        child->is_leaf = true;
        child->data = NULL;
        child->data_count = 0;
        child->data_capacity = 0;
    }
    
    node->is_leaf = false;
    
    // Redistribute items to children
    for (u32 i = 0; i < node->data_count; i++) {
        void *data = node->data[i];
        // For simplicity, we'd need bounds for each item - this is a simplified version
        // In practice, you'd store bounds with each item
    }
    
    node->data_count = 0;
}

bool octree_insert(Octree *tree, void *data, AABB bounds) {
    // Simplified insertion - in practice would need to track bounds per item
    OctreeNode *node = tree->root;
    
    // Find appropriate leaf node
    while (!node->is_leaf) {
        Vec3 center = aabb_center(node->bounds);
        int child_index = 0;
        if (bounds.min.x >= center.x) child_index |= 1;
        if (bounds.min.y >= center.y) child_index |= 2;
        if (bounds.min.z >= center.z) child_index |= 4;
        node = node->children[child_index];
    }
    
    // Add to node
    if (node->data_count >= node->data_capacity) {
        u32 new_capacity = node->data_capacity == 0 ? 4 : node->data_capacity * 2;
        node->data = (void **)realloc(node->data, sizeof(void *) * new_capacity);
        node->data_capacity = new_capacity;
    }
    
    node->data[node->data_count++] = data;
    
    // Subdivide if needed
    if (node->data_count > tree->max_items_per_node) {
        // Would need depth tracking and bounds per item for proper subdivision
        // octree_subdivide(node, 0, tree->max_depth, tree->max_items_per_node);
    }
    
    return true;
}

bool octree_remove(Octree *tree, void *data, AABB bounds) {
    // Simplified removal - would need proper bounds tracking
    (void)tree;
    (void)data;
    (void)bounds;
    return false;
}

static void octree_query_node(OctreeNode *node, AABB bounds, OctreeQueryCallback callback, void *user_data) {
    if (!aabb_intersects_aabb(node->bounds, bounds)) {
        return;
    }
    
    if (node->is_leaf) {
        for (u32 i = 0; i < node->data_count; i++) {
            callback(node->data[i], user_data);
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                octree_query_node(node->children[i], bounds, callback, user_data);
            }
        }
    }
}

void octree_query_aabb(Octree *tree, AABB bounds, OctreeQueryCallback callback, void *user_data) {
    if (tree->root) {
        octree_query_node(tree->root, bounds, callback, user_data);
    }
}

void octree_query_point(Octree *tree, Vec3 point, OctreeQueryCallback callback, void *user_data) {
    AABB point_bounds = aabb(point, point);
    octree_query_aabb(tree, point_bounds, callback, user_data);
}

void octree_query_ray(Octree *tree, Vec3 origin, Vec3 direction, f32 max_distance,
                      OctreeQueryCallback callback, void *user_data) {
    // Simplified ray query - would need proper ray-AABB intersection
    (void)tree;
    (void)origin;
    (void)direction;
    (void)max_distance;
    (void)callback;
    (void)user_data;
}

