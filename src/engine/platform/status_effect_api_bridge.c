#include "../include/platform/status_effect_api_bridge.h"
#include "core/logger.h"
#include "gameplay/combat/status_effects.h"

static uint32_t g_quality = 1;

void status_effect_apply(uint64_t entity_id, PlatformStatusEffectType effect_type,
                         float intensity) {
  // Convert bridge enum to system enum if strictly necessary, but they match
  // 0-5. Default duration to 10.0s for visual testing
  status_sys_apply_effect(entity_id, (int)effect_type, 10.0f, intensity);
}

void status_effect_remove(uint64_t entity_id, PlatformStatusEffectType effect_type) {
  status_sys_remove_effect(entity_id, (int)effect_type);
}

void status_effect_clear_all(uint64_t entity_id) {
  status_sys_clear_all_effects(entity_id);
}

void status_effect_set_quality(uint32_t quality) {
  g_quality = quality;
  // status_sys_set_quality(quality); // If implemented
}

uint32_t status_effect_get_quality(void) { return g_quality; }
