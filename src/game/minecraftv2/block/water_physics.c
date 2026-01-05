#include <block/block.h>
#include <block/water_physics.h>
#include <chunk/chunk.h>
#include <stdlib.h>

void water_physics_block_init(void) {
  // any initialization if needed
}

u8 water_get_level(Chunk *chunk, i32 x, i32 y, i32 z) {
  BlockID block = chunk_get_block(chunk, x, y, z);
  if (block != BLOCK_WATER) {
    return 0;
  }
  u8 meta = chunk_get_metadata(chunk, x, y, z);
  return meta & WATER_META_LEVEL_MASK;
}

void water_set_level(ChunkManager *manager, i32 x, i32 y, i32 z, u8 level) {
  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(manager, cp);
  if (!chunk)
    return;

  // Local coordinates
  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;

  if (level == 0) {
    // Remove block and clear metadata
    chunk_set_block(chunk, lx, ly, lz, BLOCK_AIR);
    chunk_set_metadata(chunk, lx, ly, lz, 0);
  } else {
    // Preserve source flag if present
    u8 old_meta = chunk_get_metadata(chunk, lx, ly, lz);
    u8 source_bit = old_meta & WATER_META_SOURCE_FLAG;
    u8 new_meta = (level & WATER_META_LEVEL_MASK) | source_bit;
    chunk_set_block(chunk, lx, ly, lz, BLOCK_WATER);
    chunk_set_metadata(chunk, lx, ly, lz, new_meta);
  }
}

static bool is_solid(BlockID block) {
  return block != BLOCK_AIR && block != BLOCK_WATER &&
         block != BLOCK_TALL_GRASS && block != BLOCK_SHORT_GRASS;
  // Add more non-solid blocks as needed, or use a proper property check
}

void water_set_source(ChunkManager *manager, i32 x, i32 y, i32 z,
                      bool is_source) {
  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(manager, cp);
  if (!chunk)
    return;
  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;

  u8 meta = chunk_get_metadata(chunk, lx, ly, lz);
  if (is_source)
    meta |= WATER_META_SOURCE_FLAG;
  else
    meta &= ~WATER_META_SOURCE_FLAG;
  // Ensure block is water if setting source
  if (chunk_get_block(chunk, lx, ly, lz) != BLOCK_WATER) {
    chunk_set_block(chunk, lx, ly, lz, BLOCK_WATER);
    if ((meta & WATER_META_LEVEL_MASK) == 0)
      meta = WATER_LEVEL_SOURCE | (meta & WATER_META_SOURCE_FLAG);
  }
  chunk_set_metadata(chunk, lx, ly, lz, meta);
}

bool water_is_source(Chunk *chunk, i32 x, i32 y, i32 z) {
  if (!chunk)
    return false;
  u8 meta = chunk_get_metadata(chunk, x, y, z);
  return (meta & WATER_META_SOURCE_FLAG) != 0;
}

float water_wave_phase(i32 x, i32 y, i32 z, double time_seconds) {
  // Simple deterministic pseudo-wave using sin based on world coords and time
  double phase = sin(time_seconds * 2.0 + (x * 31.0 + z * 17.0) * 0.1);
  // Small y-based attenuation
  phase *= 1.0 / (1.0 + fabs((double)y) * 0.05);
  return (float)phase; // -1..1
}

bool water_update_tick(ChunkManager *manager, i32 x, i32 y, i32 z) {
  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(manager, cp); // Start with read access
  if (!chunk)
    return false;

  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;

  BlockID current_block = chunk_get_block(chunk, lx, ly, lz);
  if (current_block != BLOCK_WATER)
    return false;

  u8 meta = chunk_get_metadata(chunk, lx, ly, lz);
  u8 current_level = meta & WATER_META_LEVEL_MASK;
  bool is_source = (meta & WATER_META_SOURCE_FLAG) != 0;

  // Step 1: Handle Downward Flow (always allowed)
  BlockID block_below = chunk_get_block(chunk, lx, ly - 1, lz);
  bool changed = false;

  if (block_below == BLOCK_AIR ||
      (block_below == BLOCK_WATER &&
       water_get_level(chunk, lx, ly - 1, lz) < WATER_LEVEL_SOURCE)) {
    // Flow down - produce a full-level (source-equivalent) block below
    // but do NOT mark the below as a source (unless original is source and we
    // want to siphon from a source). We set a full level block.
    water_set_level(manager, x, y - 1, z, WATER_LEVEL_SOURCE);
    changed = true;
  }

  // Step 2: Horizontal flow and decay only if on a supporting surface
  if (current_level > 1) {
    if (block_below == BLOCK_WATER || is_solid(block_below) ||
        (block_below == BLOCK_WATER &&
         water_get_level(chunk, lx, ly - 1, lz) == WATER_LEVEL_SOURCE)) {
      const i32 coords[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
      u8 spread_level = current_level - 1;

      for (int i = 0; i < 4; i++) {
        i32 nx = x + coords[i][0];
        i32 nz = z + coords[i][1];

        ChunkPos ncp = world_to_chunk_pos(nx, y, nz);
        Chunk *nchunk = chunk_manager_get(manager, ncp);
        i32 nlx = nx - ncp.x * CHUNK_SIZE;
        i32 nly = y - ncp.y * CHUNK_SIZE;
        i32 nlz = nz - ncp.z * CHUNK_SIZE;

        BlockID target = chunk_get_block(nchunk, nlx, nly, nlz);

        if (target == BLOCK_AIR) {
          water_set_level(manager, nx, y, nz, spread_level);
          changed = true;
        } else if (target == BLOCK_WATER) {
          u8 target_level =
              chunk_get_metadata(nchunk, nlx, nly, nlz) & WATER_META_LEVEL_MASK;
          bool target_is_source = (chunk_get_metadata(nchunk, nlx, nly, nlz) &
                                   WATER_META_SOURCE_FLAG) != 0;
          if (target_level < spread_level && !target_is_source) {
            water_set_level(manager, nx, y, nz, spread_level);
            changed = true;
          }
        } else if (!is_solid(target)) {
          // Wash away non-solids like grass
          water_set_level(manager, nx, y, nz, spread_level);
          changed = true;
        }
      }
    }
  }

  // Step 3: Decay - only flowing water decays based on neighbor levels
  if (!is_source) {
    // Compute desired level: find max neighbor level (excluding below) minus 1
    u8 best = 0;
    const i32 coords[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int i = 0; i < 4; i++) {
      i32 nx = x + coords[i][0];
      i32 nz = z + coords[i][1];
      ChunkPos ncp = world_to_chunk_pos(nx, y, nz);
      Chunk *nchunk = chunk_manager_get(manager, ncp);
      if (!nchunk)
        continue;
      i32 nlx = nx - ncp.x * CHUNK_SIZE;
      i32 nly = y - ncp.y * CHUNK_SIZE;
      i32 nlz = nz - ncp.z * CHUNK_SIZE;
      if (chunk_get_block(nchunk, nlx, nly, nlz) == BLOCK_WATER) {
        u8 nl =
            chunk_get_metadata(nchunk, nlx, nly, nlz) & WATER_META_LEVEL_MASK;
        if (nl > best)
          best = nl;
      }
    }

    u8 desired = 0;
    if (best > 0) {
      desired = (best > 1) ? best - 1 : WATER_LEVEL_MIN;
    } else {
      // No neighbors with water - decay
      desired = current_level > 1 ? current_level - 1 : 0;
    }

    if (desired == 0) {
      // Remove the flowing water
      water_set_level(manager, x, y, z, 0);
      changed = true;
    } else if (desired != current_level) {
      water_set_level(manager, x, y, z, desired);
      changed = true;
    }
  }

  return changed;
}
