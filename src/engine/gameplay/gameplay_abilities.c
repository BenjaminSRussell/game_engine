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
