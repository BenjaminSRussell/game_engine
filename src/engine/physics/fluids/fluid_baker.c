// physics/fluids/fluid_baker.c - Fluid simulation prebaking system
#include <core/logger.h>
#include <core/memory.h>
#include <physics/fluid_baker.h>
#include <physics/ocean_simulator.h>
#include <physics/sph_solver.h>
#include <stdio.h>
#include <string.h>

// Magic number for cache files
#define FLUID_CACHE_MAGIC "FLDC"
#define FLUID_CACHE_VERSION 1

// Global baking state
static struct {
  bool is_baking;
  float progress;
  FluidBakeConfig config;
} g_baker = {0};

// ============================================================================
// Compression (Placeholder - would integrate real compression libraries)
// ============================================================================

uint32_t fluid_compress_data(const void *input, uint32_t input_size,
                             void *output, uint32_t output_size,
                             FluidCompressionType type) {
  // Placeholder: just copy data (no compression)
  // In production, would use LZ4, ZSTD, or BLOSC
  if (input_size > output_size) {
    return 0;
  }

  memcpy(output, input, input_size);
  return input_size;
}

uint32_t fluid_decompress_data(const void *input, uint32_t input_size,
                               void *output, uint32_t output_size,
                               FluidCompressionType type) {
  // Placeholder: just copy data
  if (input_size > output_size) {
    return 0;
  }

  memcpy(output, input, input_size);
  return input_size;
}

// ============================================================================
// Baking Functions
// ============================================================================

bool fluid_bake_frame(const FluidBakeConfig *config, float time) {
  if (!config || !config->simulator) {
    LOG_ERROR("Invalid bake configuration");
    return false;
  }

  // Update simulator to this time
  // (Implementation depends on simulator type)

  return true;
}

bool fluid_bake_all(const FluidBakeConfig *config) {
  if (!config || !config->simulator) {
    LOG_ERROR("Invalid bake configuration");
    return false;
  }

  g_baker.is_baking = true;
  g_baker.progress = 0.0f;
  g_baker.config = *config;

  // Open output file
  FILE *file = fopen(config->output_path, "wb");
  if (!file) {
    LOG_ERROR("Failed to open output file: %s", config->output_path);
    g_baker.is_baking = false;
    return false;
  }

  // Write header
  FluidCacheHeader header = {0};
  memcpy(header.magic, FLUID_CACHE_MAGIC, 4);
  header.version = FLUID_CACHE_VERSION;
  header.type = config->type;
  header.time_step = config->time_step;
  header.compression = config->compression;

  // Calculate frame count
  float duration = config->end_time - config->start_time;
  header.frame_count = (uint32_t)(duration / config->time_step) + 1;

  // Type-specific data
  switch (config->type) {
  case FLUID_TYPE_SPH: {
    // SPHFluidSystem *sph = (SPHFluidSystem *)config->simulator;
    // header.data.sph.max_particles = sph->particle_count;
    header.data.sph.max_particles = 10000; // Placeholder
    break;
  }

  case FLUID_TYPE_OCEAN: {
    OceanSimulator *ocean = (OceanSimulator *)config->simulator;
    header.data.ocean.resolution = ocean->config.resolution;
    header.data.ocean.patch_size = ocean->config.patch_size;
    break;
  }

  default:
    break;
  }

  fwrite(&header, sizeof(FluidCacheHeader), 1, file);

  LOG_INFO("Baking fluid simulation: %u frames", header.frame_count);

  // Bake each frame
  for (uint32_t frame = 0; frame < header.frame_count; frame++) {
    float time = config->start_time + frame * config->time_step;

    // Update progress
    g_baker.progress = (float)frame / header.frame_count;
    if (config->progress_callback) {
      config->progress_callback(g_baker.progress, config->user_data);
    }

    // Simulate to this time
    switch (config->type) {
    case FLUID_TYPE_OCEAN: {
      OceanSimulator *ocean = (OceanSimulator *)config->simulator;
      ocean_set_time(ocean, time);

      // Get data
      const float *heights = ocean_get_heights(ocean);
      const Vec3 *normals =
          config->bake_normals ? ocean_get_normals(ocean) : NULL;

      // Calculate data size
      uint32_t res = ocean->config.resolution;
      uint32_t data_size = res * res * sizeof(float);
      if (normals)
        data_size += res * res * sizeof(Vec3);

      // Compress data
      void *compressed = MALLOC(data_size * 2); // Allocate extra space
      uint32_t compressed_size = fluid_compress_data(
          heights, data_size, compressed, data_size * 2, config->compression);

      // Write frame
      FluidCacheFrame frame_data = {.frame_index = frame,
                                    .timestamp = time,
                                    .data_size = data_size,
                                    .compressed_size = compressed_size};

      fwrite(&frame_data, sizeof(FluidCacheFrame), 1, file);
      fwrite(compressed, compressed_size, 1, file);

      FREE(compressed);
      break;
    }

    case FLUID_TYPE_SPH:
      // SPH baking would go here
      LOG_WARN("SPH baking not yet implemented");
      break;

    default:
      break;
    }
  }

  fclose(file);

  g_baker.is_baking = false;
  g_baker.progress = 1.0f;

  LOG_INFO("Baking complete: %s", config->output_path);

  return true;
}

bool fluid_bake_start(const FluidBakeConfig *config) {
  // For async baking, would spawn thread here
  return fluid_bake_all(config);
}

void fluid_bake_cancel(void) { g_baker.is_baking = false; }

float fluid_bake_get_progress(void) { return g_baker.progress; }

// ============================================================================
// Cache Player Functions
// ============================================================================

FluidCachePlayer *fluid_cache_load(const char *path) {
  if (!path) {
    LOG_ERROR("Invalid cache path");
    return NULL;
  }

  FILE *file = fopen(path, "rb");
  if (!file) {
    LOG_ERROR("Failed to open cache file: %s", path);
    return NULL;
  }

  FluidCachePlayer *player = CALLOC(1, sizeof(FluidCachePlayer));
  strncpy(player->path, path, sizeof(player->path) - 1);

  // Read header
  fread(&player->header, sizeof(FluidCacheHeader), 1, file);

  // Validate magic number
  if (memcmp(player->header.magic, FLUID_CACHE_MAGIC, 4) != 0) {
    LOG_ERROR("Invalid cache file magic number");
    fclose(file);
    FREE(player);
    return NULL;
  }

  // Validate version
  if (player->header.version != FLUID_CACHE_VERSION) {
    LOG_ERROR("Unsupported cache version: %u", player->header.version);
    fclose(file);
    FREE(player);
    return NULL;
  }

  // Load all frames (for non-streaming mode)
  if (!player->use_streaming) {
    player->frames =
        CALLOC(player->header.frame_count, sizeof(FluidCacheFrame));
    player->loaded_frame_count = player->header.frame_count;

    for (uint32_t i = 0; i < player->header.frame_count; i++) {
      fread(&player->frames[i], sizeof(FluidCacheFrame), 1, file);

      // Read compressed data
      player->frames[i].data = MALLOC(player->frames[i].compressed_size);
      fread(player->frames[i].data, player->frames[i].compressed_size, 1, file);
    }
  }

  fclose(file);

  player->current_time = 0.0f;
  player->current_frame = 0;
  player->is_playing = false;
  player->loop = false;

  LOG_INFO("Loaded fluid cache: %s (%u frames)", path,
           player->header.frame_count);

  return player;
}

void fluid_cache_free(FluidCachePlayer *player) {
  if (!player)
    return;

  if (player->frames) {
    for (uint32_t i = 0; i < player->loaded_frame_count; i++) {
      FREE(player->frames[i].data);
    }
    FREE(player->frames);
  }

  FREE(player);
}

void fluid_cache_play(FluidCachePlayer *player) {
  if (player)
    player->is_playing = true;
}

void fluid_cache_pause(FluidCachePlayer *player) {
  if (player)
    player->is_playing = false;
}

void fluid_cache_stop(FluidCachePlayer *player) {
  if (!player)
    return;
  player->is_playing = false;
  player->current_time = 0.0f;
  player->current_frame = 0;
}

void fluid_cache_seek(FluidCachePlayer *player, float time) {
  if (!player)
    return;

  player->current_time = time;

  // Find corresponding frame
  player->current_frame = (uint32_t)(time / player->header.time_step);
  if (player->current_frame >= player->header.frame_count) {
    player->current_frame = player->header.frame_count - 1;
  }
}

void fluid_cache_set_loop(FluidCachePlayer *player, bool loop) {
  if (player)
    player->loop = loop;
}

void fluid_cache_update(FluidCachePlayer *player, float dt) {
  if (!player || !player->is_playing)
    return;

  player->current_time += dt;

  // Calculate frame
  uint32_t new_frame =
      (uint32_t)(player->current_time / player->header.time_step);

  if (new_frame >= player->header.frame_count) {
    if (player->loop) {
      player->current_time = 0.0f;
      player->current_frame = 0;
    } else {
      player->is_playing = false;
      player->current_frame = player->header.frame_count - 1;
    }
  } else {
    player->current_frame = new_frame;
  }
}

const void *fluid_cache_get_current_data(const FluidCachePlayer *player) {
  if (!player || player->current_frame >= player->loaded_frame_count) {
    return NULL;
  }

  return player->frames[player->current_frame].data;
}

float fluid_cache_get_time(const FluidCachePlayer *player) {
  return player ? player->current_time : 0.0f;
}

uint32_t fluid_cache_get_frame(const FluidCachePlayer *player) {
  return player ? player->current_frame : 0;
}

void fluid_cache_enable_streaming(FluidCachePlayer *player,
                                  uint32_t cache_size) {
  if (!player)
    return;
  player->use_streaming = true;
  player->cache_size = cache_size;
}

void fluid_cache_preload_range(FluidCachePlayer *player, float start_time,
                               float end_time) {
  // Would implement frame preloading here
  (void)player;
  (void)start_time;
  (void)end_time;
}

// Utility functions
bool fluid_cache_get_info(const char *path, FluidCacheHeader *header) {
  if (!path || !header)
    return false;

  FILE *file = fopen(path, "rb");
  if (!file)
    return false;

  fread(header, sizeof(FluidCacheHeader), 1, file);
  fclose(file);

  return memcmp(header->magic, FLUID_CACHE_MAGIC, 4) == 0;
}

bool fluid_cache_validate(const char *path) {
  FluidCacheHeader header;
  return fluid_cache_get_info(path, &header) &&
         header.version == FLUID_CACHE_VERSION;
}
