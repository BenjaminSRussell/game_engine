// Rail placement and update logic.
// Roadmap: docs/RAIL_SYSTEM_ROADMAP.md.
#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <string.h>
#include <world/rail_system.h>

// Rail block IDs (would be defined in block registry)
#define BLOCK_RAIL 50
#define BLOCK_POWERED_RAIL 51
#define BLOCK_DETECTOR_RAIL 52

// RailType, RailSegment, and RailNetwork are all defined in rail_system.h

static RailNetwork g_rail_network = {0};

// Initialize rail system
void rail_system_init(RailNetwork *network, u32 max_segments) {
  if (!network)
    return;
  memset(network, 0, sizeof(RailNetwork));
  network->segment_capacity = max_segments;
  network->segments = (RailSegment *)calloc(max_segments, sizeof(RailSegment));
  network->segment_count = 0;
}

// Place rail segment
bool rail_place_segment(ChunkManager *chunks, i32 x, i32 y, i32 z,
                        RailType type, bool powered) {
  if (!chunks)
    return false;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get_or_create(chunks, cp);
  if (!chunk)
    return false;

  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;

  if (lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < CHUNK_SIZE && lz >= 0 &&
      lz < CHUNK_SIZE) {

    BlockID rail_block = powered ? BLOCK_POWERED_RAIL : BLOCK_RAIL;
    chunk_set_block(chunk, lx, ly, lz, rail_block);

    // Add to network
    if (g_rail_network.segment_count < g_rail_network.segment_capacity) {
      RailSegment *seg =
          &g_rail_network.segments[g_rail_network.segment_count++];
      seg->x = x;
      seg->y = y;
      seg->z = z;
      seg->type = type;
      seg->powered = powered;
    }

    return true;
  }

  return false;
}

// Generate rail network between two points
bool rail_generate_network(ChunkManager *chunks, i32 start_x, i32 start_y,
                           i32 start_z, i32 end_x, i32 end_y, i32 end_z) {
  if (!chunks)
    return false;

  // Simple straight-line rail generation
  i32 dx = end_x - start_x;
  i32 dz = end_z - start_z;
  i32 dy = end_y - start_y;

  i32 steps = abs(dx) + abs(dz) + abs(dy);
  if (steps == 0)
    return false;

  for (i32 step = 0; step <= steps; step++) {
    f32 t = (f32)step / (f32)steps;

    i32 x = start_x + (i32)(dx * t);
    i32 y = start_y + (i32)(dy * t);
    i32 z = start_z + (i32)(dz * t);

    // Determine rail type
    RailType type = RAIL_STRAIGHT_NS;
    if (abs(dx) > abs(dz)) {
      type = RAIL_STRAIGHT_EW;
    }

    // Place rail
    rail_place_segment(chunks, x, y, z, type, false);

    // Place support blocks below
    ChunkPos cp = world_to_chunk_pos(x, y - 1, z);
    Chunk *chunk = chunk_manager_get_or_create(chunks, cp);
    if (chunk) {
      i32 lx = x - cp.x * CHUNK_SIZE;
      i32 ly = (y - 1) - cp.y * CHUNK_SIZE;
      i32 lz = z - cp.z * CHUNK_SIZE;

      if (lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < CHUNK_SIZE && lz >= 0 &&
          lz < CHUNK_SIZE) {
        BlockID block = chunk_get_block(chunk, lx, ly, lz);
        if (block == BLOCK_AIR) {
          chunk_set_block(chunk, lx, ly, lz, BLOCK_COBBLESTONE);
        }
      }
    }
  }

  return true;
}

// Find rail segment at position
RailSegment *rail_find_segment(RailNetwork *network, i32 x, i32 y, i32 z) {
  if (!network)
    return NULL;

  for (u32 i = 0; i < network->segment_count; i++) {
    RailSegment *seg = &network->segments[i];
    if (seg->x == x && seg->y == y && seg->z == z) {
      return seg;
    }
  }

  return NULL;
}

// Get connected rails
u32 rail_get_connected(RailNetwork *network, i32 x, i32 y, i32 z,
                       RailSegment **out_segments, u32 max_count) {
  if (!network || !out_segments)
    return 0;

  RailSegment *current = rail_find_segment(network, x, y, z);
  if (!current)
    return 0;

  u32 count = 0;

  // Check adjacent positions based on rail type
  switch (current->type) {
  case RAIL_STRAIGHT_NS:
    // Check north and south
    if (count < max_count) {
      RailSegment *north = rail_find_segment(network, x, y, z - 1);
      if (north)
        out_segments[count++] = north;
    }
    if (count < max_count) {
      RailSegment *south = rail_find_segment(network, x, y, z + 1);
      if (south)
        out_segments[count++] = south;
    }
    break;

  case RAIL_STRAIGHT_EW:
    // Check east and west
    if (count < max_count) {
      RailSegment *east = rail_find_segment(network, x + 1, y, z);
      if (east)
        out_segments[count++] = east;
    }
    if (count < max_count) {
      RailSegment *west = rail_find_segment(network, x - 1, y, z);
      if (west)
        out_segments[count++] = west;
    }
    break;

  case RAIL_CURVE_NE:
    // Check north and east
    if (count < max_count) {
      RailSegment *north = rail_find_segment(network, x, y, z - 1);
      if (north)
        out_segments[count++] = north;
    }
    if (count < max_count) {
      RailSegment *east = rail_find_segment(network, x + 1, y, z);
      if (east)
        out_segments[count++] = east;
    }
    break;

  // Other curve types similar...
  default:
    // Check all 4 directions
    if (count < max_count) {
      RailSegment *north = rail_find_segment(network, x, y, z - 1);
      if (north)
        out_segments[count++] = north;
    }
    if (count < max_count) {
      RailSegment *south = rail_find_segment(network, x, y, z + 1);
      if (south)
        out_segments[count++] = south;
    }
    if (count < max_count) {
      RailSegment *east = rail_find_segment(network, x + 1, y, z);
      if (east)
        out_segments[count++] = east;
    }
    if (count < max_count) {
      RailSegment *west = rail_find_segment(network, x - 1, y, z);
      if (west)
        out_segments[count++] = west;
    }
    break;
  }

  return count;
}

// Free rail system
void rail_system_free(RailNetwork *network) {
  if (!network)
    return;
  if (network->segments) {
    free(network->segments);
    network->segments = NULL;
  }
  network->segment_count = 0;
  network->segment_capacity = 0;
}
