#ifndef HOTBAR_H
#define HOTBAR_H

#include "../../../../engine/include/math/vec2.h"
#include "../../../../engine/include/math/vec4.h"
#include "../game_common.h"
#include "../inventory/inventory.h"

#define HOTBAR_SLOT_COUNT 9
#define HOTBAR_MAX_STACK_SIZE 64
#define HOTBAR_ITEM_NAME_LENGTH 64
#define HOTBAR_TOOLTIP_LENGTH 256

// Hotbar slot states
typedef enum {
  HOTBAR_SLOT_EMPTY,
  HOTBAR_SLOT_OCCUPIED,
  HOTBAR_SLOT_SELECTED,
  HOTBAR_SLOT_COOLDOWN,
  HOTBAR_SLOT_DISABLED
} HotbarSlotState;

// Hotbar animation types
typedef enum {
  HOTBAR_ANIM_NONE,
  HOTBAR_ANIM_SELECT,
  HOTBAR_ANIM_DESELECT,
  HOTBAR_ANIM_PICKUP,
  HOTBAR_ANIM_PLACE,
  HOTBAR_ANIM_COOLDOWN,
  HOTBAR_ANIM_BREAK
} HotbarAnimationType;

// Hotbar switch modes
typedef enum {
  HOTBAR_SWITCH_MANUAL,
  HOTBAR_SWITCH_AUTO,
  HOTBAR_SWITCH_SMART,
  HOTBAR_SWITCH_FAVORITE
} HotbarSwitchMode;

typedef struct {
  u32 item_id;
  u32 slot_index;
  f32 last_used_time;
  u32 use_count;
  bool is_favorite;
} HotbarItemHistory;

// Hotbar slot configuration
typedef struct {
  int item_id;
  int count;
  int durability;
  int max_durability;
  char item_name[HOTBAR_ITEM_NAME_LENGTH];
  HotbarSlotState state;
  HotbarAnimationType current_animation;
  float animation_progress;
  float cooldown_remaining;
  Vec4 tint_color; // Using Vec4 for color (RGBA float)
  bool is_enchanted;
  bool has_custom_name;
  char custom_name[HOTBAR_ITEM_NAME_LENGTH];
} HotbarSlot;

// Hotbar configuration
typedef struct {
  Vec2 position;
  Vec2 size;
  float slot_size;
  float slot_spacing;
  float border_thickness;
  int selected_slot;
  bool show_numbers;
  bool show_durability;
  bool show_item_names;
  bool show_tooltips;
  bool enable_animations;
  bool auto_arrange;
  bool wrap_selection;
  Vec4 background_color;
  Vec4 border_color;
  Vec4 selected_color;
  Vec4 number_color;
  Vec4 durability_color;
  float animation_speed;
} HotbarConfig;

// Hotbar tooltip
typedef struct {
  bool enabled;
  Vec2 position;
  char text[HOTBAR_TOOLTIP_LENGTH];
  float display_time;
  float fade_time;
  Vec4 background_color;
  Vec4 text_color;
  Vec4 border_color;
} HotbarTooltip;

// Hotbar system
typedef struct {
  bool initialized;
  struct Inventory *inventory; // Linked inventory
  HotbarConfig config;
  HotbarSlot slots[HOTBAR_SLOT_COUNT];

  // Logic state
  HotbarSwitchMode switch_mode;
  f32 last_switch_time;
  bool auto_switch_enabled;
  bool wrap_around;
  HotbarItemHistory history[HOTBAR_SLOT_COUNT];
  u32 history_count;
  u32 quick_switch_slots[9];
  bool slot_locked[HOTBAR_SLOT_COUNT];

  // Visual state
  HotbarTooltip tooltip;
  int previous_selected_slot;
  float selection_change_time;
  bool is_visible;
  bool is_interactive;
  float scale_factor;
  Vec2 mouse_position;
  bool mouse_over_slot;
  int hovered_slot;
  float tooltip_delay;
  float tooltip_timer;
} HotbarSystem;

// Hotbar events
typedef enum {
  HOTBAR_EVENT_SLOT_SELECTED,
  HOTBAR_EVENT_SLOT_CHANGED,
  HOTBAR_EVENT_ITEM_USED,
  HOTBAR_EVENT_ITEM_BROKEN,
  HOTBAR_EVENT_COOLDOWN_STARTED,
  HOTBAR_EVENT_COOLDOWN_ENDED,
  HOTBAR_EVENT_TOOLTIP_SHOWN,
  HOTBAR_EVENT_TOOLTIP_HIDDEN
} HotbarEventType;

typedef struct {
  HotbarEventType type;
  int slot_index;
  int item_id;
  int count;
  float timestamp;
} HotbarEvent;

// Hotbar callback function type
typedef void (*HotbarEventCallback)(const HotbarEvent *event);

// Public API
// Public API
bool hotbar_initialize(HotbarSystem *hotbar, Inventory *inventory);
void hotbar_shutdown(void);
void hotbar_update(float delta_time);
void hotbar_render(void);

// Configuration
void hotbar_set_config(const HotbarConfig *config);
void hotbar_get_config(HotbarConfig *config);
void hotbar_set_position(float x, float y);
void hotbar_set_scale(float scale);
void hotbar_set_visibility(bool visible);
void hotbar_set_interactivity(bool interactive);

// Slot management
void hotbar_set_slot(int slot_index, int item_id, int count, int durability,
                     int max_durability);
void hotbar_clear_slot(int slot_index);
void hotbar_swap_slots(int slot1, int slot2);
void hotbar_move_stack(int from_slot, int to_slot, int amount);
void hotbar_select_slot(int slot_index);
int hotbar_get_selected_slot(void);
void hotbar_select_next_slot(void);
void hotbar_select_previous_slot(void);

// Slot information
void hotbar_get_slot(int slot_index, HotbarSlot *slot);
bool hotbar_is_slot_empty(int slot_index);
bool hotbar_is_slot_selected(int slot_index);
int hotbar_get_item_count(int slot_index);
int hotbar_get_item_id(int slot_index);

// Item operations
bool hotbar_use_item(int slot_index);
bool hotbar_can_use_item(int slot_index);
void hotbar_set_cooldown(int slot_index, float duration);
float hotbar_get_cooldown(int slot_index);
void hotbar_damage_item(int slot_index, int damage);
bool hotbar_is_item_broken(int slot_index);

// Animation system
void hotbar_play_animation(int slot_index, HotbarAnimationType animation);
void hotbar_stop_animation(int slot_index);
bool hotbar_is_animation_playing(int slot_index);
void hotbar_set_animation_speed(float speed);

// Tooltip system
void hotbar_show_tooltip(const char *text, float x, float y);
void hotbar_hide_tooltip(void);
void hotbar_set_tooltip_delay(float delay);
void hotbar_set_tooltip_colors(Vec4 bg, Vec4 text, Vec4 border);

// Input handling
void hotbar_handle_mouse_click(float x, float y, int button);
void hotbar_handle_mouse_release(float x, float y, int button);
void hotbar_handle_mouse_move(float x, float y);
void hotbar_handle_key_press(int key);
void hotbar_handle_scroll(float delta);

// Event system
void hotbar_set_event_callback(HotbarEventCallback callback);
void hotbar_trigger_event(HotbarEventType type, int slot_index, int item_id,
                          int count);

// Inventory integration
void hotbar_sync_with_inventory(Inventory *inventory);
void hotbar_update_from_inventory(void);
void hotbar_apply_to_inventory(void);

// Utility functions
int hotbar_get_slot_at_position(float x, float y);
Vec2 hotbar_get_slot_position(int slot_index);
Vec2 hotbar_get_slot_size(void);
bool hotbar_is_position_over_hotbar(float x, float y);
void hotbar_get_bounds(Vec2 *min_pos, Vec2 *max_pos);

// Enhanced logic functions
void hotbar_quick_switch_next(void);
void hotbar_quick_switch_previous(void);
bool hotbar_can_quick_switch(void);
void hotbar_set_switch_mode(HotbarSwitchMode mode);
void hotbar_update_auto_switch(float delta_time);

// Visual customization
void hotbar_set_colors(Vec4 background, Vec4 border, Vec4 selected,
                       Vec4 numbers);
void hotbar_set_slot_colors(Vec4 normal, Vec4 hover, Vec4 selected,
                            Vec4 disabled);
void hotbar_set_durability_colors(Vec4 high, Vec4 medium, Vec4 low,
                                  Vec4 critical);
void hotbar_set_font_size(float size);
void hotbar_set_border_thickness(float thickness);

// Accessibility
void hotbar_set_accessibility_mode(bool enabled);
void hotbar_set_high_contrast(bool enabled);
void hotbar_set_colorblind_mode(int mode);
void hotbar_set_reduce_motion(bool enabled);

// Save/Load
void hotbar_save_state(const char *filename);
bool hotbar_load_state(const char *filename);

// Debug utilities
void hotbar_debug_print_slots(void);
void hotbar_debug_render_bounds(void);
bool hotbar_debug_validate_state(void);

#endif // HOTBAR_H
