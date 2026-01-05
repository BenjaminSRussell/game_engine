#ifndef CORE_CONTAINERS_ADVANCED_OCTREE_H
#define CORE_CONTAINERS_ADVANCED_OCTREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Octree Octree;

typedef struct OctreeAABB {
  float min[3];
  float max[3];
} OctreeAABB;

typedef struct OctreeItem {
  OctreeAABB bounds;
  void *payload;
} OctreeItem;

typedef struct OctreeRay {
  float origin[3];
  float direction[3];
} OctreeRay;

typedef struct OctreeFrustum {
  float planes[6][4];
} OctreeFrustum;

typedef bool (*OctreeVisitFn)(const OctreeItem *item, void *user);

Octree *octree_create(OctreeAABB bounds, uint32_t max_depth,
                      uint32_t max_items);
void octree_destroy(Octree *tree);

bool octree_insert(Octree *tree, OctreeItem item);
bool octree_remove(Octree *tree, const OctreeItem *item);
bool octree_update(Octree *tree, const OctreeItem *item,
                   OctreeAABB new_bounds);

void octree_query_frustum(const Octree *tree, const OctreeFrustum *frustum,
                          OctreeVisitFn visit, void *user);
void octree_query_ray(const Octree *tree, const OctreeRay *ray,
                      OctreeVisitFn visit, void *user);
void octree_query_sphere(const Octree *tree, const float center[3],
                         float radius, OctreeVisitFn visit, void *user);
void octree_query_aabb(const Octree *tree, OctreeAABB bounds,
                       OctreeVisitFn visit, void *user);

uint64_t octree_linear_key(const float position[3], uint32_t depth,
                           const OctreeAABB *bounds);

bool octree_debug_dump(const Octree *tree, const char *path);

bool octree_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
