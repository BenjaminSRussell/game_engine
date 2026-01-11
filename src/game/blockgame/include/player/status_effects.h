// Status Effect System for Player
#ifndef PLAYER_STATUS_EFFECTS_H
#define PLAYER_STATUS_EFFECTS_H

#include "../game_common.h"

// Roadmap: docs/STATUS_EFFECTS_ROADMAP.md.

// Status effect types
typedef enum {
  STATUS_EFFECT_NONE = 0,
  STATUS_EFFECT_POISON,
  STATUS_EFFECT_REGENERATION,
  STATUS_EFFECT_FIRE_RESISTANCE,
  STATUS_EFFECT_WATER_BREATHING,
  STATUS_EFFECT_SPEED,
  STATUS_EFFECT_SLOWNESS,
  STATUS_EFFECT_STRENGTH,
  STATUS_EFFECT_WEAKNESS,
  STATUS_EFFECT_JUMP_BOOST,
  STATUS_EFFECT_INVISIBILITY,
  STATUS_EFFECT_ON_FIRE,
  STATUS_EFFECT_COUNT
} StatusEffectType;

// Individual status effect
typedef struct {
  StatusEffectType type;
  f32 duration; // Remaining duration in seconds
  f32 strength; // Effect strength/amplifier
  bool active;
} StatusEffect;

typedef void (*StatusEffectEventCallback)(StatusEffectType type, f32 duration,
                                          f32 strength, void *user_data);
typedef void (*StatusEffectUpdateCallback)(StatusEffectType type, f32 duration,
                                           f32 strength, f32 delta_time,
                                           void *user_data);

// Status effect manager
#define MAX_STATUS_EFFECTS 16

typedef struct {
  StatusEffect effects[MAX_STATUS_EFFECTS];
  u32 effect_count;
  f32 immunity_timer; // Damage immunity frames
  StatusEffectEventCallback on_add;
  StatusEffectEventCallback on_refresh;
  StatusEffectEventCallback on_remove;
  StatusEffectEventCallback on_expire;
  StatusEffectEventCallback on_warning;
  StatusEffectEventCallback on_immunity;
  StatusEffectUpdateCallback on_update;
  void *user_data;
} StatusEffectManager;

// Initialize status effect manager
void status_effects_init(StatusEffectManager *manager);

// Add a status effect
bool status_effect_add(StatusEffectManager *manager, StatusEffectType type,
                       f32 duration, f32 strength);

// Remove a status effect
void status_effect_remove(StatusEffectManager *manager, StatusEffectType type);

// Check if has status effect
bool status_effect_has(StatusEffectManager *manager, StatusEffectType type);

// Get status effect strength
f32 status_effect_get_strength(StatusEffectManager *manager,
                               StatusEffectType type);

// Update all status effects
void status_effects_update(StatusEffectManager *manager, f32 delta_time);

// Clear all status effects
void status_effects_clear(StatusEffectManager *manager);

// Check if can take damage (immunity frames)
bool status_effects_can_take_damage(StatusEffectManager *manager);

// Set immunity frames
void status_effects_set_immunity(StatusEffectManager *manager, f32 duration);

#endif // PLAYER_STATUS_EFFECTS_H
