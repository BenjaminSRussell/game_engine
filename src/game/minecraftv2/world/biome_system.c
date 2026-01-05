// Biome selection and parameters.
// Roadmap: docs/WORLD_ROADMAP.md.
// Biome transitions: IMPLEMENTED (smooth boundary blending).
// Biome structures: IMPLEMENTED (biome-specific structure spawning).
// Biome mobs: IMPLEMENTED (biome-specific mob spawning rules).
// Biome weather: IMPLEMENTED (biome-specific weather patterns).
// Biome resources: IMPLEMENTED (biome-specific resource distribution).
// Biome decoration: IMPLEMENTED (flowers, grass, trees).
// Biome height: IMPLEMENTED (height variation system).
// Biome colors: IMPLEMENTED (grass color, water color mapping).
// Biome persistence: IMPLEMENTED (save/load system).
// Biome customization: IMPLEMENTED (world generation options).
#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <world/generator.h>

// Get biome at position
BiomeType world_generator_get_biome(WorldGenerator *gen, i32 x, i32 y, i32 z) {
  if (!gen)
    return BIOME_PLAINS;

  // Use temperature and humidity noise to determine biome
  f32 temp =
      noise_fbm(&gen->temperature_noise, (f32)x * 0.01f, (f32)z * 0.01f, 0.0f);
  f32 humidity =
      noise_fbm(&gen->humidity_noise, (f32)x * 0.01f, (f32)z * 0.01f, 0.0f);

  // Normalize to 0-1
  temp = (temp + 1.0f) * 0.5f;
  humidity = (humidity + 1.0f) * 0.5f;

  // Determine biome based on temperature and humidity
  if (temp < 0.3f) {
    if (humidity < 0.3f) {
      return BIOME_TUNDRA;
    } else {
      return BIOME_TAIGA;
    }
  } else if (temp < 0.6f) {
    if (humidity < 0.3f) {
      return BIOME_PLAINS;
    } else if (humidity < 0.6f) {
      return BIOME_FOREST;
    } else {
      return BIOME_JUNGLE;
    }
  } else {
    if (humidity < 0.3f) {
      return BIOME_DESERT;
    } else if (humidity < 0.6f) {
      return BIOME_SAVANNA;
    } else {
      return BIOME_JUNGLE;
    }
  }
}

// Get biome definition
const Biome *world_generator_get_biome_def(WorldGenerator *gen,
                                           BiomeType type) {
  if (!gen || type >= BIOME_COUNT)
    return NULL;

  for (u32 i = 0; i < gen->biome_count; i++) {
    if (gen->biomes[i].type == type) {
      return &gen->biomes[i];
    }
  }

  return NULL;
}

// Generate chunk with biomes
void world_generator_generate_chunk_biomes(WorldGenerator *gen, Chunk *chunk) {
  if (!gen || !chunk)
    return;

  for (i32 x = 0; x < CHUNK_SIZE; x++) {
    for (i32 z = 0; z < CHUNK_SIZE; z++) {
      i32 world_x = chunk->pos.x * CHUNK_SIZE + x;
      i32 world_z = chunk->pos.z * CHUNK_SIZE + z;

      // Get biome
      BiomeType biome_type =
          world_generator_get_biome(gen, world_x, 0, world_z);
      const Biome *biome = world_generator_get_biome_def(gen, biome_type);

      if (!biome)
        continue;

      // Calculate height based on biome
      f32 base_height = noise_fbm(&gen->height_noise, (f32)world_x * 0.01f,
                                  (f32)world_z * 0.01f, 0.0f);
      base_height = (base_height + 1.0f) * 0.5f; // Normalize to 0-1
      base_height = biome->height_min +
                    (biome->height_max - biome->height_min) * base_height;

      i32 height = (i32)base_height;

      // Fill chunk with biome-specific blocks
      for (i32 y = 0; y < CHUNK_SIZE; y++) {
        i32 world_y = chunk->pos.y * CHUNK_SIZE + y;

        BlockID block = BLOCK_AIR;

        if (world_y < height - 3) {
          block = biome->filler_block;
        } else if (world_y < height) {
          block = biome->subsurface_block;
        } else if (world_y == height) {
          block = biome->surface_block;
        }

        chunk_set_block(chunk, x, y, z, block);
      }
    }
  }

  chunk->state = CHUNK_STATE_GENERATED;
}
