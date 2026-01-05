// Vertex Color Display API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/vertex_color_display_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

void vcol_display_set_enabled(uint64_t entity_id, bool enabled) {
  LOG_INFO("Vertex Color Display: Entity %llu %s", entity_id,
           enabled ? "Enabled" : "Disabled");
  // Set render component debug flags
}

bool vcol_display_is_enabled(uint64_t entity_id) { return false; }

void vcol_display_set_channel(uint64_t entity_id, uint32_t channel_mode) {
  LOG_INFO("Vertex Color Channel: Entity %llu Mode %u", entity_id,
           channel_mode);
}

uint32_t vcol_display_get_channel(uint64_t entity_id) {
  return 0; // Default RGB
}
