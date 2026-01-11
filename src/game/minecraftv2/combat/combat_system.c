#include <combat/combat.h>
#include <string.h>
#include <game_common.h>

void combat_system_init(CombatSystem *system) {
    if (!system) return;
    
    memset(system, 0, sizeof(CombatSystem));
    
    system->damage_multiplier = 1.0f;
    system->critical_chance = 0.05f; // 5% base crit chance
    system->critical_multiplier = 1.5f; // 150% crit damage
    system->crit_damage_multiplier = 1.0f;
    system->combo_timeout = 2.0f;
    system->attack_cooldown = 0.5f;
    
    system->stance = COMBAT_STANCE_BALANCED;
    system->stance_damage_multiplier = 1.0f;
    system->stance_defense_multiplier = 1.0f;
    
    for (int i = 0; i < DAMAGE_TYPE_COUNT; i++) {
        system->damage_resistance[i] = 0.0f;
        system->damage_vulnerability[i] = 1.0f;
    }
    
    system->finisher_threshold = 0.2f; // 20% health for finishers
    system->finisher_enabled = true;
    system->logging_enabled = true;
}

void combat_system_update(CombatSystem *system, f32 delta_time) {
    if (!system) return;
    
    // Update combo timer
    if (system->combo_count > 0) {
        system->combo_timer += delta_time;
        if (system->combo_timer > system->combo_timeout) {
            system->combo_count = 0;
            system->combo_timer = 0.0f;
            system->combo_multiplier = 1.0f;
        }
    }
    
    // Update DoTs
    for (u32 i = 0; i < system->dot_count; ) {
        system->dots[i].remaining_time -= delta_time;
        if (system->dots[i].remaining_time <= 0.0f) {
            // Remove expired DoT (swap with last)
            system->dots[i] = system->dots[--system->dot_count];
        } else {
            // Apply DoT damage logic would go here (requires ECS access)
            i++;
        }
    }
}

f32 combat_calculate_damage(CombatSystem *system, f32 base_damage, bool is_critical) {
    if (!system) return base_damage;
    
    f32 damage = base_damage * system->damage_multiplier * system->stance_damage_multiplier;
    
    if (is_critical) {
        damage *= (system->critical_multiplier * system->crit_damage_multiplier);
    }
    
    // Apply combo multiplier if applicable
    if (system->combo_count > 0) {
        damage *= (1.0f + (system->combo_count * 0.1f)); // 10% per combo hit
    }
    
    return damage;
}

// Stub for now, would integrate with ECS to apply health changes
bool combat_apply_damage(CombatSystem *system, struct World *ecs, EntityID target, f32 damage, DamageType type) {
    (void)system; (void)ecs; (void)target; (void)damage; (void)type;
    return true; 
}

void combat_animations_update(void *system, f32 delta_time) {
     (void)system; (void)delta_time;
     // Placeholder for animation updates
}
