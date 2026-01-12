// gameplay_abilities.c - Implementation
#include "include/gameplay/gameplay_abilities.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

AbilitySystemComponent *gas_create_component(void *owner) {
  AbilitySystemComponent *comp = calloc(1, sizeof(AbilitySystemComponent));
  comp->owner_actor = owner;
  comp->attribute_set = calloc(1, sizeof(AttributeSet));
  LOG_INFO("GAS Component created");
  return comp;
}

void gas_give_ability(AbilitySystemComponent *comp, GameplayAbility *ability) {
  if (comp->ability_count >= 32)
    return;
  comp->abilities[comp->ability_count].ability = ability;
  comp->abilities[comp->ability_count].is_active = false;
  comp->ability_count++;
}

bool gas_try_activate_ability(AbilitySystemComponent *comp,
                              const char *ability_name) {
  for (u32 i = 0; i < comp->ability_count; i++) {
    if (strcmp(comp->abilities[i].ability->name, ability_name) == 0) {
      // Check tags and costs
      if (comp->abilities[i].ability->can_activate(comp->owner_actor,
                                                   &comp->abilities[i])) {
        comp->abilities[i].is_active = true;
        comp->abilities[i].ability->on_activate(comp->owner_actor,
                                                &comp->abilities[i]);
        return true;
      }
    }
  }
  return false;
}

// Additional GAS system functions for complete ability management
void gas_destroy_component(AbilitySystemComponent *comp) {
  // Free ability system component memory
  if (!comp) return;

  if (comp->attribute_set) {
    free(comp->attribute_set);
  }

  free(comp);
  LOG_INFO("GAS Component destroyed");
}

void gas_update(AbilitySystemComponent *comp, f32 delta_time) {
  // Update cooldowns and effect durations
  if (!comp) return;

  // Update ability cooldowns
  for (u32 i = 0; i < comp->ability_count; i++) {
    if (comp->abilities[i].cooldown_remaining > 0.0f) {
      comp->abilities[i].cooldown_remaining -= delta_time;
    }
  }

  LOG_DEBUG("Updated GAS component: %u abilities", comp->ability_count);
}

void gas_add_attribute(AbilitySystemComponent *comp, const char *attribute_name, f32 value) {
  // Add a new attribute to the ability system component
  if (!comp || !attribute_name) return;

  if (comp->attribute_set && comp->attribute_set->attribute_count < 32) {
    strncpy(comp->attribute_set->attributes[comp->attribute_set->attribute_count].name,
           attribute_name, 63);
    comp->attribute_set->attributes[comp->attribute_set->attribute_count].value = value;
    comp->attribute_set->attribute_count++;

    LOG_DEBUG("Added attribute '%s' with value %.2f", attribute_name, value);
  }
}

void gas_set_attribute_value(AbilitySystemComponent *comp, const char *attribute_name, f32 value) {
  // Modify attribute value
  if (!comp || !attribute_name || !comp->attribute_set) return;

  for (u32 i = 0; i < comp->attribute_set->attribute_count; i++) {
    if (strcmp(comp->attribute_set->attributes[i].name, attribute_name) == 0) {
      comp->attribute_set->attributes[i].value = value;
      LOG_DEBUG("Set attribute '%s' to %.2f", attribute_name, value);
      return;
    }
  }
}

f32 gas_get_attribute_value(AbilitySystemComponent *comp, const char *attribute_name) {
  // Query attribute value
  if (!comp || !attribute_name || !comp->attribute_set) return 0.0f;

  for (u32 i = 0; i < comp->attribute_set->attribute_count; i++) {
    if (strcmp(comp->attribute_set->attributes[i].name, attribute_name) == 0) {
      return comp->attribute_set->attributes[i].value;
    }
  }

  return 0.0f;
}

void gas_grant_ability(AbilitySystemComponent *comp, GameplayAbility *ability) {
  // Unlock ability for use
  if (!comp || !ability) return;

  gas_give_ability(comp, ability);
  LOG_INFO("Granted ability '%s' to entity", ability->name);
}

bool gas_activate_ability(AbilitySystemComponent *comp, const char *ability_name) {
  // Execute ability with validation
  if (!comp || !ability_name) return false;

  if (!gas_can_activate(comp, ability_name)) {
    LOG_WARN("Cannot activate ability '%s' - prerequisites not met", ability_name);
    return false;
  }

  return gas_try_activate_ability(comp, ability_name);
}

bool gas_can_activate(AbilitySystemComponent *comp, const char *ability_name) {
  // Check ability prerequisites (cooldown, costs, state)
  if (!comp || !ability_name) return false;

  for (u32 i = 0; i < comp->ability_count; i++) {
    if (strcmp(comp->abilities[i].ability->name, ability_name) == 0) {
      // Check cooldown
      if (comp->abilities[i].cooldown_remaining > 0.0f) {
        LOG_DEBUG("Ability '%s' on cooldown for %.2f seconds", ability_name,
                 comp->abilities[i].cooldown_remaining);
        return false;
      }

      // Check if already active
      if (comp->abilities[i].is_active) {
        LOG_DEBUG("Ability '%s' already active", ability_name);
        return false;
      }

      return true;
    }
  }

  LOG_WARN("Ability '%s' not found", ability_name);
  return false;
}
