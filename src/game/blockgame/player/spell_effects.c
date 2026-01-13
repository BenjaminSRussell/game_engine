#include <player/spell_effects.h>
#include <player/player.h>
#include <audio/audio_system.h>
#include <effects/vfx/particle_system.h>
#include <core/logger.h>
#include <string.h>

extern ParticleSystem *g_particle_system;

typedef struct {
    SoundType sound;
    f32 volume;
    ParticleType particle;
    u32 particle_count;
    f32 particle_speed;
    f32 particle_lifetime;
    Vec3 particle_direction; // For directional bursts
    bool use_direction_vector; // If true, use the passed direction vector
} EffectConfig;

static EffectConfig cast_configs[SPELL_COUNT];
static bool initialized = false;

void spell_effects_init(void) {
    if (initialized) return;

    memset(cast_configs, 0, sizeof(cast_configs));

    // Initialize CAST effects

    // Fireball
    cast_configs[SPELL_FIREBALL] = (EffectConfig){
        .sound = SOUND_FIRE_BURN,
        .volume = 0.8f,
        .particle = PARTICLE_TYPE_FLAME,
        .particle_count = 20,
        .particle_speed = 2.0f,
        .particle_lifetime = 1.0f,
        .use_direction_vector = true
    };

    // Lightning
    cast_configs[SPELL_LIGHTNING] = (EffectConfig){
        .sound = SOUND_THUNDER_01,
        .volume = 0.9f,
        .particle = PARTICLE_TYPE_LIGHTNING,
        .particle_count = 10,
        .particle_speed = 1.0f,
        .particle_lifetime = 0.5f,
        .use_direction_vector = false // Lightning strikes target usually, but cast effect might be different
    };
    // Note: Lightning cast effect in original code was at target?
    // "particle_emit_burst(g_particle_system, PARTICLE_TYPE_LIGHTNING, target, vec3_zero(), 1.0f, 10, 0.5f);"
    // So for lightning, the "cast" effect happens at target in the original code.
    // We will handle this logic in spell_effects_play based on how we call it.

    // Heal
    cast_configs[SPELL_HEAL] = (EffectConfig){
        .sound = SOUND_PLAYER_HEAL,
        .volume = 0.8f,
        .particle = PARTICLE_TYPE_HEART,
        .particle_count = 10,
        .particle_speed = 1.0f,
        .particle_lifetime = 1.0f,
        .particle_direction = {0.0f, 1.0f, 0.0f},
        .use_direction_vector = false
    };

    // Teleport
    cast_configs[SPELL_TELEPORT] = (EffectConfig){
        .sound = SOUND_WIND_LIGHT,
        .volume = 0.6f,
        .particle = PARTICLE_TYPE_PORTAL,
        .particle_count = 50,
        .particle_speed = 0.5f,
        .particle_lifetime = 1.0f,
        .use_direction_vector = false
    };

    // Shield
    cast_configs[SPELL_SHIELD] = (EffectConfig){
        .sound = SOUND_CRAFTING_SUCCESS,
        .volume = 0.6f,
        .particle = PARTICLE_TYPE_ENCHANT,
        .particle_count = 30,
        .particle_speed = 1.0f,
        .particle_lifetime = 2.0f,
        .use_direction_vector = false
    };

    // Invisibility
    cast_configs[SPELL_INVISIBILITY] = (EffectConfig){
        .sound = SOUND_SWORD_SWING, // Default fallback
        .volume = 0.7f,
        .particle = PARTICLE_TYPE_SMOKE,
        .particle_count = 20,
        .particle_speed = 1.0f,
        .particle_lifetime = 1.5f,
        .use_direction_vector = false
    };

    // Flight
    cast_configs[SPELL_FLIGHT] = (EffectConfig){
        .sound = SOUND_SWORD_SWING, // Default fallback
        .volume = 0.7f,
        .particle = PARTICLE_TYPE_DUST,
        .particle_count = 20,
        .particle_speed = 2.0f,
        .particle_lifetime = 1.0f,
        .particle_direction = {0.0f, -1.0f, 0.0f},
        .use_direction_vector = false
    };

    // Freeze
    cast_configs[SPELL_FREEZE] = (EffectConfig){
        .sound = SOUND_SWORD_SWING,
        .volume = 0.7f,
        .particle = PARTICLE_TYPE_WATER_SPLASH, // Added for feedback
        .particle_count = 15,
        .particle_speed = 1.5f,
        .particle_lifetime = 0.8f,
        .use_direction_vector = true
    };

    initialized = true;
    LOG_INFO(LOG_CAT_GAME, "Spell effects system initialized");
}

void spell_effects_play(PlayerSystem *system, SpellType spell, SpellEffectType type, Vec3 position, Vec3 direction) {
    if (!initialized) {
        spell_effects_init();
    }

    if (spell >= SPELL_COUNT) return;

    // Handle audio
    if (system && system->audio_system) {
        EffectConfig *config = NULL;

        // Currently we only have explicit configs for CAST, but we could extend for others
        if (type == SPELL_EFFECT_CAST) {
            config = &cast_configs[spell];
        } else if (type == SPELL_EFFECT_TELEPORT_END) {
             // Teleport end sound same as start?
             // Use config from teleport but maybe change pitch?
             config = &cast_configs[SPELL_TELEPORT];
        }

        if (config && config->sound != SOUND_COUNT) { // Check if valid sound (assuming 0 is valid, we need a way to say "no sound")
            // Wait, enums usually start at 0. SOUND_FOOTSTEP_GRASS is 0.
            // We need a way to detect uninitialized sound.
            // In init we memset to 0. SOUND_FOOTSTEP_GRASS is 0.
            // But we set explicit sounds for all.
            // If we want "no sound", we might need a flag or specific check.
            // For now, assume initialized configs have valid sounds.

            // Note: If initialized with 0, and we didn't set it, it plays SOUND_FOOTSTEP_GRASS.
            // This is a risk. But we set all CAST configs above.

             audio_play_sound(system->audio_system, config->sound, position, config->volume, SOUND_CATEGORY_PLAYER);
        }
    }

    // Handle particles
    if (g_particle_system) {
        EffectConfig *config = NULL;
         if (type == SPELL_EFFECT_CAST || type == SPELL_EFFECT_TELEPORT_START || type == SPELL_EFFECT_TELEPORT_END) {
            config = &cast_configs[spell];

            // Special case for teleport end (reuse config)
            if (type == SPELL_EFFECT_TELEPORT_END && spell == SPELL_TELEPORT) {
                 // Same config
            }
        }

        if (config) {
            Vec3 particle_dir = config->use_direction_vector ? direction : config->particle_direction;

            // Special handling for some spells if needed
            if (spell == SPELL_LIGHTNING && type == SPELL_EFFECT_CAST) {
                 // Lightning uses target position passed as "position"
                 // And direction is usually zero or down?
                 // Original: particle_emit_burst(g_particle_system, PARTICLE_TYPE_LIGHTNING, target, vec3_zero(), 1.0f, 10, 0.5f);
                 // So we use zero velocity for lightning burst
                 particle_dir = vec3_zero();
            }

            particle_emit_burst(g_particle_system,
                                config->particle,
                                position,
                                particle_dir,
                                config->particle_speed,
                                config->particle_count,
                                config->particle_lifetime);
        }
    }
}
