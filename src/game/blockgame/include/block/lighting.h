#ifndef BLOCK_LIGHTING_H
#define BLOCK_LIGHTING_H

#include "../block/block.h"
#include "../chunk/chunk.h"
#include "../game_common.h"

// LightType is already defined in chunk.h

// Process the lighting propagation queue (call once per frame)
void lighting_process_queue(ChunkManager *manager, BlockRegistry *registry);

// Propagate light from a source block
void block_propagate_light(ChunkManager *manager, BlockRegistry *registry,
                           i32 x, i32 y, i32 z, u8 light_level, u32 light_type);

// Calculate light level at a position
u8 block_calculate_light_level(ChunkManager *chunks, BlockRegistry *registry,
                               i32 x, i32 y, i32 z);

// Get light value at a position
u8 chunk_get_light(Chunk *chunk, i32 x, i32 y, i32 z, ChunkLightType type);

#endif // BLOCK_LIGHTING_H
