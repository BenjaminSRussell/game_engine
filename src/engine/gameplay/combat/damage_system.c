// damage_system.c - Implementation
#include <core/logger.h>
#include <gameplay/combat/damage.h>
#include <ecs/components/health.h>
#include <ecs/components/transform.h>
#include <ecs/component_ids.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef RESISTANCE_COMPONENT_ID
#define RESISTANCE_COMPONENT_ID 50 // Stub if not defined
#endif

static DamageEvent *g_damage_queue = NULL;
static u32 g_max_events = 0;
static u32 g_event_count = 0;

void damage_system_init(u32 max_events_per_frame) {
  g_max_events = max_events_per_frame > 0 ? max_events_per_frame : 1024;
  g_damage_queue = (DamageEvent *)malloc(sizeof(DamageEvent) * g_max_events);
  g_event_count = 0;

  LOGI("Damage system initialized with capacity: %u", g_max_events);
}

void damage_system_shutdown(void) {
  if (g_damage_queue) {
    free(g_damage_queue);
    g_damage_queue = NULL;
  }
  g_max_events = 0;
  g_event_count = 0;

  LOGI("Damage system shutdown");
}

DamageEvent *damage_event_create(Entity source, Entity target, f32 amount,
                                 DamageType type) {
  if (g_event_count >= g_max_events) {
    LOGW("Damage event queue full, skipping event");
    return NULL;
  }

  DamageEvent *event = &g_damage_queue[g_event_count++];
  memset(event, 0, sizeof(DamageEvent));

  event->source = source;
  event->target = target;
  event->base_amount = amount;
  event->final_amount = amount; // Initially same, modified during calculation
  event->type = type;
  event->timestamp = 0.0; // Placeholder for actual game time

  return event;
}

void damage_event_emit(const DamageEvent *event) {
  if (!event || g_event_count >= g_max_events)
    return;

  // If the event isn't already in the queue, we'd need to copy it.
  // But damage_event_create already puts it in the queue.
  // This function might be for events created externally.
  // For now, let's assume events are primarily created via create().
}

void damage_event_emit_simple(Entity source, Entity target, f32 amount) {
  damage_event_create(source, target, amount, DAMAGE_TYPE_PHYSICAL);
}

void damage_system_process_events(World *world, f64 delta_time) {
  if (!world || !g_damage_queue)
    return;

  for (u32 i = 0; i < g_event_count; i++) {
    DamageEvent *event = &g_damage_queue[i];

    // 1. Get health component for target entity
    HealthComponent *health = (HealthComponent *)ecs_get_component(
        world, event->target, HEALTH_COMPONENT_ID);

    if (!health) {
      LOGW("Target entity %u has no health component", event->target.id);
      continue;
    }

    // 2. Get resistance component (optional) for damage reduction
    ResistanceComponent *resistance = (ResistanceComponent *)ecs_get_component(
        world, event->target, RESISTANCE_COMPONENT_ID);

    // 3. Calculate final damage using resistance
    f32 blocked_amount = 0.0f;
    f32 final_damage = damage_calculate_final(event, resistance, &blocked_amount);

    // 4. Apply damage to health
    health->health -= final_damage;
    health->last_damage_time = (f32)delta_time;

    LOGD("Applied damage: entity %u -> %u, base=%.2f final=%.2f blocked=%.2f, type %d",
              event->source.id, event->target.id, event->base_amount,
              final_damage, blocked_amount, event->type);

    // 5. Check for death if health <= 0
    if (health->health <= 0.0f) {
      health->health = 0.0f;
      health->is_alive = false;

      // Get transform for death position/logging
      TransformComponent *transform = (TransformComponent *)ecs_get_component(
          world, event->target, TRANSFORM_COMPONENT_ID);

      Vec3 death_pos = transform ? transform->position : vec3(0, 0, 0);

      LOGI("Entity %u died at (%.1f, %.1f, %.1f) from damage type %d by entity %u",
               event->target.id, death_pos.x, death_pos.y, death_pos.z,
               event->type, event->source.id);

      // Destroy the entity from the world
      ecs_destroy_entity(world, event->target);
    }
  }

  damage_system_clear_events();
}

void damage_system_clear_events(void) { g_event_count = 0; }

f32 damage_calculate_final(const DamageEvent *event,
                           const ResistanceComponent *resistance,
                           f32 *out_blocked_amount) {
  if (!event)
    return 0.0f;

  f32 final_damage = event->base_amount;
  f32 blocked = 0.0f;

  if (resistance) {
    // Apply type-specific resistance
    if (event->type < DAMAGE_TYPE_COUNT) {
      f32 res = resistance->resistances[event->type];
      f32 reduced = final_damage * res;
      final_damage -= reduced;
      blocked += reduced;
    }

    // Apply general armor for physical
    if (event->type == DAMAGE_TYPE_PHYSICAL) {
      f32 reduced = resistance->armor; // Simple flat reduction for now
      if (reduced > final_damage)
        reduced = final_damage;
      final_damage -= reduced;
      blocked += reduced;
    }
  }

  if (out_blocked_amount)
    *out_blocked_amount = blocked;
  return fmaxf(0.0f, final_damage);
}

const char *damage_type_to_string(DamageType type) {
  switch (type) {
  case DAMAGE_TYPE_PHYSICAL:
    return "Physical";
  case DAMAGE_TYPE_FIRE:
    return "Fire";
  case DAMAGE_TYPE_ICE:
    return "Ice";
  case DAMAGE_TYPE_POISON:
    return "Poison";
  case DAMAGE_TYPE_LIGHTNING:
    return "Lightning";
  case DAMAGE_TYPE_MAGIC:
    return "Magic";
  case DAMAGE_TYPE_HOLY:
    return "Holy";
  case DAMAGE_TYPE_DARK:
    return "Dark";
  case DAMAGE_TYPE_TRUE:
    return "True";
  default:
    return "Unknown";
  }
}

bool damage_can_crit(DamageType type) {
  return type == DAMAGE_TYPE_PHYSICAL || type == DAMAGE_TYPE_MAGIC;
}
