// Gameplay/Gameplay Ability System (GAS)
#ifndef GAMEPLAY_ABILITIES_H
#define GAMEPLAY_ABILITIES_H

#include "include/common.h"
#include "math/vec3.h"

#define MAX_ATTRIBUTES 32
#define MAX_TAGS 64
#define MAX_ACTIVE_EFFECTS 128

// Gameplay Tags (Hierarchical strings, e.g., "Status.Stunned")
typedef struct {
  u32 id; // Hashed string
  char debug_string[64];
} GameplayTag;

// Attributes (Health, Mana, Strength, etc.)
typedef struct {
  char name[32];
  f32 base_value;
  f32 current_value; // Base + modifiers
  f32 min_value;
  f32 max_value;
} GameplayAttribute;

typedef struct {
  GameplayAttribute attributes[MAX_ATTRIBUTES];
  u32 attribute_count;
} AttributeSet;

// Gameplay Effect (Buffs, Debuffs, Damage)
typedef enum {
  EFFECT_INSTANT,
  EFFECT_INFINITE,
  EFFECT_DURATION
} EffectDurationType;

typedef struct {
  char name[64];
  EffectDurationType duration_type;
  f32 duration;
  f32 period; // Tick rate

  // What it does
  char target_attribute[32];
  f32 magnitude;
  // ... modifiers, curve tables, etc.

  // Tags
  GameplayTag granted_tags[8];
  GameplayTag blocked_by_tags[8];

} GameplayEffect;

typedef struct {
  GameplayEffect *effect; // Reference
  f32 start_time;
  f32 time_remaining;
  u32 stack_count;
  void *source_actor; // Who applied it
} ActiveGameplayEffect;

// Gameplay Ability (Skill, Spell, Action)
typedef struct GameplayAbility GameplayAbility;

typedef struct {
  GameplayAbility *ability;
  bool is_active;
  f32 cooldown_remaining;
  // ... activation data
} AbilitySpec;

struct GameplayAbility {
  char name[64];

  // Tags that block this ability
  GameplayTag activation_blocked_tags[4];

  // Tags required to activate
  GameplayTag activation_required_tags[4];

  // Cost (Mana, Stamina)
  GameplayEffect *cost_effect;
  GameplayEffect *cooldown_effect;

  // Logic (Function pointers / Script hooks)
  bool (*can_activate)(void *owner, const AbilitySpec *spec);
  void (*on_activate)(void *owner, AbilitySpec *spec);
  void (*on_end)(void *owner, AbilitySpec *spec);
};

// Component that holds everything
typedef struct {
  AttributeSet *attribute_set;

  AbilitySpec abilities[32];
  u32 ability_count;

  ActiveGameplayEffect active_effects[MAX_ACTIVE_EFFECTS];
  u32 active_effect_count;

  GameplayTag active_tags[MAX_TAGS];
  u32 active_tag_count;

  void *owner_actor;

} AbilitySystemComponent;

#ifdef __cplusplus
extern "C" {
#endif

// System Management
AbilitySystemComponent *gas_create_component(void *owner);
void gas_destroy_component(AbilitySystemComponent *comp);
void gas_update(AbilitySystemComponent *comp, f32 delta_time);

// Attributes
void gas_add_attribute(AbilitySystemComponent *comp, const char *name,
                       f32 value, f32 max);
f32 gas_get_attribute_value(AbilitySystemComponent *comp, const char *name);
void gas_set_attribute_value(AbilitySystemComponent *comp, const char *name,
                             f32 value);

// Abilities
void gas_give_ability(AbilitySystemComponent *comp, GameplayAbility *ability);
bool gas_try_activate_ability(AbilitySystemComponent *comp,
                              const char *ability_name);
void gas_cancel_ability(AbilitySystemComponent *comp, const char *ability_name);

// Effects
void gas_apply_effect_to_self(AbilitySystemComponent *comp,
                              GameplayEffect *effect, void *source);
void gas_remove_effect(AbilitySystemComponent *comp, const char *effect_name);

// Tags
bool gas_has_tag(AbilitySystemComponent *comp, const char *tag_string);

#ifdef __cplusplus
}
#endif

#endif // GAMEPLAY_ABILITIES_H
