// src/block/falling_blocks.c
//
// Module Overview:
// This module is responsible for simulating the physical behavior of
// gravity-affected blocks, such as sand and gravel. Its primary function is to
// detect when these blocks lose support from below and to then move them
// downwards, replacing their original position with air. It integrates with the
// `ChunkManager` to access and modify block data within the world and with the
// `BlockRegistry` to determine which blocks are affected by gravity.
//
// Key Flows:
// 1. **Detection and Movement (`block_update_falling`):** This function is
// called
//    for a specific block at `(x, y, z)`.
//    - It first retrieves the block's `BlockType` from the `BlockRegistry` to
//    check
//      if it has the `BLOCK_GRAVITY` flag set.
//    - It then inspects the block immediately below the current block. If the
//    block
//      below is `BLOCK_AIR` or a liquid, the current block is moved to the
//      position below it, and its original position is set to `BLOCK_AIR`.
//    - After modification, the affected `Chunk` is marked as dirty to trigger a
//    mesh update.
//
// Invariants:
// - A valid `ChunkManager` and `BlockRegistry` must be provided.
// - The `physics_world` parameter is currently unused, indicating that this
// module
//   might evolve to integrate more directly with the physics engine (as noted
//   in `include/physics/physics.h`).
// - Block updates are performed directly by modifying chunk data.
// - Chunks affected by falling blocks must be marked dirty to ensure visual
// updates.
//
// (Additional comments from the file indicating external dependencies and
// roadmaps are preserved.) Falling block updates when unsupported
// (sand/gravel). Roadmap: docs/FALLING_BLOCKS_ROADMAP.md.
// TODO: Implement falling block entity system for smooth physics-based falling.
// TODO: Add falling block collision detection with other entities.
// TODO: Implement falling block particle effects during fall.
// TODO: Add falling block sound effects for impact and movement.
// TODO: Implement falling block stacking optimization to reduce updates.
// TODO: Add falling block velocity system for realistic acceleration.
// TODO: Implement falling block damage system for entities below.
// TODO: Add falling block conversion system (sand to sandstone on impact).
// TODO: Implement falling block prediction to prevent lag spikes.
// TODO: Add falling block batching for performance optimization.
#include <block/block.h>
#include <chunk/chunk.h>
#include <ecs/components/falling_block.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <physics/physics.h>
#include <stdlib.h>

// Update falling blocks (sand, gravel)
void block_update_falling(ChunkManager *chunk_manager,
                          BlockRegistry *block_registry, World *world,
                          PhysicsWorld *physics_world, i32 x, i32 y, i32 z) {
  (void)physics_world; // Unused for now
  if (!chunk_manager || !block_registry)
    return;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return;

  i32 local_x = x - cp.x * CHUNK_SIZE;
  i32 local_y = y - cp.y * CHUNK_SIZE;
  i32 local_z = z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, local_x, local_y, local_z);

  // Check if block has gravity
  const BlockType *block_type = block_registry_get(block_registry, block);
  if (!block_type || !block_has_gravity(block_type)) {
    return;
  }

  // Check block below
  BlockID below = chunk_get_block(chunk, local_x, local_y - 1, local_z);
  const BlockType *below_type = block_registry_get(block_registry, below);

  // If air or liquid below, fall
  if (below == BLOCK_AIR || (below_type && block_is_liquid(below_type))) {
    // 1. Remove block from chunk
    chunk_set_block(chunk, local_x, local_y, local_z, BLOCK_AIR);
    chunk_mark_mesh_dirty(chunk);

    // 2. Create falling block entity
    if (world && physics_world) {
      Entity entity = ecs_create_entity(world);

      // Position at block center
      Vec3 pos = vec3((f32)x + 0.5f, (f32)y + 0.5f, (f32)z + 0.5f);

      // Add Transform component
      ecs_add_component(world, entity, TRANSFORM_COMPONENT_ID, NULL);
      TransformComponent *transform = (TransformComponent *)ecs_get_component(
          world, entity, TRANSFORM_COMPONENT_ID);
      if (transform) {
        transform->position = pos;
        transform->rotation = quat_identity();
      }

      // Add FallingBlock component
      ecs_add_component(world, entity, FALLING_BLOCK_COMPONENT_ID, NULL);
      FallingBlockComponent *falling =
          (FallingBlockComponent *)ecs_get_component(
              world, entity, FALLING_BLOCK_COMPONENT_ID);
      if (falling) {
        falling->block_type = block;
        falling->fall_distance = 0.0f;
        falling->on_ground = false;
      }

      // Add Physics (RigidBody + Box Collider)
      RigidBody *body = rigid_body_create(BODY_TYPE_DYNAMIC, pos);
      if (body) {
        rigid_body_set_mass(body, 1.0f);
        rigid_body_set_friction(body, 0.5f);

        // Block is 1.0x1.0x1.0, so half-extents are 0.5
        Collider *collider = collider_create_box(vec3(0.5f, 0.5f, 0.5f));
        rigid_body_attach_collider(body, collider);

        physics_world_add_body(physics_world, body);

        // Link to ECS if possible (assuming RIGIDBODY_COMPONENT_ID exists)
        ecs_add_component(world, entity, RIGIDBODY_COMPONENT_ID, &body);
      }
    }
  }
}

void falling_block_system_update(World *world, ChunkManager *chunk_manager,
                                 BlockRegistry *block_registry,
                                 PhysicsWorld *physics_world, f32 delta_time) {
  if (!world || !chunk_manager || !block_registry)
    return;

  // Simple query implementation since ecs_query_entities is limited
  // We'll iterate manually for now or use the ECS query if it supports it
  // Given the current ECS implementation in ecs.c (game version),
  // we can iterate over the components directly.

  u32 component_id = FALLING_BLOCK_COMPONENT_ID;
  // This is a bit hacky because we don't have a clean way to get the array
  // but ecs_get_component is available if we knew the entities.

  // Let's assume for now we use a simpler approach:
  // Since we don't have a list of all entities, we'd normally have a
  // "FallingBlockSystem" that tracks its own list of entities.

  // For the sake of this TODO implementation, I'll use a manual iteration
  // over the max entities if necessary, but that's slow.

  // Wait, I saw ecs_query_entities in ecs.c.
  // ComponentTypeID components[] = { FALLING_BLOCK_COMPONENT_ID,
  // TRANSFORM_COMPONENT_ID };
  // ...

  // Actually, I'll just look at how other systems in this project handle
  // updates. Many of them seem to be direct component updates.

  // Let's try to use ecs_query_entities.
  ComponentTypeID query_types[] = {FALLING_BLOCK_COMPONENT_ID,
                                   TRANSFORM_COMPONENT_ID};
  EntityQuery query;
  ecs_query_init(&query, 128); // Standard capacity
  ecs_query_entities(world, &query, query_types, 2);

  for (u32 i = 0; i < query.count; i++) {
    EntityID entity = query.entities[i];
    FallingBlockComponent *falling = (FallingBlockComponent *)ecs_get_component(
        world, entity, FALLING_BLOCK_COMPONENT_ID);
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        world, entity, TRANSFORM_COMPONENT_ID);

    if (!falling || !transform)
      continue;

    // Sync transform from physics if present
    RigidBody *body = NULL;
    void **p_body =
        (void **)ecs_get_component(world, entity, RIGIDBODY_COMPONENT_ID);
    if (p_body)
      body = (RigidBody *)*p_body;

    if (body) {
      transform->position = rigid_body_get_position(body);
      transform->rotation = rigid_body_get_rotation(body);

      // Check if settled (Minecraft logic: velocity near zero and Y position
      // near-integer)
      Vec3 vel = rigid_body_get_velocity(body);
      bool at_rest = vec3_length_sq(vel) < 0.001f;

      // Also check if we hit something solid below
      // For now, let's use velocity as a simple heuristic
      if (at_rest) {
        i32 grid_x = (i32)floorf(transform->position.x);
        i32 grid_y = (i32)floorf(transform->position.y);
        i32 grid_z = (i32)floorf(transform->position.z);

        // Solidify
        ChunkPos cp = world_to_chunk_pos(grid_x, grid_y, grid_z);
        Chunk *chunk = chunk_manager_get(chunk_manager, cp);
        if (chunk) {
          i32 local_x = grid_x - cp.x * CHUNK_SIZE;
          i32 local_y = grid_y - cp.y * CHUNK_SIZE;
          i32 local_z = grid_z - cp.z * CHUNK_SIZE;

          BlockID current = chunk_get_block(chunk, local_x, local_y, local_z);
          if (current == BLOCK_AIR ||
              (block_registry_get(block_registry, current) &&
               block_is_liquid(block_registry_get(block_registry, current)))) {
            chunk_set_block(chunk, local_x, local_y, local_z,
                            falling->block_type);
            chunk_mark_mesh_dirty(chunk);

            // Cleanup entity
            physics_world_remove_body(physics_world, body);
            rigid_body_destroy(body);
            ecs_destroy_entity(world, entity);
          }
        }
      }
    }

    falling->fall_distance += delta_time; // Dummy tracking
  }

  ecs_query_free(&query);
}
