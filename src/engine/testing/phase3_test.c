#include "include/gameplay/crafting/crafting.h"
#include "include/gameplay/inventory/item_database.h"
#include "include/gameplay/inventory/inventory.h"
#include "include/core/logger.h"
#include <stdio.h>
#include <assert.h>

// Mock item IDs
#define ITEM_WOOD 1
#define ITEM_PLANKS 2
#define ITEM_TABLE 3

void register_test_items() {
    Item wood = { .id = ITEM_WOOD, .name = "Wood", .max_stack_size = 64, .flags = ITEM_FLAG_STACKABLE };
    Item planks = { .id = ITEM_PLANKS, .name = "Planks", .max_stack_size = 64, .flags = ITEM_FLAG_STACKABLE };
    Item table = { .id = ITEM_TABLE, .name = "Crafting Table", .max_stack_size = 64, .flags = ITEM_FLAG_STACKABLE };

    item_database_register(&wood);
    item_database_register(&planks);
    item_database_register(&table);
}

void test_shapeless_crafting() {
    printf("Testing shapeless crafting...\n");

    // Register recipe: 1 Wood -> 4 Planks
    // (Already done in default recipes, but let's verify)

    ItemStack input = item_create_stack(ITEM_WOOD, 1);
    CraftingResult result = crafting_try_craft_shapeless(&input, 1);

    if (result.success && result.output.item_id == ITEM_PLANKS && result.output.quantity == 4) {
        printf("  [PASS] Wood -> Planks recipe found and valid.\n");
    } else {
        printf("  [FAIL] Wood -> Planks recipe failed.\n");
        if (result.error_message) printf("Error: %s\n", result.error_message);
    }
}

void test_shaped_crafting() {
    printf("Testing shaped crafting...\n");

    // Recipe: 2x2 Planks -> Crafting Table
    ItemStack grid[4]; // 2x2
    grid[0] = item_create_stack(ITEM_PLANKS, 1);
    grid[1] = item_create_stack(ITEM_PLANKS, 1);
    grid[2] = item_create_stack(ITEM_PLANKS, 1);
    grid[3] = item_create_stack(ITEM_PLANKS, 1);

    CraftingResult result = crafting_try_craft_shaped(grid, 2);

    if (result.success && result.output.item_id == ITEM_TABLE) {
        printf("  [PASS] 2x2 Planks -> Crafting Table recipe found and valid.\n");
    } else {
        printf("  [FAIL] 2x2 Planks -> Crafting Table recipe failed.\n");
        if (result.error_message) printf("Error: %s\n", result.error_message);
    }
}

void test_station() {
    printf("Testing crafting station...\n");

    CraftingStation station = crafting_station_create(2);
    Container *inventory = container_create(10, 100.0f);

    // Fill grid
    station.grid[0] = item_create_stack(ITEM_PLANKS, 1);
    station.grid[1] = item_create_stack(ITEM_PLANKS, 1);
    station.grid[2] = item_create_stack(ITEM_PLANKS, 1);
    station.grid[3] = item_create_stack(ITEM_PLANKS, 1);

    crafting_station_update(&station);

    if (station.result.item_id == ITEM_TABLE) {
        printf("  [PASS] Station preview correct.\n");
    } else {
        printf("  [FAIL] Station preview incorrect.\n");
    }

    bool crafted = crafting_station_craft(&station, inventory);
    if (crafted && container_count_item(inventory, ITEM_TABLE) == 1) {
        printf("  [PASS] Station craft successful, item in inventory.\n");
    } else {
        printf("  [FAIL] Station craft failed.\n");
    }

    // Grid should be empty now
    if (station.grid[0].quantity == 0) {
        printf("  [PASS] Ingredients consumed.\n");
    } else {
        printf("  [FAIL] Ingredients not consumed properly.\n");
    }

    container_destroy(inventory);
}

int main() {
    printf("=== PHASE 3 CRAFTING TEST ===\n");

    item_database_init(100);
    register_test_items();

    crafting_system_init(100);
    crafting_register_default_recipes();

    test_shapeless_crafting();
    test_shaped_crafting();
    test_station();

    crafting_system_shutdown();
    item_database_shutdown();

    printf("=== TEST COMPLETE ===\n");
    return 0;
}
