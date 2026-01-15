// gameplay_abilities.c - Implementation
#include "include/gameplay/gameplay_abilities.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Forward declaration
bool gas_can_activate(AbilitySystemComponent *comp, const char *ability_name);

AbilitySystemComponent *gas_create_component(void *owner) {
  AbilitySystemComponent *comp = calloc(1, sizeof(AbilitySystemComponent));
  comp->owner_actor = owner;
  comp->attribute_set = calloc(1, sizeof(AttributeSet));
  LOGI("GAS Component created");
  return comp;
}

void gas_give_ability(AbilitySystemComponent *comp, GameplayAbility *ability) {
  if (comp->ability_count >= 32)
    return;
  comp->abilities[comp->ability_count].ability = ability;
  comp->abilities[comp->ability_count].is_active = false;
  comp->abilities[comp->ability_count].cooldown_remaining = 0.0f;
  comp->ability_count++;
}

void gas_apply_effect_to_self(AbilitySystemComponent *comp, GameplayEffect *effect, void *source) {
    if (!comp || !effect || !comp->attribute_set) return;

    // Apply immediate changes
    if (effect->duration_type == EFFECT_INSTANT) {
        if (strlen(effect->target_attribute) > 0) {
             // Find attribute
             for (u32 i = 0; i < comp->attribute_set->attribute_count; i++) {
                if (strcmp(comp->attribute_set->attributes[i].name, effect->target_attribute) == 0) {
                     comp->attribute_set->attributes[i].current_value += effect->magnitude;
                     // Clamp
                     if (comp->attribute_set->attributes[i].current_value > comp->attribute_set->attributes[i].max_value)
                        comp->attribute_set->attributes[i].current_value = comp->attribute_set->attributes[i].max_value;
                     if (comp->attribute_set->attributes[i].current_value < comp->attribute_set->attributes[i].min_value)
                        comp->attribute_set->attributes[i].current_value = comp->attribute_set->attributes[i].min_value;

                     LOGD("Applied instant effect '%s' to '%s': %.2f", effect->name, effect->target_attribute, effect->magnitude);
                }
             }
        }
    } else {
        // Duration effect
        if (comp->active_effect_count < MAX_ACTIVE_EFFECTS) {
            ActiveGameplayEffect *ae = &comp->active_effects[comp->active_effect_count++];
            ae->effect = effect;
            ae->time_remaining = effect->duration;
            ae->start_time = 0; // TODO: use real time
            ae->source_actor = source;
            ae->stack_count = 1;
            LOGD("Applied duration effect '%s' for %.2fs", effect->name, effect->duration);
        }
    }
}

bool gas_try_activate_ability(AbilitySystemComponent *comp,
                              const char *ability_name) {
  for (u32 i = 0; i < comp->ability_count; i++) {
    if (strcmp(comp->abilities[i].ability->name, ability_name) == 0) {
      AbilitySpec *spec = &comp->abilities[i];
      GameplayAbility *ability = spec->ability;

      // Check tags and costs (custom logic)
      if (ability->can_activate(comp->owner_actor, spec)) {

        // Check cooldown again to be safe
        if (spec->cooldown_remaining > 0.0f) {
            return false;
        }

        // Apply cost effect if present
        if (ability->cost_effect) {
             gas_apply_effect_to_self(comp, ability->cost_effect, comp->owner_actor);
        }

        // Start cooldown if present
        if (ability->cooldown_effect) {
             gas_apply_effect_to_self(comp, ability->cooldown_effect, comp->owner_actor);
             spec->cooldown_remaining = ability->cooldown_effect->duration;
        }

        spec->is_active = true;
        ability->on_activate(comp->owner_actor, spec);
        return true;
      }
    }
  }
  return false;
}

void gas_cancel_ability(AbilitySystemComponent *comp, const char *ability_name) {
  for (u32 i = 0; i < comp->ability_count; i++) {
    if (strcmp(comp->abilities[i].ability->name, ability_name) == 0) {
        if (comp->abilities[i].is_active) {
            comp->abilities[i].is_active = false;
            if (comp->abilities[i].ability->on_end) {
                comp->abilities[i].ability->on_end(comp->owner_actor, &comp->abilities[i]);
            }
        }
        return;
    }
  }
}

// Additional GAS system functions for complete ability management
void gas_destroy_component(AbilitySystemComponent *comp) {
  // Free ability system component memory
  if (!comp) return;

  if (comp->attribute_set) {
    free(comp->attribute_set);
  }

  free(comp);
  LOGI("GAS Component destroyed");
}

void gas_update(AbilitySystemComponent *comp, f32 delta_time) {
  if (!comp) return;

  // Update ability cooldowns
  for (u32 i = 0; i < comp->ability_count; i++) {
    if (comp->abilities[i].cooldown_remaining > 0.0f) {
      comp->abilities[i].cooldown_remaining -= delta_time;
      if (comp->abilities[i].cooldown_remaining < 0.0f) {
          comp->abilities[i].cooldown_remaining = 0.0f;
      }
    }
  }

  // Update active effects
  for (u32 i = 0; i < comp->active_effect_count; i++) {
      ActiveGameplayEffect *ae = &comp->active_effects[i];
      if (ae->time_remaining > 0.0f) {
          ae->time_remaining -= delta_time;
          if (ae->time_remaining <= 0.0f) {
              // Effect expired - remove by swapping with last
              comp->active_effects[i] = comp->active_effects[comp->active_effect_count - 1];
              comp->active_effect_count--;
              i--; // Re-check this index
          }
      }
  }
}

void gas_add_attribute(AbilitySystemComponent *comp, const char *attribute_name, f32 value, f32 max) {
  if (!comp || !attribute_name || !comp->attribute_set) return;

  if (comp->attribute_set->attribute_count < MAX_ATTRIBUTES) {
    u32 index = comp->attribute_set->attribute_count;
    strncpy(comp->attribute_set->attributes[index].name, attribute_name, 31);
    comp->attribute_set->attributes[index].name[31] = '\0'; // Ensure null term
    comp->attribute_set->attributes[index].base_value = value;
    comp->attribute_set->attributes[index].current_value = value;
    comp->attribute_set->attributes[index].max_value = max;
    comp->attribute_set->attributes[index].min_value = 0.0f;

    comp->attribute_set->attribute_count++;

    LOGD("Added attribute '%s' with value %.2f/%.2f", attribute_name, value, max);
  }
}

void gas_set_attribute_value(AbilitySystemComponent *comp, const char *attribute_name, f32 value) {
  if (!comp || !attribute_name || !comp->attribute_set) return;

  for (u32 i = 0; i < comp->attribute_set->attribute_count; i++) {
    if (strcmp(comp->attribute_set->attributes[i].name, attribute_name) == 0) {
      comp->attribute_set->attributes[i].current_value = value;
      // Clamp
      if (comp->attribute_set->attributes[i].current_value > comp->attribute_set->attributes[i].max_value)
          comp->attribute_set->attributes[i].current_value = comp->attribute_set->attributes[i].max_value;
      if (comp->attribute_set->attributes[i].current_value < comp->attribute_set->attributes[i].min_value)
          comp->attribute_set->attributes[i].current_value = comp->attribute_set->attributes[i].min_value;

      LOGD("Set attribute '%s' to %.2f", attribute_name, comp->attribute_set->attributes[i].current_value);
      return;
    }
  }
}

f32 gas_get_attribute_value(AbilitySystemComponent *comp, const char *attribute_name) {
  if (!comp || !attribute_name || !comp->attribute_set) return 0.0f;

  for (u32 i = 0; i < comp->attribute_set->attribute_count; i++) {
    if (strcmp(comp->attribute_set->attributes[i].name, attribute_name) == 0) {
      return comp->attribute_set->attributes[i].current_value;
    }
  }

  return 0.0f;
}

void gas_remove_effect(AbilitySystemComponent *comp, const char *effect_name) {
    if (!comp) return;
    for (u32 i = 0; i < comp->active_effect_count; i++) {
        if (strcmp(comp->active_effects[i].effect->name, effect_name) == 0) {
            // Remove
            comp->active_effects[i] = comp->active_effects[comp->active_effect_count - 1];
            comp->active_effect_count--;
            i--;
        }
    }
}

bool gas_has_tag(AbilitySystemComponent *comp, const char *tag_string) {
    if (!comp) return false;
    // Not implemented fully as tags are currently string based in checking logic
    return false;
}


void gas_grant_ability(AbilitySystemComponent *comp, GameplayAbility *ability) {
  if (!comp || !ability) return;
  gas_give_ability(comp, ability);
  LOGI("Granted ability '%s' to entity", ability->name);
}

bool gas_activate_ability(AbilitySystemComponent *comp, const char *ability_name) {
  if (!comp || !ability_name) return false;

  if (!gas_can_activate(comp, ability_name)) {
    LOGW("Cannot activate ability '%s' - prerequisites not met", ability_name);
    return false;
  }

  return gas_try_activate_ability(comp, ability_name);
}

bool gas_can_activate(AbilitySystemComponent *comp, const char *ability_name) {
  if (!comp || !ability_name) return false;

  for (u32 i = 0; i < comp->ability_count; i++) {
    if (strcmp(comp->abilities[i].ability->name, ability_name) == 0) {
      if (comp->abilities[i].cooldown_remaining > 0.0f) {
        LOGD("Ability '%s' on cooldown for %.2f seconds", ability_name,
                 comp->abilities[i].cooldown_remaining);
        return false;
      }
      if (comp->abilities[i].is_active) {
        LOGD("Ability '%s' already active", ability_name);
        return false;
      }

      // Check cost
      if (comp->abilities[i].ability->cost_effect) {
          GameplayEffect *cost = comp->abilities[i].ability->cost_effect;
          if (strlen(cost->target_attribute) > 0) {
             f32 val = gas_get_attribute_value(comp, cost->target_attribute);
             if (val + cost->magnitude < 0) {
                 LOGD("Not enough resource '%s' for ability '%s'", cost->target_attribute, ability_name);
                 return false;
             }
          }
      }

      return true;
    }
  }

  LOGW("Ability '%s' not found", ability_name);
  return false;
}
