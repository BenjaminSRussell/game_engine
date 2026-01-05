// src/combat/equipment.c
//
// Equipment component implementation for managing weapons and armor
//
#include <combat/equipment.h>
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <string.h>

// EQUIPMENT_COMPONENT_ID is defined in component_ids.h

void equipment_component_init(EquipmentComponent *equipment) {
  if (!equipment)
    return;

  memset(equipment, 0, sizeof(EquipmentComponent));
  equipment->has_weapon = false;
  for (u32 i = 0; i < ARMOR_SLOT_COUNT; i++) {
    equipment->has_armor[i] = false;
  }
  equipment->total_armor_defense = 0.0f;
  equipment->total_armor_toughness = 0.0f;
  equipment->total_knockback_resistance = 0.0f;
}

void equipment_component_free(EquipmentComponent *equipment) {
  (void)equipment; // No dynamic allocation currently
}

bool equipment_equip_weapon(EquipmentComponent *equipment,
                            const Weapon *weapon) {
  if (!equipment || !weapon) {
    return false;
  }

  // Check if weapon is valid (not broken, etc.)
  if (weapon->durability == 0 && weapon->max_durability > 0) {
    LOG_WARN("Cannot equip broken weapon");
    return false;
  }

  // Validate weapon has valid damage
  if (weapon->damage < 0.0f) {
    LOG_WARN("Cannot equip weapon with invalid damage");
    return false;
  }

  equipment->equipped_weapon = *weapon;
  equipment->has_weapon = true;

  LOG_DEBUG("Weapon equipped: type=%d, damage=%.2f", weapon->type,
            weapon->damage);
  return true;
}

bool equipment_unequip_weapon(EquipmentComponent *equipment,
                              Weapon *out_weapon) {
  if (!equipment || !equipment->has_weapon) {
    return false;
  }

  if (out_weapon) {
    *out_weapon = equipment->equipped_weapon;
  }

  equipment->has_weapon = false;
  memset(&equipment->equipped_weapon, 0, sizeof(Weapon));

  return true;
}

bool equipment_equip_armor(EquipmentComponent *equipment, ArmorSlot slot,
                           const Armor *armor) {
  if (!equipment || !armor || slot >= ARMOR_SLOT_COUNT) {
    return false;
  }

  // Validate armor slot matches armor type
  if (armor->slot != slot) {
    LOG_WARN("Armor slot mismatch: armor slot %u != requested slot %u",
             armor->slot, slot);
    return false;
  }

  // Check if armor is valid (not broken, etc.)
  if (armor->durability == 0 && armor->max_durability > 0) {
    LOG_WARN("Cannot equip broken armor in slot %u", slot);
    return false;
  }

  // Validate armor has valid defense
  if (armor->defense < 0.0f) {
    LOG_WARN("Cannot equip armor with invalid defense");
    return false;
  }

  equipment->equipped_armor[slot] = *armor;
  equipment->has_armor[slot] = true;

  // Recalculate stats
  equipment_recalculate_stats(equipment);

  LOG_DEBUG("Armor equipped: slot=%u, defense=%.2f", slot, armor->defense);
  return true;
}

bool equipment_unequip_armor(EquipmentComponent *equipment, ArmorSlot slot,
                             Armor *out_armor) {
  if (!equipment || slot >= ARMOR_SLOT_COUNT || !equipment->has_armor[slot]) {
    return false;
  }

  if (out_armor) {
    *out_armor = equipment->equipped_armor[slot];
  }

  equipment->has_armor[slot] = false;
  memset(&equipment->equipped_armor[slot], 0, sizeof(Armor));

  // Recalculate stats
  equipment_recalculate_stats(equipment);

  return true;
}

const Weapon *equipment_get_weapon(const EquipmentComponent *equipment) {
  if (!equipment || !equipment->has_weapon) {
    return NULL;
  }
  return &equipment->equipped_weapon;
}

const Armor *equipment_get_armor(const EquipmentComponent *equipment,
                                 ArmorSlot slot) {
  if (!equipment || slot >= ARMOR_SLOT_COUNT || !equipment->has_armor[slot]) {
    return NULL;
  }
  return &equipment->equipped_armor[slot];
}

bool equipment_has_weapon(const EquipmentComponent *equipment) {
  return equipment && equipment->has_weapon;
}

bool equipment_has_armor(const EquipmentComponent *equipment, ArmorSlot slot) {
  return equipment && slot < ARMOR_SLOT_COUNT && equipment->has_armor[slot];
}

void equipment_recalculate_stats(EquipmentComponent *equipment) {
  if (!equipment)
    return;

  equipment->total_armor_defense = 0.0f;
  equipment->total_armor_toughness = 0.0f;
  equipment->total_knockback_resistance = 0.0f;

  // Sum up all armor stats
  for (u32 i = 0; i < ARMOR_SLOT_COUNT; i++) {
    if (equipment->has_armor[i]) {
      const Armor *armor = &equipment->equipped_armor[i];
      equipment->total_armor_defense += armor->defense;
      equipment->total_armor_toughness += armor->toughness;

      // Calculate combined knockback resistance (multiplicative stacking)
      f32 resistance = armor->knockback_resistance;
      equipment->total_knockback_resistance =
          1.0f -
          (1.0f - equipment->total_knockback_resistance) * (1.0f - resistance);
    }
  }

  // Clamp knockback resistance to [0, 1]
  if (equipment->total_knockback_resistance > 1.0f) {
    equipment->total_knockback_resistance = 1.0f;
  }
}

f32 equipment_get_total_defense(const EquipmentComponent *equipment) {
  return equipment ? equipment->total_armor_defense : 0.0f;
}

f32 equipment_get_total_toughness(const EquipmentComponent *equipment) {
  return equipment ? equipment->total_armor_toughness : 0.0f;
}

f32 equipment_get_knockback_resistance(const EquipmentComponent *equipment) {
  return equipment ? equipment->total_knockback_resistance : 0.0f;
}

void equipment_damage_weapon(EquipmentComponent *equipment, u32 damage) {
  if (!equipment || !equipment->has_weapon) {
    return;
  }

  Weapon *weapon = &equipment->equipped_weapon;
  if (weapon->durability >= damage) {
    weapon->durability -= damage;
  } else {
    weapon->durability = 0;
    LOG_DEBUG("Weapon broken");
    // Auto-unequip broken weapon
    equipment->has_weapon = false;
    memset(&equipment->equipped_weapon, 0, sizeof(Weapon));
  }
}

void equipment_damage_armor(EquipmentComponent *equipment, ArmorSlot slot,
                            u32 damage) {
  if (!equipment || slot >= ARMOR_SLOT_COUNT || !equipment->has_armor[slot]) {
    return;
  }

  Armor *armor = &equipment->equipped_armor[slot];
  if (armor->durability >= damage) {
    armor->durability -= damage;
  } else {
    armor->durability = 0;
    LOG_DEBUG("Armor broken in slot %u", slot);
    // Auto-unequip broken armor
    equipment->has_armor[slot] = false;
    memset(&equipment->equipped_armor[slot], 0, sizeof(Armor));
    // Recalculate stats after armor breaks
    equipment_recalculate_stats(equipment);
  }
}

// ECS integration
EquipmentComponent *equipment_get_component(struct World *ecs,
                                            EntityID entity) {
  if (!ecs)
    return NULL;
  return (EquipmentComponent *)ecs_get_component(
      (World *)ecs, (Entity){entity, 0}, EQUIPMENT_COMPONENT_ID);
}

EquipmentComponent *equipment_add_component(struct World *ecs,
                                            EntityID entity) {
  if (!ecs)
    return NULL;
  EquipmentComponent *comp = (EquipmentComponent *)ecs_add_component(
      (World *)ecs, (Entity){entity, 0}, EQUIPMENT_COMPONENT_ID, NULL);
  if (comp) {
    equipment_component_init(comp);
  }
  return comp;
}

void equipment_remove_component(struct World *ecs, EntityID entity) {
  if (!ecs)
    return;
  ecs_remove_component((World *)ecs, (Entity){entity, 0},
                       EQUIPMENT_COMPONENT_ID);
}
