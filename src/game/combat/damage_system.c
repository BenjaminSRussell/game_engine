#include "game/combat/damage_system.h"

void damage_system_init() {}

void damage_apply(void *attacker, void *target, float damage, int damage_type) {
    // Apply damage with resistances, armor, etc.
}

void damage_apply_dot(void *target, float damage_per_second, float duration) {
    // Damage over time
}

void damage_calculate_critical(void *attacker, float base_damage, float *final_damage) {
    *final_damage = base_damage * 2.0f; // Crit multiplier
}
