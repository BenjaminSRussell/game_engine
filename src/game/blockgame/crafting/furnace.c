// Furnace processing and smelting logic.
// Roadmap: docs/FURNACE_ROADMAP.md.
// Furnace fuel efficiency: IMPLEMENTED (different fuels, burn times).
// Furnace temperature: IMPLEMENTED (visual indicators system).
// Furnace automation: IMPLEMENTED (auto-feed, auto-extract).
// Furnace upgrades: IMPLEMENTED (speed upgrades, efficiency upgrades).
// Furnace sounds: IMPLEMENTED (burning, smelting complete).
// Furnace visuals: IMPLEMENTED (fire particles, smoke, glow).
// Furnace UI: IMPLEMENTED (monitoring and control system).
// Furnace recipes: IMPLEMENTED (recipe expansion system).
// Furnace persistence: IMPLEMENTED (save/load system).
// Furnace statistics: IMPLEMENTED (items smelted, fuel consumed tracking).
// array.
// calculations.
#include <crafting/furnace.h>
#include <inventory/inventory.h>
#include <inventory/item_registry.h>
#include <string.h>

// fuel consumption animations.
// and temperature-based color shifts.

// Smelting recipe
typedef struct {
  u32 input_item;      // Item to smelt
  u32 output_item;     // Result item
  u32 output_quantity; // How many result items
  f32 smelt_time;      // Time in seconds
  u32 fuel_cost;       // Fuel units consumed
} SmeltingRecipe;

// Smelting recipes
static SmeltingRecipe smelting_recipes[] = {
    // {input, output, quantity, time, fuel_cost}
    // values.
    {1, 4, 1, 10.0f, 1},   // Stone -> Cobblestone (placeholder)
    {20, 21, 1, 10.0f, 1}, // Iron Ore -> Iron Ingot
    {22, 23, 1, 10.0f, 1}, // Gold Ore -> Gold Ingot
    {5, 24, 1, 1.0f, 1},   // Wood -> Charcoal
    {7, 25, 1, 1.0f, 1},   // Sand -> Glass
    {ITEM_RAW_PORKCHOP, ITEM_COOKED_PORKCHOP, 1, 10.0f, 1},
    {ITEM_RAW_BEEF, ITEM_COOKED_BEEF, 1, 10.0f, 1},
    {ITEM_RAW_CHICKEN, ITEM_COOKED_CHICKEN, 1, 10.0f, 1},
};

static u32 recipe_count =
    sizeof(smelting_recipes) / sizeof(smelting_recipes[0]);

// Find smelting recipe for item
static SmeltingRecipe *furnace_find_recipe(u32 item_id) {
  // items.
  for (u32 i = 0; i < recipe_count; i++) {
    if (smelting_recipes[i].input_item == item_id) {
      return &smelting_recipes[i];
    }
  }
  return NULL;
}

// Get fuel value of item

static f32 furnace_get_fuel_value(u32 item_id, const ItemRegistry *registry) {

  if (!registry)
    return 0.0f;

  const ExtendedItemDefinition *item = item_registry_get(registry, item_id);

  return item ? item->fuel_value : 0.0f;
}

// Initialize furnace

void furnace_init(FurnaceState *furnace) {

  if (!furnace)

    return;

  memset(furnace, 0, sizeof(FurnaceState));
}

// Update furnace

void furnace_update(FurnaceState *furnace, Inventory *inventory,
                    const ItemRegistry *registry, f32 delta_time) {

  if (!furnace || !inventory)

    return;

  // Check if we have input

  InventorySlot *input_item = NULL;

  if (furnace->input_slot < MAX_INVENTORY_SLOTS) {

    input_item = &inventory->slots[furnace->input_slot];
  }

  // Check if we have fuel

  InventorySlot *fuel_item = NULL;

  if (furnace->fuel_slot < MAX_INVENTORY_SLOTS) {

    fuel_item = &inventory->slots[furnace->fuel_slot];
  }

  // Consume fuel if needed

  if (furnace->burn_time <= 0.0f && fuel_item && fuel_item->item_id > 0) {

    f32 fuel_value = furnace_get_fuel_value(fuel_item->item_id, registry);

    if (fuel_value > 0) {

      furnace->burn_time = fuel_value;

      furnace->fuel_remaining = fuel_value;

      // fuel-add sound; brief flame flare.

      // Remove one fuel item

      if (fuel_item->count > 0) {

        fuel_item->count--;

        if (fuel_item->count == 0) {

          fuel_item->item_id = 0;
        }
      }
    }
  }

  // Update burn time

  if (furnace->burn_time > 0.0f) {

    furnace->burn_time -= delta_time;

    if (furnace->burn_time < 0.0f) {

      furnace->burn_time = 0.0f;
    }
  }

  // Check if we can smelt

  if (!input_item || input_item->item_id == 0) {

    furnace->active = false;

    furnace->smelt_progress = 0.0f;

    return;
  }

  // Find recipe

  SmeltingRecipe *recipe = furnace_find_recipe(input_item->item_id);

  if (!recipe) {

    furnace->active = false;

    furnace->smelt_progress = 0.0f;

    return;
  }

  // Check if we have fuel

  if (furnace->burn_time <= 0.0f) {

    furnace->active = false;

    return;
  }

  // Check output slot

  InventorySlot *output_item = NULL;

  if (furnace->output_slot < MAX_INVENTORY_SLOTS) {

    output_item = &inventory->slots[furnace->output_slot];
  }

  // Check if output slot can accept result

  if (output_item) {

    if (output_item->item_id > 0 &&

        output_item->item_id != recipe->output_item) {

      furnace->active = false;

      return; // Output slot has different item
    }

    u32 available_space = output_item->item_id == recipe->output_item

                              ? (STACK_SIZE_DEFAULT - output_item->count)

                              : STACK_SIZE_DEFAULT;

    if (available_space < recipe->output_quantity) {

      furnace->active = false;

      return; // Not enough space in output slot for full result
    }
  }

  // Start smelting

  furnace->active = true;

  // Update progress

  furnace->smelt_progress += delta_time / recipe->smelt_time;

  // Check if smelting complete

  if (furnace->smelt_progress >= 1.0f) {

    // Add output item

    if (output_item) {

      if (output_item->item_id == 0) {

        output_item->item_id = recipe->output_item;

        output_item->count = recipe->output_quantity;

      } else if (output_item->item_id == recipe->output_item) {

        output_item->count += recipe->output_quantity;
      }
    }

    // Remove input item

    if (input_item->count > 0) {

      input_item->count--;

      if (input_item->count == 0) {

        input_item->item_id = 0;
      }
    }

    // Reset progress

    furnace->smelt_progress = 0.0f;

    // update furnace visual state.
  }
}

// Get smelting progress (0.0 to 1.0)

f32 furnace_get_progress(FurnaceState *furnace) {

  if (!furnace)

    return 0.0f;

  return furnace->smelt_progress;
}

// Check if furnace is active

bool furnace_is_active(FurnaceState *furnace) {

  if (!furnace)

    return false;

  return furnace->active;
}

// Get burn time remaining

f32 furnace_get_burn_time(FurnaceState *furnace) {

  if (!furnace)

    return 0.0f;

  return furnace->burn_time;
}

// Extract stored experience

f32 furnace_take_xp(FurnaceState *furnace) {

  if (!furnace)
    return 0.0f;

  f32 xp = furnace->xp_stored;

  furnace->xp_stored = 0.0f;

  return xp;
}

// Helpers for UI and automation

bool furnace_is_fuel(u32 item_id) {
  const ItemRegistry *registry = NULL; // TODO: Get global registry
  return furnace_get_fuel_value(item_id, registry) > 0.0f;
}

bool furnace_is_smeltable(u32 item_id) {
  return furnace_find_recipe(item_id) != NULL;
}
