// include/combat/shield.h
//
// Purpose: Defines the public API and data structures for shield blocking and
// parry mechanics in the combat system. This header provides structures for
// shield properties, blocking states, and functions for managing defensive
// combat actions including blocking, parrying, and counter-attacks.
//
// Public APIs:
// - `ShieldType`: Enumeration for different types of shields with varying properties.
// - `Shield`: Structure defining shield properties including durability, block strength,
//   parry window, and weight.
// - `BlockingState`: Enumeration for different blocking states (idle, raising, blocking, parrying).
// - `ShieldComponent`: ECS component for entities that can use shields.
// - `shield_component_init`: Initializes a shield component with a specific shield type.
// - `shield_component_free`: Frees shield component resources.
// - `shield_start_block`: Initiates blocking animation and state.
// - `shield_stop_block`: Ends blocking state.
// - `shield_attempt_parry`: Attempts to parry an incoming attack within the timing window.
// - `shield_calculate_block_reduction`: Calculates damage reduction based on shield properties.
// - `shield_apply_damage`: Applies damage to shield durability and potentially breaks the shield.
// - `shield_can_parry`: Checks if a parry is possible based on timing and state.
// - `shield_get_parry_bonus`: Calculates damage bonus for successful parry counter-attacks.
//
// Ownership: `ShieldComponent` is owned by entities as an ECS component.
// `Shield` instances are typically owned by the `ShieldComponent`.
//
// Invariants:
// - Shield components must be initialized before use.
// - Shield durability must be between 0 and max_durability.
// - Parry timing windows are measured in seconds and must be positive.
// - Block strength is a percentage (0.0 to 1.0) representing damage reduction.

#ifndef SHIELD_H
#define SHIELD_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct ECSWorld;

// Shield types
typedef enum {
  SHIELD_TYPE_WOODEN,
  SHIELD_TYPE_STONE,
  SHIELD_TYPE_IRON,
  SHIELD_TYPE_DIAMOND,
  SHIELD_TYPE_NETHERITE,
  SHIELD_TYPE_COUNT
} ShieldType;

// Blocking states
typedef enum {
  BLOCKING_STATE_IDLE,     // Not blocking
  BLOCKING_STATE_RAISING,  // Raising shield (animation)
  BLOCKING_STATE_BLOCKING,  // Actively blocking
  BLOCKING_STATE_PARRYING, // Attempting parry (timing window)
  BLOCKING_STATE_STUNNED,  // Stunned after failed parry
  BLOCKING_STATE_COUNT
} BlockingState;

// Shield structure
typedef struct {
  ShieldType type;
  f32 durability;
  f32 max_durability;
  f32 block_strength;     // Damage reduction percentage (0.0-1.0)
  f32 parry_window;       // Time window for successful parry in seconds
  f32 raise_time;          // Time to raise shield in seconds
  f32 weight;              // Shield weight affects movement speed
  u32 enchantment_level;   // Enchantment level for special effects
} Shield;

// Shield component for ECS
typedef struct {
  Shield shield;
  BlockingState state;
  f32 state_timer;         // Timer for current state
  f32 block_angle;         // Current block direction (radians)
  Vec3 block_direction;    // Normalized block direction vector
  f32 stamina_cost;        // Stamina cost per second while blocking
  f32 parry_stamina_cost;  // Stamina cost for parry attempt
  bool is_blocking;        // Quick check for blocking state
  bool can_parry;          // Can attempt parry in current timing
  f32 last_parry_time;     // Time of last parry attempt
  f32 parry_cooldown;      // Cooldown between parry attempts
} ShieldComponent;

// Shield lifecycle functions
void shield_component_init(ShieldComponent* component, ShieldType type);
void shield_component_free(ShieldComponent* component);

// Shield blocking functions
void shield_start_block(ShieldComponent* component, Vec3 direction);
void shield_stop_block(ShieldComponent* component);
void shield_update(ShieldComponent* component, f32 delta_time);

// Shield parry functions
bool shield_attempt_parry(ShieldComponent* component, Vec3 attack_direction);
bool shield_can_parry(ShieldComponent* component, f32 current_time);
f32 shield_get_parry_bonus(ShieldComponent* component);

// Shield damage calculations
f32 shield_calculate_block_reduction(ShieldComponent* component, Vec3 attack_direction);
bool shield_apply_damage(ShieldComponent* component, f32 damage);
bool shield_is_broken(ShieldComponent* component);

// Shield utility functions
const char* shield_get_type_name(ShieldType type);
f32 shield_get_movement_penalty(ShieldComponent* component);
f32 shield_get_stamina_drain(ShieldComponent* component, f32 delta_time);

// Shield creation helpers
Shield shield_create(ShieldType type);
ShieldComponent shield_component_create(ShieldType type);

#ifdef __cplusplus
}
#endif

#endif // SHIELD_H
