// include/vfx/status_effect_visuals.h
//
// Purpose: Visual indicators for status effects (glowing auras, color overlays)
//
#ifndef STATUS_EFFECT_VISUALS_H
#define STATUS_EFFECT_VISUALS_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <include/rendering/vulkan.h>

#define MAX_STATUS_EFFECT_VISUALS 128

typedef enum {
    AURA_TYPE_NONE,
    AURA_TYPE_GLOW,              // Glowing outline
    AURA_TYPE_PARTICLES,         // Particle trail
    AURA_TYPE_OVERLAY,           // Color screen overlay
    AURA_TYPE_OUTLINE,           // Model outline
    AURA_TYPE_SHIELD             // Shield visual
} AuraType;

typedef struct {
    u32 entityId;
    Vec3 entityPos;
    AuraType auraType;
    Vec4 glowColor;
    f32 glowIntensity;           // 0.0-1.0
    f32 pulsationSpeed;          // Hz
    f32 time;
    bool active;
} StatusEffectVisual;

typedef struct {
    StatusEffectVisual visuals[MAX_STATUS_EFFECT_VISUALS];
    u32 activeCount;
    bool initialized;
} StatusEffectVisualSystem;

// Lifecycle
void status_effect_visual_init(StatusEffectVisualSystem* system);
void status_effect_visual_shutdown(StatusEffectVisualSystem* system);

// Effect management
u32 status_effect_visual_add(StatusEffectVisualSystem* system, u32 entityId,
                            Vec3 position, AuraType auraType, Vec4 color);
void status_effect_visual_remove(StatusEffectVisualSystem* system, u32 visualId);

// Status effect shortcuts
u32 status_effect_visual_poison(StatusEffectVisualSystem* system, u32 entityId, Vec3 pos);
u32 status_effect_visual_regeneration(StatusEffectVisualSystem* system, u32 entityId, Vec3 pos);
u32 status_effect_visual_strength(StatusEffectVisualSystem* system, u32 entityId, Vec3 pos);
u32 status_effect_visual_speed(StatusEffectVisualSystem* system, u32 entityId, Vec3 pos);
u32 status_effect_visual_slowness(StatusEffectVisualSystem* system, u32 entityId, Vec3 pos);
u32 status_effect_visual_fire_resistance(StatusEffectVisualSystem* system, u32 entityId, Vec3 pos);
u32 status_effect_visual_invisibility(StatusEffectVisualSystem* system, u32 entityId, Vec3 pos);

// Update and rendering
void status_effect_visual_update(StatusEffectVisualSystem* system, f32 deltaTime);
void status_effect_visual_render(StatusEffectVisualSystem* system, VulkanRenderer* renderer, f32 deltaTime);

#endif // STATUS_EFFECT_VISUALS_H
