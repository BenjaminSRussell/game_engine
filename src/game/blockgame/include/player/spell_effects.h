#ifndef SPELL_EFFECTS_H
#define SPELL_EFFECTS_H

#include <math/vec3.h>
#include <player/player_magic.h>

// Forward declaration
typedef struct PlayerSystem PlayerSystem;

typedef enum {
    SPELL_EFFECT_CAST,
    SPELL_EFFECT_CHANNEL,
    SPELL_EFFECT_IMPACT,
    SPELL_EFFECT_PROJECTILE_TRAIL,
    SPELL_EFFECT_BUFF_START,
    SPELL_EFFECT_BUFF_END,
    SPELL_EFFECT_TELEPORT_START,
    SPELL_EFFECT_TELEPORT_END
} SpellEffectType;

// Initializes the spell effect system
void spell_effects_init(void);

// Plays a visual/audio effect for a specific spell
// system: The player system (provides access to audio)
// spell: The spell type
// type: The type of effect to play
// position: The world position where the effect should occur
// direction: The direction of the effect (e.g. cast direction), or zero vector if not applicable
void spell_effects_play(PlayerSystem *system, SpellType spell, SpellEffectType type, Vec3 position, Vec3 direction);

#endif // SPELL_EFFECTS_H
