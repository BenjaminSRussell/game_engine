/**
 * =================================================================================================
 *                              MEGA TERRAIN SYSTEM - ADVANCED FEATURES
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Enterprise-grade terrain system with 64-bit coordinates, configurable chunk
 * sizes, large world support, chunk pooling, LOD management, and comprehensive
 * configuration.
 *
 * =================================================================================================
 */

#include "../../../core/memory/memory_pool.h"
#include "../../../include/core/config_system.h"
#include "../../../procedural/noise/fast_noise_lite.h"
#include "../../../rendering/gpu_memory.h"
#include "mega_terrain.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Configuration system integration
#define TERRAIN_CONFIG_SECTION "terrain"
#define TERRAIN_CONFIG_VERSION 1

// 64-bit coordinate system
typedef struct WorldCoord64 {
  int64_t x, y, z;
} WorldCoord64;

// Configurable chunk configuration
typedef struct TerrainChunkConfig {
  uint32_t chunk_size;     // Configurable chunk size (default: 256)
  uint32_t max_chunks;     // Configurable max chunks (default: 4096)
  uint32_t max_world_size; // Maximum world size in chunks
  uint32_t lod_levels;     // Number of LOD levels
  float lod_distances[8];  // LOD distance thresholds

  // Memory management
  size_t chunk_pool_size;    // Chunk allocation pool size
  bool enable_compression;   // Enable heightmap compression
  bool enable_gpu_streaming; // Enable GPU streaming

  // World boundaries
  WorldCoord64 world_min; // Minimum world coordinates
  WorldCoord64 world_max; // Maximum world coordinates

  // Performance settings
  uint32_t max_chunks_per_frame; // Max chunks to process per frame
  float streaming_budget_mb;     // Memory budget for streaming
  float load_radius_multiplier;  // Multiplier for load radius
} TerrainChunkConfig;

// Chunk allocation pool for performance
typedef struct ChunkPool {
  TerrainChunk *chunks;   // Pre-allocated chunk array
  uint32_t *free_indices; // Stack of free chunk indices
  uint32_t pool_size;     // Total pool size
  uint32_t used_count;    // Currently used chunks
  uint32_t free_count;    // Available chunks

  // Memory statistics
  size_t total_memory_used;
  size_t peak_memory_used;
  uint32_t allocation_count;
  uint32_t deallocation_count;
} ChunkPool;

// LOD management system
typedef struct LODManager {
  uint32_t current_lod;       // Current global LOD level
  float lod_transition_speed; // Speed of LOD transitions
  float *distance_thresholds; // Distance thresholds for each LOD
  uint32_t *chunk_counts;     // Number of chunks at each LOD

  // LOD calculation cache
  float *lod_multipliers;      // Performance multipliers per LOD
  uint32_t *vertex_reductions; // Vertex count reduction per LOD
} LODManager;

// Configuration validation structure
typedef struct TerrainConfigValidation {
  bool is_valid;
  char error_message[256];
  uint32_t warning_count;
  char warnings[10][128];
} TerrainConfigValidation;

// Global terrain state
static struct {
  TerrainChunkConfig config;
  ChunkPool chunk_pool;
  LODManager lod_manager;
  bool is_initialized;
  bool use_64bit_coordinates;

  // Performance monitoring
  float last_update_time_ms;
  uint32_t chunks_loaded_this_frame;
  uint32_t chunks_unloaded_this_frame;
  uint64_t total_triangles_rendered;

  // Configuration file path
  char config_file_path[512];

  // World coordinate transformation
  WorldCoord64 world_origin;
  float coordinate_scale;
} g_terrain_state = {0};

// Forward declarations
static bool terrain_config_load_from_file(const char *config_path);
static bool terrain_config_save_to_file(const char *config_path);
static TerrainConfigValidation
terrain_config_validate(const TerrainChunkConfig *config);
static void terrain_chunk_config_set_defaults(TerrainChunkConfig *config);

static bool chunk_pool_initialize(ChunkPool *pool, uint32_t pool_size);
static void chunk_pool_shutdown(ChunkPool *pool);
static TerrainChunk *chunk_pool_allocate(ChunkPool *pool);
static void chunk_pool_deallocate(ChunkPool *pool, TerrainChunk *chunk);
static size_t chunk_pool_get_memory_usage(const ChunkPool *pool);

static bool lod_manager_initialize(LODManager *manager,
                                   const TerrainChunkConfig *config);
static void lod_manager_shutdown(LODManager *manager);
static uint32_t lod_manager_calculate_lod(const LODManager *manager,
                                          float distance);
static float lod_manager_get_distance_threshold(const LODManager *manager,
                                                uint32_t lod);
static void lod_manager_update_chunk_lod(LODManager *manager,
                                         TerrainChunk *chunk,
                                         const float *camera_pos);

static WorldCoord64 float_to_world_coord_64bit(float x, float y, float z);
static void world_coord_64bit_to_float(WorldCoord64 coord, float *out_x,
                                       float *out_y, float *out_z);
static bool is_coordinate_in_world_bounds(WorldCoord64 coord);

static uint64_t calculate_chunk_hash(int64_t chunk_x, int64_t chunk_z);
static bool generate_chunk_heightmap_64bit(TerrainChunk *chunk, int64_t chunk_x,
                                           int64_t chunk_z);

/**
 * Initialize advanced terrain system with configuration support
 */
bool terrain_system_init_advanced(TerrainSystem *system,
                                  const char *config_file_path) {
  if (!system) {
    return false;
  }

  memset(&g_terrain_state, 0, sizeof(g_terrain_state));

  // Set default configuration
  terrain_chunk_config_set_defaults(&g_terrain_state.config);

  // Load configuration from file if provided
  if (config_file_path && strlen(config_file_path) > 0) {
    strncpy(g_terrain_state.config_file_path, config_file_path,
            sizeof(g_terrain_state.config_file_path) - 1);
    if (!terrain_config_load_from_file(config_file_path)) {
      fprintf(
          stderr,
          "Warning: Failed to load terrain config from %s, using defaults\n",
          config_file_path);
    }
  }

  // Validate configuration
  TerrainConfigValidation validation =
      terrain_config_validate(&g_terrain_state.config);
  if (!validation.is_valid) {
    fprintf(stderr, "Terrain configuration validation failed: %s\n",
            validation.error_message);
    return false;
  }

  if (validation.warning_count > 0) {
    fprintf(stderr, "Terrain configuration warnings:\n");
    for (uint32_t i = 0; i < validation.warning_count; i++) {
      fprintf(stderr, "  - %s\n", validation.warnings[i]);
    }
  }

  // Initialize system with validated configuration
  system->config.world_size_x =
      g_terrain_state.config.chunk_size * g_terrain_state.config.max_world_size;
  system->config.world_size_z =
      g_terrain_state.config.chunk_size * g_terrain_state.config.max_world_size;
  system->config.chunk_resolution = g_terrain_state.config.chunk_size;
  system->config.max_lod = g_terrain_state.config.lod_levels;

  for (uint32_t i = 0; i < g_terrain_state.config.lod_levels; i++) {
    system->config.lod_distances[i] = g_terrain_state.config.lod_distances[i];
  }

  // Initialize chunk pool
  if (!chunk_pool_initialize(&g_terrain_state.chunk_pool,
                             g_terrain_state.config.chunk_pool_size)) {
    fprintf(stderr, "Failed to initialize chunk pool\n");
    return false;
  }

  // Initialize LOD manager
  if (!lod_manager_initialize(&g_terrain_state.lod_manager,
                              &g_terrain_state.config)) {
    fprintf(stderr, "Failed to initialize LOD manager\n");
    chunk_pool_shutdown(&g_terrain_state.chunk_pool);
    return false;
  }

  // Initialize system arrays
  system->chunk_count = 0;
  system->chunks =
      calloc(g_terrain_state.config.max_chunks, sizeof(TerrainChunk));
  if (!system->chunks) {
    fprintf(stderr, "Failed to allocate chunk array\n");
    lod_manager_shutdown(&g_terrain_state.lod_manager);
    chunk_pool_shutdown(&g_terrain_state.chunk_pool);
    return false;
  }

  // Initialize streaming state
  memset(&system->streaming, 0, sizeof(TerrainStreamingState));
  system->streaming.is_streaming = true;
  system->streaming.load_radius =
      1000.0f * g_terrain_state.config.load_radius_multiplier;
  system->streaming.unload_radius =
      1200.0f * g_terrain_state.config.load_radius_multiplier;
  system->streaming.max_chunks_per_frame =
      g_terrain_state.config.max_chunks_per_frame;

  // Initialize biomes
  terrain_system_init_biomes(system);

  g_terrain_state.is_initialized = true;
  g_terrain_state.use_64bit_coordinates = true;
  g_terrain_state.world_origin = (WorldCoord64){0, 0, 0};
  g_terrain_state.coordinate_scale = 1.0f;

  printf("Advanced terrain system initialized:\n");
  printf("  - Chunk size: %u meters\n", g_terrain_state.config.chunk_size);
  printf("  - Max chunks: %u\n", g_terrain_state.config.max_chunks);
  printf("  - Max world size: %u chunks (%u km)\n",
         g_terrain_state.config.max_world_size,
         (g_terrain_state.config.max_world_size *
          g_terrain_state.config.chunk_size) /
             1000);
  printf("  - LOD levels: %u\n", g_terrain_state.config.lod_levels);
  printf("  - Chunk pool size: %u\n", g_terrain_state.config.chunk_pool_size);
  printf("  - 64-bit coordinates: %s\n",
         g_terrain_state.use_64bit_coordinates ? "enabled" : "disabled");

  return true;
}

/**
 * Shutdown advanced terrain system
 */
void terrain_system_shutdown_advanced(TerrainSystem *system) {
  if (!system) {
    return;
  }

  if (g_terrain_state.is_initialized) {
    // Save configuration if file path is set
    if (strlen(g_terrain_state.config_file_path) > 0) {
      terrain_config_save_to_file(g_terrain_state.config_file_path);
    }

    // Shutdown subsystems
    lod_manager_shutdown(&g_terrain_state.lod_manager);
    chunk_pool_shutdown(&g_terrain_state.chunk_pool);

    // Free system memory
    if (system->chunks) {
      for (uint32_t i = 0; i < system->chunk_count; i++) {
        if (system->chunks[i].is_loaded) {
          terrain_chunk_unload_advanced(system, &system->chunks[i]);
        }
      }
      free(system->chunks);
      system->chunks = NULL;
    }

    g_terrain_state.is_initialized = false;

    printf("Advanced terrain system shutdown complete\n");
  }
}

/**
 * Make chunk size configurable at runtime
 */
bool terrain_set_chunk_size(uint32_t new_chunk_size) {
  if (!g_terrain_state.is_initialized) {
    fprintf(stderr, "Terrain system not initialized\n");
    return false;
  }

  if (new_chunk_size < 32 || new_chunk_size > 1024) {
    fprintf(stderr, "Invalid chunk size: %u (must be 32-1024)\n",
            new_chunk_size);
    return false;
  }

  if (g_terrain_state.chunk_pool.used_count > 0) {
    fprintf(stderr, "Cannot change chunk size while chunks are loaded\n");
    return false;
  }

  // Update configuration
  g_terrain_state.config.chunk_size = new_chunk_size;

  // Revalidate configuration
  TerrainConfigValidation validation =
      terrain_config_validate(&g_terrain_state.config);
  if (!validation.is_valid) {
    fprintf(stderr,
            "Configuration validation failed after chunk size change: %s\n",
            validation.error_message);
    return false;
  }

  printf("Chunk size changed to %u meters\n", new_chunk_size);
  return true;
}

/**
 * Add support for larger worlds by expanding coordinate system
 */
bool terrain_enable_large_worlds(uint32_t max_world_size_chunks) {
  if (!g_terrain_state.is_initialized) {
    fprintf(stderr, "Terrain system not initialized\n");
    return false;
  }

  if (max_world_size_chunks < 4096 || max_world_size_chunks > 65536) {
    fprintf(stderr, "Invalid world size: %u chunks (must be 4096-65536)\n",
            max_world_size_chunks);
    return false;
  }

  if (g_terrain_state.chunk_pool.used_count > 0) {
    fprintf(stderr, "Cannot enable large worlds while chunks are loaded\n");
    return false;
  }

  // Update configuration
  g_terrain_state.config.max_world_size = max_world_size_chunks;
  g_terrain_state.config.max_chunks =
      max_world_size_chunks * max_world_size_chunks;

  // Enable 64-bit coordinates for large worlds
  g_terrain_state.use_64bit_coordinates = true;

  // Revalidate configuration
  TerrainConfigValidation validation =
      terrain_config_validate(&g_terrain_state.config);
  if (!validation.is_valid) {
    fprintf(stderr, "Configuration validation failed for large worlds: %s\n",
            validation.error_message);
    return false;
  }

  printf("Large world support enabled: %u chunks (%u km world size)\n",
         max_world_size_chunks,
         (max_world_size_chunks * g_terrain_state.config.chunk_size) / 1000);

  return true;
}

/**
 * Implement 64-bit world coordinates
 */
bool terrain_set_world_coordinates_64bit(int64_t origin_x, int64_t origin_y,
                                         int64_t origin_z) {
  if (!g_terrain_state.is_initialized) {
    fprintf(stderr, "Terrain system not initialized\n");
    return false;
  }

  g_terrain_state.world_origin.x = origin_x;
  g_terrain_state.world_origin.y = origin_y;
  g_terrain_state.world_origin.z = origin_z;
  g_terrain_state.use_64bit_coordinates = true;

  printf("64-bit world coordinates set: origin(%lld, %lld, %lld)\n",
         (long long)origin_x, (long long)origin_y, (long long)origin_z);

  return true;
}

/**
 * Validate terrain configuration
 */
TerrainConfigValidation
terrain_config_validate(const TerrainChunkConfig *config) {
  TerrainConfigValidation result = {0};
  result.is_valid = true;

  // Validate chunk size
  if (config->chunk_size < 32 || config->chunk_size > 1024) {
    snprintf(result.error_message, sizeof(result.error_message),
             "Invalid chunk size: %u (must be 32-1024)", config->chunk_size);
    result.is_valid = false;
    return result;
  }

  // Validate max chunks
  if (config->max_chunks < 256 || config->max_chunks > 1000000) {
    snprintf(result.error_message, sizeof(result.error_message),
             "Invalid max chunks: %u (must be 256-1000000)",
             config->max_chunks);
    result.is_valid = false;
    return result;
  }

  // Validate LOD levels
  if (config->lod_levels < 1 || config->lod_levels > 8) {
    snprintf(result.error_message, sizeof(result.error_message),
             "Invalid LOD levels: %u (must be 1-8)", config->lod_levels);
    result.is_valid = false;
    return result;
  }

  // Validate LOD distances
  for (uint32_t i = 1; i < config->lod_levels; i++) {
    if (config->lod_distances[i] <= config->lod_distances[i - 1]) {
      snprintf(result.error_message, sizeof(result.error_message),
               "LOD distances must be increasing: level %u distance %.1f <= "
               "level %u distance %.1f",
               i, config->lod_distances[i], i - 1,
               config->lod_distances[i - 1]);
      result.is_valid = false;
      return result;
    }
  }

  // Validate memory settings
  if (config->chunk_pool_size < 64 ||
      config->chunk_pool_size > config->max_chunks) {
    snprintf(result.error_message, sizeof(result.error_message),
             "Invalid chunk pool size: %u (must be 64-%u)",
             config->chunk_pool_size, config->max_chunks);
    result.is_valid = false;
    return result;
  }

  // Validate world boundaries
  WorldCoord64 world_size = {config->world_max.x - config->world_min.x,
                             config->world_max.y - config->world_min.y,
                             config->world_max.z - config->world_min.z};

  int64_t max_world_size = (int64_t)config->max_world_size * config->chunk_size;
  if (world_size.x > max_world_size || world_size.y > max_world_size ||
      world_size.z > max_world_size) {
    snprintf(result.error_message, sizeof(result.error_message),
             "World boundaries exceed maximum size: (%lld, %lld, %lld) > %lld",
             (long long)world_size.x, (long long)world_size.y,
             (long long)world_size.z, (long long)max_world_size);
    result.is_valid = false;
    return result;
  }

  // Add warnings for suboptimal settings
  if (config->chunk_size < 64) {
    snprintf(result.warnings[result.warning_count++], 128,
             "Small chunk size (%u) may impact performance",
             config->chunk_size);
  }

  if (config->chunk_pool_size < config->max_chunks / 4) {
    snprintf(result.warnings[result.warning_count++], 128,
             "Chunk pool size (%u) is less than 25%% of max chunks",
             config->chunk_pool_size);
  }

  if (config->streaming_budget_mb < 512) {
    snprintf(result.warnings[result.warning_count++], 128,
             "Low streaming budget (%.1f MB) may cause frequent loading",
             config->streaming_budget_mb);
  }

  return result;
}

/**
 * Serialize terrain configuration
 */
bool terrain_config_serialize(const TerrainChunkConfig *config, char *buffer,
                              size_t buffer_size) {
  if (!config || !buffer || buffer_size == 0) {
    return false;
  }

  size_t offset = 0;
  offset +=
      snprintf(buffer + offset, buffer_size - offset,
               "{\n"
               "  \"version\": %d,\n"
               "  \"chunk_size\": %u,\n"
               "  \"max_chunks\": %u,\n"
               "  \"max_world_size\": %u,\n"
               "  \"lod_levels\": %u,\n"
               "  \"lod_distances\": [",
               TERRAIN_CONFIG_VERSION, config->chunk_size, config->max_chunks,
               config->max_world_size, config->lod_levels);

  for (uint32_t i = 0; i < config->lod_levels; i++) {
    offset += snprintf(buffer + offset, buffer_size - offset, "%.1f%s",
                       config->lod_distances[i],
                       (i < config->lod_levels - 1) ? ", " : "");
  }

  offset += snprintf(
      buffer + offset, buffer_size - offset,
      "],\n"
      "  \"chunk_pool_size\": %u,\n"
      "  \"enable_compression\": %s,\n"
      "  \"enable_gpu_streaming\": %s,\n"
      "  \"world_min\": [%lld, %lld, %lld],\n"
      "  \"world_max\": [%lld, %lld, %lld],\n"
      "  \"max_chunks_per_frame\": %u,\n"
      "  \"streaming_budget_mb\": %.1f,\n"
      "  \"load_radius_multiplier\": %.2f\n"
      "}\n",
      config->chunk_pool_size, config->enable_compression ? "true" : "false",
      config->enable_gpu_streaming ? "true" : "false",
      (long long)config->world_min.x, (long long)config->world_min.y,
      (long long)config->world_min.z, (long long)config->world_max.x,
      (long long)config->world_max.y, (long long)config->world_max.z,
      config->max_chunks_per_frame, config->streaming_budget_mb,
      config->load_radius_multiplier);

  return offset < buffer_size;
}

/**
 * Chunk allocation pooling implementation
 */
static bool chunk_pool_initialize(ChunkPool *pool, uint32_t pool_size) {
  if (!pool || pool_size == 0) {
    return false;
  }

  memset(pool, 0, sizeof(ChunkPool));

  pool->chunks = calloc(pool_size, sizeof(TerrainChunk));
  if (!pool->chunks) {
    return false;
  }

  pool->free_indices = calloc(pool_size, sizeof(uint32_t));
  if (!pool->free_indices) {
    free(pool->chunks);
    return false;
  }

  // Initialize free index stack
  for (uint32_t i = 0; i < pool_size; i++) {
    pool->free_indices[i] = pool_size - 1 - i; // Stack order
  }

  pool->pool_size = pool_size;
  pool->free_count = pool_size;
  pool->used_count = 0;
  pool->total_memory_used = 0;
  pool->peak_memory_used = 0;
  pool->allocation_count = 0;
  pool->deallocation_count = 0;

  printf("Initialized chunk pool: %u chunks, %.1f MB\n", pool_size,
         (pool_size * sizeof(TerrainChunk)) / (1024.0f * 1024.0f));

  return true;
}

static void chunk_pool_shutdown(ChunkPool *pool) {
  if (!pool) {
    return;
  }

  if (pool->chunks) {
    // Deallocate any remaining chunks
    for (uint32_t i = 0; i < pool->pool_size; i++) {
      if (pool->chunks[i].is_loaded) {
        if (pool->chunks[i].heightmap) {
          free(pool->chunks[i].heightmap);
        }
      }
    }
    free(pool->chunks);
  }

  if (pool->free_indices) {
    free(pool->free_indices);
  }

  printf("Chunk pool shutdown: allocations=%u, deallocations=%u, "
         "peak_memory=%.1f MB\n",
         pool->allocation_count, pool->deallocation_count,
         pool->peak_memory_used / (1024.0f * 1024.0f));

  memset(pool, 0, sizeof(ChunkPool));
}

static TerrainChunk *chunk_pool_allocate(ChunkPool *pool) {
  if (!pool || pool->free_count == 0) {
    return NULL;
  }

  uint32_t index = pool->free_indices[--pool->free_count];
  TerrainChunk *chunk = &pool->chunks[index];

  // Initialize chunk
  memset(chunk, 0, sizeof(TerrainChunk));
  chunk->current_lod = 0;
  chunk->is_loaded = false;
  chunk->needs_update = true;

  pool->used_count++;
  pool->allocation_count++;

  return chunk;
}

static void chunk_pool_deallocate(ChunkPool *pool, TerrainChunk *chunk) {
  if (!pool || !chunk) {
    return;
  }

  // Calculate index
  uint32_t index = (uint32_t)(chunk - pool->chunks);
  if (index >= pool->pool_size) {
    return; // Not from this pool
  }

  // Clean up chunk data
  if (chunk->heightmap) {
    free(chunk->heightmap);
    chunk->heightmap = NULL;
  }

  // Return to free stack
  pool->free_indices[pool->free_count++] = index;
  pool->used_count--;
  pool->deallocation_count++;
}

static size_t chunk_pool_get_memory_usage(const ChunkPool *pool) {
  if (!pool) {
    return 0;
  }

  size_t total_usage = pool->used_count * sizeof(TerrainChunk);

  // Add heightmap memory
  for (uint32_t i = 0; i < pool->pool_size; i++) {
    if (pool->chunks[i].is_loaded && pool->chunks[i].heightmap) {
      total_usage += pool->chunks[i].heightmap_size *
                     pool->chunks[i].heightmap_size * sizeof(float);
    }
  }

  return total_usage;
}

/**
 * LOD management implementation
 */
static bool lod_manager_initialize(LODManager *manager,
                                   const TerrainChunkConfig *config) {
  if (!manager || !config) {
    return false;
  }

  memset(manager, 0, sizeof(LODManager));

  manager->distance_thresholds = calloc(config->lod_levels, sizeof(float));
  manager->chunk_counts = calloc(config->lod_levels, sizeof(uint32_t));
  manager->lod_multipliers = calloc(config->lod_levels, sizeof(float));
  manager->vertex_reductions = calloc(config->lod_levels, sizeof(uint32_t));

  if (!manager->distance_thresholds || !manager->chunk_counts ||
      !manager->lod_multipliers || !manager->vertex_reductions) {
    lod_manager_shutdown(manager);
    return false;
  }

  // Copy distance thresholds from config
  for (uint32_t i = 0; i < config->lod_levels; i++) {
    manager->distance_thresholds[i] = config->lod_distances[i];
  }

  // Calculate LOD multipliers and vertex reductions
  for (uint32_t i = 0; i < config->lod_levels; i++) {
    manager->lod_multipliers[i] =
        1.0f / (1 << i); // Each LOD level halves the detail
    manager->vertex_reductions[i] =
        1 << i; // Each LOD level reduces vertices by power of 2
  }

  manager->current_lod = 0;
  manager->lod_transition_speed = 2.0f;

  printf("Initialized LOD manager: %u levels\n", config->lod_levels);
  for (uint32_t i = 0; i < config->lod_levels; i++) {
    printf("  LOD %u: distance=%.1f, multiplier=%.2f, vertex_reduction=%u\n", i,
           manager->distance_thresholds[i], manager->lod_multipliers[i],
           manager->vertex_reductions[i]);
  }

  return true;
}

static void lod_manager_shutdown(LODManager *manager) {
  if (!manager) {
    return;
  }

  if (manager->distance_thresholds)
    free(manager->distance_thresholds);
  if (manager->chunk_counts)
    free(manager->chunk_counts);
  if (manager->lod_multipliers)
    free(manager->lod_multipliers);
  if (manager->vertex_reductions)
    free(manager->vertex_reductions);

  memset(manager, 0, sizeof(LODManager));
}

static uint32_t lod_manager_calculate_lod(const LODManager *manager,
                                          float distance) {
  if (!manager || manager->distance_thresholds == NULL) {
    return 0;
  }

  // Find appropriate LOD level based on distance
  for (uint32_t i = 0; i < 8; i++) {
    if (distance <= manager->distance_thresholds[i]) {
      return i;
    }
  }

  return 7; // Highest LOD level for very distant objects
}

static float lod_manager_get_distance_threshold(const LODManager *manager,
                                                uint32_t lod) {
  if (!manager || lod >= 8) {
    return 1000000.0f; // Very large distance
  }

  return manager->distance_thresholds[lod];
}

static void lod_manager_update_chunk_lod(LODManager *manager,
                                         TerrainChunk *chunk,
                                         const float *camera_pos) {
  if (!manager || !chunk || !camera_pos) {
    return;
  }

  // Calculate chunk center position
  float chunk_center_x =
      (chunk->chunk_x + 0.5f) * g_terrain_state.config.chunk_size;
  float chunk_center_z =
      (chunk->chunk_z + 0.5f) * g_terrain_state.config.chunk_size;

  // Calculate distance to camera
  float dx = chunk_center_x - camera_pos[0];
  float dz = chunk_center_z - camera_pos[2];
  float distance = sqrtf(dx * dx + dz * dz);

  // Calculate appropriate LOD
  uint32_t new_lod = lod_manager_calculate_lod(manager, distance);

  // Update chunk LOD with smooth transition
  if (chunk->current_lod != new_lod) {
    chunk->current_lod = new_lod;
    chunk->needs_update = true;

    // Update LOD statistics
    if (chunk->current_lod < 8) {
      manager->chunk_counts[chunk->current_lod]--;
    }
    manager->chunk_counts[new_lod]++;
  }
}

/**
 * 64-bit coordinate system implementation
 */
static WorldCoord64 float_to_world_coord_64bit(float x, float y, float z) {
  WorldCoord64 coord;
  coord.x = (int64_t)(x * g_terrain_state.coordinate_scale) +
            g_terrain_state.world_origin.x;
  coord.y = (int64_t)(y * g_terrain_state.coordinate_scale) +
            g_terrain_state.world_origin.y;
  coord.z = (int64_t)(z * g_terrain_state.coordinate_scale) +
            g_terrain_state.world_origin.z;
  return coord;
}

static void world_coord_64bit_to_float(WorldCoord64 coord, float *out_x,
                                       float *out_y, float *out_z) {
  if (out_x)
    *out_x = (float)((coord.x - g_terrain_state.world_origin.x) /
                     g_terrain_state.coordinate_scale);
  if (out_y)
    *out_y = (float)((coord.y - g_terrain_state.world_origin.y) /
                     g_terrain_state.coordinate_scale);
  if (out_z)
    *out_z = (float)((coord.z - g_terrain_state.world_origin.z) /
                     g_terrain_state.coordinate_scale);
}

static bool is_coordinate_in_world_bounds(WorldCoord64 coord) {
  return coord.x >= g_terrain_state.config.world_min.x &&
         coord.x <= g_terrain_state.config.world_max.x &&
         coord.y >= g_terrain_state.config.world_min.y &&
         coord.y <= g_terrain_state.config.world_max.y &&
         coord.z >= g_terrain_state.config.world_min.z &&
         coord.z <= g_terrain_state.config.world_max.z;
}

/**
 * Advanced terrain update with LOD management
 */
void terrain_system_update_advanced(TerrainSystem *system, float dt) {
  if (!system || !g_terrain_state.is_initialized) {
    return;
  }

  uint64_t start_time = get_performance_counter();

  // Reset frame statistics
  g_terrain_state.chunks_loaded_this_frame = 0;
  g_terrain_state.chunks_unloaded_this_frame = 0;

  // Update LOD for all loaded chunks
  for (uint32_t i = 0; i < system->chunk_count; i++) {
    if (system->chunks[i].is_loaded) {
      lod_manager_update_chunk_lod(&g_terrain_state.lod_manager,
                                   &system->chunks[i],
                                   system->streaming.camera_position);
    }
  }

  // Process streaming (load/unload chunks based on distance and LOD)
  terrain_system_process_streaming_advanced(system);

  // Update performance metrics
  uint64_t end_time = get_performance_counter();
  g_terrain_state.last_update_time_ms =
      (end_time - start_time) * 1000.0 / get_performance_frequency();

  // Log performance if update takes too long
  if (g_terrain_state.last_update_time_ms >
      16.67f) { // > 16.67ms = longer than 60fps frame
    printf("Warning: Terrain update took %.2f ms (%.1f fps)\n",
           g_terrain_state.last_update_time_ms,
           1000.0f / g_terrain_state.last_update_time_ms);
  }
}

/**
 * Get terrain performance statistics
 */
void terrain_get_performance_stats(uint32_t *chunks_loaded,
                                   uint32_t *chunks_unloaded,
                                   float *update_time_ms,
                                   uint64_t *total_triangles) {
  if (chunks_loaded)
    *chunks_loaded = g_terrain_state.chunks_loaded_this_frame;
  if (chunks_unloaded)
    *chunks_unloaded = g_terrain_state.chunks_unloaded_this_frame;
  if (update_time_ms)
    *update_time_ms = g_terrain_state.last_update_time_ms;
  if (total_triangles)
    *total_triangles = g_terrain_state.total_triangles_rendered;
}

/**
 * Get chunk pool statistics
 */
void terrain_get_chunk_pool_stats(uint32_t *pool_size, uint32_t *used_chunks,
                                  uint32_t *free_chunks, size_t *memory_usage) {
  if (pool_size)
    *pool_size = g_terrain_state.chunk_pool.pool_size;
  if (used_chunks)
    *used_chunks = g_terrain_state.chunk_pool.used_count;
  if (free_chunks)
    *free_chunks = g_terrain_state.chunk_pool.free_count;
  if (memory_usage)
    *memory_usage = chunk_pool_get_memory_usage(&g_terrain_state.chunk_pool);
}

/**
 * Default configuration settings
 */
static void terrain_chunk_config_set_defaults(TerrainChunkConfig *config) {
  if (!config)
    return;

  memset(config, 0, sizeof(TerrainChunkConfig));

  config->chunk_size = 256;
  config->max_chunks = 4096;
  config->max_world_size = 64; // 64x64 chunks = 16km x 16km world
  config->lod_levels = 6;

  // Default LOD distances
  config->lod_distances[0] = 500.0f; // Highest detail
  config->lod_distances[1] = 1000.0f;
  config->lod_distances[2] = 2000.0f;
  config->lod_distances[3] = 4000.0f;
  config->lod_distances[4] = 8000.0f;
  config->lod_distances[5] = 16000.0f; // Lowest detail

  config->enable_tessellation = false;
  config->tessellation_factor = 1.0f;

  config->chunk_pool_size = 1024;
  config->enable_compression = true;
  config->enable_gpu_streaming = false;

  // World boundaries (64-bit coordinates)
  config->world_min.x = INT64_MIN / 2;
  config->world_min.y = INT64_MIN / 2;
  config->world_min.z = INT64_MIN / 2;
  config->world_max.x = INT64_MAX / 2;
  config->world_max.y = INT64_MAX / 2;
  config->world_max.z = INT64_MAX / 2;

  config->max_chunks_per_frame = 4;
  config->streaming_budget_mb = 1024.0f;
  config->load_radius_multiplier = 1.0f;
}

/**
 * Configuration file I/O
 */
static bool terrain_config_load_from_file(const char *config_path) {
  if (!config_path)
    return false;

  FILE *file = fopen(config_path, "r");
  if (!file) {
    return false;
  }

  char buffer[16384];
  size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
  buffer[bytes_read] = '\0';
  fclose(file);

  // Parse JSON configuration (simplified)
  // In a real implementation, use a proper JSON parser

  if (strstr(buffer, "\"chunk_size\":")) {
    sscanf(strstr(buffer, "\"chunk_size\":"), "\"chunk_size\": %u",
           &g_terrain_state.config.chunk_size);
  }

  if (strstr(buffer, "\"max_chunks\":")) {
    sscanf(strstr(buffer, "\"max_chunks\":"), "\"max_chunks\": %u",
           &g_terrain_state.config.max_chunks);
  }

  printf("Loaded terrain configuration from %s\n", config_path);
  return true;
}

static bool terrain_config_save_to_file(const char *config_path) {
  if (!config_path)
    return false;

  char config_json[8192];
  if (!terrain_config_serialize(&g_terrain_state.config, config_json,
                                sizeof(config_json))) {
    return false;
  }

  FILE *file = fopen(config_path, "w");
  if (!file) {
    return false;
  }

  fprintf(file, "%s", config_json);
  fclose(file);

  printf("Saved terrain configuration to %s\n", config_path);
  return true;
}

bool terrain_config_serialize(const TerrainChunkConfig *config, char *buffer,
                              size_t buffer_size) {
  if (!config || !buffer || buffer_size == 0) {
    return false;
  }

  size_t offset = 0;
  offset +=
      snprintf(buffer + offset, buffer_size - offset,
               "{\n"
               "  \"version\": %d,\n"
               "  \"chunk_size\": %u,\n"
               "  \"max_chunks\": %u,\n"
               "  \"max_world_size\": %u,\n"
               "  \"lod_levels\": %u,\n"
               "  \"lod_distances\": [",
               TERRAIN_CONFIG_VERSION, config->chunk_size, config->max_chunks,
               config->max_world_size, config->lod_levels);

  for (uint32_t i = 0; i < config->lod_levels; i++) {
    offset += snprintf(buffer + offset, buffer_size - offset, "%.1f%s",
                       config->lod_distances[i],
                       (i < config->lod_levels - 1) ? ", " : "");
  }

  offset += snprintf(
      buffer + offset, buffer_size - offset,
      "],\n"
      "  \"chunk_pool_size\": %u,\n"
      "  \"enable_compression\": %s,\n"
      "  \"enable_gpu_streaming\": %s,\n"
      "  \"world_min\": [%lld, %lld, %lld],\n"
      "  \"world_max\": [%lld, %lld, %lld],\n"
      "  \"max_chunks_per_frame\": %u,\n"
      "  \"streaming_budget_mb\": %.1f,\n"
      "  \"load_radius_multiplier\": %.2f\n"
      "}\n",
      config->chunk_pool_size, config->enable_compression ? "true" : "false",
      config->enable_gpu_streaming ? "true" : "false",
      (long long)config->world_min.x, (long long)config->world_min.y,
      (long long)config->world_min.z, (long long)config->world_max.x,
      (long long)config->world_max.y, (long long)config->world_max.z,
      config->max_chunks_per_frame, config->streaming_budget_mb,
      config->load_radius_multiplier);

  return offset < buffer_size;
}

// Additional helper functions would be implemented here:
// - terrain_system_process_streaming_advanced()
// - terrain_chunk_unload_advanced()
// - generate_chunk_heightmap_64bit()
// - Various JSON parsing functions for configuration
// - Performance monitoring and profiling functions
//
// These would complete the enterprise-grade terrain system implementation.