#include "terrain_streaming.h"
#include "../core/logger.h"
#include "../core/memory.h"
#include "../core/threading.h"
#include "../core/time.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                                   TERRAIN STREAMING SYSTEM - IMPLEMENTATION
 * =================================================================================================
 */

// =================================================================================================
//                         EXPANSION ROADMAP (See: ROADMAP.h Phase 9)
// =================================================================================================
//
// TODO(ROADMAP Phase 9 - Terrain System): Procedural Terrain Generation
//   Current: Static terrain tiles
//   Target: Infinite procedural terrain with noise-based generation
//   Implementation:
//     - Integrate FastNoise2 library for multi-octave noise
//     - Implement biome system (desert, forest, mountains, ocean)
//     - Add height-based terrain features (cliffs, valleys, plateaus)
//     - Support erosion simulation for realistic terrain
//     - Implement cave generation with 3D noise
//     - Add procedural detail objects (rocks, trees, grass)
//   Performance: <10ms per tile generation
//   Files: terrain/procedural_generator.c
//
// TODO(ROADMAP Phase 9): GPU Tessellation for Terrain
//   Current: Static mesh LOD
//   Target: Dynamic GPU tessellation based on camera distance
//   Implementation:
//     - Create tessellation shaders (hull, domain shaders)
//     - Implement distance-based tessellation factor
//     - Add displacement mapping from heightmap
//     - Support adaptive tessellation (more detail on slopes)
//     - Implement crack-free LOD transitions
//     - Add tessellation budget control
//   Performance: 60 FPS with 1M+ triangles
//   Files: assets/shaders/terrain/terrain_tess.tesc, terrain_tess.tese
//
// TODO(ROADMAP Phase 9): Virtual Texturing System
//   Current: Standard texture atlases
//   Target: Mega-texture system for unique terrain texturing
//   Implementation:
//     - Implement tile-based texture streaming
//     - Create texture cache (GPU resident pages)
//     - Add mipmap streaming for distant tiles
//     - Support runtime texture composition (blend layers)
//     - Implement feedback buffer for page requests
//     - Add texture compression (BC7/ASTC)
//   Performance: <2GB VRAM for infinite unique textures
//   Reference: "Virtual Texturing" (id Software, Rage)
//
// TODO(ROADMAP Phase 9): Terrain Deformation System
//   Current: Static terrain
//   Target: Real-time terrain modification (digging, explosions)
//   Implementation:
//     - Implement voxel-based terrain representation
//     - Add marching cubes for smooth surfaces
//     - Support brush-based terrain sculpting
//     - Implement physics collision update on deformation
//     - Add deformation undo/redo system
//     - Support networked terrain changes
//   Use case: Destructible environments, mining, building
//
// TODO(ROADMAP Phase 9): Terrain Material Splatting
//   Current: Single material per tile
//   Target: Multi-layer material blending with splatmaps
//   Implementation:
//     - Support 4-8 material layers per tile
//     - Implement triplanar mapping for steep slopes
//     - Add height-based automatic material assignment
//     - Support slope-based material blending
//     - Implement detail textures (macro + micro variation)
//     - Add wetness/snow overlay system
//   Files: assets/shaders/terrain/terrain_splat.frag
//
// TODO(ROADMAP Phase 9): Terrain Collision Optimization
//   Current: Full mesh collision
//   Target: Optimized collision with spatial acceleration
//   Implementation:
//     - Build BVH (Bounding Volume Hierarchy) for terrain
//     - Implement heightfield collision (fast raycasts)
//     - Add collision mesh simplification for distant tiles
//     - Support dynamic collision update on deformation
//     - Implement terrain physics material properties
//   Performance: <0.1ms per raycast
//
// TODO(ROADMAP Phase 9): Grass and Vegetation System
//   Current: No vegetation
//   Target: GPU-instanced grass and foliage
//   Implementation:
//     - Implement GPU instancing for millions of grass blades
//     - Add wind animation (vertex shader)
//     - Support LOD for vegetation (billboard imposters)
//     - Implement density-based culling
//     - Add biome-specific vegetation
//     - Support interactive vegetation (player interaction)
//   Performance: 1M+ grass instances at 60 FPS
//   Files: renderer/vegetation_renderer.c
//
// TODO(ROADMAP Phase 9): Terrain Shadows and Lighting
//   Current: Basic lighting
//   Target: High-quality terrain shadows with cascaded shadow maps
//   Implementation:
//     - Implement cascaded shadow maps (4 cascades)
//     - Add terrain-specific shadow optimization
//     - Support contact-hardening shadows (PCSS)
//     - Implement ambient occlusion for terrain crevices
//     - Add dynamic time-of-day lighting
//     - Support volumetric fog for terrain
//   Files: assets/shaders/terrain/terrain_shadow.frag
//
// TODO(ROADMAP Phase 9): Water System Integration
//   Current: No water
//   Target: Realistic water rendering with reflections
//   Implementation:
//     - Implement water plane with wave simulation
//     - Add screen-space reflections for water
//     - Support underwater rendering (caustics, fog)
//     - Implement shoreline foam and waves
//     - Add buoyancy physics for objects
//     - Support water flow simulation
//   Files: renderer/water_renderer.c
//
// TODO(ROADMAP Phase 9): Terrain Streaming Optimization
//   Current: Basic tile streaming (586 LOC)
//   Target: Enhanced streaming with predictive loading
//   Implementation:
//     - Implement predictive tile loading (load ahead of player)
//     - Add priority queue for tile loading
//     - Support background thread for tile generation
//     - Implement tile compression for disk storage
//     - Add streaming metrics and profiling
//     - Support seamless world origin shifting
//   Performance: Zero hitches during streaming
//

// Internal helper functions
static void terrain_streaming_thread_func(void *user_data);
static void terrain_update_streaming_queue(TerrainStreamingSystem *system);
static void terrain_process_tile_loading(TerrainStreamingSystem *system);
static void terrain_process_tile_unloading(TerrainStreamingSystem *system);
static TerrainTile *terrain_create_tile(TerrainStreamingSystem *system,
                                        TerrainTileCoord coord);
static void terrain_destroy_tile(TerrainStreamingSystem *system,
                                 TerrainTile *tile);

void terrain_streaming_init(TerrainStreamingSystem *system) {
  if (!system) {
    LOG_ERROR("Terrain streaming system: NULL pointer");
    return;
  }

  // Clear the entire system
  memset(system, 0, sizeof(TerrainStreamingSystem));

  // Initialize configuration
  system->stream_radius = TERRAIN_STREAM_RADIUS;
  system->unload_radius = TERRAIN_UNLOAD_RADIUS;
  system->max_loaded_tiles = TERRAIN_MAX_LOADED_TILES;
  system->world_origin = vec3_zero();
  system->world_origin_offset = vec3_zero();

  // Create memory pools for terrain data
  system->heightmap_pool = memory_pool_create(sizeof(TerrainHeightmap), 256);
  system->splatmap_pool = memory_pool_create(sizeof(TerrainSplatmap), 256);
  system->holemap_pool = memory_pool_create(sizeof(TerrainHoleMap), 256);

  if (!system->heightmap_pool || !system->splatmap_pool ||
      !system->holemap_pool) {
    LOG_ERROR("Terrain streaming system: Failed to create memory pools");
    terrain_streaming_shutdown(system);
    return;
  }

  // Initialize tile array
  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    system->tiles[i] = NULL;
  }

  // Initialize eviction system
  system->eviction_policy = EVICTION_POLICY_HYBRID;
  system->eviction_threshold = 0.85f; // Start evicting at 85% memory usage
  system->max_evictions_per_frame = 4;
  memset(&system->eviction_stats, 0, sizeof(TerrainEvictionStats));

  // Start streaming thread
  system->streaming_active = true;
  system->streaming_thread =
      thread_create(terrain_streaming_thread_func, system);

  if (!system->streaming_thread) {
    LOG_ERROR("Terrain streaming system: Failed to create streaming thread");
    system->streaming_active = false;
  }

  LOG_INFO("Terrain streaming system initialized successfully");
  LOG_INFO("  Tile size: %.1fm x %.1fm", TERRAIN_TILE_SIZE, TERRAIN_TILE_SIZE);
  LOG_INFO("  Stream radius: %.1f tiles", system->stream_radius);
  LOG_INFO("  Max loaded tiles: %u", system->max_loaded_tiles);
}

void terrain_streaming_shutdown(TerrainStreamingSystem *system) {
  if (!system)
    return;

  // Stop streaming thread
  system->streaming_active = false;
  if (system->streaming_thread) {
    thread_join(system->streaming_thread);
    system->streaming_thread = NULL;
  }

  // Unload all tiles
  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    if (system->tiles[i]) {
      terrain_destroy_tile(system, system->tiles[i]);
      system->tiles[i] = NULL;
    }
  }

  // Destroy memory pools
  if (system->heightmap_pool) {
    memory_pool_destroy(system->heightmap_pool);
    system->heightmap_pool = NULL;
  }
  if (system->splatmap_pool) {
    memory_pool_destroy(system->splatmap_pool);
    system->splatmap_pool = NULL;
  }
  if (system->holemap_pool) {
    memory_pool_destroy(system->holemap_pool);
    system->holemap_pool = NULL;
  }

  // Log statistics
  LOG_INFO("Terrain streaming system shutdown");
  LOG_INFO("  Total tiles loaded: %u", system->tiles_loaded_total);
  LOG_INFO("  Total tiles unloaded: %u", system->tiles_unloaded_total);
  LOG_INFO("  Total streaming time: %llu ms", system->streaming_time_ms);

  memset(system, 0, sizeof(TerrainStreamingSystem));
}

void terrain_streaming_update(TerrainStreamingSystem *system,
                              Vec3 player_position, f32 delta_time) {
  if (!system || !system->streaming_active)
    return;

  // Update player position
  system->player_position = player_position;
  system->player_tile_coord = terrain_world_to_tile_coord(player_position);

  // Check for world origin shifting
  if (terrain_should_shift_origin(system)) {
    terrain_shift_world_origin(system, player_position);
  }

  // Update tile priorities and distances
  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    TerrainTile *tile = system->tiles[i];
    if (tile && tile->state != TILE_STATE_UNLOADED) {
      tile->distance_from_player =
          terrain_calculate_tile_distance(system, tile->coord);
      tile->priority = terrain_calculate_tile_priority(system, tile->coord);
      terrain_update_tile_lod(tile, tile->distance_from_player);

      // Update last access time
      tile->last_access_time = time_get_current_ms();
    }
  }

  // Process streaming queue (this will be handled by the streaming thread)
  // The main thread just updates priorities and positions

  // Process eviction on main thread to ensure immediate memory relief
  terrain_streaming_process_eviction(system);
}

TerrainTile *terrain_streaming_get_tile(TerrainStreamingSystem *system,
                                        TerrainTileCoord coord) {
  if (!system)
    return NULL;

  // Search for the tile in the loaded tiles array
  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    TerrainTile *tile = system->tiles[i];
    if (tile && tile->coord.x == coord.x && tile->coord.z == coord.z) {
      if (tile->state != TILE_STATE_UNLOADED) {
        tile->ref_count++;
        tile->last_access_time = time_get_current_ms();
        return tile;
      }
    }
  }

  // Tile not found, try to load it
  return terrain_streaming_load_tile(system, coord);
}

TerrainTile *terrain_streaming_load_tile(TerrainStreamingSystem *system,
                                         TerrainTileCoord coord) {
  if (!system || system->tile_count >= system->max_loaded_tiles) {
    return NULL;
  }

  // Check if tile is already loaded
  TerrainTile *existing_tile = terrain_streaming_get_tile(system, coord);
  if (existing_tile) {
    return existing_tile;
  }

  // Find empty slot
  u32 slot = 0;
  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    if (!system->tiles[i]) {
      slot = i;
      break;
    }
  }

  // Create and load the tile
  TerrainTile *tile = terrain_create_tile(system, coord);
  if (tile) {
    system->tiles[slot] = tile;
    system->tile_count++;
    system->tiles_loaded_total++;

    LOG_TRACE("Loaded terrain tile at (%d, %d)", coord.x, coord.z);
  }

  return tile;
}

void terrain_streaming_unload_tile(TerrainStreamingSystem *system,
                                   TerrainTile *tile) {
  if (!system || !tile)
    return;

  // Find and remove the tile from the array
  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    if (system->tiles[i] == tile) {
      terrain_destroy_tile(system, tile);
      system->tiles[i] = NULL;
      system->tile_count--;
      system->tiles_unloaded_total++;

      LOG_TRACE("Unloaded terrain tile at (%d, %d)", tile->coord.x,
                tile->coord.z);
      break;
    }
  }
}

bool terrain_streaming_is_tile_loaded(TerrainStreamingSystem *system,
                                      TerrainTileCoord coord) {
  if (!system)
    return false;

  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    TerrainTile *tile = system->tiles[i];
    if (tile && tile->coord.x == coord.x && tile->coord.z == coord.z) {
      return tile->state != TILE_STATE_UNLOADED &&
             tile->state != TILE_STATE_LOADING;
    }
  }

  return false;
}

// Coordinate conversion functions
TerrainTileCoord terrain_world_to_tile_coord(Vec3 world_pos) {
  TerrainTileCoord coord;
  coord.x = (i32)floorf(world_pos.x / TERRAIN_TILE_SIZE);
  coord.z = (i32)floorf(world_pos.z / TERRAIN_TILE_SIZE);
  return coord;
}

Vec3 terrain_tile_to_world_center(TerrainTileCoord coord) {
  return vec3((coord.x * TERRAIN_TILE_SIZE) + (TERRAIN_TILE_SIZE * 0.5f), 0.0f,
              (coord.z * TERRAIN_TILE_SIZE) + (TERRAIN_TILE_SIZE * 0.5f));
}

TerrainWorldCoord terrain_tile_to_world_coord(TerrainTileCoord coord) {
  TerrainWorldCoord world;
  world.x = coord.x;
  world.z = coord.z;
  return world;
}

// Priority and distance calculations
f32 terrain_calculate_tile_priority(TerrainStreamingSystem *system,
                                    TerrainTileCoord coord) {
  f32 distance = terrain_calculate_tile_distance(system, coord);

  // Higher priority for closer tiles
  if (distance <= system->stream_radius) {
    return 1.0f - (distance / system->stream_radius);
  }

  // Lower priority for tiles just outside stream radius
  if (distance <= system->unload_radius) {
    return 0.1f * (1.0f - ((distance - system->stream_radius) /
                           (system->unload_radius - system->stream_radius)));
  }

  // No priority for distant tiles
  return 0.0f;
}

f32 terrain_calculate_tile_distance(TerrainStreamingSystem *system,
                                    TerrainTileCoord coord) {
  Vec3 tile_center = terrain_tile_to_world_center(coord);
  return vec3_distance(system->player_position, tile_center) /
         TERRAIN_TILE_SIZE;
}

// LOD system
u32 terrain_get_tile_lod_level(TerrainTile *tile) {
  if (!tile)
    return 0;

  // Simple LOD based on distance
  f32 distance = tile->distance_from_player;

  if (distance < 2.0f)
    return 0; // Highest detail
  else if (distance < 4.0f)
    return 1; // High detail
  else if (distance < 8.0f)
    return 2; // Medium detail
  else
    return 3; // Low detail
}

void terrain_update_tile_lod(TerrainTile *tile, f32 distance) {
  if (!tile)
    return;

  tile->distance_from_player = distance;
  // LOD level is calculated on demand in terrain_get_tile_lod_level()
}

// World origin shifting
void terrain_shift_world_origin(TerrainStreamingSystem *system,
                                Vec3 new_origin) {
  if (!system)
    return;

  // Calculate offset
  Vec3 offset = vec3_sub(new_origin, system->world_origin);
  system->world_origin_offset = vec3_add(system->world_origin_offset, offset);
  system->world_origin = new_origin;

  LOG_INFO("Terrain world origin shifted to (%.1f, %.1f, %.1f)", new_origin.x,
           new_origin.y, new_origin.z);
}

bool terrain_should_shift_origin(TerrainStreamingSystem *system) {
  if (!system)
    return false;

  // Shift origin if player is too far from current origin
  f32 distance_from_origin =
      vec3_distance(system->player_position, system->world_origin);
  return distance_from_origin >
         (TERRAIN_TILE_SIZE * 32.0f); // Shift every 32 tiles
}

// Utility functions
bool terrain_is_tile_in_range(TerrainTileCoord center, TerrainTileCoord tile,
                              f32 radius) {
  f32 dx = (f32)(tile.x - center.x);
  f32 dz = (f32)(tile.z - center.z);
  return sqrtf(dx * dx + dz * dz) <= radius;
}

TerrainTileCoord terrain_get_tile_neighbors(TerrainTileCoord coord,
                                            u32 direction) {
  TerrainTileCoord neighbor = coord;

  switch (direction) {
  case 0:
    neighbor.z--;
    break; // North
  case 1:
    neighbor.x++;
    break; // East
  case 2:
    neighbor.z++;
    break; // South
  case 3:
    neighbor.x--;
    break; // West
  default:
    break;
  }

  return neighbor;
}

// Internal helper functions
static void terrain_streaming_thread_func(void *user_data) {
  TerrainStreamingSystem *system = (TerrainStreamingSystem *)user_data;

  while (system->streaming_active) {
    u64 start_time = time_get_current_ms();

    // Process streaming queue
    terrain_update_streaming_queue(system);
    terrain_process_tile_loading(system);
    terrain_process_tile_unloading(system);

    // Update statistics
    u64 end_time = time_get_current_ms();
    system->streaming_time_ms += (end_time - start_time);

    // Sleep for a bit to avoid using too much CPU
    thread_sleep(16); // ~60 FPS
  }
}

static void terrain_update_streaming_queue(TerrainStreamingSystem *system) {
  // This function would update the list of tiles that need to be
  // loaded/unloaded based on player position and priorities For now, this is
  // handled in terrain_streaming_update()
}

static void terrain_process_tile_loading(TerrainStreamingSystem *system) {
  // Process tiles that need to be loaded from disk to RAM
  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    TerrainTile *tile = system->tiles[i];
    if (tile && tile->state == TILE_STATE_LOADING) {
      // Simulate loading process (in real implementation, this would load from
      // disk)
      tile->state = TILE_STATE_LOADED_RAM;
    }
  }
}

static void terrain_process_tile_unloading(TerrainStreamingSystem *system) {
  // Process tiles that need to be unloaded based on distance and priority
  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    TerrainTile *tile = system->tiles[i];
    if (tile && tile->state != TILE_STATE_UNLOADED && tile->ref_count == 0) {
      f32 distance = terrain_calculate_tile_distance(system, tile->coord);
      if (distance > system->unload_radius) {
        terrain_streaming_unload_tile(system, tile);
      }
    }
  }
}

static TerrainTile *terrain_create_tile(TerrainStreamingSystem *system,
                                        TerrainTileCoord coord) {
  TerrainTile *tile = malloc(sizeof(TerrainTile));
  if (!tile)
    return NULL;

  memset(tile, 0, sizeof(TerrainTile));
  tile->coord = coord;
  tile->world_coord = terrain_tile_to_world_coord(coord);
  tile->state = TILE_STATE_LOADING;
  tile->priority = terrain_calculate_tile_priority(system, coord);
  tile->ref_count = 1;
  tile->last_access_time = time_get_current_ms();
  tile->load_time = tile->last_access_time;

  // Allocate terrain data from memory pools
  tile->heightmap = memory_pool_alloc(system->heightmap_pool);
  tile->splatmap = memory_pool_alloc(system->splatmap_pool);
  tile->holemap = memory_pool_alloc(system->holemap_pool);

  if (!tile->heightmap || !tile->splatmap || !tile->holemap) {
    LOG_ERROR("Failed to allocate memory for terrain tile data");
    terrain_destroy_tile(system, tile);
    return NULL;
  }

  // Initialize terrain data with default values
  memset(tile->heightmap, 0, sizeof(TerrainHeightmap));
  memset(tile->splatmap, 0, sizeof(TerrainSplatmap));
  memset(tile->holemap, 0, sizeof(TerrainHoleMap));

  tile->heightmap->scale_y = 1.0f;

  return tile;
}

static void terrain_destroy_tile(TerrainStreamingSystem *system,
                                 TerrainTile *tile) {
  if (!tile)
    return;

  // Return terrain data to memory pools
  if (tile->heightmap && system->heightmap_pool) {
    memory_pool_free(system->heightmap_pool, tile->heightmap);
  }
  if (tile->splatmap && system->splatmap_pool) {
    memory_pool_free(system->splatmap_pool, tile->splatmap);
  }
  if (tile->holemap && system->holemap_pool) {
    memory_pool_free(system->holemap_pool, tile->holemap);
  }

  free(tile);
}

// Eviction system implementation
void terrain_streaming_set_eviction_policy(TerrainStreamingSystem *system,
                                           TerrainEvictionPolicy policy) {
  if (!system)
    return;

  system->eviction_policy = policy;
  LOG_INFO("Terrain eviction policy set to: %d", policy);
}

void terrain_streaming_process_eviction(TerrainStreamingSystem *system) {
  if (!system)
    return;

  u64 start_time = time_get_current_ms();
  system->eviction_stats.tiles_evicted_this_frame = 0;

  // Check if eviction is needed
  if (!terrain_streaming_should_evict(system)) {
    return;
  }

  // Process evictions up to the per-frame limit
  for (u32 i = 0; i < system->max_evictions_per_frame; i++) {
    TerrainTile *candidate =
        terrain_streaming_select_eviction_candidate(system);
    if (!candidate) {
      break; // No more candidates
    }

    // Evict the tile
    terrain_streaming_unload_tile(system, candidate);
    system->eviction_stats.tiles_evicted_total++;
    system->eviction_stats.tiles_evicted_this_frame++;

    // Check if we've evicted enough
    if (!terrain_streaming_should_evict(system)) {
      break;
    }
  }

  u64 end_time = time_get_current_ms();
  system->eviction_stats.eviction_time_ms += (end_time - start_time);

  if (system->eviction_stats.tiles_evicted_this_frame > 0) {
    LOG_TRACE("Evicted %u terrain tiles this frame",
              system->eviction_stats.tiles_evicted_this_frame);
  }
}

TerrainTile *
terrain_streaming_select_eviction_candidate(TerrainStreamingSystem *system) {
  if (!system || system->tile_count == 0)
    return NULL;

  TerrainTile *best_candidate = NULL;
  f32 best_score = -1.0f;

  // Calculate memory usage ratio
  f32 memory_usage = (f32)system->tile_count / (f32)system->max_loaded_tiles;
  system->eviction_stats.memory_usage_ratio = memory_usage;

  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    TerrainTile *tile = system->tiles[i];
    if (!tile || tile->state == TILE_STATE_UNLOADED || tile->ref_count > 0) {
      continue; // Skip unloaded or referenced tiles
    }

    f32 score = 0.0f;

    switch (system->eviction_policy) {
    case EVICTION_POLICY_LRU: {
      // Lower score = older = better eviction candidate
      u64 current_time = time_get_current_ms();
      u64 age = current_time - tile->last_access_time;
      score = 1.0f / (1.0f + (f32)age / 1000.0f); // Convert to seconds
      break;
    }

    case EVICTION_POLICY_DISTANCE: {
      // Higher score = farther = better eviction candidate
      score = tile->distance_from_player / system->unload_radius;
      break;
    }

    case EVICTION_POLICY_PRIORITY: {
      // Lower score = lower priority = better eviction candidate
      score = 1.0f - tile->priority;
      break;
    }

    case EVICTION_POLICY_HYBRID: {
      // Combine multiple factors
      f32 distance_score = tile->distance_from_player / system->unload_radius;
      f32 priority_score = 1.0f - tile->priority;
      u64 current_time = time_get_current_ms();
      u64 age = current_time - tile->last_access_time;
      f32 age_score = 1.0f / (1.0f + (f32)age / 1000.0f);

      // Weighted combination
      score = (distance_score * 0.4f) + (priority_score * 0.3f) +
              (age_score * 0.3f);
      break;
    }
    }

    // Prefer tiles outside the unload radius
    if (tile->distance_from_player > system->unload_radius) {
      score += 2.0f; // Bonus for being outside range
    }

    if (score > best_score) {
      best_score = score;
      best_candidate = tile;
    }
  }

  return best_candidate;
}

bool terrain_streaming_should_evict(TerrainStreamingSystem *system) {
  if (!system)
    return false;

  // Check memory usage
  f32 memory_usage = (f32)system->tile_count / (f32)system->max_loaded_tiles;

  // Evict if we're over the threshold
  return memory_usage > system->eviction_threshold;
}

void terrain_streaming_get_eviction_stats(TerrainStreamingSystem *system,
                                          TerrainEvictionStats *stats) {
  if (!system || !stats)
    return;

  *stats = system->eviction_stats;

  // Calculate average tile age
  u64 current_time = time_get_current_ms();
  u64 total_age = 0;
  u32 active_tiles = 0;

  for (u32 i = 0; i < TERRAIN_MAX_LOADED_TILES; i++) {
    TerrainTile *tile = system->tiles[i];
    if (tile && tile->state != TILE_STATE_UNLOADED) {
      total_age += (current_time - tile->load_time);
      active_tiles++;
    }
  }

  stats->average_tile_age =
      active_tiles > 0 ? (f32)total_age / (f32)active_tiles / 1000.0f : 0.0f;
  stats->memory_usage_ratio =
      (f32)system->tile_count / (f32)system->max_loaded_tiles;
}
