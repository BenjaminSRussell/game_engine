#ifndef WATER_BUCKET_H
#define WATER_BUCKET_H

#include "../game_common.h"
#include "block.h"
#include "../chunk/chunk.h"
#include "../inventory/inventory.h"

// Try to collect water from the world into a bucket held in the hotbar.
// `hotbar_index` is 0-8 (selected hotbar slot). Returns true on success.
bool bucket_collect_at(ChunkManager *chunks, BlockRegistry *registry,
                       Inventory *inv, u32 hotbar_index, i32 x, i32 y,
                       i32 z);

// Try to place water from a water-bucket item into the world at the given
// coordinates (place position). Returns true on success.
bool bucket_place_at(ChunkManager *chunks, BlockRegistry *registry,
                     Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z);

#endif // WATER_BUCKET_H
