// Status Effect System Implementation
#include <core/logging/unified_logger.h>
#include <math.h>
#include <player/status_effects.h>
#include <string.h>

// Status effect priority (higher = more important)
static u32 status_effect_get_priority(StatusEffectType type) {
  switch (type) {
  case STATUS_EFFECT_INVISIBILITY:
    return 10; // Highest priority
  case STATUS_EFFECT_FIRE_RESISTANCE:
    return 9;
  case STATUS_EFFECT_WATER_BREATHING:
    return 8;
  case STATUS_EFFECT_STRENGTH:
    return 7;
  case STATUS_EFFECT_WEAKNESS:
    return 6;
  case STATUS_EFFECT_SPEED:
    return 5;
  case STATUS_EFFECT_SLOWNESS:
    return 4;
  case STATUS_EFFECT_JUMP_BOOST:
    return 3;
  case STATUS_EFFECT_REGENERATION:
    return 2;
  case STATUS_EFFECT_POISON:
    return 1;
  case STATUS_EFFECT_ON_FIRE:
    return 0; // Lowest priority
  default:
    return 0;
  }
}

// Status effect resistance tracking (immunity after exposure)
static f32 g_effect_resistance_timers[STATUS_EFFECT_COUNT] = {0};
#define RESISTANCE_DURATION                                                    \
  300.0f // 5 minutes of resistance after effect expires

// Check if player has resistance to an effect
static bool status_effect_has_resistance(StatusEffectType type) {
  if (type == STATUS_EFFECT_NONE || type >= STATUS_EFFECT_COUNT)
    return false;
  return g_effect_resistance_timers[type] > 0.0f;
}

// Update resistance timers
static void status_effect_update_resistance(f32 delta_time) {
  for (u32 i = 0; i < STATUS_EFFECT_COUNT; i++) {
    if (g_effect_resistance_timers[i] > 0.0f) {
      g_effect_resistance_timers[i] -= delta_time;
      if (g_effect_resistance_timers[i] < 0.0f) {
        g_effect_resistance_timers[i] = 0.0f;
      }
    }
  }
}

// Set resistance to an effect
static void status_effect_set_resistance(StatusEffectType type, f32 duration) {
  if (type == STATUS_EFFECT_NONE || type >= STATUS_EFFECT_COUNT)
    return;
  if (duration > g_effect_resistance_timers[type]) {
    g_effect_resistance_timers[type] = duration;
  }
}

void status_effects_init(StatusEffectManager *manager) {
  if (!manager)
    return;
  memset(manager, 0, sizeof(StatusEffectManager));
  manager->effect_count = 0;
  manager->immunity_timer = 0.0f;
  manager->on_add = NULL;
  manager->on_refresh = NULL;
  manager->on_remove = NULL;
  manager->on_expire = NULL;
  manager->on_warning = NULL;
  manager->on_immunity = NULL;
  manager->on_update = NULL;
  manager->user_data = NULL;
}

static void status_effect_emit(StatusEffectManager *manager,
                               StatusEffectEventCallback callback,
                               StatusEffectType type, f32 duration,
                               f32 strength) {
  if (callback) {
    callback(type, duration, strength, manager->user_data);
  }
}

static void status_effect_emit_update(StatusEffectManager *manager,
                                      StatusEffectType type, f32 duration,
                                      f32 strength, f32 delta_time) {
  if (manager->on_update) {
    manager->on_update(type, duration, strength, delta_time,
                       manager->user_data);
  }
}

// Status effect stacking and combination logic
static bool status_effect_check_combination(StatusEffectManager *manager,
                                            StatusEffectType new_type,
                                            StatusEffectType existing_type) {
  // Poison + Fire = Explosive damage effect
  if ((new_type == STATUS_EFFECT_POISON &&
       existing_type == STATUS_EFFECT_ON_FIRE) ||
      (new_type == STATUS_EFFECT_ON_FIRE &&
       existing_type == STATUS_EFFECT_POISON)) {
    // Create explosive damage effect (represented as poison with higher
    // strength)
    for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
      if (manager->effects[i].active &&
          (manager->effects[i].type == STATUS_EFFECT_POISON ||
           manager->effects[i].type == STATUS_EFFECT_ON_FIRE)) {
        // Convert to explosive effect (we'll use poison with 3x strength)
        manager->effects[i].type = STATUS_EFFECT_POISON;
        manager->effects[i].strength *= 3.0f;
        manager->effects[i].duration = fmax(manager->effects[i].duration, 5.0f);
        LOG_INFO(LOG_CAT_GAME,
            "Status effect combination: Poison + Fire = Explosive damage!");
        return true; // Combined, don't add new effect
      }
    }
  }

  // Speed + Slowness cancel each other out
  if ((new_type == STATUS_EFFECT_SPEED &&
       existing_type == STATUS_EFFECT_SLOWNESS) ||
      (new_type == STATUS_EFFECT_SLOWNESS &&
       existing_type == STATUS_EFFECT_SPEED)) {
    // Remove the opposing effect
    status_effect_remove(manager, existing_type);
    LOG_INFO(LOG_CAT_GAME, "Status effects canceled: Speed and Slowness cancel each other");
    return false; // Continue to add the new effect
  }

  // Strength + Weakness cancel each other out
  if ((new_type == STATUS_EFFECT_STRENGTH &&
       existing_type == STATUS_EFFECT_WEAKNESS) ||
      (new_type == STATUS_EFFECT_WEAKNESS &&
       existing_type == STATUS_EFFECT_STRENGTH)) {
    status_effect_remove(manager, existing_type);
    LOG_INFO(LOG_CAT_GAME,
        "Status effects canceled: Strength and Weakness cancel each other");
    return false;
  }

  return false; // No special combination
}

bool status_effect_add(StatusEffectManager *manager, StatusEffectType type,
                       f32 duration, f32 strength) {
  if (!manager || type == STATUS_EFFECT_NONE || type >= STATUS_EFFECT_COUNT)
    return false;

  // Check for resistance (immunity after exposure)
  if (status_effect_has_resistance(type)) {
    LOG_DEBUG(LOG_CAT_GAME, "Status effect %d resisted (immunity active)", type);
    return false; // Effect resisted
  }

  // Check for combination effects with existing effects
  for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
    if (manager->effects[i].active) {
      if (status_effect_check_combination(manager, type,
                                          manager->effects[i].type)) {
        return true; // Combined, don't add new effect
      }
    }
  }

  // Check if effect already exists (stacking and priority system)
  u32 existing_slot = MAX_STATUS_EFFECTS;
  u32 existing_priority = 0;

  for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
    if (manager->effects[i].type == type && manager->effects[i].active) {
      existing_slot = i;
      existing_priority = status_effect_get_priority(type);
      break;
    }
  }

  if (existing_slot < MAX_STATUS_EFFECTS) {
    // Effect exists - implement stacking
    u32 new_priority = status_effect_get_priority(type);

    // If same priority, stack by extending duration and potentially increasing
    // strength
    if (new_priority == existing_priority) {
      manager->effects[existing_slot].duration =
          fmax(manager->effects[existing_slot].duration, duration);
      // Stack strength (additive for most effects, but capped)
      manager->effects[existing_slot].strength =
          fmin(manager->effects[existing_slot].strength + strength,
               10.0f); // Cap at 10x
      status_effect_emit(manager, manager->on_refresh, type,
                         manager->effects[existing_slot].duration,
                         manager->effects[existing_slot].strength);
      return true;
    } else if (new_priority > existing_priority) {
      // Higher priority effect replaces lower priority one
      manager->effects[existing_slot].duration = duration;
      manager->effects[existing_slot].strength = strength;
      status_effect_emit(manager, manager->on_refresh, type, duration,
                         strength);
      return true;
    } else {
      // Lower priority effect, ignore
      return false;
    }
  }

  // Find empty slot (respecting priority - remove lowest priority if full)
  u32 target_slot = MAX_STATUS_EFFECTS;
  u32 lowest_priority_slot = MAX_STATUS_EFFECTS;
  u32 lowest_priority = 100; // High number means low priority

  for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
    if (!manager->effects[i].active) {
      target_slot = i;
      break;
    } else {
      // Track lowest priority effect for replacement
      u32 priority = status_effect_get_priority(manager->effects[i].type);
      if (priority < lowest_priority) {
        lowest_priority = priority;
        lowest_priority_slot = i;
      }
    }
  }

  if (target_slot < MAX_STATUS_EFFECTS) {
    // Found empty slot
    manager->effects[target_slot].type = type;
    manager->effects[target_slot].duration = duration;
    manager->effects[target_slot].strength = strength;
    manager->effects[target_slot].active = true;
    manager->effect_count++;
    status_effect_emit(manager, manager->on_add, type, duration, strength);
    return true;
  } else if (lowest_priority_slot < MAX_STATUS_EFFECTS) {
    // No empty slots, replace lowest priority effect
    u32 new_priority = status_effect_get_priority(type);
    if (new_priority > lowest_priority) {
      StatusEffectType replaced_type =
          manager->effects[lowest_priority_slot].type;
      status_effect_emit(manager, manager->on_remove, replaced_type, 0.0f,
                         0.0f);

      manager->effects[lowest_priority_slot].type = type;
      manager->effects[lowest_priority_slot].duration = duration;
      manager->effects[lowest_priority_slot].strength = strength;
      manager->effects[lowest_priority_slot].active = true;
      status_effect_emit(manager, manager->on_add, type, duration, strength);
      return true;
    }
  }

  return false; // No space and new effect doesn't have higher priority
}

void status_effect_remove(StatusEffectManager *manager, StatusEffectType type) {
  if (!manager)
    return;

  for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
    if (manager->effects[i].type == type && manager->effects[i].active) {
      manager->effects[i].active = false;
      manager->effects[i].duration = 0.0f;
      manager->effect_count--;
      status_effect_emit(manager, manager->on_remove, type, 0.0f, 0.0f);
      return;
    }
  }
}

bool status_effect_has(StatusEffectManager *manager, StatusEffectType type) {
  if (!manager)
    return false;

  for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
    if (manager->effects[i].type == type && manager->effects[i].active) {
      return true;
    }
  }
  return false;
}

f32 status_effect_get_strength(StatusEffectManager *manager,
                               StatusEffectType type) {
  if (!manager)
    return 0.0f;

  for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
    if (manager->effects[i].type == type && manager->effects[i].active) {
      return manager->effects[i].strength;
    }
  }
  return 0.0f;
}

void status_effects_update(StatusEffectManager *manager, f32 delta_time) {
  if (!manager)
    return;

  // Update resistance timers
  status_effect_update_resistance(delta_time);

  // Update immunity timer
  if (manager->immunity_timer > 0.0f) {
    manager->immunity_timer -= delta_time;
    if (manager->immunity_timer < 0.0f) {
      manager->immunity_timer = 0.0f;
    }
  }

  // Update all active effects
  for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
    if (manager->effects[i].active) {
      manager->effects[i].duration -= delta_time;

      // Remove expired effects
      if (manager->effects[i].duration <= 0.0f) {
        StatusEffectType expired_type = manager->effects[i].type;
        manager->effects[i].active = false;
        manager->effects[i].duration = 0.0f;
        manager->effect_count--;

        // Set resistance after effect expires (immunity after exposure)
        status_effect_set_resistance(expired_type, RESISTANCE_DURATION);

        status_effect_emit(manager, manager->on_expire, expired_type, 0.0f,
                           0.0f);
      } else if (manager->effects[i].duration < 5.0f) {
        status_effect_emit(
            manager, manager->on_warning, manager->effects[i].type,
            manager->effects[i].duration, manager->effects[i].strength);
      }

      status_effect_emit_update(manager, manager->effects[i].type,
                                manager->effects[i].duration,
                                manager->effects[i].strength, delta_time);
    }
  }
}

void status_effects_clear(StatusEffectManager *manager) {
  if (!manager)
    return;

  for (u32 i = 0; i < MAX_STATUS_EFFECTS; i++) {
    manager->effects[i].active = false;
    manager->effects[i].duration = 0.0f;
  }
  manager->effect_count = 0;
  status_effect_emit(manager, manager->on_remove, STATUS_EFFECT_NONE, 0.0f,
                     0.0f);
}

bool status_effects_can_take_damage(StatusEffectManager *manager) {
  if (!manager)
    return true;

  // Check immunity timer
  if (manager->immunity_timer > 0) {
    return false;
  }
  return true;
}

void status_effects_set_immunity(StatusEffectManager *manager, f32 duration) {
  if (!manager)
    return;
  manager->immunity_timer = duration;
  status_effect_emit(manager, manager->on_immunity, STATUS_EFFECT_NONE,
                     duration, 0.0f);
}
