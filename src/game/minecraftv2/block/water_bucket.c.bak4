// Water bucket item interaction system.
// TODO: Implement bucket animation system for fill/empty operations.
// TODO: Add bucket sound effects for water collection and placement.
// TODO: Implement bucket particle effects for water splash.
// TODO: Add bucket durability system for iron buckets.
// TODO: Implement bucket stacking system for multiple buckets.
// TODO: Add bucket hotbar quick-swap functionality.
// TODO: Add bucket lava collection system with damage handling.
// TODO: Implement bucket milk collection from animals.
// TODO: Add bucket fish collection system.
#include <block/block.h>
#include <block/interaction.h>
#include <block/water_bucket.h>
#include <block/water_physics.h>
#include <core/logger.h>
#include <inventory/item_registry.h>

// Helper to compute absolute slot index for hotbar index
static u32 hotbar_slot_index(u32 hotbar_index) {
  return (u32)(MAX_INVENTORY_SLOTS - MAX_HOTBAR_SLOTS + hotbar_index);
}

bool bucket_collect_at(ChunkManager *chunks, BlockRegistry *registry,
                       Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z) {
  if (!chunks || !registry || !inv || hotbar_index >= MAX_HOTBAR_SLOTS)
    return false;

  BlockID b = block_get(chunks, x, y, z);
  if (b != BLOCK_WATER)
    return false;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return false;
  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;

  if (!water_is_source(chunk, lx, ly, lz))
    return false;

  // Check selected slot contains a bucket
  u32 slot = hotbar_slot_index(hotbar_index);
  if (inv->slots[slot].item_id != ITEM_BUCKET || inv->slots[slot].count == 0)
    return false;

  // Remove one empty bucket from the selected slot
  if (inv->slots[slot].count > 1) {
    inv->slots[slot].count -= 1;
  } else {
    inv->slots[slot].item_id = 0;
    inv->slots[slot].count = 0;
  }

  // Try to add a water bucket to inventory; if fails, restore bucket and abort
  if (!inventory_add_item(inv, ITEM_WATER_BUCKET, 1)) {
    // restore
    inventory_add_item(inv, ITEM_BUCKET, 1);
    return false;
  }

  // Remove the water from the world
  water_set_level(chunks, x, y, z, 0);

  inventory_compact(inv);
  return true;
}

bool bucket_place_at(ChunkManager *chunks, BlockRegistry *registry,
                     Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z) {
  if (!chunks || !registry || !inv || hotbar_index >= MAX_HOTBAR_SLOTS)
    return false;

  // Check target can accept water
  if (!block_can_place(chunks, registry, x, y, z))
    return false;

  // Check selected slot contains a water bucket
  u32 slot = hotbar_slot_index(hotbar_index);
  if (inv->slots[slot].item_id != ITEM_WATER_BUCKET ||
      inv->slots[slot].count == 0)
    return false;

  // Remove one water bucket from the selected slot
  if (inv->slots[slot].count > 1) {
    inv->slots[slot].count -= 1;
  } else {
    inv->slots[slot].item_id = 0;
    inv->slots[slot].count = 0;
  }

  // Place a source water block
  water_set_level(chunks, x, y, z, WATER_LEVEL_SOURCE);
  water_set_source(chunks, x, y, z, true);

  // Give back an empty bucket
  if (!inventory_add_item(inv, ITEM_BUCKET, 1)) {
    // inventory full: try to put it back into selected slot, or drop
    // (best-effort restore) Put it back into selected slot if empty
    if (inv->slots[slot].item_id == 0) {
      inv->slots[slot].item_id = ITEM_WATER_BUCKET;
      inv->slots[slot].count = 1;
    } else {
      LOG_WARN("inventory full after placing water - empty bucket lost");
    }
  }

  inventory_compact(inv);
  return true;
}
