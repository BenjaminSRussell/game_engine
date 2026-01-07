// UV Visualization API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/uv_visualization_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static float g_checkerboard_scale = 1.0f;

void uv_vis_set_enabled(uint64_t entity_id, bool enabled, uint32_t channel) {
  LOG_INFO("UV Vis: Entity %llu %s (Channel %u)", entity_id,
           enabled ? "Enabled" : "Disabled", channel);
  // In real implementation, set shader override or debug flag on render
  // component
}

bool uv_vis_is_enabled(uint64_t entity_id) {
  // Mock return
  return false;
}

uint32_t uv_vis_get_channel(uint64_t entity_id) { return 0; }

void uv_vis_set_checkerboard_scale(float scale) {
  g_checkerboard_scale = scale;
}

float uv_vis_get_checkerboard_scale(void) { return g_checkerboard_scale; }
