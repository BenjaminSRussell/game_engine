#ifndef FLUID_BAKER_H
#define FLUID_BAKER_H

#include <common.h>
#include <stdbool.h>

// Simulator types
typedef enum {
  FLUID_TYPE_SPH,  // SPH particle simulation
  FLUID_TYPE_GRID, // Eulerian grid (smoke/fire)
  FLUID_TYPE_OCEAN // FFT ocean waves
} FluidSimulatorType;

// Compression types
typedef enum {
  FLUID_COMPRESSION_NONE,
  FLUID_COMPRESSION_LZ4,  // Fast compression
  FLUID_COMPRESSION_ZSTD, // Balanced
  FLUID_COMPRESSION_BLOSC // Scientific data compression
} FluidCompressionType;

// Baking configuration
typedef struct {
  // Simulation
  void *simulator;
  FluidSimulatorType type;

  // Time range
  float start_time;
  float end_time;
  float time_step; // Frame interval

  // Output
  char output_path[256];
  FluidCompressionType compression;
  uint32_t quality; // 1-10 (affects precision)

  // LOD
  bool generate_lod;
  uint32_t lod_levels; // Number of LOD levels

  // Options
  bool bake_normals;
  bool bake_velocities;
  bool bake_colors;

  // Progress callback
  void (*progress_callback)(float progress, void *user_data);
  void *user_data;
} FluidBakeConfig;

// Cache file header
typedef struct {
  char magic[4]; // "FLDC"
  uint32_t version;
  FluidSimulatorType type;
  uint32_t frame_count;
  float time_step;
  FluidCompressionType compression;

  // Type-specific data
  union {
    struct {
      uint32_t max_particles;
    } sph;

    struct {
      uint32_t grid_x, grid_y, grid_z;
    } grid;

    struct {
      uint32_t resolution;
      float patch_size;
    } ocean;
  } data;
} FluidCacheHeader;

// Frame data
typedef struct {
  uint32_t frame_index;
  float timestamp;
  uint32_t data_size;
  uint32_t compressed_size;
  void *data;
} FluidCacheFrame;

// Cache player
typedef struct {
  char path[256];
  FluidCacheHeader header;

  // Loaded frames
  FluidCacheFrame *frames;
  uint32_t loaded_frame_count;

  // Playback state
  float current_time;
  uint32_t current_frame;
  bool is_playing;
  bool loop;

  // Streaming
  bool use_streaming;
  uint32_t cache_size; // Max frames in memory

  // File handle
  void *file_handle;
} FluidCachePlayer;

// ============================================================================
// Baking API
// ============================================================================

// Start baking simulation
bool fluid_bake_start(const FluidBakeConfig *config);

// Bake single frame (for manual control)
bool fluid_bake_frame(const FluidBakeConfig *config, float time);

// Bake entire simulation (blocking)
bool fluid_bake_all(const FluidBakeConfig *config);

// Cancel ongoing bake
void fluid_bake_cancel(void);

// Get baking progress
float fluid_bake_get_progress(void);

// ============================================================================
// Cache Player API
// ============================================================================

// Load cache file
FluidCachePlayer *fluid_cache_load(const char *path);
void fluid_cache_free(FluidCachePlayer *player);

// Playback control
void fluid_cache_play(FluidCachePlayer *player);
void fluid_cache_pause(FluidCachePlayer *player);
void fluid_cache_stop(FluidCachePlayer *player);
void fluid_cache_seek(FluidCachePlayer *player, float time);
void fluid_cache_set_loop(FluidCachePlayer *player, bool loop);

// Update (call each frame)
void fluid_cache_update(FluidCachePlayer *player, float dt);

// Data access
const void *fluid_cache_get_current_data(const FluidCachePlayer *player);
float fluid_cache_get_time(const FluidCachePlayer *player);
uint32_t fluid_cache_get_frame(const FluidCachePlayer *player);

// Streaming control
void fluid_cache_enable_streaming(FluidCachePlayer *player,
                                  uint32_t cache_size);
void fluid_cache_preload_range(FluidCachePlayer *player, float start_time,
                               float end_time);

// ============================================================================
// Utility Functions
// ============================================================================

// Get cache info without loading
bool fluid_cache_get_info(const char *path, FluidCacheHeader *header);

// Validate cache file
bool fluid_cache_validate(const char *path);

// Compress/decompress data
uint32_t fluid_compress_data(const void *input, uint32_t input_size,
                             void *output, uint32_t output_size,
                             FluidCompressionType type);
uint32_t fluid_decompress_data(const void *input, uint32_t input_size,
                               void *output, uint32_t output_size,
                               FluidCompressionType type);

#endif // FLUID_BAKER_H
