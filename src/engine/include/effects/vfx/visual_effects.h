// include/vfx/visual_effects.h
//
// Purpose: High-level visual effects system coordinating particles, screen effects,
// and lighting for gameplay events (combat, block interactions, status effects).
//
// Key Features:
// - Pre-configured effect templates for common events
// - Synchronized audio-visual feedback
// - Screen-space effects (damage flash, hit markers)
// - Dynamic lighting effects (explosions, fire, magic)
// - Status effect visual overlays
//
// Public APIs:
// - vfx_init: Initialize VFX system
// - vfx_play_*: Trigger specific visual effects
// - vfx_update: Update active effects each frame
//
// Invariants:
// - VFX system requires initialized ParticleSystem
// - Effects may spawn multiple particles
// - Screen effects are temporary (duration-based)
//
#ifndef VISUAL_EFFECTS_H
#define VISUAL_EFFECTS_H

#include <common.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include "include/effects/vfx/particle_system.h"
#include "include/audio/audio_system.h"

#define MAX_ACTIVE_EFFECTS 256

typedef enum {
    VFX_BLOCK_BREAK,
    VFX_BLOCK_PLACE,
    VFX_WATER_SPLASH,
    VFX_LAVA_SPLASH,
    VFX_SWORD_SWING,
    VFX_SWORD_HIT,
    VFX_CRITICAL_HIT,
    VFX_ARROW_IMPACT,
    VFX_EXPLOSION,
    VFX_FIRE_IGNITE,
    VFX_SMOKE_PUFF,
    VFX_MAGIC_CAST,
    VFX_HEAL,
    VFX_POISON,
    VFX_DEATH,
    VFX_LEVEL_UP,
    VFX_PORTAL_ENTER,
    VFX_PORTAL_EXIT,
    VFX_TELEPORT,
    VFX_LIGHTNING_STRIKE
} VFXEffectType;

typedef struct {
    VFXEffectType type;
    Vec3 position;
    Vec3 direction;      // For directional effects
    f32 intensity;       // 0.0-1.0 scale
    f32 duration;
    f32 time_alive;
    bool active;

    // Associated particle emitters
    u32 emitter_ids[4];
    u32 emitter_count;

    // Audio sync
    u32 sound_id;
    bool audio_synced;
} VisualEffect;

typedef struct {
    f32 flash_time;
    f32 flash_duration;
    Vec4 flash_color;

    f32 shake_time;
    f32 shake_duration;
    f32 shake_intensity;

    f32 vignette_strength;
    f32 target_vignette;
} ScreenEffects;

typedef struct {
    VisualEffect effects[MAX_ACTIVE_EFFECTS];
    u32 active_effect_count;

    ScreenEffects screen_effects;

    ParticleSystem* particle_system;
    AudioSystem* audio_system;

    bool initialized;
} VFXSystem;

// Lifecycle
void vfx_init(VFXSystem* system, ParticleSystem* particles, AudioSystem* audio);
void vfx_shutdown(VFXSystem* system);

// Block effects
void vfx_play_block_break(VFXSystem* system, Vec3 position, u16 block_type);
void vfx_play_block_place(VFXSystem* system, Vec3 position, u16 block_type);

// Fluid effects
void vfx_play_water_splash(VFXSystem* system, Vec3 position, f32 intensity);
void vfx_play_lava_splash(VFXSystem* system, Vec3 position, f32 intensity);

// Combat effects
void vfx_play_sword_swing(VFXSystem* system, Vec3 start_pos, Vec3 end_pos);
void vfx_play_sword_hit(VFXSystem* system, Vec3 position, Vec3 normal);
void vfx_play_critical_hit(VFXSystem* system, Vec3 position);
void vfx_play_arrow_impact(VFXSystem* system, Vec3 position, Vec3 normal);

// Explosion and fire
void vfx_play_explosion(VFXSystem* system, Vec3 position, f32 radius);
void vfx_play_fire_ignite(VFXSystem* system, Vec3 position);

// Magic and status effects
void vfx_play_magic_cast(VFXSystem* system, Vec3 position, Vec4 color);
void vfx_play_heal_effect(VFXSystem* system, Vec3 position);
void vfx_play_poison_effect(VFXSystem* system, Vec3 position);

// Environmental effects
void vfx_play_lightning_strike(VFXSystem* system, Vec3 position);
void vfx_play_portal_effect(VFXSystem* system, Vec3 position, bool entering);
void vfx_play_teleport_effect(VFXSystem* system, Vec3 from_pos, Vec3 to_pos);

// Screen effects
void vfx_trigger_damage_flash(VFXSystem* system, f32 intensity, Vec4 color);
void vfx_trigger_screen_shake(VFXSystem* system, f32 intensity, f32 duration);
void vfx_set_vignette(VFXSystem* system, f32 strength);

// Death and special events
void vfx_play_death_effect(VFXSystem* system, Vec3 position);
void vfx_play_level_up_effect(VFXSystem* system, Vec3 position);

// Update
void vfx_update(VFXSystem* system, f32 delta_time);

// Query
bool vfx_is_effect_active(VFXSystem* system, VFXEffectType type, Vec3 position,
                         f32 radius);

#endif // VISUAL_EFFECTS_H
