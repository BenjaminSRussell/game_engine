#ifndef WATER_PHYSICS_H
#define WATER_PHYSICS_H

#include "../chunk/chunk.h"
#include "block.h"

#define WATER_LEVEL_SOURCE 7
#define WATER_LEVEL_MIN 1
#define WATER_LEVEL_MAX 7

// Metadata bit layout in Chunk.metadata:
// bits 0-2: level (1-7)
// bit 7: source flag (0 = flowing, 1 = source)
#define WATER_META_LEVEL_MASK 0x07
#define WATER_META_SOURCE_FLAG 0x80

// Initialize the water physics system
void water_physics_init(void);

// Update a water block at the given coordinates
// Returns true if the block state changed
bool water_update_tick(ChunkManager *manager, i32 x, i32 y, i32 z);

// Get the water level of a block (0 if not water)
u8 water_get_level(Chunk *chunk, i32 x, i32 y, i32 z);

// Set the water level of a block
void water_set_level(ChunkManager *manager, i32 x, i32 y, i32 z, u8 level);

// Mark/unmark a water block as a source (sources do not decay)
void water_set_source(ChunkManager *manager, i32 x, i32 y, i32 z, bool is_source);

// Query whether a block is a source
bool water_is_source(Chunk *chunk, i32 x, i32 y, i32 z);

// Query a simple deterministic wave phase for water rendering
// time_seconds is a global time to animate waves; returns -1.0..1.0 float
float water_wave_phase(i32 x, i32 y, i32 z, double time_seconds);

#endif // WATER_PHYSICS_H
