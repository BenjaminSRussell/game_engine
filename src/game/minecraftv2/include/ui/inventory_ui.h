// include/ui/inventory_ui.h
//
// Purpose: Defines the public API and data structures for the player's
// inventory and hotbar graphical user interfaces. This header is responsible
// for presenting the player's collected items in an interactive and visually
// organized manner, allowing for item management, selection, and manipulation
// through the UI. It includes structures for generic UI elements and specific
// inventory/hotbar layouts.
//
// Public APIs:
// - `UIElement`: Structure defining generic properties for any UI element,
//   including its position, size, color, border, and texture details.
// - `UISlot`: Structure representing an individual slot within the inventory or
// hotbar UI,
//   tracking its index, position, size, hover/selection state, and animation
//   timers.
// - `InventoryUI`: The main structure encapsulating the player's full inventory
// UI,
//   including its visibility, slot layout parameters, states for hovering,
//   selection, dragging items, and animation control.
// - `HotbarUI`: A specialized structure for the player's hotbar UI, similar to
// `InventoryUI`
//   but focused on the quick-access slots, including selection animation.
// - `inventory_ui_init`, `hotbar_ui_init`: Initializes the respective UI
// structures,
//   setting up their layout and default states.
// - `inventory_ui_render`, `hotbar_ui_render`: Renders the inventory and hotbar
// UIs
//   to the screen, displaying item icons and highlighting selected/hovered
//   slots.
// - `inventory_ui_handle_input`, `hotbar_ui_handle_input`: Processes user input
//   (mouse clicks, keyboard events) to enable interaction with inventory/hotbar
//   slots, such as moving items or changing selections.
// - Utility functions: `inventory_ui_get_slot_at_pos`,
// `hotbar_ui_get_slot_at_pos`
//   for converting mouse coordinates to slot indices, and setters for
//   hover/selection states.
// - Rendering helpers: `ui_render_quad`, `ui_render_item_icon`,
// `ui_render_slot_background`,
//   `ui_render_hotbar_selection` for drawing basic UI components.
//
// Ownership: `InventoryUI` and `HotbarUI` instances manage their internal state
// and `UISlot` arrays. They interact with `Inventory`, `Hotbar`, `InputState`,
// and `VulkanRenderer` (forward declared) but do not own them.
//
// Invariants:
// - `InventoryUI` and `HotbarUI` must be initialized before use.
// - The underlying `Inventory` and `Hotbar` data structures must be valid and
// synchronized
//   with the UI for correct display.
// - Input handling functions modify the UI state and potentially the underlying
// inventory.
// - `slot_size` and `padding` are crucial for correct visual layout.
//
#ifndef INVENTORY_UI_H
#define INVENTORY_UI_H

#include "../game_common.h"
#include "../inventory/inventory.h"
#include <math/vec2.h>
#include <math/vec4.h>
// #include "../inventory/hotbar.h"
#include "../block/block.h"
#include <input/controls.h>

// Forward declarations
struct VulkanRenderer;
struct InputState;

// UI rendering primitives
typedef struct {
  Vec2 position;
  Vec2 size;
  Vec4 color;
  f32 border_width;
  Vec4 border_color;
  u32 texture_id;
  Vec2 uv_min, uv_max;
} UIElement;

// UI slot state
typedef struct {
  u32 slot_index;
  Vec2 position;
  Vec2 size;
  bool is_hovered;
  bool is_selected;
  f32 hover_timer;
  Vec4 highlight_color;
} UISlot;

// Inventory UI structure
typedef struct {
  bool visible;
  u32 slot_size;
  u32 padding;
  Vec2 position;
  Vec2 size;
  u32 rows, cols;
  UISlot slots[45]; // 36 inventory + 9 hotbar + armor/crafting
  u32 hovered_slot;
  u32 selected_slot;
  bool dragging;
  u32 drag_source_slot;
  Vec2 drag_start_pos;
  Vec2 current_mouse_pos;
  f32 animation_timer;
  bool slide_in;
} InventoryUI;

// Hotbar UI structure
typedef struct {
  bool visible;
  u32 slot_size;
  u32 padding;
  Vec2 position;
  UISlot slots[9];
  u32 selected_slot;
  f32 selection_animation;
  Vec4 selection_color;
} HotbarUI;

// UI rendering functions
void inventory_ui_init(InventoryUI *ui, Vec2 position, u32 slot_size);
void hotbar_ui_init(HotbarUI *ui, Vec2 position, u32 slot_size);
void inventory_ui_render(InventoryUI *ui, Inventory *inventory,
                         struct VulkanRenderer *renderer);
void hotbar_ui_render(HotbarUI *ui, void *renderer, Inventory *inventory);

// Input handling
bool inventory_ui_handle_input(InventoryUI *ui, struct InputState *input,
                               Inventory *inventory);
bool hotbar_ui_handle_input(HotbarUI *ui, struct InputState *input,
                            Inventory *inventory);

// Utility functions
u32 inventory_ui_get_slot_at_pos(InventoryUI *ui, Vec2 mouse_pos);
u32 hotbar_ui_get_slot_at_pos(HotbarUI *ui, Vec2 mouse_pos);
void inventory_ui_set_hovered_slot(InventoryUI *ui, u32 slot_index);
void hotbar_ui_set_selected_slot(HotbarUI *ui, u32 slot_index);

// Rendering helpers
void ui_render_quad(struct VulkanRenderer *renderer, UIElement *element);
void ui_render_item_icon(struct VulkanRenderer *renderer, BlockID item_id,
                         Vec2 pos, Vec2 size);
void ui_render_slot_background(struct VulkanRenderer *renderer, UISlot *slot);
void ui_render_hotbar_selection(struct VulkanRenderer *renderer, HotbarUI *ui);

#endif // INVENTORY_UI_H
