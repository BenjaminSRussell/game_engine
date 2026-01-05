// Selection API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/selection_api_bridge.h"
#include <stdlib.h>
#include <string.h>

// Mock implementation state
#define MAX_SELECTION 1024
static uint64_t g_selection[MAX_SELECTION];
static uint32_t g_selection_count = 0;
static SelectionChangedCallback g_callback = NULL;

void selection_select(uint64_t entity_id, bool additive) {
  if (!additive) {
    g_selection_count = 0;
  }

  // Check if already selected
  for (uint32_t i = 0; i < g_selection_count; i++) {
    if (g_selection[i] == entity_id)
      return;
  }

  if (g_selection_count < MAX_SELECTION) {
    g_selection[g_selection_count++] = entity_id;
    LOG_INFO("Selected entity %llu (Total: %u)", entity_id, g_selection_count);

    if (g_callback)
      g_callback(g_selection_count);
  }
}

void selection_deselect(uint64_t entity_id) {
  for (uint32_t i = 0; i < g_selection_count; i++) {
    if (g_selection[i] == entity_id) {
      // Remove by swapping with last
      g_selection[i] = g_selection[--g_selection_count];
      LOG_INFO("Deselected entity %llu", entity_id);

      if (g_callback)
        g_callback(g_selection_count);
      return;
    }
  }
}

void selection_clear(void) {
  if (g_selection_count > 0) {
    g_selection_count = 0;
    LOG_INFO("Cleared selection");
    if (g_callback)
      g_callback(0);
  }
}

bool selection_is_selected(uint64_t entity_id) {
  for (uint32_t i = 0; i < g_selection_count; i++) {
    if (g_selection[i] == entity_id)
      return true;
  }
  return false;
}

uint32_t selection_get_count(void) { return g_selection_count; }

uint32_t selection_get_selected(uint64_t *entities, uint32_t max_count) {
  if (!entities || max_count == 0)
    return 0;

  uint32_t count =
      g_selection_count < max_count ? g_selection_count : max_count;
  memcpy(entities, g_selection, count * sizeof(uint64_t));
  return count;
}

uint64_t selection_get_primary(void) {
  if (g_selection_count == 0)
    return 0;
  return g_selection[g_selection_count - 1]; // Return last selected as primary
}

uint64_t selection_raycast(float ray_origin_x, float ray_origin_y,
                           float ray_origin_z, float ray_dir_x, float ray_dir_y,
                           float ray_dir_z) {
  LOG_INFO("Raycast selection from (%.2f, %.2f, %.2f)", ray_origin_x,
           ray_origin_y, ray_origin_z);

  // In real implementation, perform physics raycast
  // Mock: always return 0 (no hit) unless we mock a specific scenario
  return 0;
}

void selection_register_callback(SelectionChangedCallback callback) {
  g_callback = callback;
}
