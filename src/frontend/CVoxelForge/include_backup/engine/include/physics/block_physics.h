// include/physics/block_physics.h
//
// Purpose: Defines the public API and data structures for a specialized block
// physics system. This system manages interactions between dynamic physical
// entities and the static, block-based environment of the game world. It
// provides functionalities for raycasting against blocks, validating block
// placement/breaking locations, and responding to changes in the block
// landscape.
//
// Public APIs:
// - `BlockRaycast`: Structure to store the result of a raycast against blocks,
//   including hit status, position, normal, distance, and the block type hit.
// - `BlockPhysicsSystem`: The main structure encapsulating the block physics
// system,
//   holding references to the generic `PhysicsWorld`, `ChunkManager`, and
//   `BlockRegistry`.
// - `block_physics_init`: Initializes the block physics system, linking it to
// core game systems.
// - `block_physics_free`: Frees resources associated with the block physics
// system.
// - `block_physics_update`: Updates the state of block-related physics
// interactions (e.g., falling blocks, liquid flow).
// - `block_raycast`: Performs a raycast from an `origin` in a `direction`
// against blocks in the world.
// - `block_can_place_at`, `block_can_break_at`: Functions to determine if a
// block can be
//   placed or broken at specific coordinates, respecting collision and game
//   rules.
// - `block_physics_on_place`, `block_physics_on_break`: Callbacks or event
// handlers
//   to notify the physics system when a block is placed or broken.
// - `block_get_center`, `block_get_bounds`: Utility functions to get the
// world-space
//   center and bounding box dimensions for a block.
//
// Ownership: The `BlockPhysicsSystem` holds references to `PhysicsWorld`,
// `ChunkManager`, and `BlockRegistry` but does not own them. It manages its
// internal state related to block-specific physics.
//
// Invariants:
// - A `BlockPhysicsSystem` must be initialized before performing block physics
// operations.
// - Valid `PhysicsWorld`, `ChunkManager`, and `BlockRegistry` instances are
// required.
// - Block coordinates (`x`, `y`, `z`) are assumed to be integer-based.
// - `delta_time` should be consistently passed to `block_physics_update` for
// accurate time progression.
//
#ifndef BLOCK_PHYSICS_H
#define BLOCK_PHYSICS_H

#include <block/block.h>
#include <chunk/chunk.h>
#include <common.h>
#include <math/vec3.h>
#include <physics/physics_engine_core.h>

typedef struct {
  Vec3 position;
  Vec3 normal;
  f32 distance;
  BlockID block_type;
  bool hit;
} BlockRaycast;

typedef struct {
  u32 chunk_count;
  u32 capacity;
  PhysicsWorld *physics_world;
  ChunkManager *chunk_manager;
  BlockRegistry *block_registry;
} BlockPhysicsSystem;

void block_physics_init(BlockPhysicsSystem *sys, PhysicsWorld *phys,
                        ChunkManager *chunks, BlockRegistry *registry);
void block_physics_free(BlockPhysicsSystem *sys);
void block_physics_update(BlockPhysicsSystem *sys, f32 delta_time);

BlockRaycast block_raycast(BlockPhysicsSystem *sys, Vec3 origin, Vec3 direction,
                           f32 max_distance);

bool block_can_place_at(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z,
                        BlockID block_type);
bool block_can_break_at(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z);

void block_physics_on_place(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z,
                            BlockID block_type);
void block_physics_on_break(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z);

Vec3 block_get_center(i32 x, i32 y, i32 z);
Vec3 block_get_bounds(BlockID block_type);

#endif
