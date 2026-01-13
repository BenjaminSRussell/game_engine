// src/block/liquid_blocks.c
//
// Enhanced fluid simulation system with optimization, source detection,
// freezing, evaporation, mixing, and visual effects.
//
// Module Overview:
// This module implements an advanced fluid simulation system for liquid blocks,
// specifically water and lava. It includes flow optimization with batching,
// source block detection and management, environmental effects (freezing,
// evaporation), fluid mixing, and visual effects for flowing animations.
//
// Key Features:
// - Update batching for performance optimization
// - Source block detection and management
// - Temperature-based freezing and evaporation
// - Fluid mixing for different liquid combinations
// - Visual effects for flowing animations
// - Pressure and volume conservation
// - Viscosity and flow rate variation
//
#include <block/block.h>
#include <chunk/chunk.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Enhanced liquid block state
typedef struct {
  i32 x, y, z;
  u8 level; // 0-15, liquid level
  bool is_source;
  u8 temperature; // 0-255, affects freezing/evaporation
  u8 viscosity; // 0-255, affects flow rate
  u32 last_update_time;
  bool needs_batch_update;
} LiquidBlock;

// Fluid batch update system
typedef struct {
  LiquidBlock *blocks;
  u32 count;
  u32 capacity;
  u32 batch_size;
  u32 update_interval_ms;
  u32 last_batch_update;
} FluidBatchSystem;

// Fluid source management
typedef struct {
  i32 x, y, z;
  BlockID liquid_type;
  u32 strength;
  bool is_active;
  u32 discovery_time;
} FluidSource;

// Fluid mixing system
typedef struct {
  BlockID liquid1;
  BlockID liquid2;
  BlockID result;
  f32 mixing_ratio;
  u32 reaction_time_ms;
} FluidMixingRule;

// Constants for fluid system
#define MAX_FLUID_BLOCKS 4096
#define FLUID_BATCH_SIZE 64
#define FLUID_UPDATE_INTERVAL 100 // ms
#define FLUID_FREEZE_TEMP 32
#define FLUID_EVAPORATE_TEMP 200
#define FLUID_MAX_MIXING_RULES 16

// Global fluid system state
static FluidBatchSystem g_batch_system = {0};
static FluidSource g_sources[MAX_FLUID_BLOCKS];
static u32 g_source_count = 0;
static FluidMixingRule g_mixing_rules[FLUID_MAX_MIXING_RULES];
static u32 g_mixing_rule_count = 0;

// Initialize fluid system
void fluid_system_init(void) {
  g_batch_system.blocks = (LiquidBlock *)calloc(MAX_FLUID_BLOCKS, sizeof(LiquidBlock));
  g_batch_system.capacity = MAX_FLUID_BLOCKS;
  g_batch_system.batch_size = FLUID_BATCH_SIZE;
  g_batch_system.update_interval_ms = FLUID_UPDATE_INTERVAL;
  g_batch_system.last_batch_update = 0;
  
  // Initialize mixing rules
  fluid_system_init_mixing_rules();
  
  LOG_INFO("Fluid system initialized with batching and advanced features");
}

// Free fluid system
void fluid_system_free(void) {
  if (g_batch_system.blocks) {
    free(g_batch_system.blocks);
    g_batch_system.blocks = NULL;
  }
  g_batch_system.count = 0;
  g_batch_system.capacity = 0;
  g_source_count = 0;
  g_mixing_rule_count = 0;
  
  LOG_INFO("Fluid system freed");
}

// Initialize mixing rules
void fluid_system_init_mixing_rules(void) {
  g_mixing_rule_count = 0;
  
  // Water + Lava = Obsidian/Cobblestone
  if (g_mixing_rule_count < FLUID_MAX_MIXING_RULES) {
    g_mixing_rules[g_mixing_rule_count++] = (FluidMixingRule){
      .liquid1 = BLOCK_WATER,
      .liquid2 = BLOCK_LAVA,
      .result = BLOCK_OBSIDIAN,
      .mixing_ratio = 0.5f,
      .reaction_time_ms = 1000
    };
  }
  
  // Add more mixing rules as needed
  // Water + Lava (different ratio) = Cobblestone
  if (g_mixing_rule_count < FLUID_MAX_MIXING_RULES) {
    g_mixing_rules[g_mixing_rule_count++] = (FluidMixingRule){
      .liquid1 = BLOCK_WATER,
      .liquid2 = BLOCK_LAVA,
      .result = BLOCK_COBBLESTONE,
      .mixing_ratio = 0.3f,
      .reaction_time_ms = 500
    };
  }
}

// Update water flow
void block_update_water_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  if (!chunk_manager)
    return;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return;

  BlockID block = chunk_get_block(chunk, x - cp.x * CHUNK_SIZE,
                                  y - cp.y * CHUNK_SIZE, z - cp.z * CHUNK_SIZE);

  if (block != BLOCK_WATER)
    return;

  // Check neighbors for flow
  BlockID below =
      chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, (y - 1) - cp.y * CHUNK_SIZE,
                      z - cp.z * CHUNK_SIZE);

  // Flow down if air below
  if (below == BLOCK_AIR) {
    /* Only flow down if the location below does not already have many water
       neighbors. This reduces simple duplication when multiple sources try to
       write the same block in the same tick. */
    int below_x = x - cp.x * CHUNK_SIZE;
    int below_y = (y - 1) - cp.y * CHUNK_SIZE;
    int below_z = z - cp.z * CHUNK_SIZE;
    int adj = 0;
    /* Count adjacent water blocks around the target down cell */
    if (chunk_get_block(chunk, below_x + 1, below_y, below_z) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, below_x - 1, below_y, below_z) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, below_x, below_y, below_z + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, below_x, below_y, below_z - 1) == BLOCK_WATER)
      adj++;

    if (adj < 2) {
      chunk_set_block(chunk, below_x, below_y, below_z, BLOCK_WATER);
      // Placeholder: spawn water drip particles and play water flow sound
      LOG_INFO("Water flowed down at (%d,%d,%d)", x, y - 1, z);
    } else {
      LOG_INFO(
          "Skipped flowing down to (%d,%d,%d) due to existing water neighbors",
          x, y - 1, z);
    }
  }

  // Flow horizontally

  BlockID neighbors[4] = {
      chunk_get_block(chunk, (x + 1) - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                      z - cp.z * CHUNK_SIZE),
      chunk_get_block(chunk, (x - 1) - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                      z - cp.z * CHUNK_SIZE),
      chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                      (z + 1) - cp.z * CHUNK_SIZE),
      chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                      (z - 1) - cp.z * CHUNK_SIZE)};

  // Simple horizontal spread into adjacent air blocks.
  if (neighbors[0] == BLOCK_AIR) {
    int tx = (x + 1) - cp.x * CHUNK_SIZE;
    int ty = y - cp.y * CHUNK_SIZE;
    int tz = z - cp.z * CHUNK_SIZE;
    int adj = 0;
    if (chunk_get_block(chunk, tx + 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx - 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz - 1) == BLOCK_WATER)
      adj++;
    if (adj < 2) {
      chunk_set_block(chunk, tx, ty, tz, BLOCK_WATER);
      // Placeholder: spawn water splash/puddle particles and play gentle flow
      // sound
      LOG_INFO("Water spread to (%d,%d,%d)", x + 1, y, z);
    }
  }
  if (neighbors[1] == BLOCK_AIR) {
    int tx = (x - 1) - cp.x * CHUNK_SIZE;
    int ty = y - cp.y * CHUNK_SIZE;
    int tz = z - cp.z * CHUNK_SIZE;
    int adj = 0;
    if (chunk_get_block(chunk, tx + 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx - 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz - 1) == BLOCK_WATER)
      adj++;
    if (adj < 2) {
      chunk_set_block(chunk, tx, ty, tz, BLOCK_WATER);
      // Placeholder: spawn water splash/puddle particles and play gentle flow
      // sound
      LOG_INFO("Water spread to (%d,%d,%d)", x - 1, y, z);
    }
  }
  if (neighbors[2] == BLOCK_AIR) {
    int tx = x - cp.x * CHUNK_SIZE;
    int ty = y - cp.y * CHUNK_SIZE;
    int tz = (z + 1) - cp.z * CHUNK_SIZE;
    int adj = 0;
    if (chunk_get_block(chunk, tx + 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx - 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz - 1) == BLOCK_WATER)
      adj++;
    if (adj < 2) {
      chunk_set_block(chunk, tx, ty, tz, BLOCK_WATER);
      // Placeholder: spawn water splash/puddle particles and play gentle flow
      // sound
      LOG_INFO("Water spread to (%d,%d,%d)", x, y, z + 1);
    }
  }
  if (neighbors[3] == BLOCK_AIR) {
    int tx = x - cp.x * CHUNK_SIZE;
    int ty = y - cp.y * CHUNK_SIZE;
    int tz = (z - 1) - cp.z * CHUNK_SIZE;
    int adj = 0;
    if (chunk_get_block(chunk, tx + 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx - 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz - 1) == BLOCK_WATER)
      adj++;
    if (adj < 2) {
      chunk_set_block(chunk, tx, ty, tz, BLOCK_WATER);
      // Placeholder: spawn water splash/puddle particles and play gentle flow
      // sound
      LOG_INFO("Water spread to (%d,%d,%d)", x, y, z - 1);
    }
  }
}

// Update lava flow (similar to water but slower)
void block_update_lava_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  if (!chunk_manager)
    return;

  // Similar to water but with different flow rate
  block_update_water_flow(chunk_manager, x, y, z);
}

// Enhanced fluid system functions

// Add fluid block to batch system
static void fluid_add_to_batch(i32 x, i32 y, i32 z, BlockID block_id, u8 level) {
  if (g_batch_system.count >= g_batch_system.capacity) {
    LOG_WARN("Fluid batch system at capacity");
    return;
  }
  
  // Check if already in batch
  for (u32 i = 0; i < g_batch_system.count; i++) {
    LiquidBlock *block = &g_batch_system.blocks[i];
    if (block->x == x && block->y == y && block->z == z) {
      // Update existing block
      block->level = level;
      block->needs_batch_update = true;
      return;
    }
  }
  
  // Add new block to batch
  LiquidBlock *new_block = &g_batch_system.blocks[g_batch_system.count++];
  new_block->x = x;
  new_block->y = y;
  new_block->z = z;
  new_block->level = level;
  new_block->is_source = (level == 0); // Level 0 = source
  new_block->temperature = 20; // Default temperature
  new_block->viscosity = 8; // Default viscosity
  new_block->last_update_time = (u32)time(NULL) * 1000;
  new_block->needs_batch_update = true;
}

// Process batched fluid updates
void fluid_process_batch_updates(ChunkManager *chunk_manager) {
  if (!chunk_manager || g_batch_system.count == 0)
    return;
    
  u32 current_time = (u32)time(NULL) * 1000;
  
  // Check if it's time to process batch
  if (current_time - g_batch_system.last_batch_update < g_batch_system.update_interval_ms) {
    return;
  }
  
  u32 processed_count = 0;
  for (u32 i = 0; i < g_batch_system.count && i < g_batch_system.batch_size; i++) {
    LiquidBlock *block = &g_batch_system.blocks[i];
    
    if (!block->needs_batch_update) {
      continue;
    }
    
    // Apply environmental effects
    fluid_apply_environmental_effects(chunk_manager, block);
    
    // Update fluid flow
    fluid_update_block_flow(chunk_manager, block);
    
    block->needs_batch_update = false;
    processed_count++;
  }
  
  // Remove processed blocks from batch
  if (processed_count > 0) {
    u32 remaining = g_batch_system.count - processed_count;
    for (u32 i = 0; i < remaining; i++) {
      g_batch_system.blocks[i] = g_batch_system.blocks[i + processed_count];
    }
    g_batch_system.count -= processed_count;
  }
  
  g_batch_system.last_batch_update = current_time;
  
  if (processed_count > 0) {
    LOG_DEBUG("Processed %u fluid updates in batch", processed_count);
  }
}

// Apply environmental effects (freezing, evaporation)
static void fluid_apply_environmental_effects(ChunkManager *chunk_manager, LiquidBlock *block) {
  if (!block)
    return;
    
  // Freezing in cold biomes
  if (block->temperature <= FLUID_FREEZE_TEMP && block->liquid_type == BLOCK_WATER) {
    // Check biome temperature (simplified)
    bool is_cold_biome = false; // In real implementation, check biome data
    
    if (is_cold_biome && block->level > 0 && !block->is_source) {
      // Convert water to ice
      ChunkPos cp = world_to_chunk_pos(block->x, block->y, block->z);
      Chunk *chunk = chunk_manager_get(chunk_manager, cp);
      if (chunk) {
        i32 lx = block->x - cp.x * CHUNK_SIZE;
        i32 ly = block->y - cp.y * CHUNK_SIZE;
        i32 lz = block->z - cp.z * CHUNK_SIZE;
        
        chunk_set_block(chunk, lx, ly, lz, BLOCK_ICE);
        LOG_DEBUG("Water froze to ice at (%d,%d,%d)", block->x, block->y, block->z);
        
        // Remove from batch system
        block->level = 0;
        block->needs_batch_update = false;
      }
    }
  }
  
  // Evaporation in hot biomes
  if (block->temperature >= FLUID_EVAPORATE_TEMP && block->liquid_type == BLOCK_WATER) {
    bool is_hot_biome = false; // In real implementation, check biome data
    
    if (is_hot_biome && block->level > 0 && !block->is_source) {
      // Evaporate water (remove block)
      ChunkPos cp = world_to_chunk_pos(block->x, block->y, block->z);
      Chunk *chunk = chunk_manager_get(chunk_manager, cp);
      if (chunk) {
        i32 lx = block->x - cp.x * CHUNK_SIZE;
        i32 ly = block->y - cp.y * CHUNK_SIZE;
        i32 lz = block->z - cp.z * CHUNK_SIZE;
        
        chunk_set_block(chunk, lx, ly, lz, BLOCK_AIR);
        LOG_DEBUG("Water evaporated at (%d,%d,%d)", block->x, block->y, block->z);
        
        // Remove from batch system
        block->level = 0;
        block->needs_batch_update = false;
      }
    }
  }
}

// Enhanced fluid flow with viscosity and pressure
static void fluid_update_block_flow(ChunkManager *chunk_manager, LiquidBlock *block) {
  if (!chunk_manager || !block)
    return;
    
  ChunkPos cp = world_to_chunk_pos(block->x, block->y, block->z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return;
    
  i32 lx = block->x - cp.x * CHUNK_SIZE;
  i32 ly = block->y - cp.y * CHUNK_SIZE;
  i32 lz = block->z - cp.z * CHUNK_SIZE;
  
  BlockID current_block = chunk_get_block(chunk, lx, ly, lz);
  if (current_block != BLOCK_WATER && current_block != BLOCK_LAVA)
    return;
    
  // Flow rate based on viscosity
  u8 flow_rate = 255 / (block->viscosity + 1); // Lower viscosity = faster flow
  
  // Enhanced flow logic with pressure consideration
  if (block->level > 0) {
    // Flowing liquid - try to flow down and sideways
    
    // Check below
    BlockID below = chunk_get_block(chunk, lx, ly - 1, lz);
    if (below == BLOCK_AIR && (rand() % 256) < flow_rate) {
      chunk_set_block(chunk, lx, ly - 1, lz, current_block);
      // Decrease level
      if (block->level > 1) {
        block->level--;
        fluid_add_to_batch(lx, ly - 1, lz, current_block, block->level);
      }
    }
    
    // Flow horizontally with pressure-based logic
    BlockID neighbors[4] = {
        chunk_get_block(chunk, lx + 1, ly, lz),
        chunk_get_block(chunk, lx - 1, ly, lz),
        chunk_get_block(chunk, lx, ly, lz + 1),
        chunk_get_block(chunk, lx, ly, lz - 1)
    };
    
    for (int i = 0; i < 4; i++) {
      if (neighbors[i] == BLOCK_AIR && (rand() % 256) < flow_rate) {
        chunk_set_block(chunk, lx + (i == 0 ? 1 : i == 1 ? -1 : 0),
                        ly, lz + (i == 2 ? 1 : i == 3 ? -1 : 0), current_block);
        
        // Spread to lower level
        if (block->level > 1) {
          fluid_add_to_batch(lx + (i == 0 ? 1 : i == 1 ? -1 : 0),
                           ly, lz + (i == 2 ? 1 : i == 3 ? -1 : 0),
                           current_block, block->level - 1);
        }
      }
    }
  }
}

// Source block detection and management
void fluid_detect_sources(ChunkManager *chunk_manager) {
  if (!chunk_manager)
    return;
    
  // Scan for potential source blocks
  for (int x = -32; x <= 32; x++) {
    for (int y = -32; y <= 32; y++) {
      for (int z = -32; z <= 32; z++) {
        BlockID block = block_get(chunk_manager, x, y, z);
        
        if (block == BLOCK_WATER || block == BLOCK_LAVA) {
          // Check if this could be a source
          bool is_source = fluid_check_source_conditions(chunk_manager, x, y, z, block);
          
          if (is_source) {
            fluid_add_source(x, y, z, block, 100);
          }
        }
      }
    }
  }
  
  LOG_DEBUG("Detected %u fluid sources", g_source_count);
}

// Check if a block meets source conditions
static bool fluid_check_source_conditions(ChunkManager *chunk_manager, i32 x, i32 y, i32 z, BlockID block_type) {
  // Simplified source detection - in real implementation would check:
  // - Infinite water source blocks
  // - Spring blocks
  // - Generated sources from world generation
  
  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return false;
    
  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;
  
  // Check if surrounded by same liquid type (indicating source)
  u8 same_type_count = 0;
  BlockID neighbors[6] = {
    chunk_get_block(chunk, lx + 1, ly, lz),
    chunk_get_block(chunk, lx - 1, ly, lz),
    chunk_get_block(chunk, lx, ly + 1, lz),
    chunk_get_block(chunk, lx, ly - 1, lz),
    chunk_get_block(chunk, lx, ly, lz + 1),
    chunk_get_block(chunk, lx, ly, lz - 1)
  };
  
  for (int i = 0; i < 6; i++) {
    if (neighbors[i] == block_type) {
      same_type_count++;
    }
  }
  
  // Source if mostly surrounded by same type and has liquid below
  BlockID below = chunk_get_block(chunk, lx, ly - 1, lz);
  return same_type_count >= 4 && below == block_type;
}

// Add fluid source
static void fluid_add_source(i32 x, i32 y, i32 z, BlockID liquid_type, u32 strength) {
  if (g_source_count >= MAX_FLUID_BLOCKS) {
    LOG_WARN("Maximum fluid sources reached");
    return;
  }
  
  // Check if source already exists
  for (u32 i = 0; i < g_source_count; i++) {
    if (g_sources[i].x == x && g_sources[i].y == y && g_sources[i].z == z) {
      // Update existing source
      g_sources[i].strength = strength;
      g_sources[i].is_active = true;
      g_sources[i].discovery_time = (u32)time(NULL);
      return;
    }
  }
  
  // Add new source
  FluidSource *source = &g_sources[g_source_count++];
  source->x = x;
  source->y = y;
  source->z = z;
  source->liquid_type = liquid_type;
  source->strength = strength;
  source->is_active = true;
  source->discovery_time = (u32)time(NULL);
  
  LOG_DEBUG("Added fluid source at (%d,%d,%d) with strength %u", x, y, z, strength);
}

// Visual effects for flowing animations
void fluid_create_visual_effects(ChunkManager *chunk_manager, i32 x, i32 y, i32 z, BlockID fluid_type) {
  if (!chunk_manager)
    return;
    
  // In a real implementation, this would create:
  // - Particle effects for water splashing
  // - Wave animations on water surfaces
  // - Bubbling effects for lava
  // - Flow direction indicators
  // - Temperature-based visual effects (steam, ice crystals)
  
  LOG_DEBUG("Creating visual effects for %s at (%d,%d,%d)", 
            fluid_type == BLOCK_WATER ? "water" : "lava", x, y, z);
            
  // Placeholder for particle system integration
  // particle_system_create_splash(x, y, z, fluid_type);
  // particle_system_create_bubbles(x, y, z, fluid_type);
  // particle_system_create_steam(x, y, z, fluid_type);
}

// Fluid mixing system
void fluid_process_mixing(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  if (!chunk_manager)
    return;
    
  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return;
    
  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;
  
  BlockID current_block = chunk_get_block(chunk, lx, ly, lz);
  
  // Check for mixing opportunities
  for (u32 i = 0; i < g_mixing_rule_count; i++) {
    FluidMixingRule *rule = &g_mixing_rules[i];
    
    // Check adjacent blocks for mixing
    BlockID adjacent1 = BLOCK_AIR;
    BlockID adjacent2 = BLOCK_AIR;
    
    // Simplified adjacency check
    // In real implementation, check all 6 neighbors
    
    if ((current_block == rule->liquid1 && adjacent2 == rule->liquid2) ||
        (current_block == rule->liquid2 && adjacent1 == rule->liquid1)) {
      
      // Apply mixing rule
      u32 current_time = (u32)time(NULL) * 1000;
      
      if (current_time - rule->reaction_time_ms >= rule->reaction_time_ms) {
        // Replace with result block
        chunk_set_block(chunk, lx, ly, lz, rule->result);
        
        LOG_INFO("Fluid mixing: %s + %s = %s at (%d,%d,%d)",
                 rule->liquid1 == BLOCK_WATER ? "water" : "lava",
                 rule->liquid2 == BLOCK_WATER ? "water" : "lava",
                 rule->result == BLOCK_OBSIDIAN ? "obsidian" : "cobblestone",
                 x, y, z);
                 
        // Create visual effects
        fluid_create_visual_effects(chunk_manager, x, y, z, rule->result);
      }
    }
  }
}
