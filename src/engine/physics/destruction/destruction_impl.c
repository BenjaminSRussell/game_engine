#include "physics/destruction/destruction_impl.h"
#include "core/logger.h"

// Internal state for the destruction system
static struct {
  bool enabled;
  float debris_lifetime;
} destruction_context = {.enabled = true, .debris_lifetime = 10.0f};

void destruction_sys_set_enabled(bool enabled) {
  destruction_context.enabled = enabled;
  LOG_INFO("Destruction System state update: %s",
           enabled ? "ENABLED" : "DISABLED");
}

bool destruction_sys_is_enabled(void) { return destruction_context.enabled; }

void destruction_sys_set_debris_lifetime(float lifetime) {
  destruction_context.debris_lifetime = lifetime;
}

float destruction_sys_get_debris_lifetime(void) {
  return destruction_context.debris_lifetime;
}

void destruction_sys_trigger_break(uint64_t entity_id, float x, float y,
                                   float z, float force) {
  if (!destruction_context.enabled) {
    LOG_WARN("Destruction Triggered on %llu but system is DISABLED", (unsigned long long)entity_id);
    return;
  }

  LOG_INFO("Destruction System: Processing break event for Entity %llu",
           (unsigned long long)entity_id);
  LOG_DEBUG("  > Impact: [%.2f, %.2f, %.2f]", x, y, z);
  LOG_DEBUG("  > Force: %.2f", force);

  // Integration logic would go here
}
