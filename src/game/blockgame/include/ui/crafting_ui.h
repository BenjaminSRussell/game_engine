// include/ui/crafting_ui.h
//
// Purpose: Defines the public API and data structures for the crafting table
// graphical user interface. This header provides the structures and functions
// needed to render and interact with a 3x3 crafting grid, output slot, and
// player inventory integration for crafting operations.
//
// Public APIs:
// - `CraftingUI`: Structure representing the crafting table UI, including the
//   3x3 crafting grid, output slot, player inventory integration, and
//   interaction states.
// - `crafting_ui_init`: Initializes a `CraftingUI` instance with position and
// layout.
// - `crafting_ui_free`: Frees resources associated with the `CraftingUI`.
// - `crafting_ui_render`: Renders the crafting table UI to the screen.
// - `crafting_ui_handle_input`: Processes user input for crafting interactions.
// - `crafting_ui_set_crafting_table`: Links the UI to a `CraftingTable`
// instance.
// - `crafting_ui_set_player_inventory`: Links the UI to player's inventory.
// - `crafting_ui_update_output`: Updates the output slot based on current
// ingredients.
// - `crafting_ui_take_output`: Attempts to take the crafted item to player
// inventory.
//
// Ownership: `CraftingUI` manages its internal state and UI elements.
// It holds references to `CraftingTable` and `Inventory` but does not own them.
//
// Invariants:
// - `CraftingUI` must be initialized before use.
// - The linked `CraftingTable` and `Inventory` must be valid.
// - Input handling modifies both UI state and underlying crafting/inventory
// data.

#ifndef CRAFTING_UI_H
#define CRAFTING_UI_H

#include "../game_common.h"
#include "../crafting/crafting_table.h"
#include "../inventory/inventory.h"
#include <math/vec2.h>
#include <math/vec4.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct VulkanRenderer VulkanRenderer;
typedef struct InputState InputState;

// UI slot structure
typedef struct {
  Vec2 position;
  Vec2 size;
  u32 index;
  bool is_hovered;
  bool is_selected;
  f32 hover_timer;
  f32 selected_timer;
} CraftingUISlot;

// Crafting table UI structure
typedef struct {
  // Layout
  Vec2 position;
  Vec2 size;
  u32 slot_size;
  u32 padding;

  // Visibility and state
  bool visible;
  bool is_dragging;
  u32 dragged_slot;
  u32 hovered_slot;
  u32 selected_slot;

  // UI slots (3x3 grid + output + player inventory)
  CraftingUISlot crafting_slots[9];   // 3x3 crafting grid
  CraftingUISlot output_slot;         // Output result slot
  CraftingUISlot inventory_slots[36]; // Player inventory integration

  // Linked systems
  CraftingTable *crafting_table;
  Inventory *player_inventory;

  // Animation timers
  f32 open_animation;
  f32 close_animation;
  f32 output_pulse;

  // UI colors
  Vec4 background_color;
  Vec4 border_color;
  Vec4 slot_background;
  Vec4 slot_hover;
  Vec4 slot_selected;
  Vec4 output_glow;
} CraftingUI;

// Lifecycle functions
void crafting_ui_init(CraftingUI *ui, Vec2 position, u32 slot_size);
void crafting_ui_free(CraftingUI *ui);

// Rendering functions
void crafting_ui_render(CraftingUI *ui, VulkanRenderer *renderer,
                        InputState *input);

// Input handling
void crafting_ui_handle_input(CraftingUI *ui, InputState *input,
                              f32 delta_time);

// System linking
void crafting_ui_set_crafting_table(CraftingUI *ui, CraftingTable *table);
void crafting_ui_set_player_inventory(CraftingUI *ui, Inventory *inventory);

// Crafting operations
void crafting_ui_update_output(CraftingUI *ui);
bool crafting_ui_take_output(CraftingUI *ui);

// Utility functions
CraftingUISlot *crafting_ui_get_slot_at_pos(CraftingUI *ui, Vec2 mouse_pos);
void crafting_ui_set_hovered_slot(CraftingUI *ui, u32 slot_index);
void crafting_ui_set_selected_slot(CraftingUI *ui, u32 slot_index);

// Animation helpers
void crafting_ui_update_animations(CraftingUI *ui, f32 delta_time);

#ifdef __cplusplus
}
#endif

#endif // CRAFTING_UI_H
