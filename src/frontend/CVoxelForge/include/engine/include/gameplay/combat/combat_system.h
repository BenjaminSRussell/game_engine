// include/gameplay/combat/combat_system.h
//
// Purpose: Main entry point for the combat system. Integrates hitboxes,
// damage processing, and projectile physics into a single system.
//
#ifndef COMBAT_SYSTEM_H
#define COMBAT_SYSTEM_H

#include <common.h>
#include <ecs/ecs.h>
#include <math/math.h>

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

// ============================================================================
// COMBAT UTILITIES
// ============================================================================

// Spawn a temporary melee attack hitbox
Entity combat_create_melee_attack(World *world, Entity attacker, Vec3 position,
                                  Vec3 direction, f32 damage, f32 range);

// Fire a projectile from a source
Entity combat_fire_projectile(World *world, Entity source, Vec3 position,
                              Vec3 direction, f32 speed, f32 damage);

#endif // COMBAT_SYSTEM_H
