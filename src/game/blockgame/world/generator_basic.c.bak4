// Basic world generator (flat/simple terrain).
// Roadmap: docs/WORLD_ROADMAP.md.
#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <stdlib.h>
#include <world/generator.h>

// Simple heightmap-based terrain generation
void world_generator_generate_chunk_basic(WorldGenerator *generator,
                                          Chunk *chunk) {
  if (!generator || !chunk)
    return;

  // Simple heightmap generation
  for (i32 x = 0; x < CHUNK_SIZE; x++) {
    for (i32 z = 0; z < CHUNK_SIZE; z++) {
      // Calculate world position
      i32 world_x = chunk->pos.x * CHUNK_SIZE + x;
      i32 world_z = chunk->pos.z * CHUNK_SIZE + z;

      // Simple height calculation (can be replaced with noise)
      f32 height_f =
          64.0f + sinf(world_x * 0.1f) * 10.0f + cosf(world_z * 0.1f) * 10.0f;
      i32 height = (i32)height_f;

      // Fill chunk
      for (i32 y = 0; y < CHUNK_SIZE; y++) {
        i32 world_y = chunk->pos.y * CHUNK_SIZE + y;

        BlockID block = BLOCK_AIR;

        if (world_y < height - 3) {
          block = BLOCK_STONE;
        } else if (world_y < height) {
          block = BLOCK_DIRT;
        } else if (world_y == height) {
          block = BLOCK_GRASS;
        }

        chunk_set_block(chunk, x, y, z, block);
      }
    }
  }

  chunk->state = CHUNK_STATE_GENERATED;
}
