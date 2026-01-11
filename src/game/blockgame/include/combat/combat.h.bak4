#ifndef COMBAT_H
#define COMBAT_H

#include <ecs/ecs.h>
#include <game_common.h>
#include <math/vec3.h>

// Forward declarations
struct World;

// Damage types
typedef enum {
  DAMAGE_TYPE_PHYSICAL,
  DAMAGE_TYPE_MELEE,
  DAMAGE_TYPE_RANGED,
  DAMAGE_TYPE_MAGIC,
  DAMAGE_TYPE_FIRE,
  DAMAGE_TYPE_EXPLOSION,
  DAMAGE_TYPE_FALL,
  DAMAGE_TYPE_DROWNING,
  DAMAGE_TYPE_LAVA,
  DAMAGE_TYPE_SUFFOCATION,
  DAMAGE_TYPE_POISON,
  DAMAGE_TYPE_WITHER,
  DAMAGE_TYPE_STARVATION,
  DAMAGE_TYPE_COUNT
} DamageType;

typedef enum {
  COMBAT_STANCE_BALANCED,
  COMBAT_STANCE_AGGRESSIVE,
  COMBAT_STANCE_DEFENSIVE,
  COMBAT_STANCE_COUNT
} CombatStance;

// Weapon types
typedef enum {
  WEAPON_TYPE_SWORD,
  WEAPON_TYPE_AXE,
  WEAPON_TYPE_PICKAXE,
  WEAPON_TYPE_BOW,
  WEAPON_TYPE_CROSSBOW,
  WEAPON_TYPE_COUNT
} WeaponType;

// Weapon structure
typedef struct Weapon {
  WeaponType type;
  f32 damage;
  f32 attack_speed;
  f32 range;
  u32 durability;
  u32 max_durability;
} Weapon;
typedef struct Weapon Weapon;

// Armor slot
enum ArmorSlot {
  ARMOR_SLOT_HELMET,
  ARMOR_SLOT_CHESTPLATE,
  ARMOR_SLOT_LEGGINGS,
  ARMOR_SLOT_BOOTS,
  ARMOR_SLOT_COUNT
};
typedef enum ArmorSlot ArmorSlot;

// Armor structure
struct Armor {
  ArmorSlot slot;
  f32 defense;
  f32 toughness;            // Armor toughness (reduces damage)
  f32 knockback_resistance; // Knockback resistance (0.0 to 1.0)
  u32 durability;
  u32 max_durability;
};
typedef struct Armor Armor;

// Combat statistics
typedef struct {
  u32 total_hits;
  u32 total_misses;
  u32 total_crits;
  f32 total_damage_dealt;
  f32 total_damage_taken;
  u32 combat_sessions;
} CombatStats;

typedef struct {
  EntityID attacker;
  EntityID target;
  DamageType type;
  f32 damage;
  bool critical;
  f32 timestamp;
} CombatLogEntry;

typedef struct {
  EntityID target;
  DamageType type;
  f32 damage_per_second;
  f32 remaining_time;
} CombatDot;

// Combat system
typedef struct {
  f32 damage_multiplier;
  f32 critical_chance;
  f32 critical_multiplier;
  f32 crit_damage_multiplier;
  f32 combo_timeout;
  f32 attack_cooldown;

  // Combo system state
  f32 combo_timer;
  u32 combo_count;
  f32 last_attack_time;
  // Hit confirm pulse timer for HUD crosshair feedback
  f32 hit_flash_time;
  f32 hit_flash_duration;
  f32 combo_multiplier;
  f32 combo_visual_timer;

  // Combat statistics tracking
  CombatStats stats;
  CombatStance stance;
  f32 stance_damage_multiplier;
  f32 stance_defense_multiplier;
  f32 damage_resistance[DAMAGE_TYPE_COUNT];
  f32 damage_vulnerability[DAMAGE_TYPE_COUNT];
  CombatDot dots[32];
  u32 dot_count;
  CombatLogEntry log[64];
  u32 log_index;
  u32 log_count;
  bool logging_enabled;
  struct World *ecs_world;

  // Visual and audio feedback systems
  struct VFXSystem *vfx_system;
  struct AudioSystem *audio_system;

  // Finisher system
  f32 finisher_threshold; // Health percentage below which finisher is available
  bool finisher_enabled;
} CombatSystem;

// Initialize combat system
void combat_system_init(CombatSystem *system);

// Update combat system
void combat_system_update(CombatSystem *system, f32 delta_time);

// Calculate damage
f32 combat_calculate_damage(CombatSystem *system, f32 base_damage,
                            bool is_critical);

// Apply damage
bool combat_apply_damage(CombatSystem *system, struct World *ecs,
                         EntityID target, f32 damage, DamageType type);

// Check hit
bool combat_check_hit(CombatSystem *system, Vec3 attacker_pos, Vec3 target_pos,
                      f32 range, f32 angle);

// Perform attacks
bool combat_melee_attack(CombatSystem *system, struct World *ecs,
                         EntityID attacker, EntityID target);
bool combat_ranged_attack(CombatSystem *system, struct World *ecs,
                          EntityID attacker, Vec3 direction, f32 range);

// Combat extensions
void combat_set_stance(CombatSystem *system, CombatStance stance);
CombatStance combat_get_stance(const CombatSystem *system);
void combat_set_damage_modifier(CombatSystem *system, DamageType type,
                                f32 resistance, f32 vulnerability);
void combat_add_dot(CombatSystem *system, EntityID target, DamageType type,
                    f32 damage_per_second, f32 duration);
u32 combat_get_log(const CombatSystem *system, CombatLogEntry *entries,
                   u32 max_entries);
void combat_set_logging(CombatSystem *system, bool enabled);
void combat_system_bind_world(CombatSystem *system, struct World *ecs);

// Visual and audio feedback
void combat_system_bind_vfx(CombatSystem *system, struct VFXSystem *vfx);
void combat_system_bind_audio(CombatSystem *system, struct AudioSystem *audio);
void combat_trigger_critical_feedback(CombatSystem *system, Vec3 position);
void combat_trigger_finisher_feedback(CombatSystem *system, Vec3 position,
                                      EntityID target);

// Finisher system
void combat_set_finisher_enabled(CombatSystem *system, bool enabled);
void combat_set_finisher_threshold(CombatSystem *system, f32 threshold);
bool combat_can_execute_finisher(CombatSystem *system, EntityID target,
                                 struct World *ecs);

#endif // COMBAT_H
