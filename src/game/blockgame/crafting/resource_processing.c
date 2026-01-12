// Resource processing cost/energy logic.
// Roadmap: docs/RESOURCE_PROCESSING_ROADMAP.md.
// TODO: Add processing efficiency system based on machine upgrades.
// TODO: Implement processing quality system affecting output.
// TODO: Add processing failure chance system for risk.
// TODO: Implement processing automation with item pipes.
// TODO: Add processing recipe discovery system.
// TODO: Implement processing material substitution.
// TODO: Add processing time acceleration with upgrades.
// TODO: Implement processing statistics tracking.
// TODO: Add processing UI system for monitoring progress.
#include <crafting/resource_processing.h>
#include <inventory/inventory.h>
#include <string.h>

// Processing recipe
typedef struct {
  u32 input_item;
  u32 output_item;
  u32 output_quantity;
  f32 process_time;
  u32 energy_cost; // Energy units required
} ProcessingRecipe;

// Processing recipes
static ProcessingRecipe processing_recipes[] = {
    // {input, output, quantity, time, energy}
    {20, 27, 1, 5.0f, 10}, // Iron Ore -> Iron Ingot (smelting)
    {22, 28, 1, 5.0f, 10}, // Gold Ore -> Gold Ingot
    {5, 29, 1, 2.0f, 5},   // Wood -> Planks
    {29, 30, 1, 1.0f, 2},  // Planks -> Sticks
    {27, 31, 1, 3.0f, 8},  // Iron Ingot -> Iron Plate
    {28, 32, 1, 3.0f, 8},  // Gold Ingot -> Gold Plate
    {33, 34, 1, 4.0f, 12}, // Rubber -> Rubber Sheet
    {27, 35, 1, 5.0f, 15}, // Iron Ingot -> Wire
};

static u32 recipe_count =
    sizeof(processing_recipes) / sizeof(processing_recipes[0]);

// Find processing recipe
static ProcessingRecipe *processing_find_recipe(u32 item_id) {
  for (u32 i = 0; i < recipe_count; i++) {
    if (processing_recipes[i].input_item == item_id) {
      return &processing_recipes[i];
    }
  }
  return NULL;
}

// Initialize processing machine
void processing_machine_init(ProcessingMachine *machine, f32 max_energy) {
  if (!machine)
    return;
  memset(machine, 0, sizeof(ProcessingMachine));
  machine->max_energy = max_energy;
  machine->queue_head = 0;
  machine->queue_tail = 0;
}

bool processing_machine_enqueue(ProcessingMachine *machine, u32 item_id) {
  if (!machine)
    return false;
  u32 next = (machine->queue_tail + 1) % PROCESSING_QUEUE_SIZE;
  if (next == machine->queue_head) {
    return false; // queue full
  }
  machine->queue[machine->queue_tail] = item_id;
  machine->queue_tail = next;
  return true;
}

// Update processing machine
void processing_machine_update(ProcessingMachine *machine, Inventory *inventory,
                               f32 energy_input, f32 delta_time) {
  if (!machine || !inventory)
    return;

  // Add energy
  machine->energy_available += energy_input * delta_time;
  if (machine->energy_available > machine->max_energy) {
    machine->energy_available = machine->max_energy;
  }

  // Check input
  InventorySlot *input_item = NULL;
  if (machine->input_slot < MAX_INVENTORY_SLOTS) {
    input_item = &inventory->slots[machine->input_slot];
  }

  if (!input_item || input_item->item_id == 0) {
    /* If input slot is empty, attempt to dequeue from the processing queue to
     * fill it */
    if (machine->queue_head != machine->queue_tail) {
      u32 item_id = machine->queue[machine->queue_head];
      machine->queue_head = (machine->queue_head + 1) % PROCESSING_QUEUE_SIZE;
      if (machine->input_slot < MAX_INVENTORY_SLOTS) {
        InventorySlot *s = &inventory->slots[machine->input_slot];
        s->item_id = item_id;
        s->count = 1;
      }
      /* refresh pointer */
      input_item = &inventory->slots[machine->input_slot];
    }
  }
  if (!input_item || input_item->item_id == 0) {
    machine->active = false;
    machine->process_progress = 0.0f;
    return;
  }

  // Find recipe
  ProcessingRecipe *recipe = processing_find_recipe(input_item->item_id);
  if (!recipe) {
    machine->active = false;
    machine->process_progress = 0.0f;
    return;
  }

  // Check energy
  if (machine->energy_available < recipe->energy_cost) {
    machine->active = false;
    return;
  }

  // Check output slot
  InventorySlot *output_item = NULL;
  if (machine->output_slot < MAX_INVENTORY_SLOTS) {
    output_item = &inventory->slots[machine->output_slot];
  }

  if (output_item) {
    if (output_item->item_id > 0 &&
        output_item->item_id != recipe->output_item) {
      machine->active = false;
      return;
    }
    u32 available_space = output_item->item_id == recipe->output_item
                              ? (STACK_SIZE_DEFAULT - output_item->count)
                              : STACK_SIZE_DEFAULT;
    if (available_space < recipe->output_quantity) {
      machine->active = false;
      return; // Not enough space for full output
    }
  }

  // Start processing
  machine->active = true;
  machine->current_recipe = (u32)(recipe - processing_recipes);

  // Update progress
  machine->process_progress += delta_time / recipe->process_time;

  // Check if complete
  if (machine->process_progress >= 1.0f) {
    // Consume energy
    machine->energy_available -= recipe->energy_cost;
    if (machine->energy_available < 0.0f) {
      machine->energy_available = 0.0f;
    }

    // Add output (cap to stack size)
    if (output_item) {
      u32 to_add = recipe->output_quantity;
      u32 available = output_item->item_id == recipe->output_item
                          ? (STACK_SIZE_DEFAULT - output_item->count)
                          : STACK_SIZE_DEFAULT;
      u32 add = MIN(to_add, available);
      if (add == 0) {
        // No space  shouldn't happen due to earlier check, but guard anyway
      } else {
        if (output_item->item_id == 0) {
          output_item->item_id = recipe->output_item;
          output_item->count = (u16)add;
        } else if (output_item->item_id == recipe->output_item) {
          output_item->count = output_item->count + (u16)add;
        }
      }
    }

    // Remove input
    if (input_item->count > 0) {
      input_item->count--;
      if (input_item->count == 0) {
        input_item->item_id = 0;
      }
    }

    // Reset progress
    machine->process_progress = 0.0f;
  }
}

// Get processing progress
f32 processing_machine_get_progress(ProcessingMachine *machine) {
  if (!machine)
    return 0.0f;
  return machine->process_progress;
}

// Check if machine is active
bool processing_machine_is_active(ProcessingMachine *machine) {
  if (!machine)
    return false;
  return machine->active;
}

// Get energy available
f32 processing_machine_get_energy(ProcessingMachine *machine) {
  if (!machine)
    return 0.0f;
  return machine->energy_available;
}
