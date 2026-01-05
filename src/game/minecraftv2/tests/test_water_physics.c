#include <stdio.h>
#include <stdlib.h>
#include "../include/chunk/chunk.h>
#include "../include/block/water_physics.h>
#include "../include/block/block.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

int main(void) {
    ChunkManager manager;
    chunk_manager_init(&manager, 8);

    // Create chunk at non-zero position to avoid pending ticket false-positives
    ChunkPos pos = {1,0,0};
    Chunk *chunk = chunk_manager_get_or_create(&manager, pos);
    if (!chunk) fail("Failed to create chunk");

    // Local coords used for chunk-level operations
    i32 lx = 1, ly = 1, lz = 1;
    i32 world_x = pos.x * CHUNK_SIZE + lx;
    i32 world_y = ly;
    i32 world_z = lz;

    // Make sure below block is solid ground at local y=0
    chunk_set_block(chunk, lx, 0, lz, BLOCK_STONE); // ground at y=0

    // Place a source water block at world coordinates (world_x, world_y, world_z)
    water_set_level(&manager, world_x, world_y, world_z, WATER_LEVEL_SOURCE);

    // Verify initial state (local chunk queries)
    BlockID b = chunk_get_block(chunk, lx, ly, lz);
    if (b != BLOCK_WATER) fail("Water not placed at source position");

    u8 level = chunk_get_metadata(chunk, lx, ly, lz) & WATER_META_LEVEL_MASK;
    if (level != WATER_LEVEL_SOURCE) fail("Source level wrong");

    // Mark this block explicitly as a source and verify
    water_set_source(&manager, world_x, world_y, world_z, true);
    if (!water_is_source(chunk, lx, ly, lz)) fail("Block not marked as source");

    // Update tick should cause horizontal spread since below is solid
    bool changed = water_update_tick(&manager, world_x, world_y, world_z);
    if (!changed) fail("water_update_tick reported no change on source");

    // Check that one of the horizontal neighbors got water with level 6 (and not marked source)
    BlockID nb = chunk_get_block(chunk, lx + 1, ly, lz);
    u8 nlevel = chunk_get_metadata(chunk, lx + 1, ly, lz) & WATER_META_LEVEL_MASK;
    u8 nb_meta = chunk_get_metadata(chunk, lx + 1, ly, lz);

    if (nb != BLOCK_WATER) fail("Neighbor did not become water");
    if (nlevel != WATER_LEVEL_SOURCE - 1) fail("Neighbor level wrong");
    if (nb_meta & WATER_META_SOURCE_FLAG) fail("Neighbor incorrectly marked as source");

    // A source should not decay even if isolated - remove neighbors and update
    // Clear neighbors
    chunk_set_block(chunk, lx + 1, ly, lz, BLOCK_AIR);
    chunk_set_block(chunk, lx - 1, ly, lz, BLOCK_AIR);
    chunk_set_block(chunk, lx, ly, lz + 1, BLOCK_AIR);
    chunk_set_block(chunk, lx, ly, lz - 1, BLOCK_AIR);

    // Update tick: sources will still spread to adjacent air blocks, but should not decay.
    bool changed2 = water_update_tick(&manager, world_x, world_y, world_z);
    // Verify the source remains and level unchanged
    u8 after_level = chunk_get_metadata(chunk, lx, ly, lz) & WATER_META_LEVEL_MASK;
    if (!water_is_source(chunk, lx, ly, lz)) fail("Source flag lost after update");
    if (after_level != WATER_LEVEL_SOURCE) fail("Source level changed unexpectedly");

    // Unmark source; it should now decay when updated repeatedly
    water_set_source(&manager, world_x, world_y, world_z, false);
    if (water_is_source(chunk, lx, ly, lz)) fail("Source flag still set");

    bool decayed = false;
    for (int i = 0; i < 3; i++) {
        if (water_update_tick(&manager, world_x, world_y, world_z)) decayed = true;
    }
    if (!decayed) fail("Flowing water did not decay after removing source flag");

    // Test downward flow: place water above empty space
    // Clear previous setup and use world coords
    chunk_set_block(chunk, lx, ly + 1, lz, BLOCK_WATER);
    chunk_set_metadata(chunk, lx, ly + 1, lz, WATER_LEVEL_SOURCE);
    chunk_set_block(chunk, lx, ly, lz, BLOCK_AIR);

    water_set_level(&manager, world_x, world_y + 1, world_z, WATER_LEVEL_SOURCE);
    changed = water_update_tick(&manager, world_x, world_y + 1, world_z);
    if (!changed) fail("water_update_tick did not indicate downflow");

    BlockID below = chunk_get_block(chunk, lx, ly, lz);
    u8 below_level = chunk_get_metadata(chunk, lx, ly, lz);
    if (below != BLOCK_WATER) fail("Water did not flow down");
    if (below_level != WATER_LEVEL_SOURCE) fail("Downflow level not source");

    // Test removal: set level 0 removes block
    water_set_level(&manager, world_x, world_y, world_z, 0);
    // Debug: print block and metadata after removal
    BlockID after = chunk_get_block(chunk, lx, ly, lz);
    u8 after_meta = chunk_get_metadata(chunk, lx, ly, lz);
    printf("Debug: after remove block=%d meta=%u\n", (int)after, after_meta);
    if (after != BLOCK_AIR) fail("Water not removed on level 0");

    chunk_manager_free(&manager);

    printf("All water physics tests passed.\n");
    return 0;
}
