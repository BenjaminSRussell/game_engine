#include <combat/equipment.h>
#include <string.h>

void equipment_component_init(EquipmentComponent *equipment) {
    if (!equipment) return;
    memset(equipment, 0, sizeof(EquipmentComponent));
}

void equipment_component_free(EquipmentComponent *equipment) {
    if (!equipment) return;
    // Nothing dynamic to free yet
    memset(equipment, 0, sizeof(EquipmentComponent));
}

bool equipment_equip_weapon(EquipmentComponent *equipment, const Weapon *weapon) {
    if (!equipment || !weapon) return false;
    
    equipment->equipped_weapon = *weapon;
    equipment->has_weapon = true;
    
    return true;
}

bool equipment_unequip_weapon(EquipmentComponent *equipment, Weapon *out_weapon) {
    if (!equipment || !equipment->has_weapon) return false;
    
    if (out_weapon) {
        *out_weapon = equipment->equipped_weapon;
    }
    
    memset(&equipment->equipped_weapon, 0, sizeof(Weapon));
    equipment->has_weapon = false;
    
    return true;
}

bool equipment_equip_armor(EquipmentComponent *equipment, ArmorSlot slot, const Armor *armor) {
    if (!equipment || !armor || slot >= ARMOR_SLOT_COUNT) return false;
    
    equipment->equipped_armor[slot] = *armor;
    equipment->has_armor[slot] = true;
    
    equipment_recalculate_stats(equipment);
    return true;
}

bool equipment_unequip_armor(EquipmentComponent *equipment, ArmorSlot slot, Armor *out_armor) {
    if (!equipment || slot >= ARMOR_SLOT_COUNT || !equipment->has_armor[slot]) return false;
    
    if (out_armor) {
        *out_armor = equipment->equipped_armor[slot];
    }
    
    memset(&equipment->equipped_armor[slot], 0, sizeof(Armor));
    equipment->has_armor[slot] = false;
    
    equipment_recalculate_stats(equipment);
    return true;
}

void equipment_recalculate_stats(EquipmentComponent *equipment) {
    if (!equipment) return;
    
    equipment->total_armor_defense = 0.0f;
    equipment->total_armor_toughness = 0.0f;
    equipment->total_knockback_resistance = 0.0f;
    
    for (int i = 0; i < ARMOR_SLOT_COUNT; i++) {
        if (equipment->has_armor[i]) {
            equipment->total_armor_defense += equipment->equipped_armor[i].defense;
            equipment->total_armor_toughness += equipment->equipped_armor[i].toughness;
            equipment->total_knockback_resistance += equipment->equipped_armor[i].knockback_resistance;
        }
    }
    
    // Clamp knockback resistance
    if (equipment->total_knockback_resistance > 1.0f) {
        equipment->total_knockback_resistance = 1.0f;
    }
}

EquipmentComponent *equipment_add_component(World *ecs, EntityID entity) {
    (void)ecs; (void)entity;
    // Stub: Would call ECS generic add component
    return NULL; 
}

EquipmentComponent *equipment_get_component(World *ecs, EntityID entity) {
    (void)ecs; (void)entity;
    // Stub
    return NULL;
}
