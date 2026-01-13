// Biome generator implementation
#include "engine/include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <world/biome_generator.h>
#include <world/plant_varieties.h>

// Default noise parameters
static const BiomeNoiseParams DEFAULT_NOISE_PARAMS = {.temperature_scale = 0.5f,
                                                      .temperature_offset =
                                                          20.0f,
                                                      .humidity_scale = 0.3f,
                                                      .humidity_offset = 0.5f,
                                                      .elevation_scale = 0.4f,
                                                      .biome_size = 256.0f,
                                                      .noise_seed = 12345};

// Simple perlin noise-like function (placeholder for actual Perlin
// implementation)
static f32 sample_noise(f32 x, f32 z, u32 seed) {
  // Simple hash-based noise
  // Cast to u32 for bitwise ops
  i32 xi_x = (i32)(x * 73856093);
  i32 xi_z = (i32)(z * 19349663);
  u32 xi = (u32)(xi_x ^ xi_z ^ seed);
  xi = (xi << 13) ^ xi;
  f32 result =
      1.0f - ((xi * (xi * xi * 15731 + 789221) + 1376312589) & 0x7fffffff) /
                 1073741824.0f;
  return (result + 1.0f) * 0.5f; // Normalize to 0-1
}

void biome_generator_init(BiomeGenerator *gen, BiomeFeaturesSystem *features,
                          ChunkManager *chunks) {
  if (!gen)
    return;

  memset(gen, 0, sizeof(BiomeGenerator));

  gen->features = features;
  gen->chunk_manager = chunks;
  gen->noise_params = DEFAULT_NOISE_PARAMS;

  // Pre-allocate biome map (will be sized based on generation area)
  gen->map_width = 256;
  gen->map_height = 256;
  gen->biome_map =
      (u32 *)malloc(gen->map_width * gen->map_height * sizeof(u32));

  if (!gen->biome_map) {
    LOG_ERROR("Failed to allocate biome map");
    return;
  }

  gen->use_noise = true;
  gen->use_smooth_transitions = true;
  gen->initialized = true;

  LOG_INFO("Biome generator initialized");
}

void biome_generator_free(BiomeGenerator *gen) {
  if (!gen)
    return;

  if (gen->biome_map) {
    free(gen->biome_map);
    gen->biome_map = NULL;
  }

  memset(gen, 0, sizeof(BiomeGenerator));
}

f32 biome_generator_sample_temperature_noise(BiomeGenerator *gen, f32 x,
                                             f32 z) {
  if (!gen)
    return 20.0f;

  f32 scale = 1.0f / gen->noise_params.biome_size;
  f32 value = sample_noise(x * scale, z * scale, gen->noise_params.noise_seed);

  return gen->noise_params.temperature_offset +
         value * gen->noise_params.temperature_scale * 20.0f - 10.0f;
}

f32 biome_generator_sample_humidity_noise(BiomeGenerator *gen, f32 x, f32 z) {
  if (!gen)
    return 0.5f;

  f32 scale = 1.0f / gen->noise_params.biome_size;
  f32 value = sample_noise(x * scale + 1000.0f, z * scale,
                           gen->noise_params.noise_seed ^ 0xDEADBEEF);

  return gen->noise_params.humidity_offset +
         value * gen->noise_params.humidity_scale - 0.15f;
}

f32 biome_generator_sample_elevation_noise(BiomeGenerator *gen, f32 x, f32 z) {
  if (!gen)
    return 64.0f;

  f32 scale = 1.0f / gen->noise_params.biome_size;
  f32 value = sample_noise(x * scale + 2000.0f, z * scale,
                           gen->noise_params.noise_seed ^ 0xCAFEBABE);

  return value * gen->noise_params.elevation_scale;
}

BiomeType biome_generator_select_from_climate(f32 temperature, f32 humidity,
                                              f32 elevation) {
  // Clamp values
  if (temperature < -50.0f)
    temperature = -50.0f;
  if (temperature > 50.0f)
    temperature = 50.0f;
  if (humidity < 0.0f)
    humidity = 0.0f;
  if (humidity > 1.0f)
    humidity = 1.0f;

  // Elevation preference
  bool is_high = elevation > 0.6f;
  bool is_low = elevation < 0.3f;

  // Decision tree based on temperature and humidity
  if (temperature < -20.0f) {
    // Arctic regions
    return is_high ? BIOME_ICE_SPIKES : BIOME_TUNDRA;
  }

  if (temperature < 0.0f) {
    // Cold regions
    if (humidity > 0.7f) {
      return is_high ? BIOME_WINDSWEPT_HILLS : BIOME_TAIGA;
    } else {
      return is_high ? BIOME_MOUNTAINS : BIOME_TUNDRA;
    }
  }

  if (temperature < 10.0f) {
    // Cool regions
    if (humidity > 0.8f) {
      return BIOME_DARK_FOREST;
    } else if (humidity > 0.5f) {
      return is_high ? BIOME_BIRCH_FOREST
                     : BIOME_FOREST; // Assuming BIRCH defined or fallback
    } else if (humidity > 0.3f) {
      return BIOME_PLAINS;
    } else {
      return BIOME_WINDSWEPT_HILLS;
    }
  }

  if (temperature < 20.0f) {
    // Temperate regions
    if (humidity > 0.9f) {
      return BIOME_MANGROVE_SWAMP;
    } else if (humidity > 0.7f) {
      return is_high ? BIOME_FLOWER_FOREST : BIOME_SWAMP;
    } else if (humidity > 0.5f) {
      return is_high ? BIOME_CHERRY_GROVE : BIOME_FOREST;
    } else if (humidity > 0.4f) {
      return BIOME_SUNFLOWER_PLAINS;
    } else {
      return BIOME_PLAINS;
    }
  }

  if (temperature < 28.0f) {
    // Warm regions
    if (humidity > 0.85f) {
      return BIOME_BAMBOO_JUNGLE;
    } else if (humidity > 0.6f) {
      return BIOME_JUNGLE;
    } else if (humidity > 0.4f) {
      return BIOME_SAVANNA;
    } else {
      return BIOME_DESERT;
    }
  }

  // Hot regions
  if (humidity > 0.6f) {
    return BIOME_JUNGLE;
  } else if (humidity > 0.3f) {
    return BIOME_BADLANDS;
  } else {
    return BIOME_DESERT;
  }
}

BiomeType biome_generator_select_biome_from_noise(BiomeGenerator *gen,
                                                  f32 temperature, f32 humidity,
                                                  f32 elevation) {
  if (!gen)
    return BIOME_PLAINS;

  return biome_generator_select_from_climate(temperature, humidity, elevation);
}

BiomeChunkData biome_generator_get_biome_for_chunk(BiomeGenerator *gen,
                                                   i32 chunk_x, i32 chunk_y) {
  BiomeChunkData data = {0};

  if (!gen || !gen->initialized) {
    data.biome = BIOME_PLAINS;
    data.temperature = 20.0f;
    data.humidity = 0.5f;
    data.elevation = 0.5f;
    return data;
  }

  // Convert chunk coordinates to world coordinates
  f32 wx = (f32)chunk_x * 16.0f;
  f32 wz = (f32)chunk_y * 16.0f;

  // Sample noise at this location
  data.temperature = biome_generator_sample_temperature_noise(gen, wx, wz);
  data.humidity = biome_generator_sample_humidity_noise(gen, wx, wz);
  data.elevation = biome_generator_sample_elevation_noise(gen, wx, wz);

  // Select primary biome
  data.biome = biome_generator_select_from_climate(
      data.temperature, data.humidity, data.elevation);

  // Sample adjacent biomes for transitions
  if (gen->use_smooth_transitions) {
    BiomeType east_biome = biome_generator_select_from_climate(
        biome_generator_sample_temperature_noise(gen, wx + 16.0f, wz),
        biome_generator_sample_humidity_noise(gen, wx + 16.0f, wz),
        biome_generator_sample_elevation_noise(gen, wx + 16.0f, wz));

    if (east_biome != data.biome) {
      data.transition.primary_biome = data.biome;
      data.transition.secondary_biome = east_biome;
      data.transition.blend_factor = 0.2f; // 20% transition
    }
  }

  return data;
}

void biome_generator_assign_chunk_biome(BiomeGenerator *gen, Chunk *chunk,
                                        i32 chunk_x, i32 chunk_y) {
  if (!gen || !chunk || !gen->initialized)
    return;

  BiomeChunkData biome_data =
      biome_generator_get_biome_for_chunk(gen, chunk_x, chunk_y);

  // Store biome data in chunk (this would require a biome field in Chunk
  // struct) chunk->biome_type = biome_data.biome; chunk->temperature =
  // biome_data.temperature; chunk->humidity = biome_data.humidity;

  LOG_TRACE("Assigned chunk (%d, %d) to biome %u", chunk_x, chunk_y,
            biome_data.biome);
}

void biome_generator_generate_biome_map(BiomeGenerator *gen, i32 center_x,
                                        i32 center_z, u32 width, u32 height) {
  if (!gen || !gen->biome_map)
    return;

  if (width > gen->map_width || height > gen->map_height) {
    LOG_WARN("Biome map size exceeds pre-allocated buffer");
    return;
  }

  for (u32 z = 0; z < height; z++) {
    for (u32 x = 0; x < width; x++) {
      i32 chunk_x = center_x - (i32)width / 2 + (i32)x;
      i32 chunk_z = center_z - (i32)height / 2 + (i32)z;

      BiomeChunkData data =
          biome_generator_get_biome_for_chunk(gen, chunk_x, chunk_z);

      u32 idx = z * width + x;
      gen->biome_map[idx] = (u32)data.biome;
    }
  }

  LOG_INFO("Generated biome map (%u x %u)", width, height);
}

BiomeType biome_generator_sample_biome_map(BiomeGenerator *gen, f32 x, f32 z) {
  if (!gen || !gen->biome_map)
    return BIOME_PLAINS;

  // Convert world coordinates to biome map coordinates
  u32 map_x = (u32)x % gen->map_width;
  u32 map_z = (u32)z % gen->map_height;

  u32 idx = map_z * gen->map_width + map_x;
  return (BiomeType)gen->biome_map[idx];
}

void biome_generator_spawn_plants(BiomeGenerator *gen, Chunk *chunk,
                                  BiomeType biome) {
  if (!gen || !chunk)
    return;

  BiomeProfile *profile = biome_features_get_profile(gen->features, biome);
  if (!profile)
    return;

  // Placeholder for biome-specific plant spawning
  // In a real implementation:
  // 1. Query plant variety requirements
  // 2. Check biome compatibility
  // 3. Spawn plants based on biome rules

  LOG_TRACE("Spawning plants for biome %u", biome);
}

void biome_generator_generate_structures(BiomeGenerator *gen, Chunk *chunk,
                                         BiomeType biome) {
  if (!gen || !chunk)
    return;

  // Placeholder for biome-specific structure generation
  // Different biomes have different structures:
  // - Villages in plains/desert
  // - Temples in jungle
  // - Fortresses in nether
  // - etc.

  LOG_TRACE("Generating structures for biome %u", biome);
}

void biome_generator_apply_terrain_modifiers(BiomeGenerator *gen, Chunk *chunk,
                                             BiomeType biome) {
  if (!gen || !chunk)
    return;

  // Apply biome-specific terrain modifiers
  // - Desert: sand instead of grass
  // - Mountain: steep slopes
  // - Swamp: water channels
  // - etc.

  LOG_TRACE("Applying terrain modifiers for biome %u", biome);
}

void biome_generator_set_noise_params(BiomeGenerator *gen,
                                      const BiomeNoiseParams *params) {
  if (!gen || !params)
    return;
  gen->noise_params = *params;
}

BiomeNoiseParams biome_generator_get_default_noise_params(void) {
  return DEFAULT_NOISE_PARAMS;
}

BiomeGenerationStats biome_generator_get_stats(BiomeGenerator *gen) {
  BiomeGenerationStats stats = {0};

  if (!gen)
    return stats;

  // These would be accumulated during generation
  stats.chunks_processed = 0;
  stats.biomes_assigned = 0;
  stats.transitions_created = 0;
  stats.generation_time = 0.0f;

  return stats;
}
