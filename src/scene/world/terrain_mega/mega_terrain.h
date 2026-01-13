/**
 * =================================================================================================
 *                              WORLD BUILDING - MEGA TERRAIN SYSTEM
 *                                    Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Large-scale terrain system supporting 100km worlds with dynamic streaming,
 * biome blending, and procedural generation.
 *
 * =================================================================================================
 */

#ifndef MEGA_TERRAIN_H
#define MEGA_TERRAIN_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    TERRAIN CONFIGURATION
 * =================================================================================================
 */

#define TERRAIN_CHUNK_SIZE 256     // Meters per chunk
#define TERRAIN_MAX_HEIGHT 8192.0f // Maximum terrain height
#define TERRAIN_LOD_LEVELS 6       // Number of LOD levels
#define TERRAIN_MAX_CHUNKS 4096    // Max loaded chunks

// TODO(AGENT_WORLD_1): Make chunk size configurable [Difficulty: 4]
// TODO(AGENT_WORLD_1): Add support for larger worlds [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement 64-bit world coordinates [Difficulty: 7]

typedef struct TerrainConfig {
  float world_size_x;
  float world_size_z;
  float heightmap_resolution;
  uint32_t chunk_resolution;
  uint32_t max_lod;
  float lod_distances[TERRAIN_LOD_LEVELS];
  bool enable_tessellation;
  float tessellation_factor;
} TerrainConfig;

// TODO(AGENT_WORLD_1): Implement config validation [Difficulty: 3]
// TODO(AGENT_WORLD_1): Implement config serialization [Difficulty: 4]

/* =================================================================================================
 *                                    TERRAIN CHUNK
 * =================================================================================================
 */

typedef struct TerrainChunk {
  int32_t chunk_x;
  int32_t chunk_z;
  float *heightmap;
  uint32_t heightmap_size;
  uint32_t current_lod;
  bool is_loaded;
  bool is_visible;
  bool needs_update;
  float bounds_min[3];
  float bounds_max[3];
  uint32_t mesh_id;
  uint32_t material_id;
  uint32_t neighbor_chunks[4]; // N, E, S, W
} TerrainChunk;

// TODO(AGENT_WORLD_1): Implement chunk allocation pooling [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement chunk LOD management [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement chunk mesh generation [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement chunk stitching at LOD boundaries [Difficulty:
// 7]
// TODO(AGENT_WORLD_1): Implement chunk collision mesh generation [Difficulty:
// 5]
// TODO(AGENT_WORLD_1): Implement chunk normal map generation [Difficulty: 5]

/* =================================================================================================
 *                                    TERRAIN STREAMING
 * =================================================================================================
 */

typedef void (*TerrainStreamingCallback)(uint32_t loading, uint32_t loaded);

typedef struct TerrainStreamingState {
  float camera_position[3];
  float camera_velocity[3];
  float load_radius;
  float unload_radius;
  uint32_t chunks_loading;
  uint32_t chunks_loaded;
  uint32_t max_chunks_per_frame;
  bool is_streaming;
  TerrainStreamingCallback progress_callback;
} TerrainStreamingState;

// TODO(AGENT_WORLD_1): Implement async chunk loading [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement chunk priority queue [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement chunk unloading [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement chunk prefetching [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement streaming budget management [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement streaming progress callback [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement streaming pause/resume [Difficulty: 4]

/* =================================================================================================
 *                                    BIOME SYSTEM
 * =================================================================================================
 */

typedef enum BiomeType {
  BIOME_PLAINS,
  BIOME_FOREST,
  BIOME_DESERT,
  BIOME_SNOW,
  BIOME_JUNGLE,
  BIOME_SWAMP,
  BIOME_MOUNTAIN,
  BIOME_OCEAN,
  BIOME_BEACH,
  BIOME_TUNDRA,
  BIOME_SAVANNA,
  BIOME_VOLCANIC,
  BIOME_COUNT
} BiomeType;

typedef struct BiomeDefinition {
  BiomeType type;
  char name[32];
  float temperature_min, temperature_max;
  float moisture_min, moisture_max;
  float altitude_min, altitude_max;
  uint32_t surface_material;
  uint32_t vegetation_set;
  float tree_density;
  float grass_density;
  float rock_density;
} BiomeDefinition;

// TODO(AGENT_WORLD_1): Implement biome database [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement biome sampling from noise [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement biome blending at boundaries [Difficulty: 7]
// TODO(AGENT_WORLD_1): Implement biome-specific vegetation [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement biome-specific weather [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement biome-specific ambient audio [Difficulty: 5]

/* =================================================================================================
 *                                    EROSION SIMULATION
 * =================================================================================================
 */

typedef struct ErosionSettings {
  uint32_t iterations;
  float rain_rate;
  float sediment_capacity;
  float dissolution_rate;
  float deposition_rate;
  float evaporation_rate;
  float min_slope;
} ErosionSettings;

// TODO(AGENT_WORLD_1): Implement hydraulic erosion simulation [Difficulty: 8]
// TODO(AGENT_WORLD_1): Implement thermal erosion simulation [Difficulty: 7]
// TODO(AGENT_WORLD_1): Implement GPU-accelerated erosion [Difficulty: 9]
void terrain_erosion_preview(struct TerrainSystem *system,
                             ErosionSettings *settings);
void terrain_erosion_undo(struct TerrainSystem *system);

/* =================================================================================================
 *                                    TERRAIN API
 * =================================================================================================
 */

typedef struct TerrainSystem {
  TerrainConfig config;
  TerrainChunk *chunks;
  uint32_t chunk_count;
  TerrainStreamingState streaming;
  BiomeDefinition biomes[BIOME_COUNT];
} TerrainSystem;

// TODO(AGENT_WORLD_1): Implement terrain_system_init [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement terrain_system_shutdown [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement terrain_system_update [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement terrain_get_height_at [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement terrain_get_normal_at [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement terrain_get_biome_at [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement terrain_raycast [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement terrain_sculpt [Difficulty: 7]
// TODO(AGENT_WORLD_1): Implement terrain_paint_material [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement terrain_save [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement terrain_load [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement terrain_export_heightmap [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement terrain_import_heightmap [Difficulty: 4]

#endif // MEGA_TERRAIN_H
