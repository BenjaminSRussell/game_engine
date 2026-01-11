// Physics cloth simulation header
#ifndef CLOTH_SIMULATION_H
#define CLOTH_SIMULATION_H

#include "include/common.h"
#include "include/math/vec3.h"
#import <Metal/Metal.h>

#define MAX_CLOTH_PARTICLES 65536
#define MAX_CLOTH_CONSTRAINTS 200000

typedef struct {
  Vec3 position;
  Vec3 prev_position;
  Vec3 velocity;
  Vec3 normal;
  f32 mass;
  f32 inv_mass;
  bool pinned;
} ClothParticle;

typedef struct {
  u32 particle_a;
  u32 particle_b;
  f32 rest_length;
  f32 stiffness;
} ClothConstraint;

typedef struct {
  ClothParticle *particles;
  u32 particle_count;

  ClothConstraint *constraints;
  u32 constraint_count;

  Vec3 gravity;
  f32 damping;
  f32 air_resistance;
  f32 wind_strength;
  Vec3 wind_direction;

  // GPU resources
  id<MTLBuffer> particle_buffer;
  id<MTLBuffer> constraint_buffer;
  id<MTLComputePipelineState> update_pipeline;
  id<MTLComputePipelineState> constraint_pipeline;

  u32 solver_iterations;

} ClothSystem;

#ifdef __cplusplus
extern "C" {
#endif

ClothSystem *cloth_create(id<MTLDevice> device, u32 width, u32 height,
                          f32 spacing);
void cloth_destroy(ClothSystem *cloth);
void cloth_update(ClothSystem *cloth, id<MTLCommandBuffer> cmd, f32 delta_time);
void cloth_pin_particle(ClothSystem *cloth, u32 particle_id);
void cloth_apply_force(ClothSystem *cloth, Vec3 force, f32 radius,
                       Vec3 position);

#ifdef __cplusplus
}
#endif

#endif
