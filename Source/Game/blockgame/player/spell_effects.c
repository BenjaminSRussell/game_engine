// Player Spell Effects System Implementation
// Provides comprehensive spell effect management with stacking, duration, and area effects

#include <player/spell_effects.h>
#include <player/player.h>
#include <player/player_magic.h>
#include <audio/audio_system.h>
#include <effects/vfx/particle_system.h>
#include <core/logger.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Forward declaration for global particle system
extern ParticleSystem *g_particle_system;

// Effect configurations
static const SpellEffectConfig kEffectConfigs[EFFECT_COUNT] = {
    [EFFECT_DAMAGE] = {
        .type = EFFECT_DAMAGE,
        .name = "Damage",
        .description = "Instant damage effect",
        .stacking_type = STACKING_NONE,
        .max_stacks = 1,
        .is_negative = true,
        .default_duration = 0.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_BLOOD,
        .sound_effect_id = SOUND_SWORD_HIT
    },
    [EFFECT_HEAL] = {
        .type = EFFECT_HEAL,
        .name = "Heal",
        .description = "Instant healing effect",
        .stacking_type = STACKING_ADDITIVE,
        .max_stacks = 5,
        .is_negative = false,
        .default_duration = 0.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_HEART,
        .sound_effect_id = SOUND_PLAYER_HEAL
    },
    [EFFECT_SHIELD] = {
        .type = EFFECT_SHIELD,
        .name = "Shield",
        .description = "Damage absorption shield",
        .stacking_type = STACKING_REPLACE,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 10.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_ENCHANT,
        .sound_effect_id = SOUND_CRAFTING_SUCCESS
    },
    [EFFECT_FREEZE] = {
        .type = EFFECT_FREEZE,
        .name = "Freeze",
        .description = "Immobilizes target",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = true,
        .default_duration = 3.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_FROST,
        .sound_effect_id = SOUND_ICE_BREAK
    },
    [EFFECT_BURN] = {
        .type = EFFECT_BURN,
        .name = "Burn",
        .description = "Fire damage over time",
        .stacking_type = STACKING_ADDITIVE,
        .max_stacks = 3,
        .is_negative = true,
        .default_duration = 5.0f,
        .default_tick_interval = 1.0f,
        .visual_effect_id = PARTICLE_TYPE_FLAME,
        .sound_effect_id = SOUND_FIRE_BURN
    },
    [EFFECT_POISON] = {
        .type = EFFECT_POISON,
        .name = "Poison",
        .description = "Poison damage over time",
        .stacking_type = STACKING_ADDITIVE,
        .max_stacks = 5,
        .is_negative = true,
        .default_duration = 8.0f,
        .default_tick_interval = 0.5f,
        .visual_effect_id = PARTICLE_TYPE_POISON,
        .sound_effect_id = SOUND_SWORD_HIT
    },
    [EFFECT_SLOWNESS] = {
        .type = EFFECT_SLOWNESS,
        .name = "Slowness",
        .description = "Reduces movement speed",
        .stacking_type = STACKING_MULTIPLICATIVE,
        .max_stacks = 1,
        .is_negative = true,
        .default_duration = 6.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_SMOKE,
        .sound_effect_id = SOUND_SWORD_SWING
    },
    [EFFECT_HASTE] = {
        .type = EFFECT_HASTE,
        .name = "Haste",
        .description = "Increases movement and attack speed",
        .stacking_type = STACKING_MULTIPLICATIVE,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 8.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_ENCHANT,
        .sound_effect_id = SOUND_CRAFTING_SUCCESS
    },
    [EFFECT_INVISIBILITY] = {
        .type = EFFECT_INVISIBILITY,
        .name = "Invisibility",
        .description = "Makes target invisible",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 12.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_SMOKE,
        .sound_effect_id = SOUND_WIND_LIGHT
    },
    [EFFECT_FLIGHT] = {
        .type = EFFECT_FLIGHT,
        .name = "Flight",
        .description = "Allows target to fly",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 15.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_DUST,
        .sound_effect_id = SOUND_WIND_LIGHT
    },
    [EFFECT_REGENERATION] = {
        .type = EFFECT_REGENERATION,
        .name = "Regeneration",
        .description = "Health regeneration over time",
        .stacking_type = STACKING_ADDITIVE,
        .max_stacks = 3,
        .is_negative = false,
        .default_duration = 10.0f,
        .default_tick_interval = 1.0f,
        .visual_effect_id = PARTICLE_TYPE_HEART,
        .sound_effect_id = SOUND_PLAYER_HEAL
    },
    [EFFECT_MANA_REGEN] = {
        .type = EFFECT_MANA_REGEN,
        .name = "Mana Regeneration",
        .description = "Mana regeneration over time",
        .stacking_type = STACKING_ADDITIVE,
        .max_stacks = 3,
        .is_negative = false,
        .default_duration = 15.0f,
        .default_tick_interval = 0.5f,
        .visual_effect_id = PARTICLE_TYPE_ENCHANT,
        .sound_effect_id = SOUND_CRAFTING_SUCCESS
    },
    [EFFECT_STRENGTH] = {
        .type = EFFECT_STRENGTH,
        .name = "Strength",
        .description = "Increases physical damage",
        .stacking_type = STACKING_MULTIPLICATIVE,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 12.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_ENCHANT,
        .sound_effect_id = SOUND_CRAFTING_SUCCESS
    },
    [EFFECT_WEAKNESS] = {
        .type = EFFECT_WEAKNESS,
        .name = "Weakness",
        .description = "Reduces physical damage",
        .stacking_type = STACKING_MULTIPLICATIVE,
        .max_stacks = 1,
        .is_negative = true,
        .default_duration = 10.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_SMOKE,
        .sound_effect_id = SOUND_SWORD_SWING
    },
    [EFFECT_RESISTANCE] = {
        .type = EFFECT_RESISTANCE,
        .name = "Resistance",
        .description = "Reduces incoming damage",
        .stacking_type = STACKING_MULTIPLICATIVE,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 15.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_ENCHANT,
        .sound_effect_id = SOUND_CRAFTING_SUCCESS
    },
    [EFFECT_VULNERABILITY] = {
        .type = EFFECT_VULNERABILITY,
        .name = "Vulnerability",
        .description = "Increases incoming damage",
        .stacking_type = STACKING_MULTIPLICATIVE,
        .max_stacks = 1,
        .is_negative = true,
        .default_duration = 8.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_SMOKE,
        .sound_effect_id = SOUND_SWORD_SWING
    },
    [EFFECT_BLINDNESS] = {
        .type = EFFECT_BLINDNESS,
        .name = "Blindness",
        .description = "Reduces visibility range",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = true,
        .default_duration = 6.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_SMOKE,
        .sound_effect_id = SOUND_SWORD_SWING
    },
    [EFFECT_SILENCE] = {
        .type = EFFECT_SILENCE,
        .name = "Silence",
        .description = "Prevents spell casting",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = true,
        .default_duration = 4.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_SMOKE,
        .sound_effect_id = SOUND_SWORD_SWING
    },
    [EFFECT_STUN] = {
        .type = EFFECT_STUN,
        .name = "Stun",
        .description = "Immobilizes and prevents actions",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = true,
        .default_duration = 2.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_FROST,
        .sound_effect_id = SOUND_ICE_BREAK
    },
    [EFFECT_LEVITATION] = {
        .type = EFFECT_LEVITATION,
        .name = "Levitation",
        .description = "Causes target to float upward",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 8.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_DUST,
        .sound_effect_id = SOUND_WIND_LIGHT
    },
    [EFFECT_MAGNETISM] = {
        .type = EFFECT_MAGNETISM,
        .name = "Magnetism",
        .description = "Attracts nearby items",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 20.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_ENCHANT,
        .sound_effect_id = SOUND_CRAFTING_SUCCESS
    },
    [EFFECT_REFLECTION] = {
        .type = EFFECT_REFLECTION,
        .name = "Reflection",
        .description = "Reflects incoming damage",
        .stacking_type = STACKING_DURATION_REFRESH,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 10.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_ENCHANT,
        .sound_effect_id = SOUND_CRAFTING_SUCCESS
    },
    [EFFECT_LIFE_STEAL] = {
        .type = EFFECT_LIFE_STEAL,
        .name = "Life Steal",
        .description = "Heals when dealing damage",
        .stacking_type = STACKING_MULTIPLICATIVE,
        .max_stacks = 1,
        .is_negative = false,
        .default_duration = 12.0f,
        .default_tick_interval = 0.0f,
        .visual_effect_id = PARTICLE_TYPE_HEART,
        .sound_effect_id = SOUND_PLAYER_HEAL
    },
    [EFFECT_MANA_DRAIN] = {
        .type = EFFECT_MANA_DRAIN,
        .name = "Mana Drain",
        .description = "Drains mana over time",
        .stacking_type = STACKING_ADDITIVE,
        .max_stacks = 3,
        .is_negative = true,
        .default_duration = 6.0f,
        .default_tick_interval = 0.5f,
        .visual_effect_id = PARTICLE_TYPE_SMOKE,
        .sound_effect_id = SOUND_SWORD_SWING
    }
};

// Global spell effect manager
static SpellEffectManager g_effect_manager = {0};

// Helper function to get player component from entity ID
static PlayerComponent* get_player_component(EntityID entity_id) {
    // This would need to be implemented based on the ECS system
    // For now, return NULL as placeholder
    return NULL;
}

// Helper function to apply instant effects
static void apply_instant_effect(SpellEffect* effect) {
    if (!effect) return;
    
    PlayerComponent* player = get_player_component(effect->target_entity);
    if (!player) return;
    
    switch (effect->type) {
    case EFFECT_DAMAGE:
        // Apply damage to player
        LOG_DEBUG("Applying %.1f damage to entity %u", effect->magnitude, effect->target_entity);
        break;
        
    case EFFECT_HEAL:
        player_heal(NULL, effect->magnitude);
        LOG_DEBUG("Healing entity %u for %.1f HP", effect->target_entity, effect->magnitude);
        break;
        
    default:
        break;
    }
}

// Helper function to apply periodic effects (damage over time, etc.)
static void apply_periodic_effect(SpellEffect* effect, f32 delta_time) {
    if (!effect || effect->tick_interval <= 0.0f) return;
    
    effect->next_tick_time -= delta_time;
    if (effect->next_tick_time > 0.0f) return;
    
    effect->next_tick_time = effect->tick_interval;
    
    PlayerComponent* player = get_player_component(effect->target_entity);
    if (!player) return;
    
    switch (effect->type) {
    case EFFECT_BURN:
        LOG_DEBUG("Burn damage: %.1f to entity %u", effect->magnitude, effect->target_entity);
        // Apply burn damage
        break;
        
    case EFFECT_POISON:
        LOG_DEBUG("Poison damage: %.1f to entity %u", effect->magnitude, effect->target_entity);
        // Apply poison damage
        break;
        
    case EFFECT_REGENERATION:
        player_heal(NULL, effect->magnitude);
        LOG_DEBUG("Regeneration: %.1f HP to entity %u", effect->magnitude, effect->target_entity);
        break;
        
    case EFFECT_MANA_REGEN:
        if (player->magic_component.current_mana < player->magic_component.max_mana) {
            player->magic_component.current_mana = MIN(
                player->magic_component.current_mana + effect->magnitude,
                player->magic_component.max_mana
            );
            LOG_DEBUG("Mana regeneration: %.1f to entity %u", effect->magnitude, effect->target_entity);
        }
        break;
        
    case EFFECT_MANA_DRAIN:
        if (player->magic_component.current_mana > 0.0f) {
            player->magic_component.current_mana = MAX(
                player->magic_component.current_mana - effect->magnitude,
                0.0f
            );
            LOG_DEBUG("Mana drain: %.1f from entity %u", effect->magnitude, effect->target_entity);
        }
        break;
        
    default:
        break;
    }
}

// Helper function to handle effect stacking
static SpellEffect* find_existing_effect(SpellEffectManager* manager, 
                                       EntityID target, 
                                       SpellEffectType type) {
    for (u32 i = 0; i < manager->active_count; i++) {
        SpellEffect* effect = &manager->effects[i];
        if (effect->target_entity == target && effect->type == type) {
            return effect;
        }
    }
    return NULL;
}

// Helper function to handle stacking behavior
static bool handle_stacking(SpellEffectManager* manager, SpellEffect* new_effect) {
    const SpellEffectConfig* config = &kEffectConfigs[new_effect->type];
    SpellEffect* existing = find_existing_effect(manager, new_effect->target_entity, new_effect->type);
    
    if (!existing) {
        return false; // No existing effect, proceed with adding new one
    }
    
    switch (config->stacking_type) {
    case STACKING_NONE:
        // Replace existing effect
        if (new_effect->magnitude > existing->magnitude) {
            *existing = *new_effect;
            existing->effect_id = manager->next_effect_id++;
        }
        return true;
        
    case STACKING_REPLACE:
        // Replace with stronger effect
        if (new_effect->magnitude >= existing->magnitude) {
            *existing = *new_effect;
            existing->effect_id = manager->next_effect_id++;
        }
        return true;
        
    case STACKING_ADDITIVE:
        // Add magnitudes
        if (existing->stack_count < config->max_stacks) {
            existing->magnitude += new_effect->magnitude;
            existing->stack_count++;
            existing->remaining_time = MAX(existing->remaining_time, new_effect->remaining_time);
        }
        return true;
        
    case STACKING_MULTIPLICATIVE:
        // Multiply magnitudes
        if (existing->stack_count < config->max_stacks) {
            existing->magnitude *= new_effect->magnitude;
            existing->stack_count++;
            existing->remaining_time = MAX(existing->remaining_time, new_effect->remaining_time);
        }
        return true;
        
    case STACKING_DURATION_REFRESH:
        // Refresh duration, keep stronger magnitude
        existing->remaining_time = MAX(existing->remaining_time, new_effect->remaining_time);
        if (new_effect->magnitude > existing->magnitude) {
            existing->magnitude = new_effect->magnitude;
        }
        return true;
        
    default:
        return false;
    }
}

// ============================================================================
// SYSTEM LIFECYCLE IMPLEMENTATION
// ============================================================================

bool spell_effect_manager_init(SpellEffectManager* manager, u32 max_effects) {
    if (!manager || max_effects == 0) {
        LOG_ERROR("Invalid parameters for spell effect manager initialization");
        return false;
    }
    
    memset(manager, 0, sizeof(SpellEffectManager));
    manager->effects = malloc(sizeof(SpellEffect) * max_effects);
    if (!manager->effects) {
        LOG_ERROR("Failed to allocate memory for spell effects");
        return false;
    }
    
    manager->max_effects = max_effects;
    manager->next_effect_id = 1;
    manager->visual_effects_enabled = true;
    manager->sound_effects_enabled = true;
    manager->time_scale = 1.0f;
    
    LOG_INFO("Spell effect manager initialized with max %u effects", max_effects);
    return true;
}

void spell_effect_manager_shutdown(SpellEffectManager* manager) {
    if (!manager) return;
    
    if (manager->effects) {
        free(manager->effects);
        manager->effects = NULL;
    }
    
    memset(manager, 0, sizeof(SpellEffectManager));
    LOG_INFO("Spell effect manager shutdown");
}

// ============================================================================
// EFFECT APPLICATION IMPLEMENTATION
// ============================================================================

SpellEffectResult spell_effect_apply(SpellEffectManager* manager, 
                                   SpellEffectType type,
                                   EntityID target, 
                                   EntityID source,
                                   f32 magnitude, 
                                   f32 duration) {
    SpellEffectResult result = {0};
    
    if (!manager || !spell_effect_validate_parameters(type, magnitude, duration)) {
        result.error_message = "Invalid parameters";
        return result;
    }
    
    if (manager->active_count >= manager->max_effects) {
        result.error_message = "Effect manager at capacity";
        return result;
    }
    
    // Check stacking behavior
    SpellEffect temp_effect = {
        .type = type,
        .target_entity = target,
        .source_entity = source,
        .magnitude = magnitude,
        .duration = duration,
        .remaining_time = duration,
        .tick_interval = kEffectConfigs[type].default_tick_interval,
        .next_tick_time = kEffectConfigs[type].default_tick_interval,
        .stack_count = 1,
        .is_permanent = (duration <= 0.0f),
        .is_negative = kEffectConfigs[type].is_negative,
        .effect_id = manager->next_effect_id++
    };
    
    if (handle_stacking(manager, &temp_effect)) {
        result.success = true;
        result.effect_id = temp_effect.effect_id;
        manager->total_effects_applied++;
        return result;
    }
    
    // Add new effect
    SpellEffect* effect = &manager->effects[manager->active_count];
    *effect = temp_effect;
    
    // Apply instant effects immediately
    if (effect->duration <= 0.0f) {
        apply_instant_effect(effect);
        // Don't add instant effects to the active list
        result.success = true;
        result.effect_id = effect->effect_id;
        manager->total_effects_applied++;
        return result;
    }
    
    manager->active_count++;
    manager->total_effects_applied++;
    
    result.success = true;
    result.effect_id = effect->effect_id;
    
    LOG_DEBUG("Applied effect %s to entity %u (magnitude: %.1f, duration: %.1f)", 
              kEffectConfigs[type].name, target, magnitude, duration);
    
    return result;
}

u32 spell_effect_apply_area(SpellEffectManager* manager,
                           SpellEffectType type,
                           Vec3 center,
                           f32 radius,
                           EntityID source,
                           f32 magnitude,
                           f32 duration) {
    if (!manager || radius <= 0.0f) {
        return 0;
    }
    
    // This would need to query the ECS system for entities in range
    // For now, return 0 as placeholder
    LOG_DEBUG("Area effect %s at (%.1f, %.1f, %.1f) radius %.1f", 
              kEffectConfigs[type].name, center.x, center.y, center.z, radius);
    
    return 0;
}

bool spell_effect_remove(SpellEffectManager* manager, u32 effect_id) {
    if (!manager || effect_id == 0) {
        return false;
    }
    
    for (u32 i = 0; i < manager->active_count; i++) {
        if (manager->effects[i].effect_id == effect_id) {
            // Move last effect to this position
            if (i < manager->active_count - 1) {
                manager->effects[i] = manager->effects[manager->active_count - 1];
            }
            manager->active_count--;
            manager->total_effects_removed++;
            
            LOG_DEBUG("Removed effect %u", effect_id);
            return true;
        }
    }
    
    return false;
}

u32 spell_effect_remove_by_type(SpellEffectManager* manager, 
                               EntityID target, 
                               SpellEffectType type) {
    if (!manager) return 0;
    
    u32 removed_count = 0;
    for (i32 i = (i32)manager->active_count - 1; i >= 0; i--) {
        SpellEffect* effect = &manager->effects[i];
        if (effect->target_entity == target && effect->type == type) {
            // Move last effect to this position
            if (i < (i32)manager->active_count - 1) {
                *effect = manager->effects[manager->active_count - 1];
            }
            manager->active_count--;
            removed_count++;
            manager->total_effects_removed++;
        }
    }
    
    if (removed_count > 0) {
        LOG_DEBUG("Removed %u effects of type %s from entity %u", 
                  removed_count, kEffectConfigs[type].name, target);
    }
    
    return removed_count;
}

u32 spell_effect_remove_all(SpellEffectManager* manager, EntityID target) {
    if (!manager) return 0;
    
    u32 removed_count = 0;
    for (i32 i = (i32)manager->active_count - 1; i >= 0; i--) {
        if (manager->effects[i].target_entity == target) {
            // Move last effect to this position
            if (i < (i32)manager->active_count - 1) {
                manager->effects[i] = manager->effects[manager->active_count - 1];
            }
            manager->active_count--;
            removed_count++;
            manager->total_effects_removed++;
        }
    }
    
    if (removed_count > 0) {
        LOG_DEBUG("Removed all %u effects from entity %u", removed_count, target);
    }
    
    return removed_count;
}

// ============================================================================
// EFFECT QUERIES IMPLEMENTATION
// ============================================================================

bool spell_effect_has_active(SpellEffectManager* manager, 
                           EntityID target, 
                           SpellEffectType type) {
    if (!manager) return false;
    
    for (u32 i = 0; i < manager->active_count; i++) {
        SpellEffect* effect = &manager->effects[i];
        if (effect->target_entity == target && effect->type == type) {
            return true;
        }
    }
    
    return false;
}

f32 spell_effect_get_magnitude(SpellEffectManager* manager, 
                             EntityID target, 
                             SpellEffectType type) {
    if (!manager) return 0.0f;
    
    f32 total_magnitude = 0.0f;
    const SpellEffectConfig* config = &kEffectConfigs[type];
    
    for (u32 i = 0; i < manager->active_count; i++) {
        SpellEffect* effect = &manager->effects[i];
        if (effect->target_entity == target && effect->type == type) {
            switch (config->stacking_type) {
            case STACKING_ADDITIVE:
                total_magnitude += effect->magnitude;
                break;
            case STACKING_MULTIPLICATIVE:
                total_magnitude = (total_magnitude == 0.0f) ? effect->magnitude : total_magnitude * effect->magnitude;
                break;
            default:
                return effect->magnitude; // Return first found for non-stacking types
            }
        }
    }
    
    return total_magnitude;
}

f32 spell_effect_get_remaining_time(SpellEffectManager* manager, 
                                   EntityID target, 
                                   SpellEffectType type) {
    if (!manager) return 0.0f;
    
    f32 max_time = 0.0f;
    for (u32 i = 0; i < manager->active_count; i++) {
        SpellEffect* effect = &manager->effects[i];
        if (effect->target_entity == target && effect->type == type) {
            max_time = MAX(max_time, effect->remaining_time);
        }
    }
    
    return max_time;
}

u32 spell_effect_get_stack_count(SpellEffectManager* manager, 
                                EntityID target, 
                                SpellEffectType type) {
    if (!manager) return 0;
    
    u32 stack_count = 0;
    for (u32 i = 0; i < manager->active_count; i++) {
        SpellEffect* effect = &manager->effects[i];
        if (effect->target_entity == target && effect->type == type) {
            stack_count += effect->stack_count;
        }
    }
    
    return stack_count;
}

u32 spell_effect_get_entity_effects(SpellEffectManager* manager,
                                   EntityID target,
                                   SpellEffect* out_effects,
                                   u32 max_effects) {
    if (!manager || !out_effects || max_effects == 0) {
        return 0;
    }
    
    u32 count = 0;
    for (u32 i = 0; i < manager->active_count && count < max_effects; i++) {
        SpellEffect* effect = &manager->effects[i];
        if (effect->target_entity == target) {
            out_effects[count] = *effect;
            count++;
        }
    }
    
    return count;
}

// ============================================================================
// SYSTEM UPDATES IMPLEMENTATION
// ============================================================================

void spell_effect_update(SpellEffectManager* manager, f32 delta_time) {
    if (!manager) return;
    
    f32 scaled_delta = delta_time * manager->time_scale;
    
    // Process periodic effects
    spell_effect_process_dot(manager, scaled_delta);
    
    // Update effect durations and remove expired effects
    for (i32 i = (i32)manager->active_count - 1; i >= 0; i--) {
        SpellEffect* effect = &manager->effects[i];
        
        if (!effect->is_permanent) {
            effect->remaining_time -= scaled_delta;
            
            if (effect->remaining_time <= 0.0f) {
                // Move last effect to this position
                if (i < (i32)manager->active_count - 1) {
                    *effect = manager->effects[manager->active_count - 1];
                }
                manager->active_count--;
                manager->total_effects_removed++;
                
                LOG_DEBUG("Effect %s expired on entity %u", 
                          kEffectConfigs[effect->type].name, effect->target_entity);
            }
        }
    }
    
    // Update visual and audio feedback
    spell_effect_update_feedback(manager, delta_time);
}

void spell_effect_process_dot(SpellEffectManager* manager, f32 delta_time) {
    if (!manager) return;
    
    for (u32 i = 0; i < manager->active_count; i++) {
        SpellEffect* effect = &manager->effects[i];
        apply_periodic_effect(effect, delta_time);
    }
}

void spell_effect_update_feedback(SpellEffectManager* manager, f32 delta_time) {
    if (!manager) return;
    
    // This would update visual effects and play periodic sounds
    // For now, just a placeholder implementation
    (void)delta_time;
}

// ============================================================================
// CONFIGURATION IMPLEMENTATION
// ============================================================================

const SpellEffectConfig* spell_effect_get_config(SpellEffectType type) {
    if (type >= EFFECT_COUNT) {
        return NULL;
    }
    
    return &kEffectConfigs[type];
}

void spell_effect_set_visual_enabled(SpellEffectManager* manager, bool enabled) {
    if (manager) {
        manager->visual_effects_enabled = enabled;
        LOG_INFO("Spell visual effects %s", enabled ? "enabled" : "disabled");
    }
}

void spell_effect_set_sound_enabled(SpellEffectManager* manager, bool enabled) {
    if (manager) {
        manager->sound_effects_enabled = enabled;
        LOG_INFO("Spell sound effects %s", enabled ? "enabled" : "disabled");
    }
}

void spell_effect_set_time_scale(SpellEffectManager* manager, f32 scale) {
    if (manager && scale > 0.0f) {
        manager->time_scale = scale;
        LOG_INFO("Spell effect time scale set to %.2f", scale);
    }
}

// ============================================================================
// UTILITY FUNCTIONS IMPLEMENTATION
// ============================================================================

const char* spell_effect_get_name(SpellEffectType type) {
    if (type >= EFFECT_COUNT) {
        return "Unknown";
    }
    
    return kEffectConfigs[type].name;
}

bool spell_effect_is_negative(SpellEffectType type) {
    if (type >= EFFECT_COUNT) {
        return false;
    }
    
    return kEffectConfigs[type].is_negative;
}

EffectStackingType spell_effect_get_stacking_type(SpellEffectType type) {
    if (type >= EFFECT_COUNT) {
        return STACKING_NONE;
    }
    
    return kEffectConfigs[type].stacking_type;
}

f32 spell_effect_calculate_stacked_magnitude(SpellEffectType type, 
                                           f32 base_magnitude, 
                                           u32 stack_count) {
    if (type >= EFFECT_COUNT || stack_count == 0) {
        return base_magnitude;
    }
    
    const SpellEffectConfig* config = &kEffectConfigs[type];
    
    switch (config->stacking_type) {
    case STACKING_ADDITIVE:
        return base_magnitude * stack_count;
        
    case STACKING_MULTIPLICATIVE:
        return base_magnitude * powf(1.2f, stack_count - 1); // 20% increase per stack
        
    default:
        return base_magnitude;
    }
}

bool spell_effect_validate_parameters(SpellEffectType type, 
                                   f32 magnitude, 
                                   f32 duration) {
    if (type >= EFFECT_COUNT) {
        return false;
    }
    
    if (magnitude < 0.0f) {
        return false;
    }
    
    if (duration < 0.0f) {
        return false;
    }
    
    return true;
}

// ============================================================================
// STATISTICS IMPLEMENTATION
// ============================================================================

void spell_effect_get_stats(SpellEffectManager* manager,
                          u32* total_applied,
                          u32* total_removed,
                          u32* active_count) {
    if (!manager) return;
    
    if (total_applied) *total_applied = manager->total_effects_applied;
    if (total_removed) *total_removed = manager->total_effects_removed;
    if (active_count) *active_count = manager->active_count;
}

void spell_effect_reset_stats(SpellEffectManager* manager) {
    if (manager) {
        manager->total_effects_applied = 0;
        manager->total_effects_removed = 0;
        LOG_INFO("Spell effect statistics reset");
    }
}

// ============================================================================
// GLOBAL MANAGER ACCESS
// ============================================================================

SpellEffectManager* spell_effect_get_global_manager(void) {
    return &g_effect_manager;
}

bool spell_effect_global_init(u32 max_effects) {
    return spell_effect_manager_init(&g_effect_manager, max_effects);
}

void spell_effect_global_shutdown(void) {
    spell_effect_manager_shutdown(&g_effect_manager);
}
