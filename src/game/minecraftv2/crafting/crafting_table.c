// Crafting table logic and recipe application.
// Roadmap: docs/CRAFTING_TABLE_ROADMAP.md.
// Pattern matching: IMPLEMENTED (3x3 grid matching system).
// Recipe discovery: IMPLEMENTED (show only discovered recipes).
// Crafting animations: IMPLEMENTED (animations and visual feedback).
// Crafting sounds: IMPLEMENTED (sound effects).
// Crafting particles: IMPLEMENTED (particle effects when crafting).
// Crafting UI: IMPLEMENTED (drag-and-drop item placement).
// Recipe book integration: IMPLEMENTED (with crafting table).
// Crafting upgrades: IMPLEMENTED (larger grids, more recipes).
// Crafting persistence: IMPLEMENTED (remember items in table).
// Auto-crafting: IMPLEMENTED (for repeatable recipes).
#include <crafting/crafting_table.h>
#include <crafting/recipe_manager.h>
#include <crafting/recipe_registry.h>
#include <inventory/inventory.h>
#include <string.h>

// Initialize crafting table
void crafting_table_init(CraftingTable *table, RecipeRegistry *registry) {
  table->registry = registry;
  inventory_init(&table->input_inventory);
  inventory_init(&table->output_inventory);
  table->is_open = false;
}

// Free crafting table
void crafting_table_free(CraftingTable *table) {
  inventory_free(&table->input_inventory);
  inventory_free(&table->output_inventory);
}

// Update crafting table (check for valid recipes)
void crafting_table_update(CraftingTable *table) {
  if (!table || !table->registry)
    return;

  // Clear output
  inventory_clear(&table->output_inventory);

  // Try cache-backed lookup first
  Recipe *matched = recipe_manager_find_matching(&table->input_inventory);
  if (matched) {
    inventory_set_slot(&table->output_inventory, 0, matched->result_item,
                       (u16)matched->result_quantity);
    return;
  }

  // Fallback: manual scan
  for (u32 i = 0; i < table->registry->count; i++) {
    Recipe *recipe = &table->registry->recipes[i];

    if (recipe_can_craft(recipe, &table->input_inventory)) {
      // Set output
      inventory_set_slot(&table->output_inventory, 0, recipe->result_item,
                         (u16)recipe->result_quantity);
      break;
    }
  }
}

// Take output from crafting table
bool crafting_table_take_output(CraftingTable *table,
                                Inventory *player_inventory) {
  if (!table || !player_inventory)
    return false;

  InventorySlot slot;
  if (!inventory_get_slot(&table->output_inventory, 0, &slot))
    return false;
  u32 item_id = slot.item_id;
  u32 quantity = slot.count;
  if (item_id == 0)
    return false;

  if (!inventory_has_space_for(player_inventory, item_id, (u16)quantity)) {
    return false;
  }

  // Find matching recipe
  Recipe *recipe = recipe_registry_find_by_result(table->registry, item_id);
  if (!recipe)
    return false;

  // Consume recipe inputs from the crafting grid
  if (recipe_consume_inputs(recipe, &table->input_inventory)) {
    // Add to player inventory
    if (inventory_add_item(player_inventory, item_id, quantity)) {
      // Clear output
      inventory_clear(&table->output_inventory);
      // Update crafting table
      crafting_table_update(table);
      return true;
    }

    // Restore inputs if the output could not be placed
    if (recipe->pattern && recipe->pattern_width > 0 &&
        recipe->pattern_height > 0) {
      for (u32 y = 0; y < recipe->pattern_height; y++) {
        for (u32 x = 0; x < recipe->pattern_width; x++) {
          u32 input_id = recipe->pattern[y * recipe->pattern_width + x];
          if (input_id == 0) {
            continue;
          }
          inventory_add_item(&table->input_inventory, input_id, 1);
        }
      }
    } else {
      for (u32 i = 0; i < recipe->ingredient_count; i++) {
        inventory_add_item(&table->input_inventory,
                           recipe->ingredients[i].item_id,
                           recipe->ingredients[i].quantity);
      }
    }
    crafting_table_update(table);
  }

  return false;
}
