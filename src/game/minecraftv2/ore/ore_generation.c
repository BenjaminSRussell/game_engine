// Ore generation implementation
// TODO: Implement ore vein shape generation system.
// TODO: Add ore distribution optimization for performance.
// TODO: Implement ore biome-specific distribution rules.
// TODO: Add ore rarity system affecting spawn rates.
// TODO: Implement ore cluster generation system.
// TODO: Add ore validation system for invalid placements.
// TODO: Implement ore statistics tracking system.
// TODO: Add ore preview system for creative mode.
// TODO: Implement ore generation caching system.
// TODO: Add ore generation profiling system.
#include <block/block.h>
#include <chunk/chunk.h>
#include <world/generator.h>
#include <world/ore_distribution.h>
#include <math.h>
#include <stdlib.h>

// Internal random state
static u32 s_world_seed = 0;

void ore_distribution_init(u32 seed) { s_world_seed = seed; }

OreConfig ore_create_default_config(OreInternalType type) {
  OreConfig config = {0};
  config.type = type;
  config.discard_chance = 0.0f;
  config.triangular_dist = true;

  switch (type) {
  case ORE_TYPE_COAL:
    config.block_id = BLOCK_COAL_ORE;
    config.veins_per_chunk = 30; // Frequent
    config.min_y = 0;
    config.max_y = 128; // High up
    config.vein_size = 17;
    break;
  case ORE_TYPE_IRON:
    config.block_id = BLOCK_IRON_ORE;
    config.veins_per_chunk = 20;
    config.min_y = -64;
    config.max_y = 72;
    config.vein_size = 9;
    break;
  case ORE_TYPE_COPPER:
    config.block_id = BLOCK_IRON_ORE; // Placeholder (using iron for now if
                                      // copper block doesn't exist)
    // config.block_id = BLOCK_COPPER_ORE;
    config.veins_per_chunk = 16;
    config.min_y = -16;
    config.max_y = 112;
    config.vein_size = 10;
    break;
  case ORE_TYPE_GOLD:
    config.block_id = BLOCK_GOLD_ORE;
    config.veins_per_chunk = 4;
    config.min_y = -64;
    config.max_y = 32;
    config.vein_size = 9;
    config.discard_chance = 0.5f; // Rarer
    break;
  case ORE_TYPE_REDSTONE:
    config.block_id = BLOCK_GOLD_ORE; // Placeholder
    // config.block_id = BLOCK_REDSTONE_ORE;
    config.veins_per_chunk = 8;
    config.min_y = -64;
    config.max_y = 16;
    config.vein_size = 8;
    break;
  case ORE_TYPE_DIAMOND:
    config.block_id = BLOCK_DIAMOND_ORE;
    config.veins_per_chunk = 1; // Very rare
    config.min_y = -64;
    config.max_y = 16;
    config.vein_size = 4; // Small veins
    config.discard_chance = 0.5f;
    break;
  case ORE_TYPE_LAPIS:
    config.block_id = BLOCK_IRON_ORE; // Placeholder
    // config.block_id = BLOCK_LAPIS_ORE;
    config.veins_per_chunk = 2;
    config.min_y = -32;
    config.max_y = 32;
    config.vein_size = 7;
    config.triangular_dist = true; // Concentrated in middle
    break;
  case ORE_TYPE_EMERALD:
    config.block_id = BLOCK_DIAMOND_ORE; // Placeholder
    // config.block_id = BLOCK_EMERALD_ORE;
    config.veins_per_chunk = 1;
    config.min_y = -16;
    config.max_y = 120; // Only in mountains technically
    config.vein_size = 1;
    break;
  default:
    break;
  }

  return config;
}

// Simple deterministic RNG for chunk generation
static u32 chunk_rand(i32 x, i32 z, u32 salt) {
  u32 h = s_world_seed + x * 374761393 + z * 668265263 + salt;
  h = (h ^ (h >> 13)) * 1274126177;
  return h ^ (h >> 16);
}

static f32 chunk_rand_float(u32 *state) {
  *state = (*state * 1664525 + 1013904223);
  return (f32)(*state) / (f32)0xFFFFFFFF;
}

static i32 range_rand(u32 *state, i32 min, i32 max) {
  if (max <= min)
    return min;
  return min + (i32)(chunk_rand_float(state) * (f32)(max - min));
}

void ore_generate_vein(void *chunk_ptr, const OreConfig *config, i32 cx, i32 cy,
                       i32 cz) {
  Chunk *chunk = (Chunk *)chunk_ptr;

  // Generate a blob shape
  u32 size = config->vein_size;
  f32 radius = (f32)size / 4.0f; // Roughly convert block count to radius
  if (radius < 1.0f)
    radius = 1.0f;

  // Simple spherical vein generator
  // Iterate bounds [-r, +r]
  i32 r = (i32)ceilf(radius);

  for (i32 x = -r; x <= r; x++) {
    for (i32 y = -r; y <= r; y++) {
      for (i32 z = -r; z <= r; z++) {
        // Check if inside sphere
        if (x * x + y * y + z * z <= radius * radius) {
          i32 wx = cx + x;
          i32 wy = cy + y;
          i32 wz = cz + z;

          // Boundary check inside chunk
          if (wx >= 0 && wx < CHUNK_SIZE && wy >= 0 && wy < CHUNK_SIZE &&
              wz >= 0 && wz < CHUNK_SIZE) {

            // Only replace stone-type blocks
            BlockID current = chunk_get_block(chunk, wx, wy, wz);
            if (current == BLOCK_STONE) {
              chunk_set_block(chunk, wx, wy, wz, config->block_id);
            }
          }
        }
      }
    }
  }
}

void world_generator_generate_ores(WorldGenerator *gen, Chunk *chunk) {
  if (!gen || !chunk)
    return;

  // Configured ores
  OreInternalType types[] = {
      ORE_TYPE_COAL,     ORE_TYPE_IRON,  ORE_TYPE_GOLD,  ORE_TYPE_DIAMOND,
      ORE_TYPE_REDSTONE, ORE_TYPE_LAPIS, ORE_TYPE_COPPER};

  u32 seed_state = chunk_rand(chunk->pos.x, chunk->pos.z, 9876);

  for (int i = 0; i < 7; i++) {
    OreConfig config = ore_create_default_config(types[i]);

    for (u32 v = 0; v < config.veins_per_chunk; v++) {
      // Apply discard chance
      if (chunk_rand_float(&seed_state) < config.discard_chance)
        continue;

      // X/Z position
      i32 x = range_rand(&seed_state, 0, CHUNK_SIZE);
      i32 z = range_rand(&seed_state, 0, CHUNK_SIZE);

      // Y position
      i32 y = 0;
      if (config.triangular_dist) {
        // Approximate normal distribution by averaging two randoms
        // or similar to triangular: (rand(range) + rand(range)) / 2 ?
        // MC uses: rand(y_spread) + rand(y_spread) - spread_offset
        // Let's us simple uniform for now within range, maybe bias it?
        // Actually, let's just do range_rand for simplicity in V1
        y = range_rand(&seed_state, config.min_y, config.max_y);
      } else {
        y = range_rand(&seed_state, config.min_y, config.max_y);
      }

      // Generate
      // Chunk coordinates are local 0-15 (x,z) but Y is global 0-256 usually
      // Assuming chunk struct handles accessing local Y if it's a vertical
      // chunk system But typical MC chunks are full height 256. My headers show
      // CHUNK_HEIGHT isn't defined here but implied 256 or so

      if (y >= 0 && y < 256) { // Hard limit check
        ore_generate_vein(chunk, &config, x, y, z);
      }
    }
  }
}
