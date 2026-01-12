#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common.h>
#include <crafting/furnace.h>
#include <inventory/inventory.h>
#include <inventory/item_registry.h>

// Mock definitions
// Restore globals
void *g_audio_system = NULL;
void *g_particle_system = NULL;

// Test 1: Verify Furnace Fuel Logic
bool test_furnace_fuel(ItemRegistry *registry) {
  printf("Running test_furnace_fuel...\n");

  FurnaceState furnace;
  furnace_init(&furnace);
  furnace.input_slot = 0;
  furnace.fuel_slot = 1;
  furnace.output_slot = 2;

  Inventory inventory;
  inventory_init(&inventory);

  // Register items (before adding to inventory)
  // Coal
  ExtendedItemDefinition coal_def = {0};
  strcpy(coal_def.base.name, "Coal");
  coal_def.base.item_type = ITEM_TYPE_MATERIAL;
  coal_def.base.max_stack_size = 64;
  coal_def.fuel_value = 80.0f;
  item_registry_register(registry, ITEM_COAL, &coal_def);

  // Iron Ore
  ExtendedItemDefinition ore_def = {0};
  strcpy(ore_def.base.name, "Iron Ore");
  ore_def.base.item_type = ITEM_TYPE_BLOCK;
  ore_def.base.max_stack_size = 64;
  item_registry_register(registry, ITEM_IRON_ORE, &ore_def);

  // Iron Ingot
  ExtendedItemDefinition ingot_def = {0};
  strcpy(ingot_def.base.name, "Iron Ingot");
  ingot_def.base.item_type = ITEM_TYPE_MATERIAL;
  ingot_def.base.max_stack_size = 64;
  item_registry_register(registry, ITEM_IRON_INGOT, &ingot_def);

  // Set up inputs now that registry knows about them
  inventory_set_slot(&inventory, 0, ITEM_IRON_ORE, 1);
  inventory_set_slot(&inventory, 1, ITEM_COAL, 1);

  // Verify initial state
  if (furnace.burn_time > 0.0f) {
    printf("FAILED: Initial burn time should be 0\n");
    return false;
  }

  // Update furnace (should consume fuel)
  furnace_update(&furnace, &inventory, registry, 0.1f);

  if (furnace.burn_time <= 0.0f) {
    printf("FAILED: Furnace did not consume fuel. Burn time: %f\n",
           furnace.burn_time);
    return false;
  }

  if (fabs(furnace.burn_time - (80.0f - 0.1f)) > 0.01f) {
    printf("FAILED: Burn time incorrect. Expected ~79.9, got %f\n",
           furnace.burn_time);
    return false;
  }

  // Verify consumption
  InventorySlot fuel_slot;
  inventory_get_slot(&inventory, 1, &fuel_slot);
  // We had 1 coal, consumed 1, so count should be 0.
  if (fuel_slot.count != 0) {
    printf("FAILED: Fuel slot not consumed. Count: %d\n", fuel_slot.count);
    return false;
  }

  printf("PASSED\n");
  return true;
}

int main() {
  printf("=== Test Crafting ===\n");

  ItemRegistry registry = {0};
  item_registry_init(&registry);

  bool all_passed = true;
  all_passed &= test_furnace_fuel(&registry);

  if (all_passed) {
    printf("All tests passed!\n");
    return 0;
  } else {
    printf("Some tests failed.\n");
    return 1;
  }
}
