#ifndef CONTINUOUS_COLLISION_H
#define CONTINUOUS_COLLISION_H

#include "math/quat.h"
#include "math/vec3.h"
#include "physics/physics.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Continuous Collision Detection (CCD) Types
typedef enum {
  CCD_SHAPE_SPHERE = 0,
  CCD_SHAPE_BOX,
  CCD_SHAPE_CAPSULE,
  CCD_SHAPE_CONVEX_HULL,
  CCD_SHAPE_TRIANGLE_MESH,
  CCD_SHAPE_COUNT
} CCDShapeType;

// CCD Motion Mode
typedef enum {
  CCD_MOTION_LINEAR = 0,
  CCD_MOTION_ANGULAR,
  CCD_MOTION_BOTH
} CCDMotionMode;

// CCD Collision Result
typedef struct {
  bool hit;
  float time_of_impact;
  Vec3 contact_point;
  Vec3 contact_normal;
  Vec3 penetration_depth;
  EntityID entity_a;
  EntityID entity_b;
  CCDShapeType shape_a;
  CCDShapeType shape_b;
  float impact_velocity;
} CCDCollisionResult;

// CCD Shape Base
typedef struct {
  CCDShapeType type;
  Vec3 position;
  Quat rotation;
  Vec3 linear_velocity;
  Vec3 angular_velocity;
  bool is_static;
  float margin;
  uint32_t collision_mask;
  uint32_t collision_group;
} CCDShape;

// CCD Sphere
typedef struct {
  CCDShape base;
  float radius;
} CCDSphere;

// CCD Box
typedef struct {
  CCDShape base;
  Vec3 half_extents;
} CCDBox;

// CCD Capsule
typedef struct {
  CCDShape base;
  float radius;
  float height;
} CCDCapsule;

// CCD Triangle
typedef struct {
  Vec3 vertices[3];
  Vec3 normal;
  Vec3 edge_a;
  Vec3 edge_b;
  Vec3 edge_c;
} CCDTriangle;

// CCD Triangle Mesh
typedef struct {
  CCDShape base;
  CCDTriangle *triangles;
  uint32_t triangle_count;
  Vec3 *vertices;
  uint32_t vertex_count;
  uint32_t *indices;
  uint32_t index_count;
  Vec3 bounds_min;
  Vec3 bounds_max;
} CCDTriangleMesh;

// CCD Broadphase Structure
typedef struct {
  Vec3 position;
  Vec3 velocity;
  Vec3 bounds_min;
  Vec3 bounds_max;
  EntityID entity;
  CCDShapeType shape_type;
  uint32_t broadphase_index;
} CCDBroadphaseEntry;

// CCD World
typedef struct {
  CCDBroadphaseEntry *entries;
  uint32_t entry_count;
  uint32_t entry_capacity;

  // Spatial hashing for broadphase
  struct {
    uint32_t *hash_table;
    uint32_t *entry_lists;
    uint32_t table_size;
    float cell_size;
    Vec3 world_min;
    Vec3 world_max;
  } spatial_hash;

  // Configuration
  float max_time_step;
  float ccd_threshold;
  bool enable_speculative_contacts;
  bool enable_motion_clamping;
  int max_substeps;
  float contact_erp;
  float contact_cfm;

  // Statistics
  uint32_t broadphase_tests;
  uint32_t narrowphase_tests;
  uint32_t collisions_found;
  float ccd_time;
} CCDWorld;

// CCD World Functions
bool ccd_world_init(CCDWorld *world, uint32_t max_entities);
void ccd_world_cleanup(CCDWorld *world);
void ccd_world_update(CCDWorld *world, float time_step);

// Shape Management
bool ccd_world_add_sphere(CCDWorld *world, EntityID entity,
                          const CCDSphere *sphere);
bool ccd_world_add_box(CCDWorld *world, EntityID entity, const CCDBox *box);
bool ccd_world_add_capsule(CCDWorld *world, EntityID entity,
                           const CCDCapsule *capsule);
bool ccd_world_add_triangle_mesh(CCDWorld *world, EntityID entity,
                                 const CCDTriangleMesh *mesh);
bool ccd_world_remove_entity(CCDWorld *world, EntityID entity);
bool ccd_world_update_entity_motion(CCDWorld *world, EntityID entity,
                                    Vec3 linear_velocity,
                                    Vec3 angular_velocity);

// Continuous Collision Detection
bool ccd_detect_collision(const CCDShape *shape_a, const CCDShape *shape_b,
                          float time_step, CCDCollisionResult *result);
bool ccd_sphere_vs_sphere(const CCDSphere *sphere_a, const CCDSphere *sphere_b,
                          float time_step, CCDCollisionResult *result);
bool ccd_sphere_vs_box(const CCDSphere *sphere, const CCDBox *box,
                       float time_step, CCDCollisionResult *result);
bool ccd_sphere_vs_capsule(const CCDSphere *sphere, const CCDCapsule *capsule,
                           float time_step, CCDCollisionResult *result);
bool ccd_box_vs_box(const CCDBox *box_a, const CCDBox *box_b, float time_step,
                    CCDCollisionResult *result);
bool ccd_capsule_vs_capsule(const CCDCapsule *capsule_a,
                            const CCDCapsule *capsule_b, float time_step,
                            CCDCollisionResult *result);

// Broadphase Collision Detection
void ccd_broadphase_update(CCDWorld *world);
void ccd_spatial_hash_init(CCDWorld *world);
void ccd_spatial_hash_insert(CCDWorld *world, uint32_t entry_index);
void ccd_spatial_hash_remove(CCDWorld *world, uint32_t entry_index);
void ccd_spatial_hash_query(const CCDWorld *world, Vec3 bounds_min,
                            Vec3 bounds_max, uint32_t *results,
                            uint32_t *result_count);

// Ray Casting with CCD
bool ccd_ray_cast(const CCDWorld *world, Vec3 ray_start, Vec3 ray_direction,
                  float max_distance, CCDCollisionResult *result);
bool ccd_ray_cast_sphere(const CCDSphere *sphere, Vec3 ray_start,
                         Vec3 ray_direction, float max_distance,
                         CCDCollisionResult *result);
bool ccd_ray_cast_box(const CCDBox *box, Vec3 ray_start, Vec3 ray_direction,
                      float max_distance, CCDCollisionResult *result);

// Utility Functions
// Utility Functions
bool ccd_should_use_ccd(const CCDShape *shape, float time_step);
float ccd_calculate_swept_bounds(const CCDShape *shape, float time_step,
                                 Vec3 *bounds_min, Vec3 *bounds_max);
bool ccd_bounds_intersect(Vec3 min_a, Vec3 max_a, Vec3 min_b, Vec3 max_b);
uint32_t ccd_spatial_hash_position_to_cell(const CCDWorld *world,
                                           Vec3 position);
void ccd_update_broadphase_bounds(CCDWorld *world, uint32_t entry_index);

// Contact Generation and Resolution
void ccd_resolve_collision(CCDWorld *world,
                           const CCDCollisionResult *collision);
void ccd_apply_impulse(const CCDCollisionResult *collision,
                       float impulse_magnitude);
void ccd_apply_position_correction(const CCDCollisionResult *collision,
                                   float correction_magnitude);

// Motion Integration
void ccd_integrate_motion(CCDShape *shape, float time_step);
Vec3 ccd_extrapolate_position(const CCDShape *shape, float time);
Quat ccd_extrapolate_rotation(const CCDShape *shape, float time);

// Debug and Visualization
void ccd_debug_draw_bounds(const CCDWorld *world);
void ccd_debug_draw_contacts(const CCDWorld *world);
void ccd_debug_draw_swept_shapes(const CCDWorld *world, float time_step);

// Performance Profiling
typedef struct {
  uint32_t total_tests;
  uint32_t broadphase_hits;
  uint32_t narrowphase_hits;
  uint32_t collisions_resolved;
  float broadphase_time;
  float narrowphase_time;
  float resolution_time;
  float total_time;
} CCDPerformanceStats;

void ccd_get_performance_stats(const CCDWorld *world,
                               CCDPerformanceStats *stats);
void ccd_reset_performance_stats(CCDWorld *world);

#ifdef __cplusplus
}
#endif

#endif // CONTINUOUS_COLLISION_H
