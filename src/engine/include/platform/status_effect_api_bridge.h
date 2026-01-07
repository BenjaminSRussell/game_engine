// Status Effect Visuals API Bridge
// Exposes status effect rendering to VoxelForgeStudio

#ifndef STATUS_EFFECT_API_BRIDGE_H
#define STATUS_EFFECT_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Status Effect Visuals API
// ============================================================================

typedef enum {
  PLATFORM_STATUS_EFFECT_BURNING = 0,
  PLATFORM_STATUS_EFFECT_POISON = 1,
  PLATFORM_STATUS_EFFECT_FREEZING = 2,
  PLATFORM_STATUS_EFFECT_HEALING = 3,
  PLATFORM_STATUS_EFFECT_SHIELD = 4,
  PLATFORM_STATUS_EFFECT_SPEED = 5
} PlatformStatusEffectType;

/// Apply a status effect visual to an entity
/// @param entity_id Target entity
/// @param effect_type Type of effect
/// @param intensity Effect intensity (0.0 - 1.0)
void status_effect_apply(uint64_t entity_id, PlatformStatusEffectType effect_type,
                         float intensity);

/// Remove a status effect from an entity
void status_effect_remove(uint64_t entity_id, PlatformStatusEffectType effect_type);

/// Remove all status effects from an entity
void status_effect_clear_all(uint64_t entity_id);

/// Set global status effect quality (affects particle count, etc.)
void status_effect_set_quality(uint32_t quality); // 0 = Low, 1 = Med, 2 = High

/// Get quality level
uint32_t status_effect_get_quality(void);

#ifdef __cplusplus
}
#endif

#endif // STATUS_EFFECT_API_BRIDGE_H
