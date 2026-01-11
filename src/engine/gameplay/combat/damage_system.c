// damage_system.c - Implementation
#include "include/gameplay/combat/damage.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

static World *damage_world = NULL;
static DamageEvent damage_events[1024];
static u32 damage_event_count = 0;

bool damage_system_init(World *world) {
  if (!world) {
    LOG_ERROR("Damage system: NULL world provided");
    return false;
  }
  
  damage_world = world;
  damage_event_count = 0;
  
  LOG_INFO("Damage system initialized");
  return true;
}

void damage_system_shutdown(void) {
  damage_world = NULL;
  damage_event_count = 0;
  LOG_INFO("Damage system shutdown");
}

void damage_apply(Entity target, Entity source, f32 amount, DamageType type) {
  if (!damage_world || amount <= 0.0f) return;
  
  DamageEvent *event = &damage_events[damage_event_count % 1024];
  event->target_entity = target;
  event->source_entity = source;
  event->damage_amount = amount;
  event->damage_type = type;
  event->timestamp = 0.0f; // Would get actual time
  
  damage_event_count++;
  
  LOG_DEBUG("Damage applied: entity %d -> %d, amount %.2f, type %d", 
            source, target, amount, type);
}

void damage_process_events(World *world) {
  if (!world) return;
  
  for (u32 i = 0; i < damage_event_count && i < 1024; i++) {
    DamageEvent *event = &damage_events[i];
    
    // Apply damage to entity health component
    // This would integrate with the health system
    LOG_DEBUG("Processing damage event: %d -> %d, %.2f", 
              event->source_entity, event->target_entity, event->damage_amount);
  }
  
  damage_event_count = 0;
}

f32 damage_calculate(Entity attacker, Entity defender, f32 base_damage, DamageType type) {
  // Simple damage calculation with modifiers
  f32 final_damage = base_damage;
  
  // Apply damage type modifiers
  switch (type) {
    case DAMAGE_TYPE_MELEE:
      final_damage *= 1.0f;
      break;
    case DAMAGE_TYPE_RANGED:
      final_damage *= 0.9f;
      break;
    case DAMAGE_TYPE_MAGIC:
      final_damage *= 1.2f;
      break;
    case DAMAGE_TYPE_FIRE:
      final_damage *= 1.1f;
      break;
    case DAMAGE_TYPE_POISON:
      final_damage *= 0.8f;
      break;
    default:
      break;
  }
  
  return fmaxf(0.0f, final_damage);
}
