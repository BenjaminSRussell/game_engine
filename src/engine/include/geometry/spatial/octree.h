// include/spatial/octree.h
//
// Purpose: Defines a generic Octree spatial partitioning data structure, designed
// for efficient management and querying of 3D data in a hierarchical manner.
// This header provides the necessary structures and API for inserting, removing,
// and performing various spatial queries (e.g., AABB intersection, point containment,
// ray intersection) against objects stored within the octree, thereby accelerating
// operations like collision detection and frustum culling.
//
// Public APIs:
// - `AABB`: Structure representing an Axis-Aligned Bounding Box with `min` and `max`
//   `Vec3` corners, along with inline helper functions (`aabb`, `aabb_from_center_size`,
//   `aabb_contains_point`, `aabb_intersects_aabb`, `aabb_center`, `aabb_size`).
// - `OctreeNode`: Internal structure representing a node in the octree, containing its
//   `bounds`, pointers to up to 8 `children` nodes, an array of pointers to user `data`,
//   and metadata like `data_count` and `is_leaf` status.
// - `Octree`: The main structure encapsulating the octree, including its `root` node,
//   `max_depth`, and `max_items_per_node` parameters for controlling its subdivision.
// - `octree_init`: Initializes an `Octree` with a given bounding box, maximum depth,
//   and maximum items per leaf node.
// - `octree_free`: Frees all dynamically allocated memory associated with the octree.
// - `octree_insert`: Inserts a piece of user `data` associated with an `AABB` into the octree.
// - `octree_remove`: Removes a piece of user `data` associated with an `AABB` from the octree.
// - `OctreeQueryCallback`: Function pointer type for callbacks used during spatial queries.
// - `octree_query_aabb`: Performs a query for all data whose bounds intersect with a given `AABB`.
// - `octree_query_point`: Performs a query for all data whose bounds contain a given `point`.
// - `octree_query_ray`: Performs a ray intersection query against data in the octree.
//
// Ownership: An `Octree` instance owns its `OctreeNode`s and the internal arrays for `data`.
// It stores pointers to user-defined data (`void* data`), but does not own or manage the
// memory pointed to by these `data` pointers; that responsibility lies with the user.
//
// Invariants:
// - An `Octree` must be initialized with `octree_init` before use and freed with `octree_free`.
// - `max_depth` and `max_items_per_node` control the tree's subdivision; setting them too
//   high or too low can affect performance.
// - `AABB` intersection tests assume correctly defined bounding boxes.
// - `Vec3` structures (from `vec3.h`) are assumed to be correctly defined.
//
#ifndef OCTREE_H
#define OCTREE_H


#include <common.h>
#include <math/vec3.h>

// Axis-aligned bounding box
typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

// Octree node
typedef struct OctreeNode {
    AABB bounds;
    struct OctreeNode *children[8];
    void **data;           // Array of pointers to user data
    u32 data_count;
    u32 data_capacity;
    bool is_leaf;
} OctreeNode;

// Octree structure
typedef struct {
    OctreeNode *root;
    u32 max_depth;
    u32 max_items_per_node;
} Octree;

// Initialize octree
void octree_init(Octree *tree, AABB bounds, u32 max_depth, u32 max_items_per_node);
void octree_free(Octree *tree);

// Insert/remove items
bool octree_insert(Octree *tree, void *data, AABB bounds);
bool octree_remove(Octree *tree, void *data, AABB bounds);

// Query operations
typedef void (*OctreeQueryCallback)(void *data, void *user_data);
void octree_query_aabb(Octree *tree, AABB bounds, OctreeQueryCallback callback, void *user_data);
void octree_query_point(Octree *tree, Vec3 point, OctreeQueryCallback callback, void *user_data);
void octree_query_ray(Octree *tree, Vec3 origin, Vec3 direction, f32 max_distance, 
                      OctreeQueryCallback callback, void *user_data);

// AABB helpers
INLINE AABB aabb(Vec3 min, Vec3 max) {
    AABB box = {min, max};
    return box;
}

INLINE AABB aabb_from_center_size(Vec3 center, Vec3 size) {
    Vec3 half = vec3_mul(size, 0.5f);
    return aabb(vec3_sub(center, half), vec3_add(center, half));
}

INLINE bool aabb_contains_point(AABB box, Vec3 point) {
    return point.x >= box.min.x && point.x <= box.max.x &&
           point.y >= box.min.y && point.y <= box.max.y &&
           point.z >= box.min.z && point.z <= box.max.z;
}

INLINE bool aabb_intersects_aabb(AABB a, AABB b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

INLINE Vec3 aabb_center(AABB box) {
    return vec3(
        (box.min.x + box.max.x) * 0.5f,
        (box.min.y + box.max.y) * 0.5f,
        (box.min.z + box.max.z) * 0.5f
    );
}

INLINE Vec3 aabb_size(AABB box) {
    return vec3_sub(box.max, box.min);
}

#endif // OCTREE_H

