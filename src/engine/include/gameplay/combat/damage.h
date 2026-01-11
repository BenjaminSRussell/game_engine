// include/gameplay/combat/damage.h
//
// Purpose: Defines damage event system for combat. Damage events are created
// when attacks connect and are processed by the damage system to apply damage
// to health components. Supports different damage types, resistances, and
// modifiers.
//
// Public APIs:
// - DamageType: Enum for different types of damage (physical, elemental, etc.)
// - DamageEvent: Structure representing a damage instance
// - DamageComponent: Component for entities that can deal damage
// - Damage event queue management
//
// Ownership: DamageEvents are owned by the global damage event queue
//
// Invariants:
// - amount must be non-negative
// - source and target must be valid entities
// - Events are processed once per frame and then cleared
//
#ifndef DAMAGE_COMPONENT_H
#define DAMAGE_COMPONENT_H

#include "include/common.h"
#include "include/ecs/ecs.h"
#include "include/math/vec3.h"

// Damage types for resistances and weaknesses
typedef enum {
  DAMAGE_TYPE_PHYSICAL,
  DAMAGE_TYPE_MELEE,
  DAMAGE_TYPE_PROJECTILE,
  DAMAGE_TYPE_FIRE,
  DAMAGE_TYPE_ICE,
  DAMAGE_TYPE_POISON,
  DAMAGE_TYPE_LIGHTNING,
  DAMAGE_TYPE_MAGIC,
  DAMAGE_TYPE_HOLY,
  DAMAGE_TYPE_DARK,
  DAMAGE_TYPE_TRUE, // Ignores resistances
  DAMAGE_TYPE_COUNT
} DamageType;

// Damage flags for special behaviors
typedef enum {
  DAMAGE_FLAG_NONE = 0,
  DAMAGE_FLAG_CRITICAL = 1 << 0,      // Critical hit (2x damage)
  DAMAGE_FLAG_BACKSTAB = 1 << 1,      // Backstab bonus
  DAMAGE_FLAG_HEADSHOT = 1 << 2,      // Headshot bonus
  DAMAGE_FLAG_IGNORE_ARMOR = 1 << 3,  // Bypass armor
  DAMAGE_FLAG_IGNORE_SHIELD = 1 << 4, // Bypass shields
  DAMAGE_FLAG_KNOCKBACK = 1 << 5,     // Apply knockback
  DAMAGE_FLAG_STUN = 1 << 6,          // Stun target
  DAMAGE_FLAG_DOT = 1 << 7,           // Damage over time
} DamageFlag;

// Damage event - created when damage is dealt
typedef struct {
  Entity source;       // Entity that dealt the damage
  Entity target;       // Entity receiving the damage
  f32 base_amount;     // Base damage amount
  f32 final_amount;    // Final damage after modifiers
  DamageType type;     // Type of damage
  u32 flags;           // DamageFlag bitfield
  Vec3 contact_point;  // World space contact point
  Vec3 direction;      // Direction of damage (for knockback)
  f32 knockback_force; // Knockback force magnitude
  f64 timestamp;       // When damage was dealt
  void *user_data;     // Custom data for specific damage types
} DamageEvent;

// Damage component - attached to entities that can deal damage
typedef struct {
  f32 base_damage;         // Base damage amount
  DamageType damage_type;  // Type of damage dealt
  u32 damage_flags;        // DamageFlag bitfield
  Entity source_entity;    // Entity that deals the damage
  f32 critical_chance;     // Chance for critical hit (0-1)
  f32 critical_multiplier; // Critical hit damage multiplier
  f32 armor_penetration;   // Armor penetration percentage (0-1)
  f32 knockback_force;     // Force applied on hit

  // Damage over time
  bool is_dot;      // Is this damage over time?
  f32 dot_duration; // Duration in seconds
  f32 dot_interval; // Tick interval in seconds
} DamageComponent;

// Damage resistance component
typedef struct {
  f32 resistances[DAMAGE_TYPE_COUNT];   // Resistance to each damage type (0-1)
  f32 armor;                            // Physical armor value
  f32 magic_resistance;                 // General magic resistance
  bool immune_types[DAMAGE_TYPE_COUNT]; // Immunity to specific types
} ResistanceComponent;

// ============================================================================
// DAMAGE EVENT SYSTEM
// ============================================================================

// Initialize damage event system
void damage_system_init(u32 max_events_per_frame);
void damage_system_shutdown(void);

// Create and emit damage events
DamageEvent *damage_event_create(Entity source, Entity target, f32 amount,
                                 DamageType type);
void damage_event_emit(const DamageEvent *event);
void damage_event_emit_simple(Entity source, Entity target, f32 amount);

// Process all pending damage events (called once per frame)
void damage_system_process_events(World *world, f64 delta_time);

// Clear processed events
void damage_system_clear_events(void);

// ============================================================================
// DAMAGE CALCULATION
// ============================================================================

// Calculate final damage with modifiers
f32 damage_calculate_final(const DamageEvent *event,
                           const ResistanceComponent *resistance,
                           f32 *out_blocked_amount);

// Apply damage modifiers
f32 damage_apply_critical(f32 base_damage, f32 multiplier);
f32 damage_apply_resistance(f32 damage, f32 resistance);
f32 damage_apply_armor(f32 damage, f32 armor);

// Damage type helpers
const char *damage_type_to_string(DamageType type);
bool damage_can_crit(DamageType type);

// ============================================================================
// DAMAGE COMPONENT HELPERS
// ============================================================================

// Create damage components
DamageComponent damage_component_create(f32 base_damage, DamageType type);
DamageComponent damage_component_create_melee(f32 damage);
DamageComponent damage_component_create_ranged(f32 damage);
DamageComponent damage_component_create_magic(f32 damage, DamageType element);

// Resistance component
ResistanceComponent resistance_component_create(void);
void resistance_component_set(ResistanceComponent *res, DamageType type,
                              f32 value);
void resistance_component_set_immune(ResistanceComponent *res, DamageType type);

#endif // DAMAGE_COMPONENT_H
