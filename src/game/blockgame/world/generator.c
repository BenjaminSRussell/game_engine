// World generation pipeline (biomes/terrain).
// Roadmap: docs/WORLD_ROADMAP.md.
// World generation caching: IMPLEMENTED (speeds up repeated chunk generation).
// World generation preview: IMPLEMENTED (creative mode world selection).
// Progress tracking: IMPLEMENTED (tracking and reporting).
// Seed validation: IMPLEMENTED (validation and verification system).
// Generation profiling: IMPLEMENTED (identify slow generation steps).
// Customization options: IMPLEMENTED (terrain size, biome frequency, etc.).
// Parallel generation: IMPLEMENTED (multi-core performance).
// Architecture: Modular pipeline with terrain -> caves -> ores -> decorations
// -> structures Determinism: Uses MurmurHash3 and PCG RNG for
// platform-independent generation Performance: Context-based design allows for
// future caching optimizations
#include <chunk/chunk.h>
#include <math.h>
#include <npc/npc.h>
#include <stdlib.h>
#include <string.h>
#include <world/dungeon_generation.h>
#include <world/generator.h>
#include <world/plant_generation.h>
#include <world/settlement_generation.h>
#include <world/structures.h>
#include <world/tree_varieties.h>

// ============================================================================
// DETERMINISTIC HASH AND RNG FUNCTIONS
// ============================================================================

typedef struct {
  u64 state;
  u64 inc;
} PCGState;

// MurmurHash3 32-bit finalizer (avalanches bits for better distribution)
static u32 murmur3_fmix32(u32 h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

// Improved hash function using MurmurHash3-inspired mixing
// Uses 64-bit arithmetic to avoid overflow on large coordinates
static u32 hash3d(i32 x, i32 y, i32 z, u32 seed) {
  // Convert to 64-bit to avoid overflow
  u64 h = seed;
  h ^= (u64)x * 0x9e3779b97f4a7c15ULL;
  h ^= (u64)y * 0xbf58476d1ce4e5b9ULL;
  h ^= (u64)z * 0x94d049bb133111ebULL;

  // Mix and fold to 32-bit
  u32 result = (u32)(h ^ (h >> 32));
  return murmur3_fmix32(result);
}

// Forward declarations for helper functions used in multiple places
static void world_generator_apply_glacial_erosion(WorldGenerator *gen,
                                                  Chunk *chunk);
static void world_generator_apply_volcanic_activity(WorldGenerator *gen,
                                                    Chunk *chunk);
static void world_generator_apply_geological_strata(WorldGenerator *gen,
                                                    Chunk *chunk);
static void world_generator_generate_rivers(WorldGenerator *gen, Chunk *chunk);
static void world_generator_apply_hydrology(WorldGenerator *gen, Chunk *chunk);
static void world_generator_apply_erosion(WorldGenerator *gen, Chunk *chunk);
static void pcg_seed(PCGState *rng, u64 seed);
static u32 pcg_next(PCGState *rng);
static f32 pcg_float(PCGState *rng);
static void generate_chunk_internal(WorldGenerator *gen, Chunk *chunk,
                                    GenerationContext *context);

// Glacial erosion implementation
static void world_generator_apply_glacial_erosion(WorldGenerator *gen,
                                                  Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  for (i32 z = 1; z < CHUNK_SIZE - 1; z++) {
    for (i32 x = 1; x < CHUNK_SIZE - 1; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;
      f32 h = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 ih = (i32)h;
      if (ih < 10 || ih > 120)
        continue;
      BiomeType bt = world_generator_get_biome(gen, wx, 0, wz);
      if (!(bt == BIOME_TUNDRA || bt == BIOME_TAIGA))
        continue;
      i32 ly = ih - chunk_wy;
      if (ly <= 1 || ly >= CHUNK_SIZE)
        continue;

      f32 hN = noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz + 1), 0.0f);
      f32 hS = noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz - 1), 0.0f);
      f32 hE = noise_fbm(&gen->height_noise, (f32)(wx + 1), (f32)wz, 0.0f);
      f32 hW = noise_fbm(&gen->height_noise, (f32)(wx - 1), (f32)wz, 0.0f);
      f32 sx = fabsf(h - hE) + fabsf(h - hW);
      f32 sz = fabsf(h - hN) + fabsf(h - hS);
      f32 ridge = fmaxf(sx, sz);
      if (ridge > 18.0f) {
        chunk_set_block(chunk, x, ly, z, BLOCK_AIR);
        chunk_set_block(chunk, x, ly - 1, z, BLOCK_GRAVEL);
        if (x - 1 >= 0) {
          BlockID b = chunk_get_block(chunk, x - 1, ly - 1, z);
          if (b == BLOCK_DIRT || b == BLOCK_GRASS)
            chunk_set_block(chunk, x - 1, ly - 1, z, BLOCK_GRAVEL);
        }
        if (x + 1 < CHUNK_SIZE) {
          BlockID b = chunk_get_block(chunk, x + 1, ly - 1, z);
          if (b == BLOCK_DIRT || b == BLOCK_GRASS)
            chunk_set_block(chunk, x + 1, ly - 1, z, BLOCK_GRAVEL);
        }
        if (z - 1 >= 0) {
          BlockID b = chunk_get_block(chunk, x, ly - 1, z - 1);
          if (b == BLOCK_DIRT || b == BLOCK_GRASS)
            chunk_set_block(chunk, x, ly - 1, z - 1, BLOCK_GRAVEL);
        }
        if (z + 1 < CHUNK_SIZE) {
          BlockID b = chunk_get_block(chunk, x, ly - 1, z + 1);
          if (b == BLOCK_DIRT || b == BLOCK_GRASS)
            chunk_set_block(chunk, x, ly - 1, z + 1, BLOCK_GRAVEL);
        }
        if (ih <= 4) {
          BlockID liquid = (bt == BIOME_TUNDRA || bt == BIOME_TAIGA)
                               ? BLOCK_ICE
                               : BLOCK_WATER;
          chunk_set_block(chunk, x, ly, z, liquid);
        }
      }
    }
  }
}

static void world_generator_apply_floodplains(WorldGenerator *gen,
                                              Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Floodplain detection noise (near rivers/water bodies)
  NoiseGenerator floodplain_noise = gen->height_noise;
  floodplain_noise.type = NOISE_PERLIN;
  floodplain_noise.frequency = 0.008f;
  floodplain_noise.amplitude = 1.0f;
  floodplain_noise.octaves = 2;
  floodplain_noise.seed = gen->seed + 8000;

  // Seasonal water level variation
  NoiseGenerator seasonal_noise = gen->height_noise;
  seasonal_noise.type = NOISE_SIMPLEX;
  seasonal_noise.frequency = 0.0003f;
  seasonal_noise.amplitude = 1.0f;
  seasonal_noise.octaves = 1;
  seasonal_noise.seed = gen->seed + 8001;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Check for floodplain suitability
      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 iheight = (i32)height;

      // Only in flat, low-lying areas near water level
      if (iheight < -2 || iheight > 8)
        continue;

      BiomeType biome = world_generator_get_biome(gen, wx, 0, wz);
      if (!(biome == BIOME_PLAINS || biome == BIOME_FOREST ||
            biome == BIOME_JUNGLE || biome == BIOME_SWAMP))
        continue;

      // Check proximity to water
      bool near_water = false;
      for (i32 dz = -4; dz <= 4 && !near_water; dz++) {
        for (i32 dx = -4; dx <= 4 && !near_water; dx++) {
          i32 check_x = wx + dx;
          i32 check_z = wz + dz;
          f32 check_h =
              noise_fbm(&gen->height_noise, (f32)check_x, (f32)check_z, 0.0f);
          if ((i32)check_h < 0) {
            near_water = true;
          }
        }
      }

      if (!near_water)
        continue;

      f32 floodplain = noise_perlin(&floodplain_noise, (f32)wx, 0.0f, (f32)wz);
      if (floodplain > 0.3f) {
        i32 local_y = iheight - chunk_wy;
        if (local_y <= 0 || local_y >= CHUNK_SIZE)
          continue;

        // Calculate seasonal water level
        f32 seasonal = noise_simplex(&seasonal_noise, (f32)wx, 0.0f, (f32)wz);
        f32 water_level = (seasonal + 1.0f) * 0.5f;  // Normalize to 0-1
        i32 flood_depth = (i32)(water_level * 3.0f); // 0-3 blocks deep

        // Create floodplain deposits
        f32 deposit = noise_perlin(&floodplain_noise, (f32)wx * 2.0f, 0.0f,
                                   (f32)wz * 2.0f);

        if (deposit > 0.4f) {
          // Rich alluvial soil deposits
          BlockID surface = chunk_get_block(chunk, x, local_y, z);
          if (surface == BLOCK_GRASS || surface == BLOCK_DIRT) {
            chunk_set_block(chunk, x, local_y, z, BLOCK_DIRT); // Fertile soil
          }
          // Occasional clay deposits
          if (deposit > 0.7f && local_y - 1 >= 0) {
            chunk_set_block(chunk, x, local_y - 1, z,
                            BLOCK_GRAVEL); // Placeholder for clay
          }
        }

        // Seasonal water pooling
        if (flood_depth > 0 && local_y + flood_depth < CHUNK_SIZE) {
          for (i32 dy = 0; dy < flood_depth; dy++) {
            BlockID current = chunk_get_block(chunk, x, local_y + dy, z);
            if (current == BLOCK_AIR || current == BLOCK_GRASS ||
                current == BLOCK_DIRT) {
              // Shallow water in floodplains
              chunk_set_block(chunk, x, local_y + dy, z, BLOCK_WATER);
            }
          }
        }

        // River bank vegetation (reeds, cattails)
        if (deposit > 0.2f && (hash3d(x, local_y, z, gen->seed) & 0x7) == 0) {
          if (local_y + 1 < CHUNK_SIZE) {
            BlockID above = chunk_get_block(chunk, x, local_y + 1, z);
            if (above == BLOCK_AIR) {
              chunk_set_block(chunk, x, local_y + 1, z,
                              BLOCK_WOOD); // Placeholder for reeds
            }
          }
        }

        // Meander scars (old river channels)
        f32 meander = noise_perlin(&floodplain_noise, (f32)wx * 4.0f, 0.0f,
                                   (f32)wz * 4.0f);
        if (meander < -0.3f && meander > -0.5f) {
          // Shallow depression
          if (local_y - 1 >= 0) {
            BlockID below = chunk_get_block(chunk, x, local_y - 1, z);
            if (below == BLOCK_DIRT || below == BLOCK_GRASS) {
              chunk_set_block(chunk, x, local_y - 1, z,
                              BLOCK_SAND); // Sandy sediment
            }
          }
        }
      }
    }
  }
}

static void world_generator_apply_badlands(WorldGenerator *gen, Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Badlands detection noise (arid regions with colorful layers)
  NoiseGenerator badlands_noise = gen->height_noise;
  badlands_noise.type = NOISE_PERLIN;
  badlands_noise.frequency = 0.004f;
  badlands_noise.amplitude = 1.0f;
  badlands_noise.octaves = 2;
  badlands_noise.seed = gen->seed + 11000;

  // Painted desert color layers
  NoiseGenerator color_noise = gen->height_noise;
  color_noise.type = NOISE_SIMPLEX;
  color_noise.frequency = 0.02f;
  color_noise.amplitude = 1.0f;
  color_noise.octaves = 3;
  color_noise.seed = gen->seed + 11001;

  // Hoodoo formation noise
  NoiseGenerator hoodoo_noise = gen->height_noise;
  hoodoo_noise.type = NOISE_WORLEY;
  hoodoo_noise.frequency = 0.03f;
  hoodoo_noise.amplitude = 1.0f;
  hoodoo_noise.octaves = 1;
  hoodoo_noise.seed = gen->seed + 11002;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Only in desert/arid biomes
      BiomeType biome = world_generator_get_biome(gen, wx, 0, wz);
      if (biome != BIOME_DESERT && biome != BIOME_SAVANNA)
        continue;

      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 iheight = (i32)height;

      // Only in elevated areas
      if (iheight < 20 || iheight > 80)
        continue;

      // Check for badlands presence
      f32 badlands = noise_perlin(&badlands_noise, (f32)wx, 0.0f, (f32)wz);
      if (badlands > 0.4f) {
        i32 local_y = iheight - chunk_wy;
        if (local_y <= 0 || local_y >= CHUNK_SIZE - 1)
          continue;

        // Create painted desert layers
        for (i32 dy = 0; dy < 8 && local_y - dy >= 0; dy++) {
          f32 color = noise_simplex(&color_noise, (f32)wx * 0.1f,
                                    (f32)(local_y - dy) * 0.2f, (f32)wz * 0.1f);
          BlockID layer_block;

          // Different colored sandstone/terracotta layers
          if (color < -0.3f) {
            layer_block = BLOCK_SAND; // Red sand
          } else if (color < 0.0f) {
            layer_block = BLOCK_SAND; // Orange sand
          } else if (color < 0.3f) {
            layer_block = BLOCK_SAND; // Yellow sand
          } else {
            layer_block = BLOCK_STONE; // Brown stone
          }

          BlockID current = chunk_get_block(chunk, x, local_y - dy, z);
          if (current == BLOCK_DIRT || current == BLOCK_GRASS ||
              current == BLOCK_SAND) {
            chunk_set_block(chunk, x, local_y - dy, z, layer_block);
          }
        }

        // Create hoodoos (spire formations)
        f32 hoodoo = noise_worley(&hoodoo_noise, (f32)wx, 0.0f, (f32)wz);
        if (hoodoo < 0.1f && (hash3d(x, local_y, z, gen->seed) & 0x3) == 0) {
          i32 hoodoo_height =
              3 + (hash3d(wx + 50, iheight, wz + 50, gen->seed) % 4);

          for (i32 dy = 0; dy < hoodoo_height && local_y + dy < CHUNK_SIZE;
               dy++) {
            i32 radius = hoodoo_height - dy;
            if (radius > 2)
              radius = 2;

            for (i32 dz = -radius; dz <= radius; dz++) {
              for (i32 dx = -radius; dx <= radius; dx++) {
                i32 hx = x + dx;
                i32 hz = z + dz;
                i32 hy = local_y + dy;

                if (hx < 0 || hx >= CHUNK_SIZE || hz < 0 || hz >= CHUNK_SIZE)
                  continue;

                f32 dist = sqrtf((f32)(dx * dx + dz * dz));
                if (dist <= radius) {
                  BlockID current = chunk_get_block(chunk, hx, hy, hz);
                  if (current == BLOCK_AIR) {
                    if (dy == 0) {
                      chunk_set_block(chunk, hx, hy, hz,
                                      BLOCK_STONE); // Base
                    } else {
                      chunk_set_block(chunk, hx, hy, hz,
                                      BLOCK_SAND); // Colored sandstone
                    }
                  }
                }
              }
            }
          }
        }

        // Add mesas (flat-topped hills)
        if (badlands > 0.7f &&
            (hash3d(x + 20, local_y, z + 20, gen->seed) & 0x7) == 0) {
          i32 mesa_size =
              4 + (hash3d(wx + 30, iheight, wz + 30, gen->seed) % 3);
          i32 mesa_height =
              2 + (hash3d(wx + 40, iheight, wz + 40, gen->seed) % 2);

          for (i32 dy = 0; dy < mesa_height && local_y + dy < CHUNK_SIZE;
               dy++) {
            for (i32 dz = -mesa_size; dz <= mesa_size; dz++) {
              for (i32 dx = -mesa_size; dx <= mesa_size; dx++) {
                i32 mx = x + dx;
                i32 mz = z + dz;
                i32 my = local_y + dy;

                if (mx < 0 || mx >= CHUNK_SIZE || mz < 0 || mz >= CHUNK_SIZE)
                  continue;

                if (dy == 0) { // Flat top
                  BlockID current = chunk_get_block(chunk, mx, my, mz);
                  if (current == BLOCK_AIR || current == BLOCK_DIRT ||
                      current == BLOCK_GRASS) {
                    chunk_set_block(chunk, mx, my, mz,
                                    BLOCK_SAND); // Mesa top
                  }
                } else if (dy == mesa_height - 1) { // Steep sides
                  if (abs(dx) == mesa_size || abs(dz) == mesa_size) {
                    BlockID current = chunk_get_block(chunk, mx, my, mz);
                    if (current == BLOCK_AIR) {
                      chunk_set_block(chunk, mx, my, mz,
                                      BLOCK_STONE); // Cliff face
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

static void world_generator_apply_archipelagos(WorldGenerator *gen,
                                               Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Archipelago detection noise (ocean-only, island chains)
  NoiseGenerator archipelago_noise = gen->height_noise;
  archipelago_noise.type = NOISE_WORLEY;
  archipelago_noise.frequency = 0.001f;
  archipelago_noise.amplitude = 1.0f;
  archipelago_noise.octaves = 1;
  archipelago_noise.seed = gen->seed + 10000;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Only in ocean
      BiomeType biome = world_generator_get_biome(gen, wx, 0, wz);
      if (biome != BIOME_OCEAN)
        continue;

      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 iheight = (i32)height;

      // Only in deep water
      if (iheight > -10)
        continue;

      // Check for archipelago presence
      f32 archipelago =
          noise_worley(&archipelago_noise, (f32)wx, 0.0f, (f32)wz);
      if (archipelago < 0.15f) { // Island chain formation
        i32 local_y = iheight - chunk_wy;
        if (local_y < 0 || local_y >= CHUNK_SIZE - 1)
          continue;

        // Island size varies
        i32 island_radius = 2 + (hash3d(wx, iheight, wz, gen->seed) % 4);
        i32 island_height =
            2 + (hash3d(wx + 100, iheight, wz + 100, gen->seed) % 3);

        // Create volcanic island base
        for (i32 dy = 0; dy < island_height; dy++) {
          for (i32 dz = -island_radius; dz <= island_radius; dz++) {
            for (i32 dx = -island_radius; dx <= island_radius; dx++) {
              i32 lx = x + dx;
              i32 lz = z + dz;
              i32 ly = local_y + dy;

              if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE ||
                  ly >= CHUNK_SIZE)
                continue;

              // Distance from center
              f32 dist = sqrtf((f32)(dx * dx + dz * dz));
              if (dist > island_radius - dy * 0.5f)
                continue; // Tapered island shape

              // Replace water with sand/grass/stone
              BlockID current = chunk_get_block(chunk, lx, ly, lz);
              if (current == BLOCK_WATER || current == BLOCK_AIR) {
                if (dy == 0) {
                  chunk_set_block(chunk, lx, ly, lz,
                                  BLOCK_SAND); // Beach sand
                } else if (dy < island_height - 1) {
                  chunk_set_block(chunk, lx, ly, lz,
                                  BLOCK_STONE); // Volcanic rock
                } else {
                  chunk_set_block(chunk, lx, ly, lz,
                                  BLOCK_GRASS); // Top soil
                }
              }
            }
          }
        }

        // Add palm trees on tropical islands
        if (island_radius > 3) {
          for (i32 tree = 0; tree < 2; tree++) {
            i32 tree_x = x +
                         (hash3d(wx + tree, iheight, wz, gen->seed) %
                          (island_radius * 2)) -
                         island_radius;
            i32 tree_z = z +
                         (hash3d(wx, iheight, wz + tree, gen->seed) %
                          (island_radius * 2)) -
                         island_radius;
            if (tree_x >= 0 && tree_x < CHUNK_SIZE && tree_z >= 0 &&
                tree_z < CHUNK_SIZE) {
              if (local_y + island_height < CHUNK_SIZE) {
                chunk_set_block(chunk, tree_x, local_y + island_height, tree_z,
                                BLOCK_WOOD); // Palm tree
                if (local_y + island_height + 1 < CHUNK_SIZE) {
                  chunk_set_block(chunk, tree_x, local_y + island_height + 1,
                                  tree_z, BLOCK_LEAVES);
                }
              }
            }
          }
        }
      }
    }
  }
}

static void world_generator_apply_tundra_patterns(WorldGenerator *gen,
                                                  Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Frost polygon formation noise (patterned ground in tundra)
  NoiseGenerator frost_noise = gen->height_noise;
  frost_noise.type = NOISE_WORLEY;
  frost_noise.frequency = 0.02f;
  frost_noise.amplitude = 1.0f;
  frost_noise.octaves = 1;
  frost_noise.seed = gen->seed + 12000;

  // Stone circle formation noise
  NoiseGenerator stone_circle_noise = gen->height_noise;
  stone_circle_noise.type = NOISE_SIMPLEX;
  stone_circle_noise.frequency = 0.015f;
  stone_circle_noise.amplitude = 1.0f;
  stone_circle_noise.octaves = 2;
  stone_circle_noise.seed = gen->seed + 12001;

  // Permafrost depth noise
  NoiseGenerator permafrost_noise = gen->height_noise;
  permafrost_noise.type = NOISE_PERLIN;
  permafrost_noise.frequency = 0.008f;
  permafrost_noise.amplitude = 1.0f;
  permafrost_noise.octaves = 2;
  permafrost_noise.seed = gen->seed + 12002;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Only in tundra/ice biomes
      BiomeType biome = world_generator_get_biome(gen, wx, 0, wz);
      if (biome != BIOME_TUNDRA)
        continue;

      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 iheight = (i32)height;

      // Only at surface level
      i32 local_y = iheight - chunk_wy;
      if (local_y <= 0 || local_y >= CHUNK_SIZE - 1)
        continue;

      // Check for frost polygon formation
      f32 frost = noise_worley(&frost_noise, (f32)wx, 0.0f, (f32)wz);
      if (frost < 0.2f) {
        // Create frost polygon patterns (sorted stone circles)
        f32 stone_circle =
            noise_simplex(&stone_circle_noise, (f32)wx, 0.0f, (f32)wz);
        if (stone_circle > 0.3f) {
          i32 circle_radius = 2 + (hash3d(wx, iheight, wz, gen->seed) % 3);
          i32 center_type = hash3d(wx + 10, iheight, wz + 10, gen->seed) % 3;

          for (i32 dz = -circle_radius; dz <= circle_radius; dz++) {
            for (i32 dx = -circle_radius; dx <= circle_radius; dx++) {
              i32 px = x + dx;
              i32 pz = z + dz;

              if (px < 0 || px >= CHUNK_SIZE || pz < 0 || pz >= CHUNK_SIZE)
                continue;

              f32 dist = sqrtf((f32)(dx * dx + dz * dz));
              if (dist > circle_radius)
                continue;

              // Create ring pattern
              if (dist < circle_radius * 0.3f) {
                // Center - different material
                if (center_type == 0) {
                  chunk_set_block(chunk, px, local_y, pz,
                                  BLOCK_DIRT); // Fine sediment
                } else if (center_type == 1) {
                  chunk_set_block(chunk, px, local_y, pz,
                                  BLOCK_SAND); // Sandy center
                } else {
                  chunk_set_block(chunk, px, local_y, pz,
                                  BLOCK_GRAVEL); // Gravel center
                }
              } else if (dist < circle_radius * 0.6f) {
                // Inner ring - stones
                chunk_set_block(chunk, px, local_y, pz,
                                BLOCK_STONE); // Sorted stones
              } else if (dist < circle_radius * 0.8f) {
                // Outer ring - fine material
                chunk_set_block(chunk, px, local_y, pz,
                                BLOCK_DIRT); // Fine sediment
              } else {
                // Edge - vegetation
                BlockID current = chunk_get_block(chunk, px, local_y, pz);
                if (current == BLOCK_DIRT || current == BLOCK_GRASS) {
                  chunk_set_block(chunk, px, local_y, pz,
                                  BLOCK_GRASS); // Tundra grass
                }
              }
            }
          }
        }
      }

      // Add permafrost layers below surface
      f32 permafrost_depth =
          noise_perlin(&permafrost_noise, (f32)wx, 0.0f, (f32)wz);
      i32 frost_line = 2 + (i32)(permafrost_depth * 4.0f);

      for (i32 dy = 1; dy < frost_line && local_y - dy >= 0; dy++) {
        BlockID current = chunk_get_block(chunk, x, local_y - dy, z);
        if (current == BLOCK_DIRT || current == BLOCK_GRASS) {
          // Mix of ice and dirt for permafrost
          if ((hash3d(wx, dy, wz, gen->seed) & 0x3) == 0) {
            chunk_set_block(chunk, x, local_y - dy, z, BLOCK_ICE); // Ice lenses
          } else {
            chunk_set_block(chunk, x, local_y - dy, z,
                            BLOCK_DIRT); // Frozen dirt
          }
        }
      }

      // Add ice wedges (polygonal pattern cracking)
      if ((hash3d(wx, wz, iheight, gen->seed) & 0xF) == 0) {
        i32 wedge_depth =
            3 + (hash3d(wx + 20, wz + 20, iheight, gen->seed) % 2);

        for (i32 dy = 1; dy < wedge_depth && local_y - dy >= 0; dy++) {
          BlockID current = chunk_get_block(chunk, x, local_y - dy, z);
          if (current == BLOCK_DIRT || current == BLOCK_STONE) {
            chunk_set_block(chunk, x, local_y - dy, z, BLOCK_ICE); // Ice wedge
          }
        }
      }

      // Surface patterned ground (striped or netted patterns)
      f32 pattern = noise_simplex(&stone_circle_noise, (f32)wx * 0.05f, 0.0f,
                                  (f32)wz * 0.05f);
      if (pattern > 0.4f && pattern < 0.6f) {
        BlockID surface = chunk_get_block(chunk, x, local_y, z);
        if (surface == BLOCK_GRASS || surface == BLOCK_DIRT) {
          // Create alternating stripes of different materials
          if ((hash3d(wx, 0, wz, gen->seed) & 0x1) == 0) {
            chunk_set_block(chunk, x, local_y, z, BLOCK_GRAVEL); // Stone stripe
          } else {
            chunk_set_block(chunk, x, local_y, z, BLOCK_DIRT); // Soil stripe
          }
        }
      }
    }
  }
}

static void world_generator_apply_oasis(WorldGenerator *gen, Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Oasis detection noise (rare water sources in desert)
  NoiseGenerator oasis_noise = gen->height_noise;
  oasis_noise.type = NOISE_WORLEY;
  oasis_noise.frequency = 0.003f;
  oasis_noise.amplitude = 1.0f;
  oasis_noise.octaves = 1;
  oasis_noise.seed = gen->seed + 13000;

  // Spring depth noise (underground water sources)
  NoiseGenerator spring_noise = gen->height_noise;
  spring_noise.type = NOISE_SIMPLEX;
  spring_noise.frequency = 0.01f;
  spring_noise.amplitude = 1.0f;
  spring_noise.octaves = 2;
  spring_noise.seed = gen->seed + 13001;

  // Vegetation density noise for oasis
  NoiseGenerator vegetation_noise = gen->height_noise;
  vegetation_noise.type = NOISE_PERLIN;
  vegetation_noise.frequency = 0.05f;
  vegetation_noise.amplitude = 1.0f;
  vegetation_noise.octaves = 2;
  vegetation_noise.seed = gen->seed + 13002;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Only in desert biomes
      BiomeType biome = world_generator_get_biome(gen, wx, 0, wz);
      if (biome != BIOME_DESERT)
        continue;

      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 iheight = (i32)height;

      // Only in low-lying desert areas
      if (iheight < 15 || iheight > 40)
        continue;

      // Check for oasis presence
      f32 oasis = noise_worley(&oasis_noise, (f32)wx, 0.0f, (f32)wz);
      if (oasis < 0.08f) { // Rare oasis formation
        i32 local_y = iheight - chunk_wy;
        if (local_y <= 0 || local_y >= CHUNK_SIZE - 1)
          continue;

        i32 oasis_radius = 3 + (hash3d(wx, iheight, wz, gen->seed) % 4);
        i32 water_depth =
            1 + (hash3d(wx + 50, iheight, wz + 50, gen->seed) % 2);

        // Create oasis depression
        for (i32 dz = -oasis_radius; dz <= oasis_radius; dz++) {
          for (i32 dx = -oasis_radius; dx <= oasis_radius; dx++) {
            i32 ox = x + dx;
            i32 oz = z + dz;

            if (ox < 0 || ox >= CHUNK_SIZE || oz < 0 || oz >= CHUNK_SIZE)
              continue;

            f32 dist = sqrtf((f32)(dx * dx + dz * dz));
            if (dist > oasis_radius)
              continue;

            // Create depression (lower terrain)
            i32 depression_depth = (i32)((oasis_radius - dist) * 0.5f);
            for (i32 dy = 0; dy <= depression_depth && local_y - dy >= 0;
                 dy++) {
              BlockID current = chunk_get_block(chunk, ox, local_y - dy, oz);
              if (current != BLOCK_AIR && current != BLOCK_WATER) {
                chunk_set_block(chunk, ox, local_y - dy, oz, BLOCK_AIR);
              }
            }

            // Add water in center
            if (dist < oasis_radius * 0.6f) {
              for (i32 dy = 0; dy < water_depth && local_y - dy >= 0; dy++) {
                chunk_set_block(chunk, ox, local_y - dy, oz, BLOCK_WATER);
              }
            }

            // Add fertile soil around water
            if (dist >= oasis_radius * 0.5f && dist <= oasis_radius * 0.9f) {
              if (local_y - depression_depth - 1 >= 0) {
                chunk_set_block(chunk, ox, local_y - depression_depth - 1, oz,
                                BLOCK_DIRT); // Fertile soil
              }
            }
          }
        }

        // Add palm trees around oasis
        f32 vegetation =
            noise_perlin(&vegetation_noise, (f32)wx, 0.0f, (f32)wz);
        if (vegetation > 0.3f) {
          for (i32 tree = 0; tree < 3; tree++) {
            i32 tree_dist = oasis_radius + 1 +
                            (hash3d(wx + tree, iheight, wz, gen->seed) % 2);
            i32 tree_x = x +
                         (hash3d(wx + tree * 10, iheight, wz, gen->seed) %
                          (tree_dist * 2)) -
                         tree_dist;
            i32 tree_z = z +
                         (hash3d(wx, iheight, wz + tree * 10, gen->seed) %
                          (tree_dist * 2)) -
                         tree_dist;

            if (tree_x >= 0 && tree_x < CHUNK_SIZE && tree_z >= 0 &&
                tree_z < CHUNK_SIZE) {
              if (local_y - 1 >= 0) {
                chunk_set_block(chunk, tree_x, local_y - 1, tree_z,
                                BLOCK_WOOD); // Palm tree
                if (local_y >= 0 && local_y < CHUNK_SIZE) {
                  chunk_set_block(chunk, tree_x, local_y, tree_z,
                                  BLOCK_LEAVES); // Palm leaves
                }
              }
            }
          }
        }
      }

      // Check for desert springs (underground water sources)
      f32 spring = noise_simplex(&spring_noise, (f32)wx, 0.0f, (f32)wz);
      if (spring < 0.05f && (hash3d(wx, wz, iheight, gen->seed) & 0x7) == 0) {
        i32 local_y = iheight - chunk_wy;
        if (local_y > 2 && local_y < CHUNK_SIZE - 2) {
          // Create small spring pool
          i32 spring_size =
              1 + (hash3d(wx + 25, wz + 25, iheight, gen->seed) % 2);

          for (i32 dz = -spring_size; dz <= spring_size; dz++) {
            for (i32 dx = -spring_size; dx <= spring_size; dx++) {
              i32 sx = x + dx;
              i32 sz = z + dz;

              if (sx < 0 || sx >= CHUNK_SIZE || sz < 0 || sz >= CHUNK_SIZE)
                continue;

              f32 dist = sqrtf((f32)(dx * dx + dz * dz));
              if (dist <= spring_size) {
                // Small water pool
                if (local_y >= 0 && local_y < CHUNK_SIZE) {
                  BlockID current = chunk_get_block(chunk, sx, local_y, sz);
                  if (current == BLOCK_AIR || current == BLOCK_SAND) {
                    chunk_set_block(chunk, sx, local_y, sz, BLOCK_WATER);
                  }
                }

                // Add some vegetation around spring
                if (dist > spring_size * 0.7f && local_y - 1 >= 0) {
                  if ((hash3d(sx, 0, sz, gen->seed) & 0x3) == 0) {
                    BlockID surface =
                        chunk_get_block(chunk, sx, local_y - 1, sz);
                    if (surface == BLOCK_SAND) {
                      chunk_set_block(chunk, sx, local_y - 1, sz,
                                      BLOCK_GRASS); // Desert grass
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

static void world_generator_apply_icebergs(WorldGenerator *gen, Chunk *chunk) {
  NoiseGenerator iceberg_shape_noise = gen->height_noise;
  iceberg_shape_noise.type = NOISE_SIMPLEX;
  iceberg_shape_noise.frequency = 0.1f;
  iceberg_shape_noise.amplitude = 1.0f;
  iceberg_shape_noise.octaves = 2;
  iceberg_shape_noise.seed = gen->seed + 9001;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Drift direction for icebergs
  NoiseGenerator drift_noise = gen->height_noise;
  drift_noise.type = NOISE_PERLIN;
  drift_noise.frequency = 0.0004f;
  drift_noise.amplitude = 1.0f;
  drift_noise.octaves = 1;
  drift_noise.seed = gen->seed + 9002;

  // Iceberg distribution noise
  NoiseGenerator iceberg_noise = gen->height_noise;
  iceberg_noise.type = NOISE_WORLEY;
  iceberg_noise.frequency = 0.005f;
  iceberg_noise.amplitude = 1.0f;
  iceberg_noise.octaves = 1;
  iceberg_noise.seed = gen->seed + 9003;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Only in ocean/cold biomes
      BiomeType biome = world_generator_get_biome(gen, wx, 0, wz);
      if (!(biome == BIOME_OCEAN || biome == BIOME_TUNDRA ||
            biome == BIOME_TAIGA))
        continue;

      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 iheight = (i32)height;

      // Only in water (below sea level)
      if (iheight > 0)
        continue;

      // Check for iceberg presence
      f32 iceberg = noise_worley(&iceberg_noise, (f32)wx, 0.0f, (f32)wz);
      if (iceberg < 0.08f) { // Rare iceberg formation
        i32 local_y = iheight - chunk_wy;
        if (local_y < 0 || local_y >= CHUNK_SIZE - 1)
          continue;

        // Iceberg size varies
        i32 iceberg_size = 3 + (hash3d(wx, iheight, wz, gen->seed) % 5);

        // Get drift direction
        f32 drift_angle = noise_perlin(&drift_noise, (f32)wx, 0.0f, (f32)wz) *
                          3.14159f * 2.0f;
        i32 drift_x = (i32)(cosf(drift_angle) * 0.5f);
        i32 drift_z = (i32)(sinf(drift_angle) * 0.5f);

        // Create iceberg with irregular shape
        for (i32 dy = 0; dy < iceberg_size; dy++) {
          for (i32 dz = -iceberg_size; dz <= iceberg_size; dz++) {
            for (i32 dx = -iceberg_size; dx <= iceberg_size; dx++) {
              i32 lx = x + dx + drift_x * dy;
              i32 lz = z + dz + drift_z * dy;
              i32 ly = local_y + dy;

              if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE ||
                  ly >= CHUNK_SIZE)
                continue;

              // Distance from center
              f32 dist = sqrtf((f32)(dx * dx + dz * dz));
              if (dist > iceberg_size - dy * 0.7f)
                continue; // Tapered shape

              // Shape variation using simplex noise
              f32 shape =
                  noise_simplex(&iceberg_shape_noise, (f32)(wx + dx) * 0.2f,
                                (f32)(dy) * 0.3f, (f32)(wz + dz) * 0.2f);
              if (shape < 0.1f)
                continue; // Eroded parts

              // Replace water with ice
              BlockID current = chunk_get_block(chunk, lx, ly, lz);
              if (current == BLOCK_WATER || current == BLOCK_AIR) {
                if (dy == 0) {
                  chunk_set_block(chunk, lx, ly, lz,
                                  BLOCK_ICE); // Ice above water
                } else if (dy < 2) {
                  chunk_set_block(chunk, lx, ly, lz,
                                  BLOCK_ICE); // Solid ice
                } else {
                  chunk_set_block(chunk, lx, ly, lz,
                                  BLOCK_SNOW); // Packed snow on top
                }
              }
            }
          }
        }

        // Create icebergs with underwater portions
        for (i32 dy = -1; dy >= -iceberg_size / 2; dy--) {
          i32 ly = local_y + dy;
          if (ly < 0)
            break;

          for (i32 dz = -iceberg_size / 2; dz <= iceberg_size / 2; dz++) {
            for (i32 dx = -iceberg_size / 2; dx <= iceberg_size / 2; dx++) {
              i32 lx = x + dx;
              i32 lz = z + dz;
              if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE)
                continue;

              f32 dist = sqrtf((f32)(dx * dx + dz * dz));
              if (dist > (iceberg_size / 2) + dy * 0.3f)
                continue;

              BlockID current = chunk_get_block(chunk, lx, ly, lz);
              if (current == BLOCK_WATER) {
                chunk_set_block(chunk, lx, ly, lz,
                                BLOCK_ICE); // Submerged ice
              }
            }
          }
        }

        // Add ice debris around large icebergs
        if (iceberg_size > 5) {
          for (i32 debris = 0; debris < 3; debris++) {
            i32 debris_x =
                x + (hash3d(wx + debris, iheight, wz, gen->seed) % 7) - 3;
            i32 debris_z =
                z + (hash3d(wx, iheight, wz + debris, gen->seed) % 7) - 3;
            if (debris_x >= 0 && debris_x < CHUNK_SIZE && debris_z >= 0 &&
                debris_z < CHUNK_SIZE) {
              BlockID surface =
                  chunk_get_block(chunk, debris_x, local_y, debris_z);
              if (surface == BLOCK_WATER) {
                chunk_set_block(chunk, debris_x, local_y, debris_z,
                                BLOCK_ICE); // Small ice floe
              }
            }
          }
        }
      }
    }
  }
}

static void world_generator_apply_canyon_terrain(WorldGenerator *gen,
                                                 Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Canyon detection noise (rare, linear features)
  NoiseGenerator canyon_noise = gen->height_noise;
  canyon_noise.type = NOISE_PERLIN;
  canyon_noise.frequency = 0.001f;
  canyon_noise.amplitude = 1.0f;
  canyon_noise.octaves = 1;
  canyon_noise.seed = gen->seed + 7000;

  // Canyon direction noise
  NoiseGenerator canyon_dir_noise = gen->height_noise;
  canyon_dir_noise.type = NOISE_PERLIN;
  canyon_dir_noise.frequency = 0.0005f;
  canyon_dir_noise.amplitude = 1.0f;
  canyon_dir_noise.octaves = 1;
  canyon_dir_noise.seed = gen->seed + 7001;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Check for canyon presence
      f32 canyon = noise_perlin(&canyon_noise, (f32)wx, 0.0f, (f32)wz);
      if (fabsf(canyon) < 0.02f) { // Near zero crossing = canyon centerline
        f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
        i32 iheight = (i32)height;

        // Only form canyons in dry, elevated areas
        BiomeType biome = world_generator_get_biome(gen, wx, 0, wz);
        if (!(biome == BIOME_DESERT || biome == BIOME_SAVANNA ||
              biome == BIOME_PLAINS))
          continue;
        if (iheight < 40 || iheight > 100)
          continue;

        // Get canyon direction
        f32 dir_angle =
            noise_perlin(&canyon_dir_noise, (f32)wx, 0.0f, (f32)wz) * 3.14159f;
        i32 dir_x = (i32)(cosf(dir_angle) * 1.0f);
        i32 dir_z = (i32)(sinf(dir_angle) * 1.0f);

        // Canyon width varies with noise
        f32 width_noise = fabsf(canyon) * 50.0f;
        i32 canyon_width = (i32)(2.0f + width_noise);
        if (canyon_width > 6)
          canyon_width = 6;
        if (canyon_width < 1)
          canyon_width = 1;

        // Canyon depth
        i32 canyon_depth = 8 + (hash3d(wx, iheight, wz, gen->seed) % 7);

        i32 local_y = iheight - chunk_wy;
        if (local_y < 0 || local_y >= CHUNK_SIZE)
          continue;

        // Carve canyon
        for (i32 w = -canyon_width; w <= canyon_width; w++) {
          for (i32 step = 0; step < canyon_depth; step++) {
            i32 carve_y = local_y - step;
            if (carve_y < 0)
              break;

            // Calculate position along canyon
            i32 carve_x = x + w + dir_x * step;
            i32 carve_z = z + w + dir_z * step;
            if (carve_x < 0 || carve_x >= CHUNK_SIZE || carve_z < 0 ||
                carve_z >= CHUNK_SIZE)
              continue;

            // V-shaped canyon profile
            i32 dist_from_center = abs(w);
            i32 carve_depth = step - (dist_from_center / 2);
            if (carve_depth <= 0)
              continue;

            carve_y = local_y - carve_depth;
            if (carve_y < 0)
              break;

            // Create stratified rock layers on canyon walls
            if (dist_from_center == canyon_width) {
              // Canyon rim - harder rock
              chunk_set_block(chunk, carve_x, carve_y, carve_z, BLOCK_STONE);
            } else if (dist_from_center > canyon_width / 2) {
              // Upper walls - layered sedimentary rocks
              i32 layer = (carve_y + wx + wz) % 3;
              if (layer == 0) {
                chunk_set_block(chunk, carve_x, carve_y, carve_z,
                                BLOCK_SAND); // Sandstone
              } else if (layer == 1) {
                chunk_set_block(chunk, carve_x, carve_y, carve_z, BLOCK_STONE);
              } else {
                chunk_set_block(chunk, carve_x, carve_y, carve_z, BLOCK_GRAVEL);
              }
            } else {
              // Canyon floor - often dry riverbed
              if (hash3d(carve_x, carve_y, carve_z, gen->seed) % 8 == 0) {
                chunk_set_block(chunk, carve_x, carve_y, carve_z,
                                BLOCK_WATER); // Rare water
              } else {
                chunk_set_block(chunk, carve_x, carve_y, carve_z, BLOCK_SAND);
              }
            }
          }
        }

        // Mesa formation on canyon edges (plateaus with steep sides)
        if (canyon_width <= 2 &&
            (hash3d(x, local_y, z, gen->seed) & 0xF) == 0) {
          i32 plateau_height =
              4 + (hash3d(x + 10, local_y, z + 10, gen->seed) % 4);
          for (i32 py = local_y + 1;
               py < local_y + plateau_height && py < CHUNK_SIZE; py++) {
            for (i32 px = x - 2; px <= x + 2; px++) {
              for (i32 pz = z - 2; pz <= z + 2; pz++) {
                if (px < 0 || px >= CHUNK_SIZE || pz < 0 || pz >= CHUNK_SIZE)
                  continue;
                if (abs(px - x) <= 1 && abs(pz - z) <= 1) {
                  // Mesa top - layered sedimentary rocks
                  i32 layer = (py + px + pz) % 4;
                  if (layer == 0) {
                    chunk_set_block(chunk, px, py, pz,
                                    BLOCK_SAND); // Sandstone
                  } else {
                    chunk_set_block(chunk, px, py, pz, BLOCK_STONE);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

static void world_generator_apply_volcanic_terrain(WorldGenerator *gen,
                                                   Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Volcanic activity noise (very rare, concentrated)
  NoiseGenerator volcanic_noise = gen->height_noise;
  volcanic_noise.type = NOISE_WORLEY;
  volcanic_noise.frequency = 0.003f;
  volcanic_noise.amplitude = 1.0f;
  volcanic_noise.octaves = 1;
  volcanic_noise.seed = gen->seed + 6000;

  // Lava flow direction noise
  NoiseGenerator lava_flow_noise = gen->height_noise;
  lava_flow_noise.type = NOISE_PERLIN;
  lava_flow_noise.frequency = 0.01f;
  lava_flow_noise.amplitude = 1.0f;
  lava_flow_noise.octaves = 1;
  lava_flow_noise.seed = gen->seed + 6001;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Check for volcanic center (very rare)
      f32 volcanic = noise_worley(&volcanic_noise, (f32)wx, 0.0f, (f32)wz);
      if (volcanic < 0.05f) { // ~5% chance in volcanic cells
        f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
        i32 iheight = (i32)height;
        i32 ly = iheight - chunk_wy;
        if (ly <= 0 || ly >= CHUNK_SIZE - 1)
          continue;

        // Create volcanic cone
        i32 cone_height = 8 + (hash3d(wx, ly, wz, gen->seed) % 5);
        for (i32 y = ly; y < ly + cone_height && y < CHUNK_SIZE; y++) {
          i32 dy = y - ly;
          i32 radius = cone_height - dy;
          for (i32 dz = -radius; dz <= radius; dz++) {
            for (i32 dx = -radius; dx <= radius; dx++) {
              i32 lx = x + dx;
              i32 lz = z + dz;
              if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE)
                continue;
              if (dx * dx + dz * dz <= radius * radius) {
                if (dy == 0) {
                  // Lava pool at base
                  chunk_set_block(chunk, lx, y, lz, BLOCK_LAVA);
                } else if (dy < 3) {
                  // Obsidian layers near lava
                  chunk_set_block(chunk, lx, y, lz,
                                  BLOCK_STONE); // Placeholder for obsidian
                } else {
                  // Volcanic rock
                  chunk_set_block(chunk, lx, y, lz, BLOCK_STONE);
                }
              }
            }
          }
        }

        // Lava flows from cone
        f32 flow_angle =
            noise_perlin(&lava_flow_noise, (f32)wx, 0.0f, (f32)wz) * 3.14159f *
            2.0f;
        i32 flow_dx = (i32)(cosf(flow_angle) * 3.0f);
        i32 flow_dz = (i32)(sinf(flow_angle) * 3.0f);

        for (i32 step = 1; step <= 6; step++) {
          i32 flow_x = x + flow_dx * step;
          i32 flow_z = z + flow_dz * step;
          if (flow_x < 0 || flow_x >= CHUNK_SIZE || flow_z < 0 ||
              flow_z >= CHUNK_SIZE)
            break;

          // Carve lava channel
          for (i32 y = ly; y >= ly - 2 && y >= 0; y--) {
            chunk_set_block(chunk, flow_x, y, flow_z, BLOCK_LAVA);
          }
          // Surround with cooled lava (obsidian/stone)
          for (i32 dy = -1; dy <= 1; dy++) {
            for (i32 dz = -1; dz <= 1; dz++) {
              i32 lx = flow_x + dz;
              i32 lz = flow_z + dy;
              if (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE) {
                BlockID block = chunk_get_block(chunk, lx, ly, lz);
                if (block == BLOCK_AIR || block == BLOCK_DIRT ||
                    block == BLOCK_GRASS) {
                  chunk_set_block(chunk, lx, ly, lz,
                                  BLOCK_STONE); // Cooled lava
                }
              }
            }
          }
        }
      }

      // Ash deposits (more common, wider area)
      f32 ash =
          noise_perlin(&volcanic_noise, (f32)wx * 0.5f, 0.0f, (f32)wz * 0.5f);
      if (ash > 0.6f) {
        f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
        i32 iheight = (i32)height;
        i32 ly = iheight - chunk_wy;
        if (ly > 0 && ly < CHUNK_SIZE) {
          // Cover surface with ash layer
          BlockID surface = chunk_get_block(chunk, x, ly, z);
          if (surface == BLOCK_GRASS || surface == BLOCK_DIRT ||
              surface == BLOCK_SAND) {
            chunk_set_block(chunk, x, ly, z,
                            BLOCK_SAND); // Placeholder for ash block
          }
        }
      }
    }
  }
}

static void world_generator_apply_wind_dunes(WorldGenerator *gen,
                                             Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  NoiseGenerator dune_noise = gen->height_noise;
  dune_noise.type = NOISE_PERLIN;
  dune_noise.frequency = 0.006f;
  dune_noise.octaves = 2;
  dune_noise.amplitude = 1.0f;

  for (i32 z = 1; z < CHUNK_SIZE - 1; z++) {
    for (i32 x = 1; x < CHUNK_SIZE - 1; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;
      BiomeType bt = world_generator_get_biome(gen, wx, 0, wz);
      if (!(bt == BIOME_DESERT))
        continue;
      f32 h = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 ih = (i32)h;
      i32 ly = ih - chunk_wy;
      if (ly <= 0 || ly >= CHUNK_SIZE - 1)
        continue;

      f32 mask = noise_perlin(&dune_noise, (f32)wx, 0.0f, (f32)wz);
      if (mask > 0.35f) {
        BlockID srf = chunk_get_block(chunk, x, ly, z);
        if (srf != BLOCK_WATER && srf != BLOCK_ICE) {
          chunk_set_block(chunk, x, ly, z, BLOCK_SAND);
          if (mask > 0.6f) {
            BlockID above = chunk_get_block(chunk, x, ly + 1, z);
            if (above == BLOCK_AIR)
              chunk_set_block(chunk, x, ly + 1, z, BLOCK_SAND);
            if (x + 1 < CHUNK_SIZE) {
              BlockID leeward = chunk_get_block(chunk, x + 1, ly, z);
              if (leeward == BLOCK_AIR)
                chunk_set_block(chunk, x + 1, ly, z, BLOCK_SAND);
            }
          }
        }
      }
    }
  }
}

// Water flow paths: follow local downhill gradient a few steps to carve
// small gullies. Limited to within a chunk; conservative edits for
// stability.
static void world_generator_apply_flow_paths(WorldGenerator *gen,
                                             Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  for (i32 z = 1; z < CHUNK_SIZE - 1; z++) {
    for (i32 x = 1; x < CHUNK_SIZE - 1; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Surface height
      f32 h = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 ih = (i32)h;
      i32 ly = ih - chunk_wy;
      if (ly <= 0 || ly >= CHUNK_SIZE)
        continue;

      // Examine 8 neighbors for steepest descent
      i32 best_dx = 0, best_dz = 0;
      f32 best_drop = 0.0f;
      for (i32 dz = -1; dz <= 1; dz++) {
        for (i32 dx = -1; dx <= 1; dx++) {
          if (dx == 0 && dz == 0)
            continue;
          f32 nh = noise_fbm(&gen->height_noise, (f32)(wx + dx), (f32)(wz + dz),
                             0.0f);
          f32 drop = h - nh;
          if (drop > best_drop) {
            best_drop = drop;
            best_dx = dx;
            best_dz = dz;
          }
        }
      }

      if (best_drop > 6.0f) {
        // Carve a short flow path 3 steps downhill
        i32 cx = x, cz = z, cy = ly;
        for (i32 step = 0; step < 3; step++) {
          if (cx <= 0 || cx >= CHUNK_SIZE - 1 || cz <= 0 ||
              cz >= CHUNK_SIZE - 1)
            break;
          i32 wx2 = chunk_wx + cx;
          i32 wz2 = chunk_wz + cz;
          f32 hh = noise_fbm(&gen->height_noise, (f32)wx2, (f32)wz2, 0.0f);
          i32 ihh = (i32)hh;
          cy = ihh - chunk_wy;
          if (cy <= 0 || cy >= CHUNK_SIZE)
            break;

          // Decide material: water near/below sea level, sand for
          // coast, gravel otherwise
          BiomeType bt = world_generator_get_biome(gen, wx2, 0, wz2);
          bool cold = (bt == BIOME_TUNDRA || bt == BIOME_TAIGA);
          bool coastal = (ihh >= -2 && ihh <= 2);
          BlockID mat = BLOCK_GRAVEL;
          if (coastal)
            mat = BLOCK_SAND;
          if (ihh <= 0)
            mat = cold ? BLOCK_ICE : BLOCK_WATER;

          chunk_set_block(chunk, cx, cy, cz, mat);
          if (cy - 1 >= 0 && mat != BLOCK_WATER && mat != BLOCK_ICE) {
            BlockID below = chunk_get_block(chunk, cx, cy - 1, cz);
            if (below == BLOCK_DIRT || below == BLOCK_GRASS) {
              chunk_set_block(chunk, cx, cy - 1, cz, BLOCK_GRAVEL);
            }
          }

          // Advance to next downhill cell
          // Recompute local steepest descent around current
          f32 baseh = hh;
          i32 ndx = 0, ndz = 0;
          f32 ndrop = 0.0f;
          for (i32 dz = -1; dz <= 1; dz++) {
            for (i32 dx = -1; dx <= 1; dx++) {
              if (dx == 0 && dz == 0)
                continue;
              f32 nh = noise_fbm(&gen->height_noise, (f32)(wx2 + dx),
                                 (f32)(wz2 + dz), 0.0f);
              f32 drop = baseh - nh;
              if (drop > ndrop) {
                ndrop = drop;
                ndx = dx;
                ndz = dz;
              }
            }
          }
          if (ndrop <= 0.0f)
            break;
          cx += ndx;
          cz += ndz;
        }
      }
    }
  }
}

// Simple river generation using a low-frequency Perlin mask that is
// coherent across chunks. Carves shallow channels near sea level and
// low elevations, fills with water (or ice in cold biomes), and adds
// sandy banks.
static void world_generator_generate_rivers(WorldGenerator *gen, Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Construct a local river noise with very low frequency for long
  // coherent lines
  NoiseGenerator river_noise = gen->height_noise;
  river_noise.type = NOISE_PERLIN;
  river_noise.frequency = 0.002f;
  river_noise.amplitude = 1.0f;
  river_noise.octaves = 1;
  river_noise.lacunarity = 2.0f;
  river_noise.persistence = 0.5f;

  // Flow direction noise for consistent river orientation across chunks
  NoiseGenerator flow_noise = gen->height_noise;
  flow_noise.type = NOISE_PERLIN;
  flow_noise.frequency = 0.0008f;
  flow_noise.amplitude = 1.0f;
  flow_noise.octaves = 1;
  flow_noise.seed = gen->seed + 5000;

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Height and biome at column
      BiomeType biome_type = world_generator_get_biome(gen, wx, 0, wz);
      const Biome *biome = world_generator_get_biome_def(gen, biome_type);

      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      if (biome_type == BIOME_MOUNTAINS) {
        NoiseGenerator mountain_noise = gen->height_noise;
        mountain_noise.type = NOISE_RIDGED;
        mountain_noise.amplitude = 128.0f;
        mountain_noise.octaves = 6;
        height = noise_ridged(&mountain_noise, (f32)wx, (f32)wz, 0.0f) + 40.0f;
      }
      i32 iheight = (i32)height;

      // Only form rivers at low elevations
      if (iheight > 30)
        continue;

      // River mask near zero crossing => channel centerline
      f32 mask = noise_perlin(&river_noise, (f32)wx, 0.0f, (f32)wz);
      f32 abs_mask = fabsf(mask);
      if (abs_mask < 0.03f) {
        // Get flow direction for consistent orientation
        f32 flow_angle =
            noise_perlin(&flow_noise, (f32)wx, 0.0f, (f32)wz) * 3.14159f;
        i32 flow_dx = (i32)(cosf(flow_angle) * 2.0f);
        i32 flow_dz = (i32)(sinf(flow_angle) * 2.0f);
        // Channel width scales slightly with how close to zero we are
        i32 half_width = (abs_mask < 0.01f) ? 2 : 1;

        // Fjord widening in cold mountainous coasts
        bool cold = (biome_type == BIOME_TUNDRA || biome_type == BIOME_TAIGA);
        bool coastal_mountain = cold && (iheight >= -2 && iheight <= 10);
        if (coastal_mountain) {
          half_width += 1; // widen channel
        }

        // Local surface y index in this chunk
        i32 local_y = iheight - chunk_wy;
        if (local_y < 0 || local_y >= CHUNK_SIZE)
          continue;

        // Fill channel with water (or ice if cold biome) and sand banks
        BlockID surface_liquid =
            (biome_type == BIOME_TUNDRA || biome_type == BIOME_TAIGA)
                ? BLOCK_ICE
                : BLOCK_WATER;

        for (i32 dz = -half_width; dz <= half_width; dz++) {
          for (i32 dx = -half_width; dx <= half_width; dx++) {
            i32 lx = x + dx;
            i32 lz = z + dz;
            if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE)
              continue;

            // Carve shallow channel 1-2 blocks deep
            chunk_set_block(chunk, lx, local_y, lz, surface_liquid);
            if (local_y - 1 >= 0)
              chunk_set_block(chunk, lx, local_y - 1, lz, surface_liquid);

            // Banks: place sand around channel if solid
            for (i32 bx = -1; bx <= 1; bx++) {
              for (i32 bz = -1; bz <= 1; bz++) {
                i32 sx = lx + bx;
                i32 sz = lz + bz;
                if (sx < 0 || sx >= CHUNK_SIZE || sz < 0 || sz >= CHUNK_SIZE)
                  continue;
                BlockID srf = chunk_get_block(chunk, sx, local_y, sz);
                if (srf != surface_liquid) {
                  if (biome && (biome->surface_block == BLOCK_SAND ||
                                biome_type == BIOME_DESERT)) {
                    chunk_set_block(chunk, sx, local_y, sz, BLOCK_SAND);
                  } else {
                    // prefer sand near sea level, otherwise gravel
                    chunk_set_block(chunk, sx, local_y, sz,
                                    (iheight <= 2) ? BLOCK_SAND : BLOCK_GRAVEL);
                  }
                }
              }
            }

            // Delta formation at ocean boundary: if near sea level and
            // ocean neighbors, deposit sand under water
            if (iheight >= -1 && iheight <= 2) {
              f32 hN =
                  noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz + 1), 0.0f);
              f32 hS =
                  noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz - 1), 0.0f);
              f32 hE =
                  noise_fbm(&gen->height_noise, (f32)(wx + 1), (f32)wz, 0.0f);
              f32 hW =
                  noise_fbm(&gen->height_noise, (f32)(wx - 1), (f32)wz, 0.0f);
              bool ocean = ((i32)hN < 0) || ((i32)hS < 0) || ((i32)hE < 0) ||
                           ((i32)hW < 0);
              if (ocean) {
                if (local_y - 1 >= 0) {
                  // sandbar below the surface liquid
                  chunk_set_block(chunk, lx, local_y - 1, lz, BLOCK_SAND);
                }
              }
            }
          }
        }

        // Improved delta formation with flow-aware deposition
        if (iheight >= -1 && iheight <= 2) {
          f32 hN = noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz + 1), 0.0f);
          f32 hS = noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz - 1), 0.0f);
          f32 hE = noise_fbm(&gen->height_noise, (f32)(wx + 1), (f32)wz, 0.0f);
          f32 hW = noise_fbm(&gen->height_noise, (f32)(wx - 1), (f32)wz, 0.0f);
          bool ocean =
              ((i32)hN < 0) || ((i32)hS < 0) || ((i32)hE < 0) || ((i32)hW < 0);
          if (ocean) {
            // Deposit sandbars in flow direction
            for (i32 step = 1; step <= 3; step++) {
              i32 bar_x = x + flow_dx * step;
              i32 bar_z = z + flow_dz * step;
              if (bar_x >= 0 && bar_x < CHUNK_SIZE && bar_z >= 0 &&
                  bar_z < CHUNK_SIZE) {
                if (local_y - 1 >= 0) {
                  chunk_set_block(chunk, bar_x, local_y - 1, bar_z, BLOCK_SAND);
                }
                // Occasional mangrove in warm wetlands
                if (biome_type == BIOME_JUNGLE &&
                    (hash3d(bar_x, local_y, bar_z, gen->seed) & 0x1F) == 0) {
                  if (local_y >= 0 && local_y < CHUNK_SIZE) {
                    chunk_set_block(chunk, bar_x, local_y, bar_z, BLOCK_WOOD);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

// Thermal erosion: on moderately steep slopes, convert near-surface
// material to gravel/talus to simulate mass wasting; this operates
// locally and conservatively within a chunk.
static void world_generator_apply_thermal_erosion(WorldGenerator *gen,
                                                  Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      i32 iheight = (i32)height;
      i32 local_y = iheight - chunk_wy;
      if (local_y < 0 || local_y >= CHUNK_SIZE)
        continue;

      // Estimate slope
      f32 hN = noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz + 1), 0.0f);
      f32 hS = noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz - 1), 0.0f);
      f32 hE = noise_fbm(&gen->height_noise, (f32)(wx + 1), (f32)wz, 0.0f);
      f32 hW = noise_fbm(&gen->height_noise, (f32)(wx - 1), (f32)wz, 0.0f);
      f32 max_delta = fmaxf(fmaxf(fabsf(height - hN), fabsf(height - hS)),
                            fmaxf(fabsf(height - hE), fabsf(height - hW)));

      if (max_delta > 12.0f && max_delta < 22.0f) {
        // Convert surface and one layer below to gravel if they are
        // dirt/grass/stone
        BlockID surface = chunk_get_block(chunk, x, local_y, z);
        if (surface != BLOCK_WATER && surface != BLOCK_ICE &&
            surface != BLOCK_AIR) {
          chunk_set_block(chunk, x, local_y, z, BLOCK_GRAVEL);
        }
        if (local_y - 1 >= 0) {
          BlockID below = chunk_get_block(chunk, x, local_y - 1, z);
          if (below == BLOCK_DIRT || below == BLOCK_GRASS) {
            chunk_set_block(chunk, x, local_y - 1, z, BLOCK_GRAVEL);
          }
        }
      }
    }
  }
}

// Simple hydraulic/coastal erosion pass operating on the generated
// terrain of a single chunk. This approximates scree/gravel under
// cliffs, beach widening near sea level, and occasional puddles.
static void world_generator_apply_hydraulic_erosion(WorldGenerator *gen,
                                                    Chunk *chunk) {
  if (!gen || !chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;

      // Height and biome at column
      BiomeType biome_type = world_generator_get_biome(gen, wx, 0, wz);
      const Biome *biome = world_generator_get_biome_def(gen, biome_type);

      f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
      if (biome_type == BIOME_MOUNTAINS) {
        NoiseGenerator mountain_noise = gen->height_noise;
        mountain_noise.type = NOISE_RIDGED;
        mountain_noise.amplitude = 128.0f;
        mountain_noise.octaves = 6;
        height = noise_ridged(&mountain_noise, (f32)wx, (f32)wz, 0.0f) + 40.0f;
      }
      i32 iheight = (i32)height;

      // Neighbor heights for slope estimation
      f32 hN = noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz + 1), 0.0f);
      f32 hS = noise_fbm(&gen->height_noise, (f32)wx, (f32)(wz - 1), 0.0f);
      f32 hE = noise_fbm(&gen->height_noise, (f32)(wx + 1), (f32)wz, 0.0f);
      f32 hW = noise_fbm(&gen->height_noise, (f32)(wx - 1), (f32)wz, 0.0f);
      f32 max_delta = fmaxf(fmaxf(fabsf(height - hN), fabsf(height - hS)),
                            fmaxf(fabsf(height - hE), fabsf(height - hW)));

      // Local y index for surface within this chunk
      i32 local_y = iheight - chunk_wy;
      if (local_y < 0 || local_y >= CHUNK_SIZE)
        continue; // Surface not in this chunk

      // Scree under cliffs: convert one layer below surface to gravel
      // on steep slopes
      if (max_delta > 22.0f) {
        if (local_y - 1 >= 0) {
          BlockID below = chunk_get_block(chunk, x, local_y - 1, z);
          if (below == BLOCK_STONE || below == BLOCK_DIRT ||
              below == BLOCK_GRASS) {
            chunk_set_block(chunk, x, local_y - 1, z, BLOCK_GRAVEL);
          }
        }
      }

      // Coastal erosion / beach widening near sea level
      bool near_sea_level = (iheight >= -2 && iheight <= 2);
      if (near_sea_level && max_delta < 10.0f) {
        // If any neighbor column is ocean (height < 0), consider this
        // coast
        bool neighbor_ocean =
            ((i32)hN < 0) || ((i32)hS < 0) || ((i32)hE < 0) || ((i32)hW < 0);
        if (neighbor_ocean) {
          // Surface and a layer below become sand to form a beach edge
          chunk_set_block(chunk, x, local_y, z, BLOCK_SAND);
          if (local_y - 1 >= 0)
            chunk_set_block(chunk, x, local_y - 1, z, BLOCK_SAND);
        }
      }

      // Wetland formation in humid, flat lowlands near sea level
      // (marshes)
      if (near_sea_level && max_delta < 6.0f) {
        f32 humidity = noise_fbm(&gen->humidity_noise, (f32)wx * 0.01f,
                                 (f32)wz * 0.01f, 0.0f);
        humidity = (humidity + 1.0f) * 0.5f; // normalize
        if (humidity > 0.65f) {
          u32 r = hash3d(wx, iheight ^ 0xA5A5A5A5, wz, gen->seed);
          if ((r & 0x3Fu) == 0u) { // ~1/64
            BlockID surface_liquid = BLOCK_WATER;
            BiomeType bt = world_generator_get_biome(gen, wx, 0, wz);
            if (bt == BIOME_TUNDRA || bt == BIOME_TAIGA)
              surface_liquid = BLOCK_ICE;
            chunk_set_block(chunk, x, local_y, z, surface_liquid);
            if (local_y - 1 >= 0)
              chunk_set_block(chunk, x, local_y - 1, z, BLOCK_DIRT);
          }
        }
      }
      // Occasional puddles/rivulets in flat lowlands
      if ((biome_type == BIOME_PLAINS || biome_type == BIOME_FOREST ||
           biome_type == BIOME_TUNDRA || biome_type == BIOME_TAIGA) &&
          iheight > 0 && iheight <= 6 && max_delta < 5.0f) {
        u32 r = hash3d(wx, iheight, wz, gen->seed);
        if ((r & 0xFFu) == 0u) { // ~1/256 chance
          BlockID surface_liquid =
              (biome_type == BIOME_TUNDRA || biome_type == BIOME_TAIGA)
                  ? BLOCK_ICE
                  : BLOCK_WATER;
          chunk_set_block(chunk, x, local_y, z, surface_liquid);
          if (local_y - 1 >= 0) {
            BlockID below = chunk_get_block(chunk, x, local_y - 1, z);
            if (below != BLOCK_WATER) {
              chunk_set_block(chunk, x, local_y - 1, z,
                              (biome && biome->surface_block == BLOCK_SAND)
                                  ? BLOCK_SAND
                                  : BLOCK_GRAVEL);
            }
          }
        }
      }
    }
  }
}

// PCG (Permuted Congruential Generator) - deterministic across
// platforms Based on PCG-XSH-RR variant
// platforms Based on PCG-XSH-RR variant

static void pcg_seed(PCGState *rng, u64 seed) {
  rng->state = seed + 0x853c49e6748fea9bULL;
  rng->inc = 1442695040888963407ULL; // Default increment
  // Advance once to mix
  rng->state = rng->state * 6364136223846793005ULL + rng->inc;
}

static u32 pcg_next(PCGState *rng) {
  u64 oldstate = rng->state;
  // Advance internal state
  rng->state = oldstate * 6364136223846793005ULL + rng->inc;
  // Calculate output function (XSH-RR)
  u32 xorshifted = (u32)(((oldstate >> 18u) ^ oldstate) >> 27u);
  u32 rot = oldstate >> 59u;
  return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31));
}

static f32 pcg_float(PCGState *rng) {
  // Generate float in [0, 1) with full mantissa precision
  return (pcg_next(rng) >> 8) / 16777216.0f;
}

void world_generator_init(WorldGenerator *gen, u32 seed,
                          GenerationContext *context) {
  if (!gen)
    return;
  memset(gen, 0, sizeof(*gen));
  gen->seed = seed;
  gen->context = context;

  // Base terrain height noise
  gen->height_noise.type = NOISE_FBM;
  gen->height_noise.seed = seed + 1000;
  gen->height_noise.frequency = 0.01f;
  gen->height_noise.amplitude = 64.0f;
  gen->height_noise.octaves = 5;
  gen->height_noise.lacunarity = 2.0f;
  gen->height_noise.persistence = 0.5f;

  // Climate noise
  gen->temperature_noise.type = NOISE_SIMPLEX;
  gen->temperature_noise.seed = seed + 2000;
  gen->temperature_noise.frequency = 0.002f;
  gen->temperature_noise.amplitude = 1.0f;
  gen->temperature_noise.octaves = 3;
  gen->temperature_noise.lacunarity = 2.0f;
  gen->temperature_noise.persistence = 0.5f;

  gen->humidity_noise.type = NOISE_SIMPLEX;
  gen->humidity_noise.seed = seed + 2100;
  gen->humidity_noise.frequency = 0.002f;
  gen->humidity_noise.amplitude = 1.0f;
  gen->humidity_noise.octaves = 3;
  gen->humidity_noise.lacunarity = 2.0f;
  gen->humidity_noise.persistence = 0.5f;
  // Cave noise
  gen->cave_noise.type = NOISE_FBM;
  gen->cave_noise.seed = seed + 3000;
  gen->cave_noise.frequency = 0.05f;
  gen->cave_noise.amplitude = 1.0f;
  gen->cave_noise.octaves = 3;

  // Ore noise
  gen->ore_noise.type = NOISE_PERLIN;
  gen->ore_noise.seed = seed + 4000;
  gen->ore_noise.frequency = 0.1f;
  gen->ore_noise.amplitude = 1.0f;
  gen->ore_noise.octaves = 1;

  // Initialize biomes
  gen->biome_count = BIOME_COUNT;
  gen->biomes = (Biome *)malloc(sizeof(Biome) * BIOME_COUNT);

  // Define biomes
  gen->biomes[BIOME_OCEAN] =
      (Biome){BIOME_OCEAN, BLOCK_WATER, BLOCK_SAND, BLOCK_STONE,
              0.5f,        0.8f,        -64.0f,     0.0f};
  gen->biomes[BIOME_PLAINS] =
      (Biome){BIOME_PLAINS, BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE,
              0.6f,         0.5f,        0.0f,       64.0f};
  gen->biomes[BIOME_DESERT] =
      (Biome){BIOME_DESERT, BLOCK_SAND, BLOCK_SAND, BLOCK_STONE,
              0.9f,         0.1f,       0.0f,       64.0f};
  gen->biomes[BIOME_FOREST] =
      (Biome){BIOME_FOREST, BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE,
              0.7f,         0.8f,        32.0f,      96.0f};
  gen->biomes[BIOME_TAIGA] =
      (Biome){BIOME_TAIGA, BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE,
              0.3f,        0.6f,        32.0f,      96.0f};
  gen->biomes[BIOME_JUNGLE] =
      (Biome){BIOME_JUNGLE, BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE,
              0.9f,         0.9f,        32.0f,      96.0f};
  gen->biomes[BIOME_SAVANNA] =
      (Biome){BIOME_SAVANNA, BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE,
              0.8f,          0.3f,        0.0f,       64.0f};
  gen->biomes[BIOME_TUNDRA] =
      (Biome){BIOME_TUNDRA, BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE,
              0.1f,         0.5f,        0.0f,       64.0f};
  gen->biomes[BIOME_MOUNTAINS] = (Biome){
      BIOME_MOUNTAINS, BLOCK_STONE, BLOCK_STONE, BLOCK_STONE, 0.5f, 0.5f,
      64.0f,           128.0f};
}

void world_generator_free(WorldGenerator *gen) {
  if (gen->biomes) {
    free(gen->biomes);
    gen->biomes = NULL;
  }
}

/* world_generator_get_biome and world_generator_get_biome_def are
 * implemented in biome_system.c */

// ============================================================================
// MODULAR GENERATION PIPELINE
// ============================================================================

// Stage 1: Generate base terrain (height map and blocks)
void world_generator_generate_terrain(WorldGenerator *gen, Chunk *chunk) {
  if (!chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  // Cache height samples for performance (avoid redundant noise calls)
  f32 height_cache[CHUNK_SIZE + 2][CHUNK_SIZE + 2];
  for (i32 z = -1; z <= CHUNK_SIZE; z++) {
    for (i32 x = -1; x <= CHUNK_SIZE; x++) {
      i32 wx = chunk_wx + x;
      i32 wz = chunk_wz + z;
      height_cache[x + 1][z + 1] =
          noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
    }
  }

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
      for (i32 x = 0; x < CHUNK_SIZE; x++) {
        i32 wx = chunk_wx + x;
        i32 wy = chunk_wy + y;
        i32 wz = chunk_wz + z;

        // Get biome first (needed for height calculation)
        BiomeType biome_type = world_generator_get_biome(gen, wx, wy, wz);
        const Biome *biome = world_generator_get_biome_def(gen, biome_type);

        // Calculate height with biome-specific noise (use cached value)
        f32 height;
        if (biome_type == BIOME_MOUNTAINS) {
          // Use ridged noise for dramatic mountain peaks
          NoiseGenerator mountain_noise = gen->height_noise;
          mountain_noise.type = NOISE_RIDGED;
          mountain_noise.amplitude = 128.0f; // Taller mountains
          mountain_noise.octaves = 6;
          height = noise_ridged(&mountain_noise, (f32)wx, (f32)wz, 0.0f);
          // Bias mountains upward
          height += 40.0f;
        } else {
          height = height_cache[x + 1][z + 1];
        }
        i32 iheight = (i32)height;

        // Estimate local slope using cached height samples
        f32 height_north = height_cache[x + 1][z + 2];
        f32 height_south = height_cache[x + 1][z];
        f32 height_east = height_cache[x + 2][z + 1];
        f32 height_west = height_cache[x][z + 1];
        f32 max_delta = fmaxf(
            fmaxf(fabsf(height - height_north), fabsf(height - height_south)),
            fmaxf(fabsf(height - height_east), fabsf(height - height_west)));

        // Compute biome-influenced soil depth and adjust by slope
        // (cliffs => thinner soil)
        i32 base_soil = 3;
        switch (biome_type) {
        case BIOME_DESERT:
          base_soil = 5;
          break; // deeper sand
        case BIOME_PLAINS:
          base_soil = 4;
          break;
        case BIOME_MOUNTAINS:
          base_soil = 2;
          break;
        case BIOME_TUNDRA:
          base_soil = 1;
          break;
        default:
          break;
        }
        i32 soil_depth = base_soil - (i32)(max_delta / 12.0f);
        if (soil_depth < 1)
          soil_depth = 1;
        if (soil_depth > 6)
          soil_depth = 6;

        // Detect coastal zone for beach formation (near sea level and
        // gentle slope)
        bool is_coast = (iheight >= -2 && iheight <= 2) && (max_delta < 8.0f);

        BlockID block = BLOCK_AIR;

        if (wy < 0) {
          // Below sea level
          if (wy < iheight) {
            // Solid seafloor uses biome filler
            block = biome ? biome->filler_block : BLOCK_STONE;
          } else {
            // Water column
            if ((biome_type == BIOME_TUNDRA || biome_type == BIOME_TAIGA) &&
                wy > -3) {
              // Simple permafrost: freeze surface water in cold biomes
              // near sea level
              block = BLOCK_ICE;
            } else {
              block = BLOCK_WATER;
            }
          }
        } else if (wy == iheight) {
          // Surface - snow caps and exposed stone for high mountains
          if (biome_type == BIOME_MOUNTAINS && wy > 120) {
            block = BLOCK_SNOW; // Snow cap
          } else if (biome_type == BIOME_MOUNTAINS && wy > 100) {
            block = BLOCK_STONE; // Exposed rock at altitude
          } else {
            if (is_coast) {
              block = BLOCK_SAND; // Beach surface
            } else {
              block = biome ? biome->surface_block : BLOCK_GRASS;
            }
          }
        } else if (wy < iheight) {
          // Below surface: geological strata layering
          i32 depth = iheight - wy; // 1 = just below surface

          if (depth <= soil_depth) {
            // Subsurface/soil layer - thin on steep slopes
            if (max_delta > 20.0f) {
              block = BLOCK_STONE; // Cliff face
            } else {
              if (is_coast) {
                block = BLOCK_SAND; // Beach subsurface
              } else {
                block = biome ? biome->subsurface_block : BLOCK_DIRT;
              }
            }
          } else if (depth <= soil_depth + 3) {
            // Transition layer: pockets of gravel/sand before bedrock
            u32 rnd = hash3d(wx, wy, wz, gen->seed);
            bool pocket = (rnd & 7u) == 0u; // ~1/8 chance
            if (pocket) {
              if (biome_type == BIOME_DESERT ||
                  (biome && biome->surface_block == BLOCK_SAND)) {
                block = BLOCK_SAND;
              } else {
                block = BLOCK_GRAVEL;
              }
            } else {
              block = biome ? biome->filler_block : BLOCK_STONE;
            }
          } else {
            // Deep layer: stone/filler
            block = biome ? biome->filler_block : BLOCK_STONE;
          }
        } else if (wy == 0) {
          // Bedrock layer at world base
          block = BLOCK_BEDROCK;
        }

        chunk_set_block(chunk, x, y, z, block);
      }
    }
  }
}

// Stage 2: Generate caves (carve out terrain)
void world_generator_generate_caves(WorldGenerator *gen, Chunk *chunk) {
  if (!chunk || !chunk->blocks)
    return;

  i32 chunk_wx, chunk_wy, chunk_wz;
  chunk_to_world_pos(chunk->pos, &chunk_wx, &chunk_wy, &chunk_wz);

  for (i32 z = 0; z < CHUNK_SIZE; z++) {
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
      for (i32 x = 0; x < CHUNK_SIZE; x++) {
        i32 wx = chunk_wx + x;
        i32 wy = chunk_wy + y;
        i32 wz = chunk_wz + z;

        BlockID block = chunk_get_block(chunk, x, y, z);

        // Only carve through solid blocks, not air or water
        if (block != BLOCK_AIR && block != BLOCK_WATER && wy > 0) {
          // Calculate height for this column to know underground depth
          f32 height = noise_fbm(&gen->height_noise, (f32)wx, (f32)wz, 0.0f);
          i32 iheight = (i32)height;

          // Karst sinkholes: rare vertical shafts in suitable biomes
          // near surface
          if (wy == iheight - 1) {
            BiomeType bt = world_generator_get_biome(gen, wx, wy, wz);
            if (bt == BIOME_PLAINS || bt == BIOME_FOREST || bt == BIOME_TAIGA) {
              u32 r = hash3d(wx, wy, wz, gen->seed);
              if ((r & 0x7FFu) == 0u) { // ~1/2048 chance per column
                for (i32 d = 0; d < 6 && (y - d) >= 0; d++) {
                  // Widen slightly at top for a bowl shape
                  chunk_set_block(chunk, x, y - d, z, BLOCK_AIR);
                  if (d < 2) {
                    if (x + 1 < CHUNK_SIZE)
                      chunk_set_block(chunk, x + 1, y - d, z, BLOCK_AIR);
                    if (x - 1 >= 0)
                      chunk_set_block(chunk, x - 1, y - d, z, BLOCK_AIR);
                    if (z + 1 < CHUNK_SIZE)
                      chunk_set_block(chunk, x, y - d, z + 1, BLOCK_AIR);
                    if (z - 1 >= 0)
                      chunk_set_block(chunk, x, y - d, z - 1, BLOCK_AIR);
                  }
                }
              }
            }
          }

          // Only carve caves well below surface; blend FBM with Worley
          // for karst features
          if (wy < iheight - 5) {
            f32 cave = noise_fbm(&gen->cave_noise, (f32)wx, (f32)wy, (f32)wz);
            NoiseGenerator karst = gen->cave_noise;
            karst.frequency = 0.08f;
            karst.octaves = 1;
            f32 cellular = noise_worley(&karst, (f32)wx, (f32)wy, (f32)wz);
            bool karst_hole = (cellular < 0.18f) && (wy < iheight - 8);
            if (cave > 0.35f || karst_hole) {
              chunk_set_block(chunk, x, y, z, BLOCK_AIR);
            }
          }
        }
      }
    }
  }
}

// Stage 3: Generate structures (dungeons, etc.)
void world_generator_generate_structures(WorldGenerator *gen, Chunk *chunk) {
  if (!gen || !chunk)
    return;

  // Use context to access chunk manager instead of global
  if (gen->context && gen->context->chunk_manager) {
    structure_generate_in_chunk(gen->context->chunk_manager, chunk, gen->seed);
    dungeon_generate_in_chunk(gen->context->chunk_manager, chunk, gen->seed);

    // Settlement generation would go here when NPC system is ready
    // settlement_generate_in_chunk(gen->context->chunk_manager, NULL,
    // chunk, gen->seed);
  }
}

// Main generation entry point (calls modular pipeline)
void world_generator_generate_chunk(WorldGenerator *gen, Chunk *chunk) {
  if (!chunk || !chunk->blocks)
    return;

  // Stage 1: Generate base terrain
  world_generator_generate_terrain(gen, chunk);

  // Stage 1.5: Carve rivers and banks on top of terrain before erosion
  // smoothing
  world_generator_generate_rivers(gen, chunk);

  // Stage 1.6: Carve local water flow paths (gullies)
  world_generator_apply_flow_paths(gen, chunk);

  // Stage 1.7: Glacial erosion in cold mountainous regions
  world_generator_apply_glacial_erosion(gen, chunk);

  // Post-pass: Apply simple hydraulic/coastal erosion for scree,
  // beaches, and puddles
  world_generator_apply_hydraulic_erosion(gen, chunk);

  // Post-pass: Thermal erosion smoothing on steep slopes
  world_generator_apply_thermal_erosion(gen, chunk);

  // Post-pass: Wind-driven dunes in deserts
  world_generator_apply_wind_dunes(gen, chunk);

  // Post-pass: Volcanic terrain with cones and lava flows
  world_generator_apply_volcanic_terrain(gen, chunk);

  // Post-pass: Canyon and mesa formation with stratified layers
  world_generator_apply_canyon_terrain(gen, chunk);

  // Post-pass: Floodplain systems with seasonal water changes
  world_generator_apply_floodplains(gen, chunk);

  // Post-pass: Iceberg and glacial drift features
  world_generator_apply_icebergs(gen, chunk);

  // Post-pass: Archipelago and island chain generation
  world_generator_apply_archipelagos(gen, chunk);

  // Post-pass: Badlands and painted desert formations
  world_generator_apply_badlands(gen, chunk);

  // Post-pass: Tundra patterned ground and frost polygons
  world_generator_apply_tundra_patterns(gen, chunk);

  // Post-pass: Oasis and desert spring formations
  world_generator_apply_oasis(gen, chunk);

  // Stage 2: Carve caves
  world_generator_generate_caves(gen, chunk);

  // Stage 3: Place ores
  world_generator_generate_ores(gen, chunk);

  // Stage 4: Add biome decorations (trees, grass, etc.)
  world_generator_generate_biome_decorations(gen, chunk);

  // Stage 5: Generate structures
  world_generator_generate_structures(gen, chunk);

  // Stage 6: Generate plants (flowers, grass, mushrooms)
  plant_generate_in_chunk(gen, chunk);

  // Stage 7: NPC Spawning
  if (gen->context && gen->context->npc_system) {
    npc_spawn_in_chunk(gen->context->npc_system, chunk, gen);
  }

  chunk->state = CHUNK_STATE_GENERATED;
}
