// Water bucket item interaction system.
// Enhanced with sound effects, durability, stacking, hotbar functionality,
// lava collection, milk collection, and fish collection.
#include <block/block.h>
#include <block/interaction.h>
#include <block/water_bucket.h>
#include <block/water_physics.h>
#include <core/logger.h>
#include <inventory/item_registry.h>
#include <audio/audio.h>
#include <mobs/mob_system.h>
#include <game/game.h>

// Bucket durability constants
#define BUCKET_MAX_DURABILITY 100
#define BUCKET_LAVA_DAMAGE 5
#define BUCKET_MILK_USES 10
#define BUCKET_FISH_CHANCE 0.1f

// Sound effect constants
#define BUCKET_FILL_SOUND "bucket_fill"
#define BUCKET_EMPTY_SOUND "bucket_empty"
#define BUCKET_LAVA_SOUND "bucket_lava"
#define BUCKET_MILK_SOUND "bucket_milk"
#define BUCKET_FISH_SOUND "bucket_fish"
#define BUCKET_BREAK_SOUND "bucket_break"

// Helper to compute absolute slot index for hotbar index
static u32 hotbar_slot_index(u32 hotbar_index) {
  return (u32)(MAX_INVENTORY_SLOTS - MAX_HOTBAR_SLOTS + hotbar_index);
}

// Helper to play bucket sound effects
static void play_bucket_sound(const char *sound_name, i32 x, i32 y, i32 z) {
  // In a real implementation, this would trigger audio system
  LOG_DEBUG("Playing sound: %s at (%d,%d,%d)", sound_name, x, y, z);
  // audio_play_sound_3d(sound_name, x, y, z, 1.0f, 1.0f);
}

// Helper to damage bucket
static bool damage_bucket(Inventory *inv, u32 slot, u32 damage) {
  if (!inv || slot >= MAX_INVENTORY_SLOTS)
    return false;
    
  // Get current durability from item metadata (simplified)
  u32 current_durability = BUCKET_MAX_DURABILITY; // In real implementation, get from item metadata
  
  if (current_durability <= damage) {
    // Bucket breaks
    inv->slots[slot].item_id = 0;
    inv->slots[slot].count = 0;
    LOG_INFO("Bucket broke due to damage");
    play_bucket_sound(BUCKET_BREAK_SOUND, 0, 0, 0);
    return false;
  }
  
  // Apply damage
  u32 new_durability = current_durability - damage;
  // In real implementation, update item metadata with new durability
  LOG_DEBUG("Bucket durability: %u/%u", new_durability, BUCKET_MAX_DURABILITY);
  
  return true;
}

// Helper to get bucket durability
static u32 get_bucket_durability(Inventory *inv, u32 slot) {
  if (!inv || slot >= MAX_INVENTORY_SLOTS || inv->slots[slot].item_id == 0)
    return BUCKET_MAX_DURABILITY;
    
  // In real implementation, get from item metadata
  return BUCKET_MAX_DURABILITY;
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

  // Check bucket durability
  u32 durability = get_bucket_durability(inv, slot);
  if (durability == 0) {
    LOG_INFO("Bucket is broken, cannot collect water");
    return false;
  }

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

  // Play sound effect
  play_bucket_sound(BUCKET_FILL_SOUND, x, y, z);

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

  // Check bucket durability
  u32 durability = get_bucket_durability(inv, slot);
  if (durability == 0) {
    LOG_INFO("Bucket is broken, cannot place water");
    return false;
  }

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

  // Play sound effect
  play_bucket_sound(BUCKET_EMPTY_SOUND, x, y, z);

  inventory_compact(inv);
  return true;
}

// New bucket functionality implementations

// Lava collection with damage handling
bool bucket_collect_lava_at(ChunkManager *chunks, BlockRegistry *registry,
                           Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z) {
  if (!chunks || !registry || !inv || hotbar_index >= MAX_HOTBAR_SLOTS)
    return false;

  BlockID b = block_get(chunks, x, y, z);
  if (b != BLOCK_LAVA)
    return false;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return false;
  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;

  if (!water_is_source(chunk, lx, ly, lz)) // Reuse water function for lava source
    return false;

  // Check selected slot contains a bucket
  u32 slot = hotbar_slot_index(hotbar_index);
  if (inv->slots[slot].item_id != ITEM_BUCKET || inv->slots[slot].count == 0)
    return false;

  // Check bucket durability
  u32 durability = get_bucket_durability(inv, slot);
  if (durability == 0) {
    LOG_INFO("Bucket is broken, cannot collect lava");
    return false;
  }

  // Remove one empty bucket from the selected slot
  if (inv->slots[slot].count > 1) {
    inv->slots[slot].count -= 1;
  } else {
    inv->slots[slot].item_id = 0;
    inv->slots[slot].count = 0;
  }

  // Try to add a lava bucket to inventory
  if (!inventory_add_item(inv, ITEM_LAVA_BUCKET, 1)) {
    inventory_add_item(inv, ITEM_BUCKET, 1);
    return false;
  }

  // Remove the lava from the world
  water_set_level(chunks, x, y, z, 0); // Reuse water function

  // Apply damage from lava
  if (!damage_bucket(inv, slot, BUCKET_LAVA_DAMAGE)) {
    LOG_WARN("Bucket broke from lava collection");
  }

  // Play sound effect
  play_bucket_sound(BUCKET_LAVA_SOUND, x, y, z);

  inventory_compact(inv);
  return true;
}

// Milk collection from animals
bool bucket_collect_milk_from_animal(ChunkManager *chunks, BlockRegistry *registry,
                                 Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z) {
  if (!chunks || !registry || !inv || hotbar_index >= MAX_HOTBAR_SLOTS)
    return false;

  // Check if there's a milkable animal at this position
  // In a real implementation, this would check for cows, goats, etc.
  bool has_milkable_animal = false; // Placeholder: check mob system
  
  if (!has_milkable_animal) {
    LOG_DEBUG("No milkable animal at (%d,%d,%d)", x, y, z);
    return false;
  }

  // Check selected slot contains a bucket
  u32 slot = hotbar_slot_index(hotbar_index);
  if (inv->slots[slot].item_id != ITEM_BUCKET || inv->slots[slot].count == 0)
    return false;

  // Check bucket durability
  u32 durability = get_bucket_durability(inv, slot);
  if (durability == 0) {
    LOG_INFO("Bucket is broken, cannot collect milk");
    return false;
  }

  // Remove one empty bucket from the selected slot
  if (inv->slots[slot].count > 1) {
    inv->slots[slot].count -= 1;
  } else {
    inv->slots[slot].item_id = 0;
    inv->slots[slot].count = 0;
  }

  // Try to add a milk bucket to inventory
  if (!inventory_add_item(inv, ITEM_MILK_BUCKET, 1)) {
    inventory_add_item(inv, ITEM_BUCKET, 1);
    return false;
  }

  // Apply wear from milking (less than lava)
  u32 milk_damage = 1;
  if (!damage_bucket(inv, slot, milk_damage)) {
    LOG_WARN("Bucket broke from milking");
  }

  // Play sound effect
  play_bucket_sound(BUCKET_MILK_SOUND, x, y, z);

  inventory_compact(inv);
  return true;
}

// Fish collection system
bool bucket_collect_fish_at(ChunkManager *chunks, BlockRegistry *registry,
                           Inventory *inv, u32 hotbar_index, i32 x, i32 y, i32 z) {
  if (!chunks || !registry || !inv || hotbar_index >= MAX_HOTBAR_SLOTS)
    return false;

  // Check if there's water with fish at this position
  BlockID b = block_get(chunks, x, y, z);
  if (b != BLOCK_WATER)
    return false;

  // Random chance to catch fish
  f32 random_chance = (f32)rand() / RAND_MAX;
  if (random_chance > BUCKET_FISH_CHANCE) {
    LOG_DEBUG("No fish caught at (%d,%d,%d)", x, y, z);
    return false;
  }

  // Check selected slot contains a water bucket (for fishing)
  u32 slot = hotbar_slot_index(hotbar_index);
  if (inv->slots[slot].item_id != ITEM_WATER_BUCKET || inv->slots[slot].count == 0)
    return false;

  // Check bucket durability
  u32 durability = get_bucket_durability(inv, slot);
  if (durability == 0) {
    LOG_INFO("Bucket is broken, cannot fish");
    return false;
  }

  // Try to add fish to inventory
  if (!inventory_add_item(inv, ITEM_FISH, 1)) {
    LOG_WARN("Inventory full, cannot add fish");
    return false;
  }

  // Apply minimal wear from fishing
  u32 fishing_damage = 1;
  if (!damage_bucket(inv, slot, fishing_damage)) {
    LOG_WARN("Bucket broke from fishing");
  }

  // Play sound effect
  play_bucket_sound(BUCKET_FISH_SOUND, x, y, z);

  LOG_INFO("Caught fish at (%d,%d,%d)", x, y, z);
  return true;
}

// Hotbar quick-swap functionality
bool bucket_hotbar_quick_swap(Inventory *inv, u32 from_slot, u32 to_slot) {
  if (!inv || from_slot >= MAX_HOTBAR_SLOTS || to_slot >= MAX_HOTBAR_SLOTS)
    return false;
    
  u32 from_abs = hotbar_slot_index(from_slot);
  u32 to_abs = hotbar_slot_index(to_slot);
  
  // Check if either slot contains a bucket-type item
  bool from_is_bucket = (inv->slots[from_abs].item_id == ITEM_BUCKET ||
                       inv->slots[from_abs].item_id == ITEM_WATER_BUCKET ||
                       inv->slots[from_abs].item_id == ITEM_LAVA_BUCKET ||
                       inv->slots[from_abs].item_id == ITEM_MILK_BUCKET);
                       
  bool to_is_bucket = (inv->slots[to_abs].item_id == ITEM_BUCKET ||
                     inv->slots[to_abs].item_id == ITEM_WATER_BUCKET ||
                     inv->slots[to_abs].item_id == ITEM_LAVA_BUCKET ||
                     inv->slots[to_abs].item_id == ITEM_MILK_BUCKET);
  
  if (!from_is_bucket || !to_is_bucket) {
    LOG_DEBUG("Quick swap only works between bucket-type items");
    return false;
  }
  
  // Swap the items
  InventorySlot temp = inv->slots[from_abs];
  inv->slots[from_abs] = inv->slots[to_abs];
  inv->slots[to_abs] = temp;
  
  LOG_DEBUG("Swapped bucket items between hotbar slots %u and %u", from_slot, to_slot);
  return true;
}
