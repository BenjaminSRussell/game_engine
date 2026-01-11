// include/crafting/brewing.h
//
// Purpose: Defines the potion brewing system for creating magical potions
// with various effects. Supports 15+ potion types with different durations,
// amplitudes, and applicable effects.
//
// Features:
// - 5 base potion types (Water, Awkward, Thick, Mundane, Extended)
// - 15+ effect potions (Speed, Strength, Healing, etc)
// - Potion combinations and upgrades
// - Ingredient system (crushed crystals, essence, etc)
// - Brewing stand mechanics
//
// Public APIs:
// - `PotionType`: Enumeration of potion types
// - `PotionEffect`: Individual potion with duration and level
// - `BrewingStand`: State for brewing stand block
// - `brewing_stand_init`: Initialize brewing stand
// - `brewing_get_recipe`: Get potion recipe by inputs
// - `brewing_process_potion`: Create potion from ingredients
// - `potion_get_effect_name`: Get effect display name
//

#ifndef BREWING_H
#define BREWING_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>

// Potion types
typedef enum {
  POTION_NONE = 0,
  POTION_WATER,    // Base potion (no effect)
  POTION_AWKWARD,  // Base, used for mixing
  POTION_THICK,    // Base, opaque
  POTION_MUNDANE,  // Base, no effect
  POTION_EXTENDED, // Extended duration potions

  // Effect potions
  POTION_SPEED,           // Speed boost
  POTION_SLOWNESS,        // Movement slowdown
  POTION_STRENGTH,        // Damage boost
  POTION_HEALING,         // Instant health
  POTION_HARMING,         // Instant damage
  POTION_REGENERATION,    // Health regeneration
  POTION_RESISTANCE,      // Damage resistance
  POTION_FIRE_RESISTANCE, // Fire immunity
  POTION_WATER_BREATHING, // Underwater breathing
  POTION_INVISIBILITY,    // Invisibility
  POTION_BLINDNESS,       // Vision obstruction
  POTION_NIGHT_VISION,    // See in darkness
  POTION_HUNGER,          // Hunger increase
  POTION_WEAKNESS,        // Reduced damage
  POTION_POISON,          // Poison damage
  POTION_WITHER,          // Wither effect
  POTION_LEVITATION,      // Floating effect
  POTION_COUNT
} PotionType;

// Potion effect with duration
typedef struct {
  PotionType type;
  u32 duration_ticks; // Effect duration in game ticks (20 ticks = 1s)
  u8 amplifier;       // Effect level (0-3 typically)
  bool is_splash;     // Splash potion or regular
  bool is_lingering;  // Lingering cloud or instant
} PotionEffect;

// Brewing ingredient
typedef enum {
  BREW_INGREDIENT_NONE = 0,
  BREW_INGREDIENT_NETHER_WART,      // Base ingredient
  BREW_INGREDIENT_REDSTONE,         // Extend duration
  BREW_INGREDIENT_GLOWSTONE,        // Upgrade effect level
  BREW_INGREDIENT_SPIDER_EYE,       // Vision/poison effects
  BREW_INGREDIENT_GHAST_TEAR,       // Healing
  BREW_INGREDIENT_GOLDEN_CARROT,    // Night vision
  BREW_INGREDIENT_MAGMA_CREAM,      // Fire resistance
  BREW_INGREDIENT_SUGAR,            // Speed
  BREW_INGREDIENT_FERMENTED_EYE,    // Negative effects
  BREW_INGREDIENT_BLAZE_POWDER,     // Strength
  BREW_INGREDIENT_TURTLE_SHELL,     // Resistance
  BREW_INGREDIENT_PHANTOM_MEMBRANE, // Levitation
  BREW_INGREDIENT_DRAGON_BREATH,    // Lingering potions
  BREW_INGREDIENT_COUNT
} BrewingIngredient;

// Brewing recipe
typedef struct {
  PotionType input_potion;      // Input potion type
  BrewingIngredient ingredient; // What to add
  PotionType output_potion;     // Result potion
  u32 brew_ticks;               // Time to brew (160 ticks = 8s default)
} BrewingRecipe;

// Brewing stand state
typedef struct BrewingStand {
  EntityID entity_id;
  Vec3 position;
  PotionEffect bottles[3];
  BrewingIngredient current_ingredient;
  u32 ingredient_count;
  u32 brew_ticks_remaining;
  u32 brew_ticks_total;
  f32 brew_progress;
  bool is_brewing;
  bool is_powered;
  u8 fuel_charges;
  u8 fuel_max_charges;
} BrewingStand;

// ============= Brewing Functions =============

// Initialize brewing stand
void brewing_stand_init(BrewingStand *stand, Vec3 position, EntityID entity_id);

// Free brewing stand
void brewing_stand_free(BrewingStand *stand);

// Start brewing with ingredient
bool brewing_start_brew(BrewingStand *stand, u32 bottle_index,
                        BrewingIngredient ingredient, u32 amount);

// Update brewing process
// Forward declarations for systems used by optional VFX/audio hooks
#include <effects/vfx/particle_system.h>
struct AudioSystem; /* forward */
void brewing_stand_update(BrewingStand *stand, f32 delta_time,
                          struct ParticleSystem *particle_system,
                          struct AudioSystem *audio_system);

// Add fuel to brewing stand
bool brewing_stand_add_fuel(BrewingStand *stand, u32 item_id);
bool brewing_is_fuel(u32 item_id);

// Visual & audio helpers (optional): emit particles and sounds for brewing
// events
void brewing_emit_complete_vfx(const BrewingStand *stand,
                               struct ParticleSystem *particle_system,
                               struct AudioSystem *audio_system);
void brewing_emit_progress_vfx(const BrewingStand *stand,
                               struct ParticleSystem *particle_system);

// Get potion at bottle index
const PotionEffect *brewing_get_bottle(const BrewingStand *stand,
                                       u32 bottle_index);

// Set bottle contents
void brewing_set_bottle(BrewingStand *stand, u32 bottle_index,
                        PotionEffect potion);

// Check if recipe exists
bool brewing_has_recipe(PotionType input, BrewingIngredient ingredient);

// Get recipe result
PotionType brewing_get_result(PotionType input, BrewingIngredient ingredient);

// ============= Potion Effects =============

// Apply potion effect to player
void potion_apply_effect(EntityID player_entity, PotionEffect effect);

// Remove potion effect
void potion_remove_effect(EntityID player_entity, PotionType type);

// Update active potion effects
// TODO: Iterate active effects, apply modifiers (health, speed) to Entity
// components.
void potion_update_effects(EntityID player_entity, f32 delta_time);

// Get effect name
const char *potion_get_effect_name(PotionType type);

// Get effect description
const char *potion_get_description(PotionType type);

// ============= Recipe Functions =============

// Get potion recipe count
u32 brewing_get_recipe_count(void);

// Get recipe by index
const BrewingRecipe *brewing_get_recipe(u32 index);

// Initialize default recipes
void brewing_init_default_recipes(void);

#endif // BREWING_H
