#ifndef DAMAGE_CALC_H
#define DAMAGE_CALC_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ARMOR_TYPE_NONE = 0,
    ARMOR_TYPE_LEATHER,
    ARMOR_TYPE_CHAIN,
    ARMOR_TYPE_IRON,
    ARMOR_TYPE_GOLD,
    ARMOR_TYPE_DIAMOND,
    ARMOR_TYPE_NETHERITE,
    ARMOR_TYPE_COUNT
} ArmorType;

typedef enum {
    DAMAGE_TYPE_PHYSICAL = 0,
    DAMAGE_TYPE_FIRE,
    DAMAGE_TYPE_MAGIC,
    DAMAGE_TYPE_POISON,
    DAMAGE_TYPE_FALL,
    DAMAGE_TYPE_DROWNING,
    DAMAGE_TYPE_EXPLOSION,
    DAMAGE_TYPE_LIGHTNING,
    DAMAGE_TYPE_COLD,
    DAMAGE_TYPE_THORNS,
    DAMAGE_TYPE_STARVATION,
    DAMAGE_TYPE_SUFFOCATION,
    DAMAGE_TYPE_LAVA,
    DAMAGE_TYPE_COUNT
} DamageType;

/* Calculate armor mitigation for damage.
   Returns the percentage of damage that is mitigated (0.0 to 1.0).
   Takes armor type, durability ratio, damage type, and armor level into account. */
f32 damage_calc_armor_mitigation(ArmorType armor, f32 durability_ratio, DamageType dtype);

/* Apply armor mitigation to raw damage amount.
   Returns the actual damage to be taken after armor reduction. */
f32 damage_apply_armor(f32 raw_damage, ArmorType armor, f32 durability_ratio, DamageType dtype);

#ifdef __cplusplus
}
#endif

#endif /* DAMAGE_CALC_H */
