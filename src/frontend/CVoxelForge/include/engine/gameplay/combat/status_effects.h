#ifndef STATUS_EFFECTS_H
#define STATUS_EFFECTS_H

#include <stdbool.h>
#include <stdint.h>

// Must match API enum
typedef enum {
  EFFECT_BURNING = 0,
  EFFECT_POISON = 1,
  EFFECT_FREEZING = 2,
  EFFECT_HEALING = 3,
  EFFECT_SHIELD = 4,
  EFFECT_SPEED = 5
} StatusEffectType;

void status_sys_init(void);
void status_sys_update(float delta_time);

void status_sys_apply_effect(uint64_t entity_id, StatusEffectType type,
                             float duration, float magnitude);
void status_sys_remove_effect(uint64_t entity_id, StatusEffectType type);
void status_sys_clear_all_effects(uint64_t entity_id);

typedef struct {
  StatusEffectType type;
  float duration_remaining;
  float magnitude;
} ActiveEffectInfo;

// Returns number of effects written to 'out_effects'.
uint32_t status_sys_get_active_effects(uint64_t entity_id,
                                       ActiveEffectInfo *out_effects,
                                       uint32_t max_count);

#endif // STATUS_EFFECTS_H
