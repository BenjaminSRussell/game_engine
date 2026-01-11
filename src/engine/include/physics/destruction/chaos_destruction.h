// physics/destruction/chaos_destruction.h
// Chaos-equivalent destruction system (fracture, debris)
#ifndef CHAOS_DESTRUCTION_H
#define CHAOS_DESTRUCTION_H

#include "include/common.h"
#include "include/math/vec3.h"
#include <Metal/Metal.h>

#define CHAOS_MAX_FRAGMENTS 10000
#define CHAOS_MAX_DESTRUCTIBLES 256

// Fracture pattern
typedef enum {
  FRACTURE_VORONOI, // Random cell-based
  FRACTURE_RADIAL,  // Explode from center
  FRACTURE_SLICED,  // Planar cuts
  FRACTURE_CLUSTER  // Grouped chunks
} FracturePattern;

// Destructible mesh
typedef struct {
  char name[64];

  // Original mesh
  id<MTLBuffer> base_mesh_vertices;
  id<MTLBuffer> base_mesh_indices;
  u32 vertex_count;
  u32 triangle_count;

  // Fracture configuration
  FracturePattern fracture_pattern;
  u32 fragment_count;
  f32 fragment_min_size;
  f32 impact_threshold; // Force needed to break

  // Generated fragments
  id<MTLBuffer> *fragment_meshes;
  Vec3 *fragment_centers;
  f32 *fragment_masses;
  Mat4 *fragment_transforms;
  u32 *fragment_physics_ids; // Link to physics bodies

  bool is_fractured;
  bool fragments_generated;

} DestructibleMesh;

// Destruction event
typedef struct {
  Vec3 impact_point;
  Vec3 impact_normal;
  f32 impact_force;
  u32 destructible_id;
  f32 damage_radius;
} DestructionEvent;

typedef struct {
  DestructibleMesh destructibles[CHAOS_MAX_DESTRUCTIBLES];
  u32 destructible_count;

  // Fragment pool
  u32 active_fragments;
  f32 fragment_lifetime; // Auto-cleanup after N seconds

  // GPU resources
  id<MTLDevice> device;
  id<MTLComputePipelineState> fracture_pipeline;

  // Physics integration
  void *physics_world;

} ChaosDestructionSystem;

#ifdef __cplusplus
extern "C" {
#endif

// System lifecycle
ChaosDestructionSystem *chaos_create(id<MTLDevice> device, void *physics_world);
void chaos_destroy(ChaosDestructionSystem *chaos);

// Destructible management
u32 chaos_add_destructible(ChaosDestructionSystem *chaos,
                           id<MTLBuffer> mesh_vertices,
                           id<MTLBuffer> mesh_indices, u32 vertex_count,
                           u32 triangle_count);

void chaos_configure_fracture(ChaosDestructionSystem *chaos,
                              u32 destructible_id, FracturePattern pattern,
                              u32 fragment_count, f32 impact_threshold);

// Pre-fracture (generate fragments ahead of time)
void chaos_generate_fragments(ChaosDestructionSystem *chaos,
                              u32 destructible_id);

// Runtime destruction
void chaos_apply_damage(ChaosDestructionSystem *chaos, u32 destructible_id,
                        const Vec3 *impact_point, const Vec3 *impact_direction,
                        f32 force);

// Render fragments
void chaos_render_fragments(ChaosDestructionSystem *chaos,
                            id<MTLRenderCommandEncoder> encoder,
                            const Mat4 *view_proj);

// Cleanup
void chaos_update(ChaosDestructionSystem *chaos, f32 delta_time);

#ifdef __cplusplus
}
#endif

#endif // CHAOS_DESTRUCTION_H
