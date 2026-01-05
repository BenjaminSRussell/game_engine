// Biome blending and transition logic.
// Roadmap: docs/WORLD_ROADMAP.md.
#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <world/generator.h>

// Get blended biome at position (for smooth transitions)
BiomeType world_generator_get_biome_blended(WorldGenerator *gen, i32 x, i32 y,
                                            i32 z, f32 *blend_factor) {
  if (!gen)
    return BIOME_PLAINS;

  // Sample biome at multiple points for blending
  BiomeType biome_center = world_generator_get_biome(gen, x, y, z);
  BiomeType biome_x = world_generator_get_biome(gen, x + 8, y, z);
  BiomeType biome_z = world_generator_get_biome(gen, x, y, z + 8);

  // If all same, no blending needed
  if (biome_center == biome_x && biome_center == biome_z) {
    if (blend_factor)
      *blend_factor = 1.0f;
    return biome_center;
  }

  // Calculate blend factor based on distance to biome boundaries
  f32 temp =
      noise_fbm(&gen->temperature_noise, (f32)x * 0.01f, (f32)z * 0.01f, 0.0f);
  f32 hum =
      noise_fbm(&gen->humidity_noise, (f32)x * 0.01f, (f32)z * 0.01f, 0.0f);
  temp = (temp + 1.0f) * 0.5f;
  hum = (hum + 1.0f) * 0.5f;

  // Use fractional parts for blending
  f32 temp_frac = temp - floorf(temp * 10.0f) / 10.0f;
  f32 hum_frac = hum - floorf(hum * 10.0f) / 10.0f;
  f32 blend = (temp_frac + hum_frac) * 0.5f;

  if (blend_factor) {
    *blend_factor = blend;
  }

  // Return dominant biome
  return biome_center;
}

// Generate chunk with biome transitions
void world_generator_generate_chunk_with_transitions(WorldGenerator *gen,
                                                     Chunk *chunk) {
  if (!gen || !chunk)
    return;

  for (i32 x = 0; x < CHUNK_SIZE; x++) {
    for (i32 z = 0; z < CHUNK_SIZE; z++) {
      i32 world_x = chunk->pos.x * CHUNK_SIZE + x;
      i32 world_z = chunk->pos.z * CHUNK_SIZE + z;

      // Get blended biome
      f32 blend_factor;
      BiomeType biome_type = world_generator_get_biome_blended(
          gen, world_x, 0, world_z, &blend_factor);
      const Biome *biome = world_generator_get_biome_def(gen, biome_type);

      if (!biome)
        continue;

      // Calculate height with biome influence
      f32 base_height = noise_fbm(&gen->height_noise, (f32)world_x * 0.01f,
                                  (f32)world_z * 0.01f, 0.0f);
      base_height = (base_height + 1.0f) * 0.5f;
      base_height = biome->height_min +
                    (biome->height_max - biome->height_min) * base_height;

      i32 height = (i32)base_height;

      // Fill chunk with biome blocks
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
