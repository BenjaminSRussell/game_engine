// include/player/spell_effects.h
//
// Purpose: Defines the spell effect system for managing magical effects, buffs,
// debuffs, and environmental spell interactions. This header provides the framework
// for applying, managing, and removing spell effects on entities and the environment.
//
// Public APIs:
// - `SpellEffectType`: Enumeration defining various spell effect types (e.g.,
//   `EFFECT_DAMAGE`, `EFFECT_HEAL`, `EFFECT_SHIELD`, `EFFECT_FREEZE`).
// - `SpellEffect`: Structure to hold individual spell effect data including type,
//   duration, magnitude, source entity, and stacking behavior.
// - `SpellEffectManager`: Main system for managing all active spell effects with
//   update, cleanup, and query capabilities.
// - `spell_effect_manager_init()`: Initializes the spell effect system.
// - `spell_effect_apply()`: Applies a spell effect to a target entity.
// - `spell_effect_remove()`: Removes a specific spell effect from an entity.
// - `spell_effect_update()`: Updates all active spell effects and handles duration.
// - `spell_effect_has_active()`: Checks if an entity has a specific effect type.
// - `spell_effect_get_magnitude()`: Gets the combined magnitude of all effects of a type.
//
// Ownership: The `SpellEffectManager` manages the lifecycle of all spell effects.
// It interacts with the entity system and player components for effect application.
//
// Invariants:
// - Spell effects must be initialized with `spell_effect_manager_init()` before use.
// - `spell_effect_update()` should be called once per frame for proper duration management.
// - Effect stacking follows specific rules based on effect type and configuration.
// - All effects are automatically removed when their duration expires.

#ifndef SPELL_EFFECTS_H
#define SPELL_EFFECTS_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <stdbool.h>

// Forward declarations
typedef struct PlayerSystem PlayerSystem;
typedef struct PlayerComponent PlayerComponent;

// Spell effect types
typedef enum {
    EFFECT_DAMAGE = 0,
    EFFECT_HEAL,
    EFFECT_SHIELD,
    EFFECT_FREEZE,
    EFFECT_BURN,
    EFFECT_POISON,
    EFFECT_SLOWNESS,
    EFFECT_HASTE,
    EFFECT_INVISIBILITY,
    EFFECT_FLIGHT,
    EFFECT_REGENERATION,
    EFFECT_MANA_REGEN,
    EFFECT_STRENGTH,
    EFFECT_WEAKNESS,
    EFFECT_RESISTANCE,
    EFFECT_VULNERABILITY,
    EFFECT_BLINDNESS,
    EFFECT_SILENCE,
    EFFECT_STUN,
    EFFECT_LEVITATION,
    EFFECT_MAGNETISM,
    EFFECT_REFLECTION,
    EFFECT_LIFE_STEAL,
    EFFECT_MANA_DRAIN,
    EFFECT_COUNT
} SpellEffectType;

// Effect stacking behavior
typedef enum {
    STACKING_NONE = 0,        // No stacking - replace existing effect
    STACKING_REPLACE,          // Replace with stronger effect
    STACKING_ADDITIVE,         // Add magnitudes together
    STACKING_MULTIPLICATIVE,   // Multiply magnitudes
    STACKING_DURATION_REFRESH  // Refresh duration, keep stronger magnitude
} EffectStackingType;

// Spell effect structure
typedef struct SpellEffect {
    SpellEffectType type;
    EntityID target_entity;
    EntityID source_entity;
    f32 magnitude;
    f32 duration;
    f32 remaining_time;
    f32 tick_interval;         // For damage over time effects
    f32 next_tick_time;
    u32 stack_count;
    bool is_permanent;
    bool is_negative;
    Vec3 effect_position;      // For area effects
    f32 effect_radius;         // For area effects
    u32 effect_id;            // Unique identifier
    void* custom_data;         // For effect-specific data
} SpellEffect;

// Effect configuration
typedef struct {
    SpellEffectType type;
    const char* name;
    const char* description;
    EffectStackingType stacking_type;
    u32 max_stacks;
    bool is_negative;
    f32 default_duration;
    f32 default_tick_interval;
    u32 visual_effect_id;     // For particle effects
    u32 sound_effect_id;      // For audio feedback
} SpellEffectConfig;

// Spell effect manager
typedef struct {
    SpellEffect* effects;
    u32 max_effects;
    u32 active_count;
    u32 next_effect_id;
    bool visual_effects_enabled;
    bool sound_effects_enabled;
    f32 time_scale;
    u32 total_effects_applied;
    u32 total_effects_removed;
} SpellEffectManager;

// Effect application result
typedef struct {
    bool success;
    u32 effect_id;
    const char* error_message;
} SpellEffectResult;

// ============================================================================
// SYSTEM LIFECYCLE
// ============================================================================

// Initialize spell effect manager
bool spell_effect_manager_init(SpellEffectManager* manager, u32 max_effects);

// Shutdown spell effect manager and cleanup
void spell_effect_manager_shutdown(SpellEffectManager* manager);

// ============================================================================
// EFFECT APPLICATION
// ============================================================================

// Apply a spell effect to a target
SpellEffectResult spell_effect_apply(SpellEffectManager* manager, 
                                   SpellEffectType type,
                                   EntityID target, 
                                   EntityID source,
                                   f32 magnitude, 
                                   f32 duration);

// Apply area effect to all entities in radius
u32 spell_effect_apply_area(SpellEffectManager* manager,
                           SpellEffectType type,
                           Vec3 center,
                           f32 radius,
                           EntityID source,
                           f32 magnitude,
                           f32 duration);

// Remove a specific spell effect
bool spell_effect_remove(SpellEffectManager* manager, u32 effect_id);

// Remove all effects of a type from an entity
u32 spell_effect_remove_by_type(SpellEffectManager* manager, 
                               EntityID target, 
                               SpellEffectType type);

// Remove all effects from an entity
u32 spell_effect_remove_all(SpellEffectManager* manager, EntityID target);

// ============================================================================
// EFFECT QUERIES
// ============================================================================

// Check if entity has active effect of type
bool spell_effect_has_active(SpellEffectManager* manager, 
                           EntityID target, 
                           SpellEffectType type);

// Get combined magnitude of all effects of type on entity
f32 spell_effect_get_magnitude(SpellEffectManager* manager, 
                             EntityID target, 
                             SpellEffectType type);

// Get remaining time for effect type on entity
f32 spell_effect_get_remaining_time(SpellEffectManager* manager, 
                                   EntityID target, 
                                   SpellEffectType type);

// Get stack count for effect type on entity
u32 spell_effect_get_stack_count(SpellEffectManager* manager, 
                                EntityID target, 
                                SpellEffectType type);

// Get all active effects on an entity
u32 spell_effect_get_entity_effects(SpellEffectManager* manager,
                                   EntityID target,
                                   SpellEffect* out_effects,
                                   u32 max_effects);

// ============================================================================
// SYSTEM UPDATES
// ============================================================================

// Update all active spell effects
void spell_effect_update(SpellEffectManager* manager, f32 delta_time);

// Process damage over time effects
void spell_effect_process_dot(SpellEffectManager* manager, f32 delta_time);

// Update visual and audio effects
void spell_effect_update_feedback(SpellEffectManager* manager, f32 delta_time);

// ============================================================================
// CONFIGURATION
// ============================================================================

// Get effect configuration
const SpellEffectConfig* spell_effect_get_config(SpellEffectType type);

// Enable/disable visual effects
void spell_effect_set_visual_enabled(SpellEffectManager* manager, bool enabled);

// Enable/disable sound effects
void spell_effect_set_sound_enabled(SpellEffectManager* manager, bool enabled);

// Set global time scale for effect durations
void spell_effect_set_time_scale(SpellEffectManager* manager, f32 scale);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get effect type name
const char* spell_effect_get_name(SpellEffectType type);

// Check if effect type is negative (debuff)
bool spell_effect_is_negative(SpellEffectType type);

// Get stacking behavior for effect type
EffectStackingType spell_effect_get_stacking_type(SpellEffectType type);

// Calculate effect magnitude with stacking
f32 spell_effect_calculate_stacked_magnitude(SpellEffectType type, 
                                           f32 base_magnitude, 
                                           u32 stack_count);

// Validate effect parameters
bool spell_effect_validate_parameters(SpellEffectType type, 
                                   f32 magnitude, 
                                   f32 duration);

// ============================================================================
// STATISTICS
// ============================================================================

// Get effect system statistics
void spell_effect_get_stats(SpellEffectManager* manager,
                          u32* total_applied,
                          u32* total_removed,
                          u32* active_count);

// Reset effect system statistics
void spell_effect_reset_stats(SpellEffectManager* manager);

#endif // SPELL_EFFECTS_H
