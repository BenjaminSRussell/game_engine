/**
 * @file cloth_collision.h
 * @brief Cloth self-collision detection and response
 */

#ifndef RENDER_CLOTH_COLLISION_H
#define RENDER_CLOTH_COLLISION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_CLOTH_VERTICES 10000
#define MAX_CLOTH_TRIANGLES 20000

typedef struct {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
} ClothVertex;

typedef struct {
    int v0, v1, v2;
} ClothTriangle;

typedef struct {
    ClothVertex vertices[MAX_CLOTH_VERTICES];
    ClothTriangle triangles[MAX_CLOTH_TRIANGLES];
    int vertex_count;
    int triangle_count;
} ClothMesh;

bool cloth_collision_init(ClothMesh* cloth);
void cloth_collision_update(ClothMesh* cloth, float dt);
bool cloth_self_collision_detect(ClothMesh* cloth);
void cloth_collision_resolve(ClothMesh* cloth);
void cloth_collision_cleanup(ClothMesh* cloth);

// ========================================
// Type definitions
// ========================================

typedef struct ClothCollisionWorld ClothCollisionWorld;

typedef struct ClothCollisionHandle {
    uint32_t id;
} ClothCollisionHandle;

typedef enum ClothCollisionPrimitiveType {
    CLOTH_COLLISION_SPHERE = 0,
    CLOTH_COLLISION_PLANE = 1,
    CLOTH_COLLISION_CAPSULE = 2,
    CLOTH_COLLISION_BOX = 3
} ClothCollisionPrimitiveType;

typedef struct ClothCollisionSphere {
    float center[3];
    float radius;
} ClothCollisionSphere;

typedef struct ClothCollisionPlane {
    float normal[3];
    float offset;
} ClothCollisionPlane;

typedef struct ClothCollisionCapsule {
    float point_a[3];
    float point_b[3];
    float radius;
} ClothCollisionCapsule;

typedef struct ClothCollisionBox {
    float min[3];
    float max[3];
} ClothCollisionBox;

typedef struct ClothCollisionPrimitive {
    ClothCollisionPrimitiveType type;
    bool enabled;
    union {
        ClothCollisionSphere sphere;
        ClothCollisionPlane plane;
        ClothCollisionCapsule capsule;
        ClothCollisionBox box;
    } data;
} ClothCollisionPrimitive;

typedef struct ClothCollisionStats {
    uint32_t primitive_count;
    uint64_t collision_tests;
    uint64_t collisions;
} ClothCollisionStats;

typedef struct ClothCollisionGpuProfiler {
    void (*begin)(const char *label, void *user_data);
    void (*end)(const char *label, void *user_data);
    void *user_data;
} ClothCollisionGpuProfiler;

typedef struct ClothCollisionConfig {
    uint32_t max_primitives;
    float default_restitution;
    float default_friction;
    bool enable_profiling;
} ClothCollisionConfig;

// ========================================
// Function declarations
// ========================================

ClothCollisionWorld *cloth_collision_create(const ClothCollisionConfig *config);
void cloth_collision_destroy(ClothCollisionWorld *world);

ClothCollisionHandle cloth_collision_add_sphere(ClothCollisionWorld *world,
                                                const float *center,
                                                float radius);
ClothCollisionHandle cloth_collision_add_plane(ClothCollisionWorld *world,
                                               const float *normal,
                                               float offset);
ClothCollisionHandle cloth_collision_add_capsule(ClothCollisionWorld *world,
                                                 const float *point_a,
                                                 const float *point_b,
                                                 float radius);
ClothCollisionHandle cloth_collision_add_box(ClothCollisionWorld *world,
                                             const float *min_bounds,
                                             const float *max_bounds);
void cloth_collision_remove(ClothCollisionWorld *world,
                            ClothCollisionHandle handle);
void cloth_collision_clear(ClothCollisionWorld *world);

void cloth_collision_set_profiler(ClothCollisionWorld *world,
                                  const ClothCollisionGpuProfiler *profiler);
void cloth_collision_get_stats(const ClothCollisionWorld *world,
                               ClothCollisionStats *out_stats);

// ========================================
// Main logic
// ========================================

void cloth_collision_resolve(ClothCollisionWorld *world, float *positions,
                             float *velocities, uint32_t particle_count,
                             float particle_radius, float restitution,
                             float friction);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_CLOTH_COLLISION_H */
