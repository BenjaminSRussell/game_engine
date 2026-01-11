#include "../include/ui/hotbar.h"
#include "common.h"
#include "core/logger.h"
#include "../include/inventory/inventory.h"
#include <string.h>
#include <time.h>

// Helper macros


static f32 get_current_time(void) { return (f32)clock() / CLOCKS_PER_SEC; }

// Global hotbar system instance (singleton-like for easier access if needed)
static HotbarSystem *g_hotbar = NULL;

bool hotbar_initialize(HotbarSystem *hotbar, Inventory *inventory) {
  if (!hotbar || !inventory) {
    LOG_ERROR("Hotbar initialization failed: Invalid pointers");
    return false;
  }

  memset(hotbar, 0, sizeof(HotbarSystem));
  hotbar->inventory = inventory;
  g_hotbar = hotbar;

  // Default configuration
  hotbar->config.slot_size = 40.0f;
  hotbar->config.slot_spacing = 4.0f;
  hotbar->config.border_thickness = 2.0f;
  hotbar->config.show_numbers = true;
  hotbar->config.show_durability = true;
  hotbar->config.show_item_names = true;
  hotbar->config.show_tooltips = true;
  hotbar->config.enable_animations = true;
  hotbar->config.wrap_selection = true;
  hotbar->config.animation_speed = 5.0f;

  // Default colors
  hotbar->config.background_color = (Vec4){0.0f, 0.0f, 0.0f, 0.5f};
  hotbar->config.border_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
  hotbar->config.selected_color = (Vec4){1.0f, 1.0f, 1.0f, 0.8f};
  hotbar->config.number_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};

  // Logic defaults
  hotbar->switch_mode = HOTBAR_SWITCH_MANUAL;
  hotbar->auto_switch_enabled = false;
  hotbar->wrap_around = true;

  // Initialize quick switch bindings (1-9)
  for (int i = 0; i < 9; i++) {
    hotbar->quick_switch_slots[i] = i;
  }

  hotbar->initialized = true;
  LOG_INFO("Hotbar system initialized");
  return true;
}

void hotbar_shutdown(void) {
  if (g_hotbar) {
    memset(g_hotbar, 0, sizeof(HotbarSystem));
    g_hotbar = NULL;
  }
}

void hotbar_update(float delta_time) {
  if (!g_hotbar || !g_hotbar->initialized)
    return;

  // Update animations
  for (int i = 0; i < HOTBAR_SLOT_COUNT; i++) {
    HotbarSlot *slot = &g_hotbar->slots[i];
    if (slot->current_animation != HOTBAR_ANIM_NONE) {
      slot->animation_progress += delta_time * g_hotbar->config.animation_speed;
      if (slot->animation_progress >= 1.0f) {
        slot->animation_progress = 0.0f;
        slot->current_animation = HOTBAR_ANIM_NONE;
      }
    }

    if (slot->cooldown_remaining > 0.0f) {
      slot->cooldown_remaining -= delta_time;
      if (slot->cooldown_remaining < 0.0f)
        slot->cooldown_remaining = 0.0f;
    }
  }

  // Auto-switch logic
  hotbar_update_auto_switch(delta_time);
}

// Basic hotbar operations
void hotbar_select_slot(int slot_index) {
  if (!g_hotbar)
    return;

  if (slot_index < 0)
    slot_index = 0;
  if (slot_index >= HOTBAR_SLOT_COUNT)
    slot_index = HOTBAR_SLOT_COUNT - 1;

  if (g_hotbar->config.selected_slot != slot_index) {
    g_hotbar->previous_selected_slot = g_hotbar->config.selected_slot;
    g_hotbar->config.selected_slot = slot_index;
    g_hotbar->selection_change_time = get_current_time();

    // Sync with inventory
    if (g_hotbar->inventory) {
      inventory_select_hotbar(g_hotbar->inventory, (u32)slot_index);
    }
  }
}

int hotbar_get_selected_slot(void) {
  return g_hotbar ? g_hotbar->config.selected_slot : 0;
}

void hotbar_scroll(float delta) {
  if (!g_hotbar)
    return;

  int current = g_hotbar->config.selected_slot;
  int next = current - (int)delta; // Scroll down (negative) moves next

  if (g_hotbar->wrap_around) {
    while (next < 0)
      next += HOTBAR_SLOT_COUNT;
    while (next >= HOTBAR_SLOT_COUNT)
      next -= HOTBAR_SLOT_COUNT;
  } else {
    next = CLAMP(next, 0, HOTBAR_SLOT_COUNT - 1);
  }

  hotbar_select_slot(next);
}

// Item operations linked to inventory
int hotbar_get_item_id(int slot_index) {
  if (!g_hotbar || !g_hotbar->inventory)
    return 0;
  if (slot_index < 0 || slot_index >= HOTBAR_SLOT_COUNT)
    return 0;

  // Map hotbar slot to inventory slot logic
  // Assuming inventory handles the mapping internally via get_slot
  // Using unified logic where hotbar slots are the first 9 or last 9
  // Adjust based on inventory implementation details
  u32 inv_slot = (MAX_INVENTORY_SLOTS - HOTBAR_SLOT_COUNT) + slot_index;
  InventorySlot slot_data;
  if (inventory_get_slot(g_hotbar->inventory, inv_slot, &slot_data)) {
    return (int)slot_data.item_id;
  }
  return 0;
}

int hotbar_get_item_count(int slot_index) {
  if (!g_hotbar || !g_hotbar->inventory)
    return 0;
  if (slot_index < 0 || slot_index >= HOTBAR_SLOT_COUNT)
    return 0;

  u32 inv_slot = (MAX_INVENTORY_SLOTS - HOTBAR_SLOT_COUNT) + slot_index;
  InventorySlot slot_data;
  if (inventory_get_slot(g_hotbar->inventory, inv_slot, &slot_data)) {
    return (int)slot_data.count;
  }
  return 0;
}

bool hotbar_use_item(int slot_index) {
  if (!g_hotbar || !g_hotbar->inventory)
    return false;

  int count = hotbar_get_item_count(slot_index);
  if (count > 0) {
    u32 inv_slot = (MAX_INVENTORY_SLOTS - HOTBAR_SLOT_COUNT) + slot_index;
    // Basic use logic: decrement count
    // In a real system, this would trigger item usage effects
    InventorySlot s;
    if (inventory_get_slot(g_hotbar->inventory, inv_slot, &s)) {
      bool success = inventory_set_slot(g_hotbar->inventory, inv_slot,
                                        s.item_id, s.count - 1);

      // Record history
      if (success) {
        // Find or add history entry
        // Simplified history tracking here
        g_hotbar->last_switch_time = get_current_time();
      }
      return success;
    }
  }
  return false;
}

// Quick switch logic
void hotbar_quick_switch_next(void) {
  if (!g_hotbar)
    return;
  int start = g_hotbar->config.selected_slot;
  int next = (start + 1) % HOTBAR_SLOT_COUNT;
  while (next != start) {
    if (hotbar_get_item_id(next) != 0) { // Not empty
      hotbar_select_slot(next);
      return;
    }
    next = (next + 1) % HOTBAR_SLOT_COUNT;
  }
}

void hotbar_quick_switch_previous(void) {
  if (!g_hotbar)
    return;
  int start = g_hotbar->config.selected_slot;
  int prev = (start + HOTBAR_SLOT_COUNT - 1) % HOTBAR_SLOT_COUNT;
  while (prev != start) {
    if (hotbar_get_item_id(prev) != 0) {
      hotbar_select_slot(prev);
      return;
    }
    prev = (prev + HOTBAR_SLOT_COUNT - 1) % HOTBAR_SLOT_COUNT;
  }
}

bool hotbar_can_quick_switch(void) {
  if (!g_hotbar)
    return false;
  return (get_current_time() - g_hotbar->last_switch_time) >= 0.2f;
}

void hotbar_update_auto_switch(float delta_time) {
  if (!g_hotbar || !g_hotbar->auto_switch_enabled)
    return;

  int current_slot = g_hotbar->config.selected_slot;
  if (hotbar_get_item_id(current_slot) == 0) {
    // Current item ran out, switch to next available
    int next_slot = -1;
    // Simple search for next non-empty
    for (int i = 1; i < HOTBAR_SLOT_COUNT; i++) {
      int check = (current_slot + i) % HOTBAR_SLOT_COUNT;
      if (hotbar_get_item_id(check) != 0) {
        next_slot = check;
        break;
      }
    }

    if (next_slot != -1) {
      hotbar_select_slot(next_slot);
    }
  }
}

// Getters for specific properties
bool hotbar_is_slot_empty(int slot_index) {
  return hotbar_get_item_id(slot_index) == 0;
}

bool hotbar_is_slot_selected(int slot_index) {
  return g_hotbar && g_hotbar->config.selected_slot == slot_index;
}

void hotbar_set_colors(Vec4 background, Vec4 border, Vec4 selected,
                       Vec4 numbers) {
  if (!g_hotbar)
    return;
  g_hotbar->config.background_color = background;
  g_hotbar->config.border_color = border;
  g_hotbar->config.selected_color = selected;
  g_hotbar->config.number_color = numbers;
}
