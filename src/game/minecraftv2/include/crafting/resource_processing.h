// include/crafting/resource_processing.h
//
// Purpose: Defines the public API and data structures for a generic `ProcessingMachine`,
// which represents any in-game block or entity capable of processing resources. This system
// models machines that consume input items and energy to produce output items over time.
// It tracks the machine's state, processing progress, and energy levels.
//
// Public APIs:
// - `ProcessingMachine`: Structure representing the state of a resource processing unit,
//   including its active status, input and output inventory slots, current recipe,
//   processing progress, and energy management details.
// - `processing_machine_init`: Initializes a `ProcessingMachine` instance with its maximum energy capacity.
// - `processing_machine_update`: Updates the machine's state each frame, handling energy consumption,
//   processing progression, and item transfers.
// - `processing_machine_get_progress`: Returns the current processing progress as a normalized value (0.0 to 1.0).
// - `processing_machine_is_active`: Checks if the machine is currently engaged in a processing task.
// - `processing_machine_get_energy`: Returns the current energy stored within the machine.
//
// Ownership: A `ProcessingMachine` instance manages its internal state variables. It interacts
// with an `Inventory` instance (typically a block inventory) for input and output, but does not own it.
//
// Invariants:
// - A `ProcessingMachine` must be initialized with `processing_machine_init` before use.
// - `processing_machine_update` should be called regularly (e.g., every game tick) to ensure proper
//   processing progression and energy management.
// - The `Inventory` structure (defined in `inventory.h`) is assumed to be correctly defined.
// - Recipes and energy consumption rates are assumed to be defined elsewhere and integrated with this system.
//
#ifndef RESOURCE_PROCESSING_H
#define RESOURCE_PROCESSING_H


#include "../game_common.h"
#include "../inventory/inventory.h"

// Automation side configuration (for pipes/hoppers)
typedef enum {
    MACHINE_SIDE_NONE,
    MACHINE_SIDE_INPUT,
    MACHINE_SIDE_OUTPUT,
    MACHINE_SIDE_FUEL
} MachineSideConfig;

// Processing machine state
#define PROCESSING_QUEUE_SIZE 8
typedef struct {
    bool active;
    u32 input_slot;
    u32 output_slot;
    u32 current_recipe;
    f32 process_progress;
    f32 energy_available;
    f32 max_energy;

    // Configuration for 6 faces (Up, Down, North, South, East, West)
    MachineSideConfig side_config[6];

    /* Simple FIFO queue of item IDs to process (counts are per-item = 1)
       This allows queuing multiple items for processing instead of relying on
       a single input slot being manually refilled. */
    u32 queue[PROCESSING_QUEUE_SIZE];
    u32 queue_head; /* index of next item to dequeue */
    u32 queue_tail; /* index of next slot to enqueue into */
} ProcessingMachine;

/* Queue an item for processing. Returns true on success. */
bool processing_machine_enqueue(ProcessingMachine *machine, u32 item_id);

// Initialize processing machine
void processing_machine_init(ProcessingMachine *machine, f32 max_energy);

// Update processing machine
void processing_machine_update(ProcessingMachine *machine, Inventory *inventory, f32 energy_input, f32 delta_time);

// Configure machine sides
// TODO: Validate side_index (0-5).
void processing_machine_set_side(ProcessingMachine *machine, u8 side_index, MachineSideConfig config);
MachineSideConfig processing_machine_get_side(ProcessingMachine *machine, u8 side_index);

// Get processing progress (0.0 to 1.0)
f32 processing_machine_get_progress(ProcessingMachine *machine);

// Check if machine is active
bool processing_machine_is_active(ProcessingMachine *machine);

// Get energy available
f32 processing_machine_get_energy(ProcessingMachine *machine);

#endif // RESOURCE_PROCESSING_H
