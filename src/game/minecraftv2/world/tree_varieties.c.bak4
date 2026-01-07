// Tree variety definitions and shapes.
// Roadmap: docs/WORLD_ROADMAP.md.
#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <stdlib.h>
#include <world/tree_varieties.h>

// Tree types are defined in header

// Simple hash for deterministic placement
static u32 tree_hash(i32 x, i32 z, u32 seed) {
  return ((u32)x * 73856093) ^ ((u32)z * 19349663) ^ seed;
}

// Generate oak tree
void tree_generate_oak(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed) {
  if (!chunk)
    return;

  u32 hash = tree_hash(x, z, seed);
  i32 trunk_height = 4 + (hash % 3); // 4-6 blocks

  // Place trunk
  for (i32 i = 0; i < trunk_height; i++) {
    i32 tree_y = y + i;
    i32 local_y = tree_y - chunk->pos.y * CHUNK_SIZE;

    if (local_y >= 0 && local_y < CHUNK_SIZE) {
      chunk_set_block(chunk, x, local_y, z, BLOCK_WOOD);
    }
  }

  // Place leaves (spherical)
  i32 leaves_y = y + trunk_height;
  i32 local_leaves_y = leaves_y - chunk->pos.y * CHUNK_SIZE;

  for (i32 dx = -2; dx <= 2; dx++) {
    for (i32 dz = -2; dz <= 2; dz++) {
      for (i32 dy = 0; dy <= 2; dy++) {
        i32 lx = x + dx;
        i32 lz = z + dz;
        i32 ly = local_leaves_y + dy;

        // Spherical pattern
        f32 dist = sqrtf((f32)(dx * dx + dz * dz + dy * dy));
        if (dist <= 2.0f && ly >= 0 && ly < CHUNK_SIZE) {
          if (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE) {
            chunk_set_block(chunk, lx, ly, lz, BLOCK_LEAVES);
          }
        }
      }
    }
  }
}

// Generate birch tree
void tree_generate_birch(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed) {
  if (!chunk)
    return;

  u32 hash = tree_hash(x, z, seed);
  i32 trunk_height = 5 + (hash % 3); // 5-7 blocks (taller)

  // Place trunk (white wood - using BLOCK_WOOD for now, could be different
  // block)
  for (i32 i = 0; i < trunk_height; i++) {
    i32 tree_y = y + i;
    i32 local_y = tree_y - chunk->pos.y * CHUNK_SIZE;

    if (local_y >= 0 && local_y < CHUNK_SIZE) {
      chunk_set_block(chunk, x, local_y, z, BLOCK_WOOD);
    }
  }

  // Place leaves (smaller, more sparse)
  i32 leaves_y = y + trunk_height;
  i32 local_leaves_y = leaves_y - chunk->pos.y * CHUNK_SIZE;

  for (i32 dx = -1; dx <= 1; dx++) {
    for (i32 dz = -1; dz <= 1; dz++) {
      for (i32 dy = 0; dy <= 1; dy++) {
        i32 lx = x + dx;
        i32 lz = z + dz;
        i32 ly = local_leaves_y + dy;

        if (ly >= 0 && ly < CHUNK_SIZE) {
          if (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE) {
            chunk_set_block(chunk, lx, ly, lz, BLOCK_LEAVES);
          }
        }
      }
    }
  }
}

// Generate spruce tree
void tree_generate_spruce(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed) {
  if (!chunk)
    return;

  u32 hash = tree_hash(x, z, seed);
  i32 trunk_height = 6 + (hash % 4); // 6-9 blocks (tall)

  // Place trunk
  for (i32 i = 0; i < trunk_height; i++) {
    i32 tree_y = y + i;
    i32 local_y = tree_y - chunk->pos.y * CHUNK_SIZE;

    if (local_y >= 0 && local_y < CHUNK_SIZE) {
      chunk_set_block(chunk, x, local_y, z, BLOCK_WOOD);
    }
  }

  // Place leaves in layers (conical shape)
  for (i32 layer = 0; layer < 3; layer++) {
    i32 leaves_y = y + trunk_height - layer * 2;
    i32 local_leaves_y = leaves_y - chunk->pos.y * CHUNK_SIZE;
    i32 radius = 2 - layer;

    for (i32 dx = -radius; dx <= radius; dx++) {
      for (i32 dz = -radius; dz <= radius; dz++) {
        if (abs(dx) + abs(dz) <= radius + 1) {
          i32 lx = x + dx;
          i32 lz = z + dz;

          if (local_leaves_y >= 0 && local_leaves_y < CHUNK_SIZE) {
            if (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE) {
              chunk_set_block(chunk, lx, local_leaves_y, lz, BLOCK_LEAVES);
            }
          }
        }
      }
    }
  }
}

// Generate rubber tree (produces rubber)
void tree_generate_rubber(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed) {
  if (!chunk)
    return;

  u32 hash = tree_hash(x, z, seed);
  i32 trunk_height = 5 + (hash % 3); // 5-7 blocks

  // Place trunk (rubber wood - using BLOCK_WOOD for now)
  for (i32 i = 0; i < trunk_height; i++) {
    i32 tree_y = y + i;
    i32 local_y = tree_y - chunk->pos.y * CHUNK_SIZE;

    if (local_y >= 0 && local_y < CHUNK_SIZE) {
      chunk_set_block(chunk, x, local_y, z, BLOCK_WOOD);
    }
  }

  // Place leaves (similar to oak but larger)
  i32 leaves_y = y + trunk_height;
  i32 local_leaves_y = leaves_y - chunk->pos.y * CHUNK_SIZE;

  for (i32 dx = -2; dx <= 2; dx++) {
    for (i32 dz = -2; dz <= 2; dz++) {
      for (i32 dy = 0; dy <= 3; dy++) {
        i32 lx = x + dx;
        i32 lz = z + dz;
        i32 ly = local_leaves_y + dy;

        f32 dist = sqrtf((f32)(dx * dx + dz * dz + dy * dy));
        if (dist <= 2.5f && ly >= 0 && ly < CHUNK_SIZE) {
          if (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE) {
            chunk_set_block(chunk, lx, ly, lz, BLOCK_LEAVES);
          }
        }
      }
    }
  }

  // Add rubber taps on trunk (could be special blocks)
  // For now, just mark with wood blocks at specific heights
  for (i32 i = 2; i < trunk_height - 1; i += 2) {
    i32 tree_y = y + i;
    i32 local_y = tree_y - chunk->pos.y * CHUNK_SIZE;

    if (local_y >= 0 && local_y < CHUNK_SIZE) {
      // Could place special "rubber tap" blocks here
      // For now, just use existing blocks
    }
  }
}

// Generate jungle tree (very tall)
void tree_generate_jungle(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed) {
  if (!chunk)
    return;

  u32 hash = tree_hash(x, z, seed);
  i32 trunk_height = 8 + (hash % 5); // 8-12 blocks (very tall)

  // Place trunk
  for (i32 i = 0; i < trunk_height; i++) {
    i32 tree_y = y + i;
    i32 local_y = tree_y - chunk->pos.y * CHUNK_SIZE;

    if (local_y >= 0 && local_y < CHUNK_SIZE) {
      chunk_set_block(chunk, x, local_y, z, BLOCK_WOOD);
    }
  }

  // Place leaves (large and dense)
  i32 leaves_y = y + trunk_height;
  i32 local_leaves_y = leaves_y - chunk->pos.y * CHUNK_SIZE;

  for (i32 dx = -3; dx <= 3; dx++) {
    for (i32 dz = -3; dz <= 3; dz++) {
      for (i32 dy = 0; dy <= 3; dy++) {
        i32 lx = x + dx;
        i32 lz = z + dz;
        i32 ly = local_leaves_y + dy;

        f32 dist = sqrtf((f32)(dx * dx + dz * dz + dy * dy));
        if (dist <= 3.0f && ly >= 0 && ly < CHUNK_SIZE) {
          if (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE) {
            chunk_set_block(chunk, lx, ly, lz, BLOCK_LEAVES);
          }
        }
      }
    }
  }
}

// Generate tree based on type
void tree_generate(Chunk *chunk, i32 x, i32 y, i32 z, TreeType type, u32 seed) {
  switch (type) {
  case TREE_OAK:
    tree_generate_oak(chunk, x, y, z, seed);
    break;
  case TREE_BIRCH:
    tree_generate_birch(chunk, x, y, z, seed);
    break;
  case TREE_SPRUCE:
    tree_generate_spruce(chunk, x, y, z, seed);
    break;
  case TREE_RUBBER:
    tree_generate_rubber(chunk, x, y, z, seed);
    break;
  case TREE_JUNGLE:
    tree_generate_jungle(chunk, x, y, z, seed);
    break;
  default:
    tree_generate_oak(chunk, x, y, z, seed);
    break;
  }
}
