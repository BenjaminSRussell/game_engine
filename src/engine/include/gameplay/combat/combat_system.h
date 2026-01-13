// include/gameplay/combat/combat_system.h
//
// Purpose: Main entry point for the combat system. Integrates hitboxes,
// damage processing, and projectile physics into a single system.
//
#ifndef COMBAT_SYSTEM_H
#define COMBAT_SYSTEM_H

#include "engine/include/common.h"
#include <ecs/ecs.h>
#include "engine/include/math/math.h"

// ============================================================================
// SYSTEM LIFECYCLE
// ============================================================================

// Initialize combat system and register components
bool combat_system_init(World *world);

// Shutdown combat system and cleanup subsystems
void combat_system_shutdown(void);

// ============================================================================
// SYSTEM UPDATE
// ============================================================================

// Main update loop for combat mechanics
// Processes hitboxes, projectiles, and damage events
void combat_system_update(World *world, f32 delta_time);

// Internal update for hitbox collisions
void combat_system_update_hitboxes(World *world, f32 delta_time);

// Update combo timers and processing
void combat_process_combos(World *world, f32 delta_time);

// Update status effects on all entities
void combat_update_status_effects(World *world, f32 delta_time);

// Process area of effect damage
void combat_process_area_effects(World *world, f32 delta_time);

// ============================================================================
// COMBAT UTILITIES
// ============================================================================

// Spawn a temporary melee attack hitbox
Entity combat_create_melee_attack(World *world, Entity attacker, Vec3 position,
                                  Vec3 direction, f32 damage, f32 range);

// Fire a projectile from a source
Entity combat_fire_projectile(World *world, Entity source, Vec3 position,
                              Vec3 direction, f32 speed, f32 damage);

// Create an ability attack effect
Entity combat_create_ability_attack(World *world, Entity caster, Vec3 position,
                                   Vec3 direction, uint32_t ability_id, f32 damage);

// Apply a status effect to a target
bool combat_apply_status_effect(World *world, Entity target, Entity source,
                               uint32_t status_id, f32 duration);

// Create an area of effect damage zone
void combat_create_area_effect(World *world, Vec3 center, f32 radius,
                              Entity source, f32 damage, uint32_t damage_type);

// ============================================================================
// COMBO SYSTEM
// ============================================================================

// Start a combo sequence for an attacker
bool combat_start_combo(World *world, Entity attacker, uint32_t combo_id);

// Advance a combo with an attack
bool combat_advance_combo(World *world, Entity attacker, uint32_t attack_id);

#endif // COMBAT_SYSTEM_H
