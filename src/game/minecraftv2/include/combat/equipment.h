// include/combat/equipment.h
//
// Purpose: Equipment component system for managing weapons and armor on
// entities This system integrates with the ECS to provide equipment slots and
// stats
//
#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <combat/combat.h>
#include <ecs/ecs.h>
#include <game_common.h>

// Armor slots
// ArmorSlot is defined in combat.h

// Equipment component - attached to entities to track their equipment
typedef struct {
  Weapon equipped_weapon;                 // Currently equipped weapon
  Armor equipped_armor[ARMOR_SLOT_COUNT]; // Armor pieces (one per slot)
  bool has_weapon;                        // True if weapon is equipped
  bool has_armor[ARMOR_SLOT_COUNT]; // True for each armor slot if equipped

  // Calculated stats (cached for performance)
  f32 total_armor_defense;        // Sum of all armor defense values
  f32 total_armor_toughness;      // Sum of all armor toughness values
  f32 total_knockback_resistance; // Combined knockback resistance (0-1)
} EquipmentComponent;

// Equipment system functions
void equipment_component_init(EquipmentComponent *equipment);
void equipment_component_free(EquipmentComponent *equipment);

// Equip/unequip functions
bool equipment_equip_weapon(EquipmentComponent *equipment,
                            const Weapon *weapon);
bool equipment_unequip_weapon(EquipmentComponent *equipment,
                              Weapon *out_weapon);
bool equipment_equip_armor(EquipmentComponent *equipment, ArmorSlot slot,
                           const Armor *armor);
bool equipment_unequip_armor(EquipmentComponent *equipment, ArmorSlot slot,
                             Armor *out_armor);

// Query functions
const Weapon *equipment_get_weapon(const EquipmentComponent *equipment);
const Armor *equipment_get_armor(const EquipmentComponent *equipment,
                                 ArmorSlot slot);
bool equipment_has_weapon(const EquipmentComponent *equipment);
bool equipment_has_armor(const EquipmentComponent *equipment, ArmorSlot slot);

// Calculate and cache stats
void equipment_recalculate_stats(EquipmentComponent *equipment);
f32 equipment_get_total_defense(const EquipmentComponent *equipment);
f32 equipment_get_total_toughness(const EquipmentComponent *equipment);
f32 equipment_get_knockback_resistance(const EquipmentComponent *equipment);

// Apply durability damage
void equipment_damage_weapon(EquipmentComponent *equipment, u32 damage);
void equipment_damage_armor(EquipmentComponent *equipment, ArmorSlot slot,
                            u32 damage);

// ECS integration helpers
EquipmentComponent *equipment_get_component(World *ecs, EntityID entity);
EquipmentComponent *equipment_add_component(World *ecs, EntityID entity);
void equipment_remove_component(World *ecs, EntityID entity);

#endif // EQUIPMENT_H
