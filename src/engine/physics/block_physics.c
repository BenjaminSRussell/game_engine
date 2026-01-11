// block_physics.c - Block Physics Implementation
#include <include/physics/block_physics.h>
#include <include/core/logger.h>
#include <include/block/block.h>
#include <include/chunk/chunk.h>
#include <include/math/vec3.h>
#include <include/physics/physics_engine_core.h>
#include <stdlib.h>
#include <string.h>

void block_physics_init(BlockPhysicsSystem *sys, PhysicsWorld *phys,
                        ChunkManager *chunks, BlockRegistry *registry) {
  if (!sys || !phys || !chunks || !registry) {
    LOG_ERROR("Block physics: NULL parameters provided");
    return;
  }
  
  sys->physics_world = phys;
  sys->chunk_manager = chunks;
  sys->block_registry = registry;
  sys->chunk_count = 0;
  sys->capacity = 1024;
  
  // Calculate initial chunk count
  // This would typically come from the chunk manager
  sys->chunk_count = 1024; // Placeholder
  
  LOG_INFO("Block physics system initialized");
}

void block_physics_free(BlockPhysicsSystem *sys) {
  if (!sys) return;
  
  sys->physics_world = NULL;
  sys->chunk_manager = NULL;
  sys->block_registry = NULL;
  sys->chunk_count = 0;
  sys->capacity = 0;
  
  LOG_INFO("Block physics system freed");
}

void block_physics_update(BlockPhysicsSystem *sys, f32 delta_time) {
  if (!sys || !sys->physics_world) return;
  
  // Update falling blocks
  block_physics_update_falling_blocks(sys, delta_time);
  
  // Update liquid physics
  block_physics_update_liquid_physics(sys, delta_time);
  
  // Update redstone circuits
  block_physics_update_redstone_circuits(sys, delta_time);
  
  // Update block destruction
  block_physics_update_block_destruction(sys, delta_time);
}

BlockRaycast block_raycast(BlockPhysicsSystem *sys, Vec3 origin, Vec3 direction,
                           f32 max_distance) {
  BlockRaycast result = {0};
  
  if (!sys || !sys->chunk_manager) {
    return result;
  }
  
  // Normalize direction
  f32 length = vec3_length(&direction);
  if (length < 0.001f) {
    return result;
  }
  Vec3 normalized_dir = vec3_scale(direction, 1.0f / length);
  
  // Step through the ray
  f32 current_distance = 0.0f;
  f32 step_size = 0.1f;
  
  while (current_distance < max_distance) {
    Vec3 current_pos = vec3_add(origin, vec3_scale(normalized_dir, current_distance));
    
    // Convert to block coordinates
    i32 block_x = (i32)floorf(current_pos.x);
    i32 block_y = (i32)floorf(current_pos.y);
    i32 block_z = (i32)floorf(current_pos.z);
    
    // Check if block exists at this position
    BlockID block_type = chunk_get_block(sys->chunk_manager, block_x, block_y, block_z);
    if (block_type != BLOCK_AIR) {
      result.hit = true;
      result.position = current_pos;
      result.distance = current_distance;
      result.block_type = block_type;
      
      // Calculate normal (simplified - would use actual block face normal)
      result.normal = (Vec3){0.0f, 1.0f, 0.0f};
      
      LOG_DEBUG("Block raycast hit: type=%d at (%d,%d,%d) distance=%.2f", 
                block_type, block_x, block_y, block_z, current_distance);
      break;
    }
    
    current_distance += step_size;
  }
  
  return result;
}

bool block_can_place_at(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z,
                        BlockID block_type) {
  if (!sys || !sys->chunk_manager) return false;
  
  // Check if position is within world bounds
  if (!chunk_is_position_valid(sys->chunk_manager, x, y, z)) {
    return false;
  }
  
  // Check if block exists at position
  BlockID existing_block = chunk_get_block(sys->chunk_manager, x, y, z);
  if (existing_block != BLOCK_AIR) {
    return false;
  }
  
  // Check if block type can be placed
  if (!block_registry_can_place(sys->block_registry, block_type)) {
    return false;
  }
  
  // Check surrounding blocks for support (some blocks require support)
  if (!block_has_support(sys->chunk_manager, x, y, z, block_type)) {
    return false;
  }
  
  return true;
}

bool block_can_break_at(BlockPhysicsSystem * sys, i32 x, i32 y, i32 z) {
  if (!sys || !sys->chunk_manager) return false;
  
  // Check if position is within world bounds
  if (!chunk_is_position_valid(sys->chunk_manager, x, y, z)) {
    return false;
  }
  
  // Check if block exists at position
  BlockID block_type = chunk_get_block(sys->chunk_manager, x, y, z);
  if (block_type == BLOCK_AIR) {
    return false;
  }
  
  // Check if block type can be broken
  if (!block_registry_can_break(sys->block_registry, block_type)) {
    return false;
  }
  
  // Check tool requirements
  if (!block_has_required_tool(sys->chunk_manager, x, y, z, block_type)) {
    return false;
  }
  
  return true;
}

void block_physics_on_place(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z,
                            BlockID block_type) {
  if (!sys || !sys->chunk_manager) return;
  
  // Place the block
  chunk_set_block(sys->chunk_manager, x, y, z, block_type);
  
  // Trigger physics updates for surrounding blocks
  block_physics_trigger_neighbor_updates(sys, x, y, z);
  
  // Notify redstone system of new block
  redstone_on_block_place(sys->physics_world, x, y, z, block_type);
  
  LOG_DEBUG("Block placed: type=%d at (%d,%d,%d)", block_type, x, y, z);
}

void block_physics_on_break(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z) {
  if (!sys || !sys->chunk_manager) return;
  
  BlockID block_type = chunk_get_block(sys->chunk_manager, x, y, z);
  if (block_type == BLOCK_AIR) return;
  
  // Remove the block
  chunk_set_block(sys->chunk_manager, x, y, z, BLOCK_AIR);
  
  // Drop items if block has inventory
  block_drop_items(sys->chunk_manager, x, y, z, block_type);
  
  // Trigger physics updates for surrounding blocks
  block_physics_trigger_neighbor_updates(sys, x, y, z);
  
  // Notify redstone system of block removal
  redstone_on_block_break(sys->physics_world, x, y, z, block_type);
  
  LOG_DEBUG("Block broken: type=%d at (%d,%d,%d)", block_type, x, y, z);
}

Vec3 block_get_center(i32 x, i32 y, i32 z) {
  // Block centers are at integer coordinates + 0.5
  return (Vec3){(f32)x + 0.5f, (f32)y + 0.5f, (f32)z + 0.5f};
}

Vec3 block_get_bounds(BlockID block_type) {
  // Get block dimensions from registry
  Vec3 size = block_registry_get_block_size(block_type);
  return size;
}

// Internal helper functions
static void block_physics_update_falling_blocks(BlockPhysicsSystem *sys, f32 delta_time) {
  if (!sys || !sys->physics_world) return;
  
  // Iterate through all chunks and update falling blocks
  for (u32 i = 0; i < sys->chunk_count; i++) {
    Chunk *chunk = chunk_get_chunk(sys->chunk_manager, i);
    if (!chunk) continue;
    
    for (u32 j = 0; j < CHUNK_SIZE; j++) {
      for (u32 k = 0; k < CHUNK_HEIGHT; k++) {
        for (u32 l = 0; l < CHUNK_DEPTH; l++) {
          BlockID block_type = chunk_get_block(chunk, j, k, l);
          
          // Check if this block should fall
          if (block_should_fall(block_type)) {
            Vec3 block_pos = block_get_center(j, k, l);
            Vec3 velocity = (Vec3){0.0f, -9.8f * delta_time, 0.0f};
            
            // Apply gravity
            Vec3 new_pos = vec3_add(block_pos, vec3_scale(velocity, delta_time));
            
            // Check if new position is valid
            i32 new_x = (i32)floorf(new_pos.x);
            i32 new_y = (i32)floorf(new_pos.y);
            i32 new_z = (i32)floorf(new_pos.z);
            
            if (chunk_is_position_valid(sys->chunk_manager, new_x, new_y, new_z)) {
              BlockID below_block = chunk_get_block(chunk, new_x, new_y - 1, new_z);
              if (below_block != BLOCK_AIR && below_block != BLOCK_WATER) {
                // Block has support, stop falling
                continue;
              }
              
              // Move the block
              chunk_set_block(chunk, new_x, new_y, new_z, block_type);
              chunk_set_block(chunk, j, k, l, BLOCK_AIR);
            }
          }
        }
      }
    }
  }
}

static void block_physics_update_liquid_physics(BlockPhysicsSystem *sys, f32 delta_time) {
  if (!sys || !sys->physics_world) return;
  
  // Update water and lava flow
  block_physics_update_water_flow(sys, delta_time);
  block_physics_update_lava_flow(sys, delta_time);
}

static void block_physics_update_water_flow(BlockPhysicsSystem *sys, f32 delta_time) {
  // Simplified water flow simulation
  for (u32 i = 0; i < sys->chunk_count; i++) {
    Chunk *chunk = chunk_get_chunk(sys->chunk_manager, i);
    if (!chunk) continue;
    
    for (u32 j = 0; j < CHUNK_SIZE; j++) {
      for (u32 k = 0; k < CHUNK_HEIGHT; k++) {
        for (u32 l = 0; l < CHUNK_DEPTH; l++) {
          BlockID block_type = chunk_get_block(chunk, j, k, l);
          
          if (block_type == BLOCK_WATER) {
            // Check if water can flow to lower positions
            Vec3 block_pos = block_get_center(j, k, l);
            
            // Try to flow down
            if (k > 0) {
              BlockID below = chunk_get_block(chunk, j, k - 1, l);
              if (below == BLOCK_AIR) {
                // Water flows down
                chunk_set_block(chunk, j, k - 1, l, BLOCK_WATER);
                chunk_set_block(chunk, j, k, l, BLOCK_AIR);
              }
            }
            
            // Try to flow horizontally
            if (j > 0) {
              BlockID left = chunk_get_block(chunk, j - 1, k, l);
              if (left == BLOCK_AIR) {
                chunk_set_block(chunk, j - 1, k, l, BLOCK_WATER);
                chunk_set_block(chunk, j, k, l, BLOCK_AIR);
              }
            }
          }
        }
      }
    }
  }
}

static void block_physics_update_lava_flow(BlockPhysicsSystem *sys, f32 delta_time) {
  // Similar to water but with different properties
  for (u32 i = 0; i < sys->chunk_count; i++) {
    Chunk *chunk = chunk_get_chunk(sys->chunk_manager, i);
    if (!chunk) continue;
    
    for (u32 j = 0; j < CHUNK_SIZE; j++) {
      for (u32 k = 0; k < CHUNK_HEIGHT; k++) {
        for (u32 l = 0; l < CHUNK_DEPTH; l++) {
          BlockID block_type = chunk_get_block(chunk, j, k, l);
          
          if (block_type == BLOCK_LAVA) {
            // Lava flows slower than water
            Vec3 block_pos = block_get_center(j, k, l);
            
            // Try to flow down
            if (k > 0) {
              BlockID below = chunk_get_block(chunk, j, k - 1, l);
              if (below == BLOCK_AIR || below == BLOCK_WATER) {
                // Lava flows down slowly
                if (below == BLOCK_WATER) {
                  // Lava and water create obsidian
                  chunk_set_block(chunk, j, k - 1, l, BLOCK_OBSIDIAN);
                  chunk_set_block(chunk, j, k, l, BLOCK_AIR);
                } else {
                  chunk_set_block(chunk, j, k - 1, l, BLOCK_LAVA);
                  chunk_set_block(chunk, j, k, l, BLOCK_AIR);
                }
              }
            }
          }
        }
      }
    }
  }
}

static void block_physics_update_redstone_circuits(BlockPhysicsSystem *sys, f32 delta_time) {
  if (!sys || !sys->physics_world) return;
  
  // Update redstone signals through the physics world
  redstone_update_circuits(sys->physics_world, delta_time);
}

static void block_physics_update_block_destruction(BlockPhysics *sys, f32 delta_time) {
  if (!sys || !sys->physics_world) return;
  
  // Update block destruction animations and particle effects
  destruction_update_blocks(sys->physics_world, delta_time);
}

static void block_physics_trigger_neighbor_updates(BlockPhysics *sys, i32 x, i32 y, i32 z) {
  if (!sys || !sys->chunk_manager) return;
  
  // Update physics for surrounding blocks that might be affected
  // This is important for blocks that rely on neighbors for support
  
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      for (int dz = -1; dz <= 1; dz++) {
        if (dx == 0 && dy == 0 && dz == 0) continue;
        
        i32 nx = x + dx;
        i32 ny = y + dy;
        i32 nz = z + dz;
        
        if (chunk_is_position_valid(sys->chunk_manager, nx, ny, nz)) {
          BlockID neighbor_block = chunk_get_block(sys->chunk_manager, nx, ny, nz);
          
          // Update physics for neighbor block
          block_update_physics(sys->physics_world, nx, ny, nz, neighbor_block);
        }
      }
    }
  }
}

static bool block_should_fall(BlockID block_type) {
  // Blocks that should fall when unsupported
  switch (block_type) {
    case BLOCK_SAND:
    case BLOCK_GRAVEL:
    case BLOCK_DIRT:
    case BLOCK_COBBLESTONE:
    case BLOCK_WOOD:
      return true;
    default:
      return false;
  }
}

static bool block_has_support(BlockManager *chunk_manager, i32 x, i32 y, i32 z, BlockID block_type) {
  // Check if block has support from below
  if (y <= 0) return true; // Assume ground support at y=0
  
  BlockID below_block = chunk_get_block(chunk_manager, x, y - 1, z);
  return below_block != BLOCK_AIR;
}

static bool block_has_required_tool(BlockManager *chunk_manager, i32 x, i32 y, i32 z, BlockID block_type) {
  // Check if breaking this block requires specific tools
  switch (block_type) {
    case BLOCK_OBSIDIAN:
    case BLOCK_DIAMOND:
    case BLOCK_GOLD_ORE:
    case BLOCK_IRON_ORE:
      return true;
    default:
      return false;
  }
}

static void block_drop_items(BlockManager *chunk_manager, i32 x, i32 y, i32 z, BlockID block_type) {
  // Drop items when block is broken
  // This would integrate with the inventory/item system
  LOG_DEBUG("Dropping items for block type %d at (%d,%d,%d)", block_type, x, y, z);
}

static void block_drop_items(BlockManager *chunk_manager, i32 x, i32 y, i32 z, BlockID block_type) {
  // Drop items when block is broken
  // This would integrate with the inventory/item system
  LOG_DEBUG("Dropping items for block type %d at (%d,%d,%d)", block_type, x, y, z);
}

static void block_update_physics(PhysicsWorld *world, i32 x, i32 y, i32 z, BlockID block_type) {
  // Update physics state for a specific block
  // This would integrate with the main physics engine
  LOG_DEBUG("Updating physics for block %d at (%d,%d,%d)", block_type, x, y, z);
}

static void redstone_on_block_place(PhysicsWorld *world, i32 x, i32 y, i32 z, BlockID block_type) {
  // Notify redstone system of new block placement
  redstone_on_block_place(world, x, y, z, block_type);
}

static void redstone_on_block_break(PhysicsWorld *world, i32 x, i32 y, i32 z, BlockID block_type) {
  // Notify redstone system of block removal
  redstone_on_block_break(world, x, y, z, block_type);
}

static void redstone_update_circuits(PhysicsWorld *world, f32 delta_time) {
  // Update redstone circuit simulation
  redstone_update(world, delta_time);
}

static void destruction_update_blocks(PhysicsWorld *world, f32 delta_time) {
  // Update block destruction animations and effects
  destruction_update_blocks(world, delta_time);
}

static void destruction_update_blocks(PhysicsWorld *world, f32 delta_time) {
  // Update block destruction animations and effects
  destruction_update(world, delta_time);
}

static void redstone_update(PhysicsWorld *world, f32 delta_time) {
  // Update redstone circuit simulation
  redstone_update(world, delta_time);
}

static void redstone_update(PhysicsWorld *world, f32 delta_time) {
  // Update redstone circuit simulation
  redstone_update(world, delta_time);
}

static void destruction_update(PhysicsWorld *world, f32 delta_time) {
  // Update destruction effects
  destruction_update(world, delta_time);
}

static void redstone_update(PhysicsWorld *world, f32 delta_time) {
  // Update redstone circuit simulation
  redstone_update(world, delta_time);
}

static void destruction_update(PhysicsWorld *world, f32 delta_time) {
  // Update destruction effects
  destruction_update(world, delta_time);
}
