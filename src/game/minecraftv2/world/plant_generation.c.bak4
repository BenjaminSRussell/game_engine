// Plant generation with biome-specific placement.
// Roadmap: docs/PLANT_GENERATION_ROADMAP.md.
#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <stdlib.h>
#include <world/generator.h>
#include <world/plant_generation.h>

// Hash function for deterministic plant placement
static u32 plant_hash(i32 x, i32 z, u32 seed) {
  return ((u32)x * 73856093) ^ ((u32)z * 19349663) ^ seed;
}

// Check if block can support a plant
bool plant_can_place_on(BlockID block, PlantType type) {
  switch (type) {
  case PLANT_LILY_PAD:
    return block == BLOCK_WATER;
  case PLANT_CACTUS:
    return block == BLOCK_SAND;
  case PLANT_DEAD_BUSH:
    return block == BLOCK_SAND || block == BLOCK_DIRT;
  default:
    // Most plants grow on grass or dirt
    return block == BLOCK_GRASS || block == BLOCK_DIRT;
  }
}

// Get plant block ID from type and variant
static BlockID plant_get_block_id(PlantType type, u32 variant) {
  switch (type) {
  case PLANT_GRASS_SHORT:
    return BLOCK_SHORT_GRASS;
  case PLANT_GRASS_TALL:
    return BLOCK_TALL_GRASS;
  case PLANT_FLOWER_MIXED:
    // Randomly select flower color based on variant
    switch (variant % 4) {
    case 0:
      return BLOCK_FLOWER_RED;
    case 1:
      return BLOCK_FLOWER_YELLOW;
    case 2:
      return BLOCK_FLOWER_BLUE;
    case 3:
      return BLOCK_FLOWER_WHITE;
    }
    return BLOCK_FLOWER_RED;
  case PLANT_MUSHROOM:
    return (variant % 2 == 0) ? BLOCK_MUSHROOM_BROWN : BLOCK_MUSHROOM_RED;
  case PLANT_DEAD_BUSH:
    return BLOCK_DEAD_BUSH;
  case PLANT_FERN:
    return BLOCK_FERN;
  case PLANT_LILY_PAD:
    return BLOCK_LILY_PAD;
  case PLANT_CACTUS:
    return BLOCK_CACTUS;
  }
  return BLOCK_AIR;
}

// Try to place a plant at the given location
bool plant_try_place(Chunk *chunk, i32 x, i32 y, i32 z, PlantType type,
                     u32 seed) {
  if (!chunk)
    return false;

  // Get current block and block below
  BlockID current = chunk_get_block(chunk, x, y, z);
  if (current != BLOCK_AIR)
    return false;

  BlockID below = chunk_get_block(chunk, x, y - 1, z);
  if (!plant_can_place_on(below, type))
    return false;

  // Place the plant
  BlockID plant_block = plant_get_block_id(type, seed);
  chunk_set_block(chunk, x, y, z, plant_block);

  return true;
}

// Generate plants for a specific biome pattern
static void plant_generate_biome_pattern(WorldGenerator *gen, Chunk *chunk,
                                         BiomeType biome) {
  if (!gen || !chunk)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Use billow noise for natural plant clustering
  NoiseGenerator plant_density_noise = {.type = NOISE_BILLOW,
                                        .seed = gen->seed + 5000,
                                        .frequency = 0.1f,
                                        .amplitude = 1.0f,
                                        .octaves = 2,
                                        .lacunarity = 2.0f,
                                        .persistence = 0.5f};

  // Iterate through chunk columns (not every block)
  for (i32 x = 1; x < CHUNK_SIZE - 1; x++) {
    for (i32 z = 1; z < CHUNK_SIZE - 1; z++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Find surface block
      i32 surface_y = -1;
      for (i32 y = CHUNK_SIZE - 1; y >= 0; y--) {
        BlockID block = chunk_get_block(chunk, x, y, z);
        if (block != BLOCK_AIR && block != BLOCK_WATER) {
          surface_y = y;
          break;
        }
      }

      if (surface_y < 0 || surface_y >= CHUNK_SIZE - 1)
        continue;

      // Get density from noise
      f32 density = noise_billow(&plant_density_noise, (f32)wx, 0.0f, (f32)wz);
      density = (density + 1.0f) * 0.5f; // Normalize to [0, 1]

      // Deterministic random for this position
      u32 hash = plant_hash(wx, wz, gen->seed);
      f32 rand = (hash % 1000) / 1000.0f;

      // Biome-specific plant generation
      BlockID surface_block = chunk_get_block(chunk, x, surface_y, z);

      switch (biome) {
      case BIOME_PLAINS:
        // High grass density, occasional flowers
        if (surface_block == BLOCK_GRASS) {
          if (density > 0.3f && rand < 0.7f) {
            plant_try_place(
                chunk, x, surface_y + 1, z,
                (hash % 3 == 0) ? PLANT_GRASS_TALL : PLANT_GRASS_SHORT, hash);

          } else if (rand < 0.05f) {
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_FLOWER_MIXED,
                            hash);
          }
        }
        break;

      case BIOME_FOREST:
        // Moderate grass, mushrooms in shade, ferns
        if (surface_block == BLOCK_GRASS) {
          if (density > 0.5f && rand < 0.4f) {
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_GRASS_SHORT,
                            hash);
          } else if (density > 0.6f && rand < 0.2f) {
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_FERN, hash);
          } else if (rand < 0.1f) {
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_MUSHROOM, hash);
          }
        }
        break;

      case BIOME_JUNGLE:
        // Very high vegetation density
        if (surface_block == BLOCK_GRASS) {
          if (density > 0.2f && rand < 0.9f) {
            PlantType type = (hash % 3 == 0)   ? PLANT_GRASS_TALL
                             : (hash % 3 == 1) ? PLANT_FERN
                                               : PLANT_FLOWER_MIXED;
            plant_try_place(chunk, x, surface_y + 1, z, type, hash);
          }
        }
        break;

      case BIOME_DESERT:
        // Sparse cacti and dead bushes
        if (surface_block == BLOCK_SAND) {
          if (rand < 0.02f) { // Very rare
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_CACTUS, hash);
          } else if (rand < 0.05f) {
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_DEAD_BUSH, hash);
          }
        }
        break;

      case BIOME_TAIGA:
        // Moderate grass, some ferns
        if (surface_block == BLOCK_GRASS) {
          if (density > 0.4f && rand < 0.3f) {
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_GRASS_SHORT,
                            hash);
          } else if (rand < 0.15f) {
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_FERN, hash);
          }
        }
        break;

      case BIOME_OCEAN:
        // Lily pads on water surface
        if (surface_block == BLOCK_WATER &&
            chunk->pos.y * CHUNK_SIZE + surface_y >= 0) {
          if (rand < 0.05f) {
            plant_try_place(chunk, x, surface_y + 1, z, PLANT_LILY_PAD, hash);
          }
        }
        break;

      default:
        break;
      }
    }
  }
}

// Main entry point: generate plants for entire chunk
void plant_generate_in_chunk(WorldGenerator *gen, Chunk *chunk) {
  if (!gen || !chunk)
    return;

  // Sample biome at chunk center
  i32 cx = chunk->pos.x * CHUNK_SIZE + CHUNK_SIZE / 2;
  i32 cy = chunk->pos.y * CHUNK_SIZE + CHUNK_SIZE / 2;
  i32 cz = chunk->pos.z * CHUNK_SIZE + CHUNK_SIZE / 2;

  BiomeType biome = world_generator_get_biome(gen, cx, cy, cz);

  // Generate plants based on biome
  plant_generate_biome_pattern(gen, chunk, biome);
}
