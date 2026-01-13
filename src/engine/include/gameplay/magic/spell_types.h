#ifndef SPELL_TYPES_H
#define SPELL_TYPES_H

#include "include/common.h"

// Maximum number of elements that can be combined
#define MAX_SPELL_ELEMENTS 5

// Enumeration of basic magic elements
typedef enum {
    ELEMENT_NONE = 0,
    ELEMENT_FIRE,
    ELEMENT_WATER,
    ELEMENT_EARTH,
    ELEMENT_AIR,
    ELEMENT_LIGHTNING,
    ELEMENT_ICE,
    ELEMENT_ARCANE,
    ELEMENT_LIFE,
    ELEMENT_DEATH,
    ELEMENT_COUNT
} MagicElement;

// Types of spells that can be cast
typedef enum {
    SPELL_TYPE_PROJECTILE,
    SPELL_TYPE_BEAM,
    SPELL_TYPE_AREA_OF_EFFECT,
    SPELL_TYPE_SELF_BUFF,
    SPELL_TYPE_SUMMON,
    SPELL_TYPE_COUNT
} SpellType;

// Definition of a spell resulting from a combination
typedef struct {
    u32 id;                     // Unique identifier
    char name[64];              // Display name
    char description[256];      // Description

    SpellType type;             // Type of spell
    MagicElement primary_element; // Dominant element

    f32 mana_cost;              // Cost to cast
    f32 cooldown;               // Cooldown in seconds
    f32 cast_time;              // Time to cast

    f32 base_damage;            // Base damage value
    f32 range;                  // Range in meters
    f32 radius;                 // AOE radius (if applicable)
    f32 duration;               // Duration (if applicable)

    // Integration with Gameplay Ability System
    char ability_name[64];      // Name of the GAS ability to trigger
} SpellDefinition;

// A combination of elements
typedef struct {
    MagicElement elements[MAX_SPELL_ELEMENTS];
    u32 element_count;
} SpellCombination;

#endif // SPELL_TYPES_H
