// src/vfx/visual_effects.c
//
// Implementation of high-level visual effects system for coordinated
// particle, audio, and screen effects.
//
#include <effects/vfx/visual_effects.h>
#include <core/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void vfx_init(VFXSystem* system, ParticleSystem* particles, AudioSystem* audio) {
    if (!system || !particles || !audio) {
        return;
    }

    memset(system, 0, sizeof(VFXSystem));
    system->particle_system = particles;
    system->audio_system = audio;
    system->active_effect_count = 0;

    // Initialize screen effects
    system->screen_effects.flash_time = 0.0f;
    system->screen_effects.flash_duration = 0.0f;
    system->screen_effects.flash_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    system->screen_effects.shake_time = 0.0f;
    system->screen_effects.shake_duration = 0.0f;
    system->screen_effects.shake_intensity = 0.0f;
    system->screen_effects.vignette_strength = 0.0f;
    system->screen_effects.target_vignette = 0.0f;

    // Initialize all effects as inactive
    for (u32 i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        system->effects[i].active = false;
    }

    system->initialized = true;
}

void vfx_shutdown(VFXSystem* system) {
    if (!system || !system->initialized) {
        return;
    }

    // Clear all active effects
    for (u32 i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        if (system->effects[i].active) {
            // Stop associated audio
            if (system->effects[i].audio_synced && system->audio_system) {
                audio_stop_sound(system->audio_system, system->effects[i].sound_id);
            }
            system->effects[i].active = false;
        }
    }

    system->initialized = false;
}

// Helper: Create a visual effect and return its ID
static u32 vfx_create_effect(VFXSystem* system, VFXEffectType type, Vec3 position) {
    if (!system || !system->initialized) {
        return 0xFFFFFFFF;
    }

    // Find free effect slot
    u32 effect_id = 0xFFFFFFFF;
    for (u32 i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        if (!system->effects[i].active) {
            effect_id = i;
            break;
        }
    }

    if (effect_id == 0xFFFFFFFF) {
        return 0xFFFFFFFF;
    }

    VisualEffect* effect = &system->effects[effect_id];
    memset(effect, 0, sizeof(VisualEffect));

    effect->type = type;
    effect->position = position;
    effect->active = true;
    effect->time_alive = 0.0f;
    effect->intensity = 1.0f;
    effect->emitter_count = 0;

    system->active_effect_count++;
    return effect_id;
}

void vfx_play_block_break(VFXSystem* system, Vec3 position, u16 block_type) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_BLOCK_BREAK, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 1.0f;

    // Create particle burst for block breaking dust
    ParticleSystem* particles = system->particle_system;

    Vec3 velocity_spread = {2.0f, 3.0f, 2.0f};
    particle_emit_burst(particles, PARTICLE_TYPE_DUST, position,
                       (Vec3){0.0f, 0.0f, 0.0f}, 2.0f, 12, 1.5f);

    // Play block break sound
    if (system->audio_system) {
        u32 sound_id = audio_play_sound(system->audio_system, SOUND_BLOCK_BREAK,
                                       position, 0.7f, SOUND_CATEGORY_BLOCK);
        if (sound_id != 0xFFFFFFFF) {
            effect->sound_id = sound_id;
            effect->audio_synced = true;
        }
    }

            position.x, position.y, position.z);
}

void vfx_play_block_place(VFXSystem* system, Vec3 position, u16 block_type) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_BLOCK_PLACE, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 0.5f;

    // Smaller dust burst for placement
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_DUST, position,
                       (Vec3){0.0f, 0.5f, 0.0f}, 1.0f, 6, 0.8f);

    // Play block place sound
    if (system->audio_system) {
        u32 sound_id = audio_play_sound(system->audio_system, SOUND_BLOCK_PLACE,
                                       position, 0.6f, SOUND_CATEGORY_BLOCK);
        if (sound_id != 0xFFFFFFFF) {
            effect->sound_id = sound_id;
            effect->audio_synced = true;
        }
    }
}

void vfx_play_water_splash(VFXSystem* system, Vec3 position, f32 intensity) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_WATER_SPLASH, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->intensity = intensity;
    effect->duration = 1.0f;

    // Water splash particles
    u32 particle_count = (u32)(8.0f * intensity);
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_WATER_SPLASH,
                       position, (Vec3){0.0f, 3.0f, 0.0f}, 3.0f,
                       particle_count, 1.2f);

    // Play water splash sound
    if (system->audio_system) {
        u32 sound_id = audio_play_sound(system->audio_system, SOUND_WATER_SPLASH,
                                       position, 0.5f + (intensity * 0.3f),
                                       SOUND_CATEGORY_BLOCK);
        if (sound_id != 0xFFFFFFFF) {
            effect->sound_id = sound_id;
            effect->audio_synced = true;
        }
    }
}

void vfx_play_lava_splash(VFXSystem* system, Vec3 position, f32 intensity) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_LAVA_SPLASH, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->intensity = intensity;
    effect->duration = 2.0f;

    // Flame particles for lava splash
    u32 particle_count = (u32)(6.0f * intensity);
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_FLAME, position,
                       (Vec3){0.0f, 2.0f, 0.0f}, 2.0f, particle_count, 2.0f);
}

void vfx_play_sword_swing(VFXSystem* system, Vec3 start_pos, Vec3 end_pos) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_SWORD_SWING, start_pos);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->direction = end_pos;
    effect->duration = 0.3f;

    // Small spark trail along swing arc
    Vec3 mid_point = {
        (start_pos.x + end_pos.x) * 0.5f,
        (start_pos.y + end_pos.y) * 0.5f,
        (start_pos.z + end_pos.z) * 0.5f
    };

    particle_emit_burst(system->particle_system, PARTICLE_TYPE_SPARK, mid_point,
                       (Vec3){0.0f, 0.0f, 0.0f}, 1.0f, 4, 0.5f);
}

void vfx_play_sword_hit(VFXSystem* system, Vec3 position, Vec3 normal) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_SWORD_HIT, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->direction = normal;
    effect->duration = 0.5f;

    // Spark burst from impact
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_SPARK, position,
                       normal, 2.0f, 8, 0.8f);

    // Play impact sound
    if (system->audio_system) {
        u32 sound_id = audio_play_sound(system->audio_system, SOUND_SWORD_HIT,
                                       position, 0.8f, SOUND_CATEGORY_PLAYER);
        if (sound_id != 0xFFFFFFFF) {
            effect->sound_id = sound_id;
            effect->audio_synced = true;
        }
    }

    // Screen shake on hit
    vfx_trigger_screen_shake(system, 0.3f, 0.1f);
}

void vfx_play_critical_hit(VFXSystem* system, Vec3 position) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_CRITICAL_HIT, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 0.7f;

    // Bright yellow spark explosion
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_CRITICAL_HIT,
                       position, (Vec3){0.0f, 0.0f, 0.0f}, 3.0f, 16, 1.0f);

    // Damage flash on screen
    vfx_trigger_damage_flash(system, 1.0f, (Vec4){1.0f, 1.0f, 0.0f, 0.5f});

    // Screen shake
    vfx_trigger_screen_shake(system, 0.5f, 0.15f);
}

void vfx_play_arrow_impact(VFXSystem* system, Vec3 position, Vec3 normal) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_ARROW_IMPACT, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->direction = normal;
    effect->duration = 0.4f;

    // Small impact sparks
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_SPARK, position,
                       normal, 1.5f, 4, 0.6f);
}

void vfx_play_explosion(VFXSystem* system, Vec3 position, f32 radius) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_EXPLOSION, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->intensity = radius / 10.0f;  // Normalize by typical radius
    effect->duration = 2.0f;

    // Explosion particles
    u32 particle_count = (u32)(20.0f * (radius / 5.0f));
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_EXPLOSION, position,
                       (Vec3){0.0f, 0.0f, 0.0f}, radius * 0.4f, particle_count, 2.0f);

    // Smoke cloud
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_SMOKE, position,
                       (Vec3){0.0f, 2.0f, 0.0f}, radius * 0.3f, 10, 3.0f);

    // Play explosion sound
    if (system->audio_system) {
        u32 sound_id = audio_play_sound_2d(system->audio_system, SOUND_EXPLOSION,
                                          1.0f, SOUND_CATEGORY_BLOCK);
        if (sound_id != 0xFFFFFFFF) {
            effect->sound_id = sound_id;
            effect->audio_synced = true;
        }
    }

    // Heavy screen shake
    vfx_trigger_screen_shake(system, 1.0f, 0.3f);

    // Damage flash
    vfx_trigger_damage_flash(system, 0.8f, (Vec4){1.0f, 0.5f, 0.0f, 0.6f});
}

void vfx_play_fire_ignite(VFXSystem* system, Vec3 position) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_FIRE_IGNITE, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 3.0f;

    // Flame burst
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_FLAME, position,
                       (Vec3){0.0f, 1.0f, 0.0f}, 1.0f, 8, 2.0f);

    // Play fire sound
    if (system->audio_system) {
        u32 sound_id = audio_play_sound(system->audio_system, SOUND_FIRE_BURN,
                                       position, 0.6f, SOUND_CATEGORY_BLOCK);
        if (sound_id != 0xFFFFFFFF) {
            effect->sound_id = sound_id;
            effect->audio_synced = true;
        }
    }
}

void vfx_play_magic_cast(VFXSystem* system, Vec3 position, Vec4 color) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_MAGIC_CAST, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 1.0f;

    // Magic particles with custom color
    u32 emitter_id = emitter_create(system->particle_system, position,
                                   PARTICLE_TYPE_MAGIC_GLOW, 30, 1.0f);
    if (emitter_id != 0xFFFFFFFF) {
        emitter_set_color_range(system->particle_system, emitter_id, color,
                              (Vec4){color.x * 0.5f, color.y * 0.5f, color.z * 0.5f, 0.0f});
        effect->emitter_ids[effect->emitter_count++] = emitter_id;
    }
}

void vfx_play_heal_effect(VFXSystem* system, Vec3 position) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_HEAL, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 1.5f;

    // Green magic particles
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_MAGIC_GLOW, position,
                       (Vec3){0.0f, 1.0f, 0.0f}, 1.5f, 12, 1.2f);

    // Play heal sound
    audio_play_sound_2d(system->audio_system, SOUND_PLAYER_HEAL, 1.0f, SOUND_CATEGORY_PLAYER);
}

void vfx_play_poison_effect(VFXSystem* system, Vec3 position) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_POISON, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 2.0f;

    // Purple/green toxic particles
    u32 emitter_id = emitter_create(system->particle_system, position,
                                   PARTICLE_TYPE_MAGIC_GLOW, 20, 2.0f);
    if (emitter_id != 0xFFFFFFFF) {
        emitter_set_color_range(system->particle_system, emitter_id,
                              (Vec4){0.5f, 1.0f, 0.2f, 1.0f},
                              (Vec4){0.2f, 0.5f, 0.1f, 0.0f});
        effect->emitter_ids[effect->emitter_count++] = emitter_id;
    }
}

void vfx_play_lightning_strike(VFXSystem* system, Vec3 position) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_LIGHTNING_STRIKE, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 0.5f;

    // Electric blue particles
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_SPARK, position,
                       (Vec3){0.0f, 0.0f, 0.0f}, 3.0f, 20, 0.8f);

    // Screen flash (white for lightning)
    vfx_trigger_damage_flash(system, 1.0f, (Vec4){1.0f, 1.0f, 1.0f, 0.7f});

    // Heavy screen shake
    vfx_trigger_screen_shake(system, 0.8f, 0.2f);
}

void vfx_play_portal_effect(VFXSystem* system, Vec3 position, bool entering) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, entering ? VFX_PORTAL_ENTER : VFX_PORTAL_EXIT,
                                     position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 1.0f;

    // Purple portal particles
    u32 emitter_id = emitter_create(system->particle_system, position,
                                   PARTICLE_TYPE_MAGIC_GLOW, 40, 1.0f);
    if (emitter_id != 0xFFFFFFFF) {
        emitter_set_color_range(system->particle_system, emitter_id,
                              (Vec4){0.8f, 0.2f, 1.0f, 1.0f},
                              (Vec4){0.4f, 0.1f, 0.5f, 0.0f});
        effect->emitter_ids[effect->emitter_count++] = emitter_id;
    }
}

void vfx_play_teleport_effect(VFXSystem* system, Vec3 from_pos, Vec3 to_pos) {
    if (!system || !system->initialized) {
        return;
    }

    // Exit portal at from position
    vfx_play_portal_effect(system, from_pos, false);

    // Entry portal at to position
    vfx_play_portal_effect(system, to_pos, true);
}

void vfx_trigger_damage_flash(VFXSystem* system, f32 intensity, Vec4 color) {
    if (!system || !system->initialized) {
        return;
    }

    system->screen_effects.flash_time = 0.0f;
    system->screen_effects.flash_duration = 0.25f;
    system->screen_effects.flash_color = color;

    // Scale color intensity
    system->screen_effects.flash_color.w = intensity;
}

void vfx_trigger_screen_shake(VFXSystem* system, f32 intensity, f32 duration) {
    if (!system || !system->initialized) {
        return;
    }

    system->screen_effects.shake_time = 0.0f;
    system->screen_effects.shake_duration = duration;
    system->screen_effects.shake_intensity = intensity;
}

void vfx_set_vignette(VFXSystem* system, f32 strength) {
    if (!system || !system->initialized) {
        return;
    }

    system->screen_effects.target_vignette = (strength < 0.0f) ? 0.0f :
                                             ((strength > 1.0f) ? 1.0f : strength);
}

void vfx_play_death_effect(VFXSystem* system, Vec3 position) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_DEATH, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 2.0f;

    // Red explosion
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_EXPLOSION, position,
                       (Vec3){0.0f, 0.0f, 0.0f}, 4.0f, 30, 2.0f);

    // Screen effect (red damage flash)
    vfx_trigger_damage_flash(system, 1.0f, (Vec4){1.0f, 0.2f, 0.2f, 0.8f});

    // Screen shake
    vfx_trigger_screen_shake(system, 1.0f, 0.5f);
}

void vfx_play_level_up_effect(VFXSystem* system, Vec3 position) {
    if (!system || !system->initialized || !system->particle_system) {
        return;
    }

    u32 effect_id = vfx_create_effect(system, VFX_LEVEL_UP, position);
    if (effect_id == 0xFFFFFFFF) {
        return;
    }

    VisualEffect* effect = &system->effects[effect_id];
    effect->duration = 2.0f;

    // Golden particles spiraling upward
    Vec3 upward_velocity = {0.0f, 3.0f, 0.0f};
    particle_emit_burst(system->particle_system, PARTICLE_TYPE_MAGIC_GLOW, position,
                       upward_velocity, 2.0f, 20, 2.0f);
}

void vfx_update(VFXSystem* system, f32 delta_time) {
    if (!system || !system->initialized) {
        return;
    }

    // Update screen effects
    ScreenEffects* screen = &system->screen_effects;

    // Update flash effect
    if (screen->flash_time < screen->flash_duration) {
        screen->flash_time += delta_time;
    }

    // Update shake effect
    if (screen->shake_time < screen->shake_duration) {
        screen->shake_time += delta_time;
    }

    // Smoothly interpolate vignette
    screen->vignette_strength += (screen->target_vignette - screen->vignette_strength) * 0.1f;

    // Update visual effects
    for (u32 i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        VisualEffect* effect = &system->effects[i];
        if (!effect->active) {
            continue;
        }

        effect->time_alive += delta_time;

        // Check if effect duration expired
        if (effect->duration > 0.0f && effect->time_alive >= effect->duration) {
            effect->active = false;
            system->active_effect_count--;

            // Stop associated audio
            if (effect->audio_synced && system->audio_system) {
                audio_stop_sound(system->audio_system, effect->sound_id);
            }

            // Destroy associated emitters
            for (u32 j = 0; j < effect->emitter_count; j++) {
                emitter_destroy(system->particle_system, effect->emitter_ids[j]);
            }
        }
    }
}

bool vfx_is_effect_active(VFXSystem* system, VFXEffectType type, Vec3 position,
                         f32 radius) {
    if (!system || !system->initialized) {
        return false;
    }

    for (u32 i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        VisualEffect* effect = &system->effects[i];
        if (!effect->active || effect->type != type) {
            continue;
        }

        // Check distance
        f32 dx = effect->position.x - position.x;
        f32 dy = effect->position.y - position.y;
        f32 dz = effect->position.z - position.z;
        f32 distance = sqrtf(dx * dx + dy * dy + dz * dz);

        if (distance <= radius) {
            return true;
        }
    }

    return false;
}
