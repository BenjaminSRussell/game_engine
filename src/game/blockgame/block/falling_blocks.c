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
#include <ecs/component_ids.h>
#include <ecs/components/falling_block.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <physics/physics.h>
#include <stdlib.h>

// Enhanced falling block entity system with smooth physics
#define MAX_FALLING_BLOCKS 1024
#define FALLING_BLOCK_TERMINAL_VELOCITY 15.0f
#define FALLING_BLOCK_ACCELERATION 9.81f

typedef struct {
    Entity entity;
    Vec3 initial_position;
    Vec3 velocity;
    f32 fall_time;
    f32 total_distance;
    bool is_settled;
    u32 impact_count;
} FallingBlockState;

static FallingBlockState g_falling_blocks[MAX_FALLING_BLOCKS];
static u32 g_falling_block_count = 0;

// Enhanced falling block creation with better physics
static Entity create_falling_block_entity(World *world, PhysicsWorld *physics_world, 
                                          BlockID block_type, Vec3 position) {
    if (!world || !physics_world)
        return INVALID_ENTITY;
    
    Entity entity = ecs_create_entity(world);
    
    // Enhanced Transform component
    ecs_add_component(world, entity, TRANSFORM_COMPONENT_ID, NULL);
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        world, entity, TRANSFORM_COMPONENT_ID);
    if (transform) {
        transform->position = position;
        transform->rotation = quat_identity();
        transform->scale = vec3(1.0f, 1.0f, 1.0f);
    }
    
    // Enhanced FallingBlock component
    ecs_add_component(world, entity, FALLING_BLOCK_COMPONENT_ID, NULL);
    FallingBlockComponent *falling = (FallingBlockComponent *)ecs_get_component(
        world, entity, FALLING_BLOCK_COMPONENT_ID);
    if (falling) {
        falling->block_type = block_type;
        falling->fall_distance = 0.0f;
        falling->on_ground = false;
    }
    
    // Enhanced Physics with realistic properties
    RigidBody *body = rigid_body_create(BODY_TYPE_DYNAMIC, position);
    if (body) {
        // Set realistic mass based on block type
        f32 mass = 1.0f; // Default mass
        const BlockType *block_type_info = block_registry_get(block_registry, block_type);
        if (block_type_info) {
            // Different materials have different densities
            switch (block_type) {
                case BLOCK_SAND: mass = 1.5f; break;
                case BLOCK_GRAVEL: mass = 2.0f; break;
                case BLOCK_DIRT: mass = 1.2f; break;
                default: mass = 1.0f; break;
            }
        }
        
        rigid_body_set_mass(body, mass);
        rigid_body_set_friction(body, 0.3f);
        rigid_body_set_restitution(body, 0.1f); // Low bounciness
        // rigid_body_set_gravity_scale(body, 1.0f); // Function not available
        
        // Enhanced collider with proper shape
        Collider *collider = collider_create_box(vec3(0.5f, 0.5f, 0.5f));
        rigid_body_attach_collider(body, collider);
        
        // Set initial downward velocity for more realistic falling
        Vec3 initial_velocity = vec3(0.0f, -0.5f, 0.0f);
        rigid_body_set_velocity(body, initial_velocity);
        
        physics_world_add_body(physics_world, body);
        
        // Link to ECS
        ecs_add_component(world, entity, RIGIDBODY_COMPONENT_ID, &body);
    }
    
    // Track falling block state
    if (g_falling_block_count < MAX_FALLING_BLOCKS) {
        g_falling_blocks[g_falling_block_count] = (FallingBlockState){
            .entity = entity,
            .initial_position = position,
            .velocity = vec3(0.0f, -0.5f, 0.0f),
            .fall_time = 0.0f,
            .total_distance = 0.0f,
            .is_settled = false,
            .impact_count = 0
        };
        g_falling_block_count++;
    }
    
    return entity;
}
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

    // 2. Create enhanced falling block entity
    if (world && physics_world) {
      Vec3 position = vec3((f32)x + 0.5f, (f32)y + 0.5f, (f32)z + 0.5f);
      create_falling_block_entity(world, physics_world, block, position);
    }
  }
}

void falling_block_system_update(World *world, ChunkManager *chunk_manager,
                                 BlockRegistry *block_registry,
                                 PhysicsWorld *physics_world, f32 delta_time) {
  if (!world || !chunk_manager || !block_registry)
    return;

  // Enhanced query for falling blocks
  ComponentType query_types[] = {FALLING_BLOCK_COMPONENT_ID,
                                 TRANSFORM_COMPONENT_ID};
  QueryDesc desc = {.all_components = query_types, .all_count = 2};
  Query *query = ecs_query_create(world, &desc);

  Entity entity;
  void *components[2];
  while (ecs_query_next(query, &entity, components)) {
    FallingBlockComponent *falling = (FallingBlockComponent *)components[0];
    TransformComponent *transform = (TransformComponent *)components[1];

    if (!falling || !transform)
      continue;

    // Enhanced physics sync
    RigidBody *body = NULL;
    void **p_body = (void **)ecs_get_component(world, entity, RIGIDBODY_COMPONENT_ID);
    if (p_body)
      body = (RigidBody *)*p_body;

    if (body) {
      // Update transform from physics
      transform->position = rigid_body_get_position(body);
      transform->rotation = rigid_body_get_rotation(body);

      // Get velocity for enhanced physics calculations
      Vec3 velocity = rigid_body_get_velocity(body);
      
      // Apply terminal velocity limit
      if (velocity.y < -FALLING_BLOCK_TERMINAL_VELOCITY) {
        velocity.y = -FALLING_BLOCK_TERMINAL_VELOCITY;
        rigid_body_set_velocity(body, velocity);
      }

      // Enhanced collision detection
      Vec3 current_pos = transform->position;
      i32 grid_x = (i32)floorf(current_pos.x);
      i32 grid_y = (i32)floorf(current_pos.y);
      i32 grid_z = (i32)floorf(current_pos.z);

      // Check for collision with blocks below
      bool has_collision_below = false;
      ChunkPos cp = world_to_chunk_pos(grid_x, grid_y - 1, grid_z);
      Chunk *chunk_below = chunk_manager_get(chunk_manager, cp);
      if (chunk_below) {
        i32 local_x = grid_x - cp.x * CHUNK_SIZE;
        i32 local_y = (grid_y - 1) - cp.y * CHUNK_SIZE;
        i32 local_z = grid_z - cp.z * CHUNK_SIZE;
        
        BlockID block_below = chunk_get_block(chunk_below, local_x, local_y, local_z);
        const BlockType *block_below_type = block_registry_get(block_registry, block_below);
        
        if (block_below != BLOCK_AIR && 
            (!block_below_type || !block_is_liquid(block_below_type))) {
          has_collision_below = true;
        }
      }

      // Enhanced settling conditions
      bool at_rest = vec3_length_sq(velocity) < 0.01f && has_collision_below;
      
      // Update falling block state tracking
      for (u32 i = 0; i < g_falling_block_count; i++) {
        if (g_falling_blocks[i].entity.id == entity.id) {
          g_falling_blocks[i].velocity = velocity;
          g_falling_blocks[i].fall_time += delta_time;
          g_falling_blocks[i].total_distance += vec3_length(velocity) * delta_time;
          
          if (has_collision_below && !g_falling_blocks[i].is_settled) {
            g_falling_blocks[i].impact_count++;
          }
          break;
        }
      }

      // Enhanced settling logic
      FallingBlockState *state = NULL;
      for (u32 i = 0; i < g_falling_block_count; i++) {
        if (g_falling_blocks[i].entity.id == entity.id) {
          state = &g_falling_blocks[i];
          break;
        }
      }
      
      if (state && at_rest && state->fall_time > 0.1f) { // Minimum fall time
        // Check if position is valid for placement
        ChunkPos current_cp = world_to_chunk_pos(grid_x, grid_y, grid_z);
        Chunk *current_chunk = chunk_manager_get(chunk_manager, current_cp);
        if (current_chunk) {
          i32 local_x = grid_x - current_cp.x * CHUNK_SIZE;
          i32 local_y = grid_y - current_cp.y * CHUNK_SIZE;
          i32 local_z = grid_z - current_cp.z * CHUNK_SIZE;

          BlockID current_block = chunk_get_block(current_chunk, local_x, local_y, local_z);
          const BlockType *current_type = block_registry_get(block_registry, current_block);
          
          // Place block if space is available
          if (current_block == BLOCK_AIR || 
              (current_type && block_is_liquid(current_type))) {
            chunk_set_block(current_chunk, local_x, local_y, local_z, falling->block_type);
            chunk_mark_mesh_dirty(current_chunk);

            // Create impact effects
            if (falling->fall_distance > 2.0f) {
              // TODO: Add particle effects and sound effects here
              // falling_block_create_impact_effects(grid_x, grid_y, grid_z, falling->block_type);
            }

            // Cleanup entity
            physics_world_remove_body(physics_world, body);
            rigid_body_destroy(body);
            ecs_destroy_entity(world, entity);
            
            // Remove from tracking
            for (u32 i = 0; i < g_falling_block_count; i++) {
              if (g_falling_blocks[i].entity.id == entity.id) {
                // Move last element to current position
                if (i < g_falling_block_count - 1) {
                  g_falling_blocks[i] = g_falling_blocks[g_falling_block_count - 1];
                }
                g_falling_block_count--;
                break;
              }
            }
          }
        }
      }
    }

    falling->fall_distance += delta_time;
  }

  ecs_query_destroy(world, query);
}
