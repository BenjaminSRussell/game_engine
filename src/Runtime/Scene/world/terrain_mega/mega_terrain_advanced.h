/**
 * =================================================================================================
 *                              MEGA TERRAIN SYSTEM - ADVANCED HEADER
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Enterprise-grade terrain system with 64-bit coordinates, configurable chunk sizes,
 * large world support, chunk pooling, LOD management, and comprehensive configuration.
 *
 * =================================================================================================
 */

#ifndef MEGA_TERRAIN_ADVANCED_H
#define MEGA_TERRAIN_ADVANCED_H

#include "mega_terrain.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =================================================================================================
 *                                    CONFIGURATION SYSTEM
 * =================================================================================================
 */

/**
 * Terrain configuration validation result
 */
typedef struct TerrainConfigValidation {
    bool is_valid;
    char error_message[256];
    uint32_t warning_count;
    char warnings[10][128];
} TerrainConfigValidation;

/**
 * Advanced terrain configuration with 64-bit support
 */
typedef struct TerrainAdvancedConfig {
    // Basic settings
    uint32_t chunk_size;                    // Configurable chunk size (32-1024)
    uint32_t max_chunks;                    // Maximum loaded chunks (256-1000000)
    uint32_t max_world_size;                // World size in chunks (64-65536)
    uint32_t lod_levels;                    // Number of LOD levels (1-8)
    float lod_distances[8];                 // Distance thresholds for each LOD
    
    // Memory management
    uint32_t chunk_pool_size;               // Pre-allocated chunk pool size
    bool enable_compression;                // Enable heightmap compression
    bool enable_gpu_streaming;              // Enable GPU-accelerated streaming
    size_t max_memory_usage;                // Maximum memory usage in bytes
    
    // 64-bit world coordinates
    int64_t world_origin_x;                 // World origin X coordinate
    int64_t world_origin_y;                 // World origin Y coordinate
    int64_t world_origin_z;                 // World origin Z coordinate
    int64_t world_min_x;                    // Minimum world X boundary
    int64_t world_min_y;                    // Minimum world Y boundary
    int64_t world_min_z;                    // Minimum world Z boundary
    int64_t world_max_x;                    // Maximum world X boundary
    int64_t world_max_y;                    // Maximum world Y boundary
    int64_t world_max_z;                    // Maximum world Z boundary
    
    // Performance settings
    uint32_t max_chunks_per_frame;          // Max chunks to process per frame
    float streaming_budget_mb;              // Memory budget for streaming
    float load_radius_multiplier;           // Multiplier for load/unload radii
    float streaming_update_interval;        // Update interval in seconds
    
    // Quality settings
    bool enable_tessellation;               // Enable GPU tessellation
    float tessellation_factor;              // Tessellation quality factor
    bool enable_normal_mapping;             // Enable normal mapping
    bool enable_parallax_mapping;           // Enable parallax occlusion mapping
    
    // Advanced features
    bool enable_64bit_coordinates;          // Enable 64-bit coordinate system
    bool enable_chunk_pooling;              // Enable chunk allocation pooling
    bool enable_lod_management;             // Enable level-of-detail system
    bool enable_async_loading;              // Enable asynchronous chunk loading
    bool enable_compression;                // Enable data compression
    bool enable_procedural_generation;      // Enable procedural terrain generation
} TerrainAdvancedConfig;

/**
 * Initialize advanced terrain system with configuration support
 * This is the main initialization function that replaces the basic terrain_system_init
 */
bool terrain_system_init_advanced(TerrainSystem* system, const char* config_file_path);

/**
 * Shutdown advanced terrain system
 */
void terrain_system_shutdown_advanced(TerrainSystem* system);

/**
 * Make chunk size configurable at runtime
 * Can only be changed when no chunks are loaded
 */
bool terrain_set_chunk_size(uint32_t new_chunk_size);

/**
 * Add support for larger worlds by expanding coordinate system
 * Enables 64-bit coordinates and increases world size limits
 */
bool terrain_enable_large_worlds(uint32_t max_world_size_chunks);

/**
 * Implement 64-bit world coordinates
 * Sets world origin and enables 64-bit coordinate precision
 */
bool terrain_set_world_coordinates_64bit(int64_t origin_x, int64_t origin_y, int64_t origin_z);

/**
 * Validate terrain configuration
 * Returns validation result with detailed error messages and warnings
 */
TerrainConfigValidation terrain_config_validate(const TerrainAdvancedConfig* config);

/**
 * Serialize terrain configuration to JSON
 */
bool terrain_config_serialize(const TerrainAdvancedConfig* config, char* buffer, size_t buffer_size);

/**
 * Deserialize terrain configuration from JSON
 */
bool terrain_config_deserialize(TerrainAdvancedConfig* config, const char* json_data);

/**
 * Save terrain configuration to file
 */
bool terrain_config_save_to_file(const TerrainAdvancedConfig* config, const char* file_path);

/**
 * Load terrain configuration from file
 */
bool terrain_config_load_from_file(TerrainAdvancedConfig* config, const char* file_path);

/* =================================================================================================
 *                                    CHUNK ALLOCATION POOLING
 * =================================================================================================
 */

/**
 * Chunk allocation pool statistics
 */
typedef struct ChunkPoolStats {
    uint32_t pool_size;                     // Total pool capacity
    uint32_t used_chunks;                   // Currently allocated chunks
    uint32_t free_chunks;                   // Available for allocation
    size_t total_memory_used;               // Total memory usage in bytes
    size_t peak_memory_used;                // Peak memory usage in bytes
    uint32_t allocation_count;              // Total allocations performed
    uint32_t deallocation_count;            // Total deallocations performed
    float memory_efficiency;                // Memory usage efficiency (0-1)
    uint32_t fragmentation_count;           // Number of fragmented allocations
} ChunkPoolStats;

/**
 * Get current chunk pool statistics
 */
void terrain_get_chunk_pool_stats(ChunkPoolStats* stats);

/**
 * Resize chunk allocation pool
 * Can only be done when no chunks are allocated
 */
bool terrain_resize_chunk_pool(uint32_t new_pool_size);

/**
 * Optimize chunk pool memory usage
 * Defragments and compacts memory allocation
 */
bool terrain_optimize_chunk_pool(void);

/**
 * Get detailed chunk pool memory map
 */
typedef struct ChunkMemoryMap {
    uint32_t total_chunks;
    uint32_t allocated_chunks;
    uint32_t free_chunks;
    uint32_t fragmented_chunks;
    size_t largest_free_block;
    size_t smallest_free_block;
    float average_fragmentation;
} ChunkMemoryMap;

void terrain_get_chunk_memory_map(ChunkMemoryMap* map);

/* =================================================================================================
 *                                    LEVEL-OF-DETAIL MANAGEMENT
 * =================================================================================================
 */

/**
 * LOD system configuration
 */
typedef struct LODConfig {
    uint32_t lod_levels;                    // Number of LOD levels (1-8)
    float distance_thresholds[8];           // Distance thresholds for each LOD
    float transition_speed;                 // LOD transition speed
    bool enable_smooth_transitions;         // Enable smooth LOD transitions
    float hysteresis_factor;                // Hysteresis to prevent LOD flickering
    uint32_t max_lod_changes_per_frame;     // Limit LOD changes per frame
} LODConfig;

/**
 * LOD system statistics
 */
typedef struct LODStats {
    uint32_t chunks_per_lod[8];             // Number of chunks at each LOD level
    uint32_t total_lod_changes;             // Total LOD changes this frame
    float average_lod_distance;             // Average distance to chunks
    uint32_t highest_lod_used;              // Highest LOD level currently in use
    uint32_t lowest_lod_used;               // Lowest LOD level currently in use
    float lod_distribution[8];              // Percentage distribution across LOD levels
} LODStats;

/**
 * Configure LOD system
 */
bool terrain_configure_lod(const LODConfig* config);

/**
 * Get current LOD configuration
 */
void terrain_get_lod_config(LODConfig* config);

/**
 * Get LOD system statistics
 */
void terrain_get_lod_stats(LODStats* stats);

/**
 * Force LOD level for specific chunk
 */
bool terrain_force_chunk_lod(int32_t chunk_x, int32_t chunk_z, uint32_t lod_level);

/**
 * Reset forced LOD levels
 */
void terrain_reset_forced_lods(void);

/**
 * Calculate optimal LOD for distance
 */
uint32_t terrain_calculate_optimal_lod(float distance);

/* =================================================================================================
 *                                    64-BIT COORDINATE SYSTEM
 * =================================================================================================
 */

/**
 * 64-bit world coordinate
 */
typedef struct WorldCoord64 {
    int64_t x, y, z;
} WorldCoord64;

/**
 * Convert float coordinates to 64-bit world coordinates
 */
WorldCoord64 terrain_float_to_world_coord_64bit(float x, float y, float z);

/**
 * Convert 64-bit world coordinates to float coordinates
 */
void terrain_world_coord_64bit_to_float(WorldCoord64 coord, float* out_x, float* out_y, float* out_z);

/**
 * Check if 64-bit coordinate is within world bounds
 */
bool terrain_is_coordinate_in_world_bounds_64bit(WorldCoord64 coord);

/**
 * Get world boundaries in 64-bit coordinates
 */
void terrain_get_world_bounds_64bit(WorldCoord64* out_min, WorldCoord64* out_max);

/**
 * Set world origin for 64-bit coordinate system
 */
void terrain_set_world_origin_64bit(int64_t origin_x, int64_t origin_y, int64_t origin_z);

/**
 * Get world origin for 64-bit coordinate system
 */
WorldCoord64 terrain_get_world_origin_64bit(void);

/**
 * Calculate chunk coordinates from 64-bit world coordinates
 */
void terrain_get_chunk_from_world_coord_64bit(WorldCoord64 world_coord, int64_t* out_chunk_x, int64_t* out_chunk_z);

/**
 * Get world coordinate of chunk center
 */
WorldCoord64 terrain_get_chunk_center_64bit(int64_t chunk_x, int64_t chunk_z);

/* =================================================================================================
 *                                    LARGE WORLD SUPPORT
 * =================================================================================================
 */

/**
 * Large world configuration
 */
typedef struct LargeWorldConfig {
    uint32_t world_size_chunks;             // World size in chunks
    bool enable_infinite_worlds;            // Enable infinite world generation
    bool enable_world_streaming;            // Enable world data streaming
    float streaming_radius;                 // Radius for active streaming
    float preload_radius;                   // Radius for preloading
    uint32_t max_active_chunks;             // Maximum simultaneously active chunks
    size_t streaming_buffer_size;           // Buffer size for streaming data
} LargeWorldConfig;

/**
 * Configure large world support
 */
bool terrain_configure_large_world(const LargeWorldConfig* config);

/**
 * Get large world configuration
 */
void terrain_get_large_world_config(LargeWorldConfig* config);

/**
 * Enable infinite world generation
 */
bool terrain_enable_infinite_worlds(bool enable);

/**
 * Set world generation seed for infinite worlds
 */
bool terrain_set_world_seed(uint64_t seed);

/**
 * Get current world generation seed
 */
uint64_t terrain_get_world_seed(void);

/**
 * Force load specific world region
 */
bool terrain_force_load_region(int64_t min_chunk_x, int64_t min_chunk_z, 
                              int64_t max_chunk_x, int64_t max_chunk_z);

/* =================================================================================================
 *                                    PERFORMANCE MONITORING
 * =================================================================================================
 */

/**
 * Terrain system performance metrics
 */
typedef struct TerrainPerformanceMetrics {
    // Timing metrics
    float update_time_ms;                   // Terrain update time per frame
    float streaming_time_ms;                // Chunk streaming time
    float lod_update_time_ms;               // LOD calculation time
    float mesh_generation_time_ms;          // Mesh generation time
    
    // Memory metrics
    size_t total_memory_used;               // Total memory usage
    size_t peak_memory_used;                // Peak memory usage
    size_t texture_memory_used;             // Texture memory usage
    size_t mesh_memory_used;                // Mesh memory usage
    
    // Chunk metrics
    uint32_t total_chunks;                  // Total chunks in world
    uint32_t loaded_chunks;                 // Currently loaded chunks
    uint32_t visible_chunks;                // Currently visible chunks
    uint32_t chunks_loaded_this_frame;      // Chunks loaded this frame
    uint32_t chunks_unloaded_this_frame;    // Chunks unloaded this frame
    
    // LOD metrics
    uint32_t lod_changes_this_frame;        // LOD changes this frame
    uint32_t chunks_per_lod[8];             // Distribution across LOD levels
    
    // Triangle and draw call metrics
    uint64_t total_triangles_rendered;      // Total triangles rendered
    uint32_t draw_calls_per_frame;          // Draw calls per frame
    uint32_t texture_uploads_per_frame;     // Texture uploads per frame
} TerrainPerformanceMetrics;

/**
 * Get comprehensive terrain performance metrics
 */
void terrain_get_performance_metrics(TerrainPerformanceMetrics* metrics);

/**
 * Reset performance metrics
 */
void terrain_reset_performance_metrics(void);

/**
 * Enable/disable performance monitoring
 */
void terrain_set_performance_monitoring(bool enabled);

/**
 * Get terrain system memory breakdown
 */
typedef struct TerrainMemoryBreakdown {
    size_t chunk_pool_memory;               // Chunk pool memory
    size_t heightmap_memory;                // Heightmap data memory
    size_t mesh_memory;                     // Mesh data memory
    size_t texture_memory;                  // Texture memory
    size_t streaming_buffer_memory;         // Streaming buffer memory
    size_t lod_data_memory;                 // LOD system memory
    size_t overhead_memory;                 // System overhead memory
} TerrainMemoryBreakdown;

void terrain_get_memory_breakdown(TerrainMemoryBreakdown* breakdown);

/* =================================================================================================
 *                                    ERROR HANDLING AND LOGGING
 * =================================================================================================
 */

/**
 * Terrain system error codes
 */
typedef enum TerrainError {
    TERRAIN_ERROR_NONE = 0,
    TERRAIN_ERROR_INVALID_PARAMETER,
    TERRAIN_ERROR_OUT_OF_MEMORY,
    TERRAIN_ERROR_INVALID_CONFIGURATION,
    TERRAIN_ERROR_FILE_IO_FAILURE,
    TERRAIN_ERROR_VERSION_MISMATCH,
    TERRAIN_ERROR_CORRUPTED_DATA,
    TERRAIN_ERROR_GPU_RESOURCE_FAILURE,
    TERRAIN_ERROR_MESH_GENERATION_FAILURE,
    TERRAIN_ERROR_LOD_CALCULATION_FAILURE,
    TERRAIN_ERROR_CHUNK_POOL_EXHAUSTED,
    TERRAIN_ERROR_WORLD_BOUNDARY_EXCEEDED,
    TERRAIN_ERROR_64BIT_COORDINATE_OVERFLOW
} TerrainError;

/**
 * Get last terrain system error
 */
TerrainError terrain_get_last_error(void);

/**
 * Get error description string
 */
const char* terrain_get_error_string(TerrainError error);

/**
 * Set terrain system logging level
 */
void terrain_set_log_level(int level); // 0=off, 1=errors, 2=warnings, 3=info, 4=debug

/**
 * Terrain system logging callback
 */
typedef void (*TerrainLogCallback)(int level, const char* message);
void terrain_set_log_callback(TerrainLogCallback callback);

/* =================================================================================================
 *                                    UTILITY FUNCTIONS
 * =================================================================================================
 */

/**
 * Get terrain system version information
 */
void terrain_get_version(int* major, int* minor, int* patch);

/**
 * Check if advanced terrain system is properly initialized
 */
bool terrain_is_advanced_initialized(void);

/**
 * Get terrain system memory usage
 */
size_t terrain_get_memory_usage(void);

/**
 * Force garbage collection of terrain resources
 */
void terrain_garbage_collect(void);

/**
 * Get terrain system capabilities
 */
typedef struct TerrainCapabilities {
    bool supports_64bit_coordinates;        // 64-bit coordinate support
    bool supports_gpu_tessellation;         // GPU tessellation support
    bool supports_compute_shaders;          // Compute shader support
    bool supports_virtual_texturing;        // Virtual texturing support
    bool supports_sparse_textures;          // Sparse texture support
    uint32_t max_texture_size;              // Maximum texture size
    uint64_t max_gpu_memory;                // Maximum GPU memory
} TerrainCapabilities;

void terrain_get_capabilities(TerrainCapabilities* caps);

/**
 * Calculate terrain statistics for a region
 */
typedef struct TerrainRegionStats {
    float average_height;                   // Average height in region
    float height_variance;                  // Height variance in region
    uint32_t biome_count;                   // Number of different biomes
    float slope_average;                    // Average slope in region
    float roughness;                        // Terrain roughness metric
} TerrainRegionStats;

bool terrain_calculate_region_stats(int64_t min_x, int64_t min_z, int64_t max_x, int64_t max_z,
                                   TerrainRegionStats* stats);

/**
 * Export terrain data to various formats
 */
bool terrain_export_heightmap(const char* file_path, int64_t min_x, int64_t min_z, 
                             int64_t max_x, int64_t max_z, const char* format);

bool terrain_export_mesh(const char* file_path, int64_t chunk_x, int64_t chunk_z, const char* format);

bool terrain_export_normal_map(const char* file_path, int64_t chunk_x, int64_t chunk_z);

/**
 * Import terrain data from various formats
 */
bool terrain_import_heightmap(const char* file_path, int64_t offset_x, int64_t offset_z);

bool terrain_import_mesh(const char* file_path, int64_t chunk_x, int64_t chunk_z);

/* =================================================================================================
 *                                    ADVANCED UPDATE FUNCTION
 * =================================================================================================
 */

/**
 * Advanced terrain system update with all features
 * Replaces the basic terrain_system_update function
 */
void terrain_system_update_advanced(TerrainSystem* system, float dt);

#ifdef __cplusplus
}
#endif

#endif // MEGA_TERRAIN_ADVANCED_H