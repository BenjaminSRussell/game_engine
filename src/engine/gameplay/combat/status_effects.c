// status_effects.c - Status Effects System Implementation
#include <include/gameplay/combat/status_effects.h>
#include <include/core/logger.h>
#include <include/ecs/component_ids.h>
#include <include/ecs/ecs.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_ACTIVE_EFFECTS 16
#define MAX_STATUS_EFFECTS 1024
#define EFFECT_UPDATE_INTERVAL 0.1f
#define EFFECT_STACK_LIMIT 10

typedef struct {
  StatusEffectType type;
  float duration;
  float duration_remaining;
  float magnitude;
  float tick_timer;
  u32 stack_count;
  u64 source_entity_id;
  bool is_permanent;
  bool is_active;
} StatusEffectInstance;

typedef struct {
  u64 entity_id;
  StatusEffectInstance effects[MAX_ACTIVE_EFFECTS];
  u32 effect_count;
  bool is_dirty;
} EntityStatusEffects;

typedef struct {
  EntityStatusEffects entities[MAX_STATUS_EFFECTS];
  u32 entity_count;
  bool is_initialized;
  float current_time;
  float update_timer;
} StatusEffectSystem;

static StatusEffectSystem g_status_system = {0};

// Helper functions
static const char* get_effect_name(StatusEffectType type) {
  switch (type) {
    case EFFECT_BURNING: return "Burning";
    case EFFECT_POISON: return "Poison";
    case EFFECT_FREEZING: return "Freezing";
    case EFFECT_HEALING: return "Healing";
    case EFFECT_SHIELD: return "Shield";
    case EFFECT_SPEED: return "Speed";
    default: return "Unknown";
  }
}

static bool can_stack_with(StatusEffectType type, StatusEffectType existing_type) {
  // Same effects can stack
  if (type == existing_type) return true;
  
  // Some effects are mutually exclusive
  switch (type) {
    case EFFECT_BURNING:
      return existing_type != EFFECT_FREEZING;
    case EFFECT_FREEZING:
      return existing_type != EFFECT_BURNING;
    case EFFECT_HEALING:
      return existing_type != EFFECT_POISON;
    case EFFECT_POISON:
      return existing_type != EFFECT_HEALING;
    default:
      return true;
  }
}

static void apply_effect_logic(StatusEffectInstance *effect, u64 entity_id, float delta_time) {
  if (!effect || !effect->is_active) return;
  
  effect->duration_remaining -= delta_time;
  effect->tick_timer += delta_time;
  
  // Check if effect has expired
  if (!effect->is_permanent && effect->duration_remaining <= 0.0f) {
    effect->is_active = false;
    LOG_DEBUG("Effect %s expired for entity %llu", get_effect_name(effect->type), entity_id);
    return;
  }
  
  // Apply effect logic based on type (every EFFECT_UPDATE_INTERVAL seconds)
  if (effect->tick_timer >= EFFECT_UPDATE_INTERVAL) {
    effect->tick_timer = 0.0f;
    
    switch (effect->type) {
      case EFFECT_BURNING:
        // Apply fire damage over time
        LOG_DEBUG("Burning: Applied %.1f fire damage to entity %llu", effect->magnitude, entity_id);
        // This would integrate with the health system
        break;
        
      case EFFECT_POISON:
        // Apply poison damage over time
        LOG_DEBUG("Poison: Applied %.1f poison damage to entity %llu", effect->magnitude, entity_id);
        // This would integrate with the health system
        break;
        
      case EFFECT_FREEZING:
        // Slow down entity movement
        LOG_DEBUG("Freezing: Slowed entity %llu by %.1f%%", entity_id, effect->magnitude * 100.0f);
        // This would integrate with the movement system
        break;
        
      case EFFECT_HEALING:
        // Heal entity over time
        LOG_DEBUG("Healing: Healed entity %llu for %.1f HP", entity_id, effect->magnitude);
        // This would integrate with the health system
        break;
        
      case EFFECT_SHIELD:
        // Provide damage absorption
        LOG_DEBUG("Shield: Entity %llu has %.1f shield points", entity_id, effect->magnitude);
        // This would integrate with the damage system
        break;
        
      case EFFECT_SPEED:
        // Increase movement speed
        LOG_DEBUG("Speed: Entity %llu speed increased by %.1f%%", entity_id, effect->magnitude * 100.0f);
        // This would integrate with the movement system
        break;
    }
  }
}

static EntityStatusEffects* find_or_create_entity_effects(u64 entity_id) {
  // Look for existing entity effects
  for (u32 i = 0; i < g_status_system.entity_count; i++) {
    if (g_status_system.entities[i].entity_id == entity_id) {
      return &g_status_system.entities[i];
    }
  }
  
  // Create new entity effects if we have space
  if (g_status_system.entity_count < MAX_STATUS_EFFECTS) {
    EntityStatusEffects *entity_effects = &g_status_system.entities[g_status_system.entity_count++];
    memset(entity_effects, 0, sizeof(EntityStatusEffects));
    entity_effects->entity_id = entity_id;
    entity_effects->is_dirty = true;
    return entity_effects;
  }
  
  return NULL;
}

static StatusEffectInstance* find_effect(EntityStatusEffects *entity_effects, StatusEffectType type) {
  if (!entity_effects) return NULL;
  
  for (u32 i = 0; i < entity_effects->effect_count; i++) {
    if (entity_effects->effects[i].type == type && entity_effects->effects[i].is_active) {
      return &entity_effects->effects[i];
    }
  }
  
  return NULL;
}

static StatusEffectInstance* add_effect(EntityStatusEffects *entity_effects, StatusEffectType type) {
  if (!entity_effects || entity_effects->effect_count >= MAX_ACTIVE_EFFECTS) {
    return NULL;
  }
  
  // Check if we can stack with existing effects
  for (u32 i = 0; i < entity_effects->effect_count; i++) {
    if (entity_effects->effects[i].type == type && entity_effects->effects[i].is_active) {
      if (!can_stack_with(type, entity_effects->effects[i].type)) {
        return NULL; // Can't stack with this effect
      }
    }
  }
  
  // Find an inactive slot or create a new one
  for (u32 i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
    if (!entity_effects->effects[i].is_active) {
      StatusEffectInstance *effect = &entity_effects->effects[i];
      memset(effect, 0, sizeof(StatusEffectInstance));
      effect->type = type;
      effect->is_active = true;
      effect->stack_count = 1;
      
      if (i >= entity_effects->effect_count) {
        entity_effects->effect_count = i + 1;
      }
      
      entity_effects->is_dirty = true;
      return effect;
    }
  }
  
  return NULL;
}

static void remove_effect(EntityStatusEffects *entity_effects, StatusEffectType type) {
  if (!entity_effects) return;
  
  for (u32 i = 0; i < entity_effects->effect_count; i++) {
    if (entity_effects->effects[i].type == type) {
      entity_effects->effects[i].is_active = false;
      entity_effects->is_dirty = true;
      
      // Remove expired effects from the end
      while (entity_effects->effect_count > 0 && 
             !entity_effects->effects[entity_effects->effect_count - 1].is_active) {
        entity_effects->effect_count--;
      }
      
      LOG_DEBUG("Removed effect %s from entity %llu", get_effect_name(type), entity_effects->entity_id);
      return;
    }
  }
}

static void cleanup_entity_effects(u32 index) {
  if (index >= g_status_system.entity_count) return;
  
  EntityStatusEffects *entity_effects = &g_status_system.entities[index];
  
  // Check if entity has no active effects
  bool has_active_effects = false;
  for (u32 i = 0; i < entity_effects->effect_count; i++) {
    if (entity_effects->effects[i].is_active) {
      has_active_effects = true;
      break;
    }
  }
  
  if (!has_active_effects) {
    // Remove entity from the list
    if (index < g_status_system.entity_count - 1) {
      g_status_system.entities[index] = g_status_system.entities[g_status_system.entity_count - 1];
    }
    g_status_system.entity_count--;
  }
}

// Public API
void status_sys_init(void) {
  if (g_status_system.is_initialized) {
    LOG_WARN("Status effects system already initialized");
    return;
  }
  
  memset(&g_status_system, 0, sizeof(StatusEffectSystem));
  g_status_system.is_initialized = true;
  g_status_system.current_time = 0.0f;
  g_status_system.update_timer = 0.0f;
  
  LOG_INFO("Status effects system initialized");
}

void status_sys_shutdown(void) {
  if (!g_status_system.is_initialized) return;
  
  memset(&g_status_system, 0, sizeof(StatusEffectSystem));
  g_status_system.is_initialized = false;
  
  LOG_INFO("Status effects system shutdown");
}

void status_sys_update(float delta_time) {
  if (!g_status_system.is_initialized) return;
  
  g_status_system.current_time += delta_time;
  g_status_system.update_timer += delta_time;
  
  // Update all active effects
  for (u32 i = 0; i < g_status_system.entity_count; i++) {
    EntityStatusEffects *entity_effects = &g_status_system.entities[i];
    
    bool has_active_effects = false;
    
    for (u32 j = 0; j < entity_effects->effect_count; j++) {
      StatusEffectInstance *effect = &entity_effects->effects[j];
      
      if (effect->is_active) {
        has_active_effects = true;
        apply_effect_logic(effect, entity_effects->entity_id, delta_time);
      }
    }
    
    // Clean up entities with no active effects
    if (!has_active_effects) {
      cleanup_entity_effects(i);
      i--; // Adjust index since we removed an element
    }
  }
}

void status_sys_apply_effect(u64 entity_id, StatusEffectType type, float duration, float magnitude) {
  if (!g_status_system.is_initialized) return;
  
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (!entity_effects) {
    LOG_WARN("Failed to apply effect %s to entity %llu - system full", 
             get_effect_name(type), entity_id);
    return;
  }
  
  // Check for existing effect of same type
  StatusEffectInstance *existing_effect = find_effect(entity_effects, type);
  if (existing_effect) {
    // Stack the effect
    if (existing_effect->stack_count < EFFECT_STACK_LIMIT) {
      existing_effect->stack_count++;
      existing_effect->magnitude += magnitude;
      existing_effect->duration_remaining = fmaxf(existing_effect->duration_remaining, duration);
      
      LOG_DEBUG("Stacked effect %s on entity %llu (stack count: %u, magnitude: %.1f)", 
               get_effect_name(type), entity_id, existing_effect->stack_count, existing_effect->magnitude);
    } else {
      LOG_DEBUG("Effect %s on entity %llu reached stack limit", get_effect_name(type), entity_id);
    }
    return;
  }
  
  // Add new effect
  StatusEffectInstance *effect = add_effect(entity_effects, type);
  if (!effect) {
    LOG_WARN("Failed to add effect %s to entity %llu - entity full", 
             get_effect_name(type), entity_id);
    return;
  }
  
  effect->duration = duration;
  effect->duration_remaining = duration;
  effect->magnitude = magnitude;
  effect->tick_timer = 0.0f;
  effect->is_permanent = (duration <= 0.0f);
  
  LOG_DEBUG("Applied effect %s to entity %llu (duration: %.1f, magnitude: %.1f)", 
           get_effect_name(type), entity_id, duration, magnitude);
}

void status_sys_apply_effect_with_source(u64 entity_id, StatusEffectType type, float duration, 
                                        float magnitude, u64 source_entity_id) {
  status_sys_apply_effect(entity_id, type, duration, magnitude);
  
  // Set source entity if effect was successfully applied
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (entity_effects) {
    StatusEffectInstance *effect = find_effect(entity_effects, type);
    if (effect) {
      effect->source_entity_id = source_entity_id;
      LOG_DEBUG("Set effect %s source to entity %llu", get_effect_name(type), source_entity_id);
    }
  }
}

void status_sys_remove_effect(u64 entity_id, StatusEffectType type) {
  if (!g_status_system.is_initialized) return;
  
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (entity_effects) {
    remove_effect(entity_effects, type);
  }
}

void status_sys_clear_all_effects(u64 entity_id) {
  if (!g_status_system.is_initialized) return;
  
  // Find and remove entity effects
  for (u32 i = 0; i < g_status_system.entity_count; i++) {
    if (g_status_system.entities[i].entity_id == entity_id) {
      LOG_DEBUG("Cleared all effects from entity %llu", entity_id);
      cleanup_entity_effects(i);
      return;
    }
  }
}

bool status_sys_has_effect(u64 entity_id, StatusEffectType type) {
  if (!g_status_system.is_initialized) return false;
  
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (!entity_effects) return false;
  
  return find_effect(entity_effects, type) != NULL;
}

float status_sys_get_effect_magnitude(u64 entity_id, StatusEffectType type) {
  if (!g_status_system.is_initialized) return 0.0f;
  
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (!entity_effects) return 0.0f;
  
  StatusEffectInstance *effect = find_effect(entity_effects, type);
  return effect ? effect->magnitude : 0.0f;
}

float status_sys_get_effect_remaining_time(u64 entity_id, StatusEffectType type) {
  if (!g_status_system.is_initialized) return 0.0f;
  
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (!entity_effects) return 0.0f;
  
  StatusEffectInstance *effect = find_effect(entity_effects, type);
  return effect ? effect->duration_remaining : 0.0f;
}

u32 status_sys_get_effect_stack_count(u64 entity_id, StatusEffectType type) {
  if (!g_status_system.is_initialized) return 0;
  
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (!entity_effects) return 0;
  
  StatusEffectInstance *effect = find_effect(entity_effects, type);
  return effect ? effect->stack_count : 0;
}

uint32_t status_sys_get_active_effects(u64 entity_id, ActiveEffectInfo *out_effects, uint32_t max_count) {
  if (!g_status_system.is_initialized || !out_effects) return 0;
  
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (!entity_effects) return 0;
  
  uint32_t written = 0;
  for (u32 i = 0; i < entity_effects->effect_count && written < max_count; i++) {
    StatusEffectInstance *effect = &entity_effects->effects[i];
    
    if (effect->is_active) {
      out_effects[written].type = effect->type;
      out_effects[written].duration_remaining = effect->duration_remaining;
      out_effects[written].magnitude = effect->magnitude;
      written++;
    }
  }
  
  return written;
}

void status_sys_get_statistics(u32 *out_total_entities, u32 *out_total_effects, 
                              u32 *out_active_effects) {
  if (!out_total_entities || !out_total_effects || !out_active_effects) return;
  
  *out_total_entities = g_status_system.entity_count;
  *out_total_effects = 0;
  *out_active_effects = 0;
  
  for (u32 i = 0; i < g_status_system.entity_count; i++) {
    EntityStatusEffects *entity_effects = &g_status_system.entities[i];
    
    for (u32 j = 0; j < entity_effects->effect_count; j++) {
      (*out_total_effects)++;
      
      if (entity_effects->effects[j].is_active) {
        (*out_active_effects)++;
      }
    }
  }
}

void status_sys_debug_print_entity(u64 entity_id) {
  if (!g_status_system.is_initialized) return;
  
  EntityStatusEffects *entity_effects = find_or_create_entity_effects(entity_id);
  if (!entity_effects) {
    LOG_DEBUG("Entity %llu has no status effects", entity_id);
    return;
  }
  
  LOG_DEBUG("=== Status Effects for Entity %llu ===", entity_id);
  LOG_DEBUG("Total effects: %u", entity_effects->effect_count);
  
  for (u32 i = 0; i < entity_effects->effect_count; i++) {
    StatusEffectInstance *effect = &entity_effects->effects[i];
    
    if (effect->is_active) {
      LOG_DEBUG("  %s: magnitude=%.1f, remaining=%.1fs, stacks=%u, source=%llu",
               get_effect_name(effect->type), effect->magnitude, effect->duration_remaining,
               effect->stack_count, effect->source_entity_id);
    }
  }
  
  LOG_DEBUG("=================================");
}

void status_sys_debug_print_all(void) {
  if (!g_status_system.is_initialized) return;
  
  LOG_DEBUG("=== Status Effects System Status ===");
  LOG_DEBUG("Total entities with effects: %u", g_status_system.entity_count);
  LOG_DEBUG("Current time: %.2f", g_status_system.current_time);
  
  u32 total_effects = 0, active_effects = 0;
  status_sys_get_statistics(NULL, &total_effects, &active_effects);
  
  LOG_DEBUG("Total effects: %u", total_effects);
  LOG_DEBUG("Active effects: %u", active_effects);
  
  for (u32 i = 0; i < g_status_system.entity_count; i++) {
    EntityStatusEffects *entity_effects = &g_status_system.entities[i];
    status_sys_debug_print_entity(entity_effects->entity_id);
  }
  
  LOG_DEBUG("==================================");
}

bool status_sys_is_initialized(void) {
  return g_status_system.is_initialized;
}
