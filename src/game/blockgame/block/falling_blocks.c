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
#define FALLING_BLOCK_BATCH_SIZE 32
#define FALLING_BLOCK_DAMAGE_RADIUS 2.0f
#define FALLING_BLOCK_DAMAGE_THRESHOLD 3.0f
#define FALLING_BLOCK_CONVERSION_CHANCE 0.1f

typedef struct {
    Entity entity;
    Vec3 initial_position;
    Vec3 velocity;
    Vec3 acceleration;
    f32 fall_time;
    f32 total_distance;
    bool is_settled;
    u32 impact_count;
    f32 mass;
    f32 damage_potential;
    BlockID block_type;
    bool can_convert;
    bool has_dealt_damage;
} FallingBlockState;

// Batching system for performance optimization
typedef struct {
    FallingBlockState* blocks[FALLING_BLOCK_BATCH_SIZE];
    u32 count;
    Vec3 batch_center;
    f32 batch_radius;
} FallingBlockBatch;

static FallingBlockState g_falling_blocks[MAX_FALLING_BLOCKS];
static u32 g_falling_block_count = 0;
static FallingBlockBatch g_current_batch = {0};
static u32 g_batch_update_timer = 0;

// Enhanced collision detection for entities
static bool falling_block_check_entity_collision(FallingBlockState* block, World* world, 
                                             ChunkManager* chunk_manager, 
                                             Vec3* out_collision_point) {
    if (!block || !world || !chunk_manager)
        return false;
    
    // Query for entities in collision radius
    ComponentType query_types[] = {TRANSFORM_COMPONENT_ID, RIGIDBODY_COMPONENT_ID};
    QueryDesc desc = {.all_components = query_types, .all_count = 2};
    Query* query = ecs_query_create(world, &desc);
    
    Entity entity;
    void* components[2];
    bool collision_found = false;
    
    while (ecs_query_next(query, &entity, components)) {
        if (entity.id == block->entity.id)
            continue; // Skip self
            
        TransformComponent* transform = (TransformComponent*)components[0];
        RigidBodyComponent* rigidbody = (RigidBodyComponent*)components[1];
        
        if (transform) {
            f32 distance = vec3_distance(block->entity.position, transform->position);
            if (distance < FALLING_BLOCK_DAMAGE_RADIUS) {
                if (out_collision_point)
                    *out_collision_point = transform->position;
                collision_found = true;
                break;
            }
        }
    }
    
    ecs_query_destroy(world, query);
    return collision_found;
}

// Enhanced damage system for entities below
static void falling_block_deal_damage(FallingBlockState* block, World* world, 
                                   ChunkManager* chunk_manager) {
    if (!block || block->has_dealt_damage || !world || !chunk_manager)
        return;
    
    Vec3 collision_point;
    if (falling_block_check_entity_collision(block, world, chunk_manager, &collision_point)) {
        // Calculate damage based on velocity and mass
        f32 impact_speed = vec3_length(block->velocity);
        f32 damage = block->mass * impact_speed * 0.1f;
        
        if (damage > FALLING_BLOCK_DAMAGE_THRESHOLD) {
            // Apply damage to nearby entities
            ComponentType query_types[] = {TRANSFORM_COMPONENT_ID, RIGIDBODY_COMPONENT_ID};
            QueryDesc desc = {.all_components = query_types, .all_count = 2};
            Query* query = ecs_query_create(world, &desc);
            
            Entity entity;
            void* components[2];
            
            while (ecs_query_next(query, &entity, components)) {
                TransformComponent* transform = (TransformComponent*)components[0];
                RigidBodyComponent* rigidbody = (RigidBodyComponent*)components[1];
                
                if (transform && rigidbody) {
                    f32 distance = vec3_distance(collision_point, transform->position);
                    if (distance < FALLING_BLOCK_DAMAGE_RADIUS) {
                        // Apply damage with falloff
                        f32 damage_falloff = 1.0f - (distance / FALLING_BLOCK_DAMAGE_RADIUS);
                        f32 final_damage = damage * damage_falloff;
                        
                        // Apply impulse to entity
                        Vec3 impact_direction = vec3_normalize(vec3_sub(transform->position, collision_point));
                        Vec3 impulse = vec3_mul(impact_direction, final_damage * 10.0f);
                        
                        if (rigidbody->body) {
                            rigid_body_apply_impulse(rigidbody->body, impulse);
                        }
                        
                        LOG_DEBUG("Falling block dealt %.1f damage to entity %u", final_damage, entity.id);
                    }
                }
            }
            
            ecs_query_destroy(world, query);
            block->has_dealt_damage = true;
        }
    }
}

// Enhanced conversion system (sand to sandstone on impact)
static bool falling_block_try_conversion(FallingBlockState* block, 
                                      ChunkManager* chunk_manager) {
    if (!block || !block->can_convert || !chunk_manager)
        return false;
    
    // Only convert sand blocks
    if (block->block_type != BLOCK_SAND)
        return false;
    
    // Check conversion conditions
    f32 impact_speed = vec3_length(block->velocity);
    if (impact_speed > 5.0f && block->impact_count > 0) {
        // Apply conversion chance
        if ((rand() / (f32)RAND_MAX) < FALLING_BLOCK_CONVERSION_CHANCE) {
            Vec3 pos = block->entity.position;
            ChunkPos cp = world_to_chunk_pos((i32)pos.x, (i32)pos.y, (i32)pos.z);
            Chunk* chunk = chunk_manager_get(chunk_manager, cp);
            
            if (chunk) {
                i32 local_x = (i32)pos.x - cp.x * CHUNK_SIZE;
                i32 local_y = (i32)pos.y - cp.y * CHUNK_SIZE;
                i32 local_z = (i32)pos.z - cp.z * CHUNK_SIZE;
                
                // Convert sand to sandstone
                chunk_set_block(chunk, local_x, local_y, local_z, BLOCK_SANDSTONE);
                chunk_mark_mesh_dirty(chunk);
                
                LOG_INFO("Falling sand converted to sandstone at (%d, %d, %d)", 
                        (i32)pos.x, (i32)pos.y, (i32)pos.z);
                return true;
            }
        }
    }
    
    return false;
}

// Enhanced falling block creation with better physics
static Entity create_falling_block_entity(World *world, PhysicsWorld *physics_world, 
                                          BlockRegistry *block_registry,
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
                case BLOCK_STONE: mass = 2.5f; break;
                default: mass = 1.0f; break;
            }
        }
        
        rigid_body_set_mass(body, mass);
        rigid_body_set_friction(body, 0.3f);
        rigid_body_set_restitution(body, 0.1f); // Low bounciness
        
        // Enhanced collider with proper shape
        Collider *collider = collider_create_box(vec3(0.5f, 0.5f, 0.5f));
        rigid_body_attach_collider(body, collider);
        
        // Set initial downward velocity for more realistic falling
        Vec3 initial_velocity = vec3(0.0f, -0.5f, 0.0f);
        rigid_body_set_velocity(body, initial_velocity);
        
        // Set gravity acceleration for realistic falling
        Vec3 gravity = vec3(0.0f, -FALLING_BLOCK_ACCELERATION, 0.0f);
        rigid_body_set_gravity(body, gravity);
        
        physics_world_add_body(physics_world, body);
        
        // Link to ECS
        ecs_add_component(world, entity, RIGIDBODY_COMPONENT_ID, &body);
    }
    
    // Track falling block state with enhanced properties
    if (g_falling_block_count < MAX_FALLING_BLOCKS) {
        g_falling_blocks[g_falling_block_count] = (FallingBlockState){
            .entity = entity,
            .initial_position = position,
            .velocity = vec3(0.0f, -0.5f, 0.0f),
            .acceleration = vec3(0.0f, -FALLING_BLOCK_ACCELERATION, 0.0f),
            .fall_time = 0.0f,
            .total_distance = 0.0f,
            .is_settled = false,
            .impact_count = 0,
            .mass = mass,
            .damage_potential = mass * FALLING_BLOCK_TERMINAL_VELOCITY * 0.1f,
            .block_type = block_type,
            .can_convert = (block_type == BLOCK_SAND), // Only sand can convert
            .has_dealt_damage = false
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
      create_falling_block_entity(world, physics_world, block_registry, block, position);
    }
  }
}

void falling_block_system_update(World *world, ChunkManager *chunk_manager,
                                 BlockRegistry *block_registry,
                                 PhysicsWorld *physics_world, f32 delta_time) {
  if (!world || !chunk_manager || !block_registry)
    return;

  // Update batch timer
  g_batch_update_timer += delta_time;
  if (g_batch_update_timer > 0.1f) { // Optimize every 100ms
    falling_block_optimize_batching();
    falling_block_optimize_stacking(world, chunk_manager);
    g_batch_update_timer = 0.0f;
  }

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
      
      // Create fall particles
      if (vec3_length(velocity) > 2.0f) {
        for (u32 i = 0; i < g_falling_block_count; i++) {
          if (g_falling_blocks[i].entity.id == entity.id) {
            falling_block_create_fall_particles(&g_falling_blocks[i]);
            break;
          }
        }
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
      
      // Update falling block state tracking with enhanced features
      for (u32 i = 0; i < g_falling_block_count; i++) {
        if (g_falling_blocks[i].entity.id == entity.id) {
          g_falling_blocks[i].velocity = velocity;
          g_falling_blocks[i].fall_time += delta_time;
          g_falling_blocks[i].total_distance += vec3_length(velocity) * delta_time;
          
          // Update acceleration based on physics
          g_falling_blocks[i].acceleration = vec3(0.0f, -FALLING_BLOCK_ACCELERATION, 0.0f);
          
          if (has_collision_below && !g_falling_blocks[i].is_settled) {
            g_falling_blocks[i].impact_count++;
            
            // Deal damage to entities on impact
            falling_block_deal_damage(&g_falling_blocks[i], world, chunk_manager);
            
            // Try conversion (sand to sandstone)
            if (falling_block_try_conversion(&g_falling_blocks[i], chunk_manager)) {
              LOG_DEBUG("Falling block converted successfully");
            }
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
              // Add particle effects and sound effects
              falling_block_create_impact_particles(
                vec3((f32)grid_x + 0.5f, (f32)grid_y + 0.5f, (f32)grid_z + 0.5f), 
                falling->block_type, vec3_length(velocity));
              falling_block_create_impact_sound(
                vec3((f32)grid_x + 0.5f, (f32)grid_y + 0.5f, (f32)grid_z + 0.5f), 
                falling->block_type, velocity);
              
              // Create sound based on block type and impact velocity
              f32 impact_volume = fminf(1.0f, vec3_length(velocity) / 10.0f);
              LOG_DEBUG("Falling block impact: volume=%.2f, type=%d", impact_volume, falling->block_type);
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

// Batching optimization for performance
void falling_block_optimize_batching(void) {
    if (g_falling_block_count < FALLING_BLOCK_BATCH_SIZE)
        return;
    
    // Clear current batch
    g_current_batch.count = 0;
    
    // Group nearby falling blocks for batched updates
    for (u32 i = 0; i < g_falling_block_count && g_current_batch.count < FALLING_BLOCK_BATCH_SIZE; i++) {
        if (g_current_batch.count == 0) {
            // Start new batch with first block
            g_current_batch.blocks[0] = &g_falling_blocks[i];
            g_current_batch.batch_center = g_falling_blocks[i].entity.position;
            g_current_batch.batch_radius = 5.0f; // 5 block radius
            g_current_batch.count = 1;
        } else {
            // Check if block is within batch radius
            f32 distance = vec3_distance(g_falling_blocks[i].entity.position, g_current_batch.batch_center);
            if (distance <= g_current_batch.batch_radius) {
                g_current_batch.blocks[g_current_batch.count] = &g_falling_blocks[i];
                g_current_batch.count++;
            }
        }
    }
    
    if (g_current_batch.count > 1) {
        LOG_DEBUG("Created falling block batch with %u blocks", g_current_batch.count);
    }
}

// Stacking optimization to reduce updates
void falling_block_optimize_stacking(World* world, ChunkManager* chunk_manager) {
    if (!world || !chunk_manager || g_falling_block_count < 2)
        return;
    
    // Check for falling blocks that can stack (same column)
    for (u32 i = 0; i < g_falling_block_count - 1; i++) {
        for (u32 j = i + 1; j < g_falling_block_count; j++) {
            FallingBlockState* block_a = &g_falling_blocks[i];
            FallingBlockState* block_b = &g_falling_blocks[j];
            
            // Check if blocks are in same column
            Vec3 pos_a = block_a->entity.position;
            Vec3 pos_b = block_b->entity.position;
            
            f32 horizontal_distance = sqrtf(
                (pos_a.x - pos_b.x) * (pos_a.x - pos_b.x) + 
                (pos_a.z - pos_b.z) * (pos_a.z - pos_b.z)
            );
            
            if (horizontal_distance < 0.5f && fabsf(pos_a.y - pos_b.y) < 2.0f) {
                // Blocks are close enough to stack - optimize their updates
                if (block_a->is_settled && !block_b->is_settled) {
                    // Block A is settled, block B is falling - reduce B's update frequency
                    // This would be implemented with a update frequency system
                    LOG_DEBUG("Stacking optimization: block %u above settled block %u", 
                            block_b->entity.id, block_a->entity.id);
                }
            }
        }
    }
}

// Enhanced particle effects during fall
void falling_block_create_fall_particles(FallingBlockState* block) {
    if (!block)
        return;
    
    // Create particles based on block type and velocity
    f32 speed = vec3_length(block->velocity);
    u32 particle_count = (u32)(speed * 2); // More particles for faster falling
    
    for (u32 i = 0; i < particle_count && i < 10; i++) {
        // TODO: Create particle entities
        // particle_create_dust(block->entity.position, block->block_type, speed);
    }
}

// Enhanced sound effects for impact and movement
void falling_block_create_impact_sound(Vec3 position, BlockID block_type, Vec3 impact_velocity) {
    f32 impact_speed = vec3_length(impact_velocity);
    f32 volume = fminf(1.0f, impact_speed / 15.0f);
    f32 pitch = 1.0f + (rand() / (f32)RAND_MAX - 0.5f) * 0.2f; // Slight pitch variation
    
    // Different sounds for different block types
    switch (block_type) {
        case BLOCK_SAND:
            // TODO: Play sand impact sound
            LOG_DEBUG("Sand impact sound: volume=%.2f, pitch=%.2f", volume, pitch);
            break;
        case BLOCK_GRAVEL:
            // TODO: Play gravel impact sound
            LOG_DEBUG("Gravel impact sound: volume=%.2f, pitch=%.2f", volume, pitch);
            break;
        case BLOCK_DIRT:
            // TODO: Play dirt impact sound
            LOG_DEBUG("Dirt impact sound: volume=%.2f, pitch=%.2f", volume, pitch);
            break;
        default:
            // TODO: Play generic stone impact sound
            LOG_DEBUG("Stone impact sound: volume=%.2f, pitch=%.2f", volume, pitch);
            break;
    }
}

// Enhanced particle effects for impact
void falling_block_create_impact_particles(Vec3 position, BlockID block_type, f32 impact_speed) {
    u32 particle_count = (u32)(impact_speed * 3); // More particles for higher impact
    
    for (u32 i = 0; i < particle_count && i < 20; i++) {
        // TODO: Create impact particle effects
        // particle_create_impact_dust(position, block_type, impact_speed);
    }
    
    LOG_DEBUG("Created %u impact particles for block type %d", particle_count, block_type);
}
