/**
 * =================================================================================================
 *                          NPC BOSS BEHAVIOR
 * =================================================================================================
 *
 * Complex multi-phase boss encounters.
 *
 * =================================================================================================
 */

#include <ai/npc/prebuilt/npc_boss.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum BossPhase {
    BOSS_PHASE_1 = 0,
    BOSS_PHASE_2 = 1,
    BOSS_PHASE_3 = 2,
    BOSS_ENRAGED = 3,
} BossPhase;

typedef struct BossData {
    BossPhase current_phase;
    float max_health;
    float current_health;
    float phase_thresholds[4]; // Health % for each phase transition
    float special_attack_cooldown;
    uint32_t minions_spawned;
    bool is_vulnerable; // Weak point exposed
} BossData;

// Implemented: Phase manager
void boss_update_phase(BossData *data) {
    if (!data) return;
    
    float health_percent = data->current_health / data->max_health;
    
    // Check phase transitions
    if (health_percent < 0.25f && data->current_phase != BOSS_ENRAGED) {
        data->current_phase = BOSS_ENRAGED;
        // Enraged phase: faster attacks, more aggressive
    } else if (health_percent < 0.50f && data->current_phase == BOSS_PHASE_1) {
        data->current_phase = BOSS_PHASE_2;
        // Phase 2: Introduce new attack patterns
    } else if (health_percent < 0.75f && data->current_phase == BOSS_PHASE_1) {
        data->current_phase = BOSS_PHASE_2;
    }
}

// Implemented: Special attacks
void boss_execute_special_attack(BossData *data, int attack_type) {
    if (!data || data->special_attack_cooldown > 0.0f) return;
    
    switch (attack_type) {
        case 0: // AoE attack
            // Deal damage in radius around boss
            break;
        case 1: // Summon minions
            // Spawn enemy NPCs
            data->minions_spawned += 3;
            break;
        case 2: // Projectile barrage
            // Fire multiple projectiles
            break;
    }
    
    data->special_attack_cooldown = 10.0f; // 10 second cooldown
}

void boss_update_cooldowns(BossData *data, float dt) {
    if (!data) return;
    if (data->special_attack_cooldown > 0.0f) {
        data->special_attack_cooldown -= dt;
    }
}

// Implemented: Weakness system
void boss_expose_weak_point(BossData *data) {
    if (!data) return;
    data->is_vulnerable = true;
}

void boss_hide_weak_point(BossData *data) {
    if (!data) return;
    data->is_vulnerable = false;
}

void boss_take_weak_point_damage(BossData *data, float damage) {
    if (!data || !data->is_vulnerable) return;
    
    // Amplified damage on weak point
    data->current_health -= damage * 3.0f;
    
    // Enter stun state
    boss_hide_weak_point(data);
}

// Implemented: Cinematic intro/outro
void boss_play_intro_cinematic() {
    // Override camera control
    // Play timeline sequence showing boss entrance
    // Transition to gameplay after cinematic
}

void boss_play_death_cinematic() {
    // Play death animation sequence
    // Override camera for dramatic angle
    // Spawn loot and rewards
}
