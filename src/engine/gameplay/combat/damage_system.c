#include "ecs/components/health.h"
#include "gameplay/combat/damage.h"
#include "gameplay/combat/hitbox.h"
#include <core/memory.h>
#include <ecs/component_ids.h>
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>
#include <core/logger.h>

// Global damage event queue
static struct {
  DamageEvent *events;
  u32 count;
  u32 capacity;
  bool initialized;
} g_damage_queue = {0};

// ============================================================================
// DAMAGE SYSTEM INITIALIZATION
// ============================================================================

void damage_system_init(u32 max_events_per_frame) {
  if (g_damage_queue.initialized)
    return;

  g_damage_queue.capacity = max_events_per_frame;
  g_damage_queue.events = (DamageEvent *)memory_alloc(
      sizeof(DamageEvent) * max_events_per_frame, __FILE__, __LINE__);
  g_damage_queue.count = 0;
  g_damage_queue.initialized = true;
}

void damage_system_shutdown(void) {
  if (!g_damage_queue.initialized)
    return;

  if (g_damage_queue.events) {
    memory_free(g_damage_queue.events);
  }
  memset(&g_damage_queue, 0, sizeof(g_damage_queue));
}

// ============================================================================
// DAMAGE EVENT CREATION
// ============================================================================

DamageEvent *damage_event_create(Entity source, Entity target, f32 amount,
                                 DamageType type) {
  if (!g_damage_queue.initialized) {
    damage_system_init(1000);
  }

  if (g_damage_queue.count >= g_damage_queue.capacity) {
    return NULL; // Queue full
  }

  DamageEvent *event = &g_damage_queue.events[g_damage_queue.count++];
  memset(event, 0, sizeof(DamageEvent));

  event->source = source;
  event->target = target;
  event->base_amount = amount;
  event->final_amount = amount;
  event->type = type;
  event->flags = DAMAGE_FLAG_NONE;
  event->knockback_force = 0.0f;
  event->timestamp = 0.0; // Should be set by caller

  return event;
}

void damage_event_emit(const DamageEvent *event) {
  if (!event || !g_damage_queue.initialized)
    return;

  if (g_damage_queue.count >= g_damage_queue.capacity) {
    return; // Queue full
  }

  g_damage_queue.events[g_damage_queue.count++] = *event;
}

void damage_event_emit_simple(Entity source, Entity target, f32 amount) {
  DamageEvent event = {0};
  event.source = source;
  event.target = target;
  event.base_amount = amount;
  event.final_amount = amount;
  event.type = DAMAGE_TYPE_PHYSICAL;
  event.flags = DAMAGE_FLAG_NONE;

  damage_event_emit(&event);
}

// ============================================================================
// DAMAGE CALCULATION
// ============================================================================

f32 damage_calculate_final(const DamageEvent *event,
                           const ResistanceComponent *resistance,
                           f32 *out_blocked_amount) {
  if (!event)
    return 0.0f;

  f32 damage = event->base_amount;
  f32 blocked = 0.0f;

  // Apply damage flags
  if (event->flags & DAMAGE_FLAG_CRITICAL) {
    damage *= 2.0f;
  }
  if (event->flags & DAMAGE_FLAG_BACKSTAB) {
    damage *= 1.5f;
  }
  if (event->flags & DAMAGE_FLAG_HEADSHOT) {
    damage *= 3.0f;
  }

  // Apply resistances if not ignored
  if (resistance && !(event->flags & DAMAGE_FLAG_IGNORE_ARMOR)) {
    // Check immunity
    if (resistance->immune_types[event->type]) {
      damage = 0.0f;
      blocked = event->base_amount;
    } else {
      // Apply resistance
      f32 resist = resistance->resistances[event->type];
      f32 resist_amount = damage * resist;
      damage -= resist_amount;
      blocked += resist_amount;

      // Apply armor for physical damage
      if (event->type == DAMAGE_TYPE_PHYSICAL) {
        f32 armor_reduction = damage_apply_armor(damage, resistance->armor);
        blocked += (damage - armor_reduction);
        damage = armor_reduction;
      }

      // Apply magic resistance for magic damage types
      if (event->type >= DAMAGE_TYPE_FIRE && event->type <= DAMAGE_TYPE_DARK) {
        f32 magic_resist = damage * resistance->magic_resistance;
        damage -= magic_resist;
        blocked += magic_resist;
      }
    }
  }

  if (out_blocked_amount) {
    *out_blocked_amount = blocked;
  }

  return fmaxf(0.0f, damage);
}

f32 damage_apply_critical(f32 base_damage, f32 multiplier) {
  return base_damage * multiplier;
}

f32 damage_apply_resistance(f32 damage, f32 resistance) {
  return damage * (1.0f - resistance);
}

f32 damage_apply_armor(f32 damage, f32 armor) {
  // Armor formula: damage * 100 / (100 + armor)
  return damage * 100.0f / (100.0f + armor);
}

// ============================================================================
// DAMAGE EVENT PROCESSING
// ============================================================================

void damage_system_process_events(World *world, f64 delta_time) {
  if (!g_damage_queue.initialized || !world)
    return;

  // Process all damage events
  for (u32 i = 0; i < g_damage_queue.count; i++) {
    DamageEvent *event = &g_damage_queue.events[i];

    // Get target health component
    HealthComponent *health = (HealthComponent *)ecs_get_component(
        world, event->target, HEALTH_COMPONENT_ID);

    if (!health)
      continue;

    // Get resistance component if exists
    ResistanceComponent *resistance = (ResistanceComponent *)ecs_get_component(
        world, event->target, RESISTANCE_COMPONENT_ID);

    // Calculate final damage
    f32 blocked = 0.0f;
    f32 final_damage = damage_calculate_final(event, resistance, &blocked);
    event->final_amount = final_damage;

    // Apply damage to health
    health->health -= final_damage;

    // Clamp health to 0
    if (health->health < 0.0f) {
      health->health = 0.0f;
      // Emit death event
      LOG_INFO("Entity %u died!", event->target);
      // In a full implementation, we would queue a gameplay event here
      // event_emit(EVENT_ENTITY_DEATH, event->target);
    }

    // Handle knockback
    if (event->knockback_force > 0.0f) {
        // TODO: Get Rigidbody component and apply impulse
        // RigidbodyComponent* rb = ecs_get_component(world, event->target, RIGIDBODY_COMPONENT_ID);
        // if (rb) { ... }
    }

    // Handle status effects (stun, DOT, etc.)
    // This would require a StatusEffectComponent
    // StatusEffectComponent* effects = ecs_get_component(world, event->target, STATUS_EFFECT_COMPONENT_ID);
    // if (effects) { ... }
  }
}

void damage_system_clear_events(void) {
  if (g_damage_queue.initialized) {
    g_damage_queue.count = 0;
  }
}

// ============================================================================
// DAMAGE TYPE HELPERS
// ============================================================================

const char *damage_type_to_string(DamageType type) {
  static const char *type_names[] = {"Physical", "Fire",      "Ice",
                                     "Poison",   "Lightning", "Magic",
                                     "Holy",     "Dark",      "True"};

  if (type >= 0 && type < DAMAGE_TYPE_COUNT) {
    return type_names[type];
  }
  return "Unknown";
}

bool damage_can_crit(DamageType type) {
  // True damage cannot crit
  return type != DAMAGE_TYPE_TRUE;
}

// ============================================================================
// COMPONENT HELPERS
// ============================================================================

DamageComponent damage_component_create(f32 base_damage, DamageType type) {
  DamageComponent comp = {0};
  comp.base_damage = base_damage;
  comp.damage_type = type;
  comp.damage_flags = DAMAGE_FLAG_NONE;
  comp.critical_chance = 0.05f; // 5% base crit
  comp.critical_multiplier = 2.0f;
  comp.armor_penetration = 0.0f;
  comp.knockback_force = 0.0f;
  comp.is_dot = false;
  return comp;
}

DamageComponent damage_component_create_melee(f32 damage) {
  DamageComponent comp = damage_component_create(damage, DAMAGE_TYPE_PHYSICAL);
  comp.knockback_force = 5.0f;
  comp.critical_chance = 0.1f; // 10% crit for melee
  return comp;
}

DamageComponent damage_component_create_ranged(f32 damage) {
  DamageComponent comp = damage_component_create(damage, DAMAGE_TYPE_PHYSICAL);
  comp.knockback_force = 2.0f;
  comp.critical_chance = 0.15f; // 15% crit for ranged
  return comp;
}

DamageComponent damage_component_create_magic(f32 damage, DamageType element) {
  DamageComponent comp = damage_component_create(damage, element);
  comp.knockback_force = 1.0f;
  comp.critical_chance = 0.05f;
  return comp;
}

ResistanceComponent resistance_component_create(void) {
  ResistanceComponent comp = {0};
  for (u32 i = 0; i < DAMAGE_TYPE_COUNT; i++) {
    comp.resistances[i] = 0.0f;
    comp.immune_types[i] = false;
  }
  comp.armor = 0.0f;
  comp.magic_resistance = 0.0f;
  return comp;
}

void resistance_component_set(ResistanceComponent *res, DamageType type,
                              f32 value) {
  if (res && type >= 0 && type < DAMAGE_TYPE_COUNT) {
    res->resistances[type] = value;
  }
}

void resistance_component_set_immune(ResistanceComponent *res,
                                     DamageType type) {
  if (res && type >= 0 && type < DAMAGE_TYPE_COUNT) {
    res->immune_types[type] = true;
  }
}
