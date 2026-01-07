// include/combat/combat_animations.h
//
// Purpose: Defines the API and data structures for managing combat-related animations
// within the game. This header provides enumerations for different animation types
// (idle, attack, hurt, death), a `CombatAnimationSystem` structure to track the
// state of animations, and functions to control animation playback and query their status.
//
// Public APIs:
// - `AnimationType`: Enumeration categorizing various combat animation states.
// - `CombatAnimationSystem`: Structure to manage global animation parameters,
//   such as attack cooldowns and the collection of active animations.
// - `combat_animations_init`: Initializes the combat animation system.
// - `combat_animations_start_attack`: Triggers an attack animation for a specified weapon type
//   and direction.
// - `combat_animations_update`: Updates the state of all active animations each frame.
// - `combat_animations_get_weapon_offset`: Provides the current offset for weapon rendering
//   based on the ongoing animation.
// - `combat_animations_is_damage_window`: Checks if the current attack animation is within
//   its active damage-dealing phase.
// - `combat_animations_can_attack`: Determines if an attack can be initiated, considering
//   cooldown periods.
// - `combat_animations_get_cooldown_remaining`: Returns the time remaining until another
//   attack can be performed.
//
// Ownership: The `CombatAnimationSystem` owns the collection of `AnimationState` objects.
// These functions manipulate the animation state, which typically affects rendering.
//
// Invariants:
// - `CombatAnimationSystem` must be initialized before use.
// - `delta_time` should be consistently passed for accurate animation progression.
// - `current_time` should reflect the absolute game time for cooldown calculations.
// - `AnimationState` is an internal detail, with management exposed through the `CombatAnimationSystem`.
//
#ifndef COMBAT_ANIMATIONS_H
#define COMBAT_ANIMATIONS_H


#include "../game_common.h"
#include <math/vec3.h>
#include "combat.h"

// Animation types
typedef enum {
    ANIMATION_IDLE,
    ANIMATION_ATTACK,
    ANIMATION_BLOCK,
    ANIMATION_HURT,
    ANIMATION_DEATH,
    ANIMATION_COUNT
} AnimationType;

// Animation state (forward declaration)
typedef struct AnimationState AnimationState;

// Combat animation system
typedef struct CombatAnimationSystem {
    AnimationState *animations;
    u32 animation_count;
    u32 animation_capacity;
    f32 attack_cooldown;
    f32 last_attack_time;
    f32 time_seconds;
} CombatAnimationSystem;

// Initialize combat animation system
void combat_animations_init(CombatAnimationSystem *system);

// Start attack animation
void combat_animations_start_attack(CombatAnimationSystem *system, WeaponType weapon_type, Vec3 attack_direction);

// Update animations (call every frame)
void combat_animations_update(CombatAnimationSystem *system, f32 delta_time);

// Get current weapon offset for rendering
Vec3 combat_animations_get_weapon_offset(CombatAnimationSystem *system);

// Check if attack animation is in damage window
bool combat_animations_is_damage_window(CombatAnimationSystem *system);

// Check if can attack (not on cooldown)
bool combat_animations_can_attack(CombatAnimationSystem *system, f32 current_time);

// Get attack cooldown remaining
f32 combat_animations_get_cooldown_remaining(CombatAnimationSystem *system, f32 current_time);

#endif // COMBAT_ANIMATIONS_H
