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

// Enhanced bucket functionality

// Lava collection with damage handling
bool bucket_collect_lava_at(ChunkManager *chunks, BlockRegistry *registry,
                           Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z);

// Milk collection from animals
bool bucket_collect_milk_from_animal(ChunkManager *chunks, BlockRegistry *registry,
                                 Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z);

// Fish collection system
bool bucket_collect_fish_at(ChunkManager *chunks, BlockRegistry *registry,
                           Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z);

// Hotbar quick-swap functionality
bool bucket_hotbar_quick_swap(Inventory *inv, u32 from_slot, u32 to_slot);

#endif // WATER_BUCKET_H
