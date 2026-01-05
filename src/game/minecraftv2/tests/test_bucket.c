#include <stdio.h>
#include <stdlib.h>
#include "../include/chunk/chunk.h>
#include "../include/block/water_physics.h>
#include "../include/block/block.h>
#include "../include/block/water_bucket.h>
#include "../include/inventory/inventory.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

int main(void) {
    ChunkManager manager;
    chunk_manager_init(&manager, 8);

    BlockRegistry registry;
    block_registry_init(&registry, 256);
    block_registry_init_defaults(&registry);

    // Create chunk
    ChunkPos pos = {1,0,0};
    Chunk *chunk = chunk_manager_get_or_create(&manager, pos);
    if (!chunk) fail("Failed to create chunk");

    i32 lx = 1, ly = 2, lz = 1;
    i32 world_x = pos.x * CHUNK_SIZE + lx;
    i32 world_y = ly;
    i32 world_z = lz;

    // Ensure ground below for placement
    chunk_set_block(chunk, lx, 0, lz, BLOCK_STONE);

    // Place a source water block
    water_set_level(&manager, world_x, world_y, world_z, WATER_LEVEL_SOURCE);
    water_set_source(&manager, world_x, world_y, world_z, true);

    // Setup inventory with an empty bucket in selected hotbar slot 0
    Inventory inv;
    inventory_init(&inv);
    inv.selected_hotbar = 0;
    u32 hotbar_start = MAX_INVENTORY_SLOTS - MAX_HOTBAR_SLOTS;
    inventory_set_slot(&inv, hotbar_start + 0, ITEM_BUCKET, 1);

    // Collect water
    if (!bucket_collect_at(&manager, &registry, &inv, 0, world_x, world_y, world_z))
        fail("bucket_collect_at failed to collect water");

    // Verify water removed
    BlockID after = chunk_get_block(chunk, lx, ly, lz);
    if (after != BLOCK_AIR) fail("Water not removed after collecting");

    // Verify we now have a water bucket
    u16 water_bucket_count = inventory_get_item_count(&inv, ITEM_WATER_BUCKET);
    if (water_bucket_count != 1) fail("Inventory did not contain water bucket after collect");

    // Now test placing water: put a stone below and ensure empty place above
    i32 place_x = world_x;
    i32 place_y = world_y; // put water back at same y
    i32 place_z = world_z + 2; // choose an adjacent location

    // Ensure block below is solid
    ChunkPos place_cp = world_to_chunk_pos(place_x, place_y - 1, place_z);
    Chunk *place_chunk = chunk_manager_get_or_create(&manager, place_cp);
    chunk_set_block(place_chunk, place_x - place_cp.x * CHUNK_SIZE, 0, place_z - place_cp.z * CHUNK_SIZE, BLOCK_STONE);

    // Put a water bucket in selected slot
    // Clear inventory and set selected slot
    inventory_clear(&inv);
    inventory_set_slot(&inv, hotbar_start + 0, ITEM_WATER_BUCKET, 1);

    if (!bucket_place_at(&manager, &registry, &inv, 0, place_x, place_y, place_z))
        fail("bucket_place_at failed to place water");

    // Verify water placed
    ChunkPos cp2 = world_to_chunk_pos(place_x, place_y, place_z);
    Chunk *c2 = chunk_manager_get(&manager, cp2);
    if (!c2) fail("Chunk missing after place");
    BlockID placed = chunk_get_block(c2, place_x - cp2.x * CHUNK_SIZE, place_y - cp2.y * CHUNK_SIZE, place_z - cp2.z * CHUNK_SIZE);
    if (placed != BLOCK_WATER) fail("Water block not placed");

    // Verify we got an empty bucket back
    u16 empty_bucket_count = inventory_get_item_count(&inv, ITEM_BUCKET);
    if (empty_bucket_count != 1) fail("Empty bucket not returned after placing water");

    chunk_manager_free(&manager);

    printf("PASS: bucket collect/place tests succeeded\n");
    return 0;
}
