#include "gameplay/combat/status_effects.h"
#include "core/logger.h"
#include <string.h>

#define MAX_ENTITIES 64
#define MAX_EFFECTS_PER_ENTITY 8

typedef struct {
  uint64_t entity_id;
  ActiveEffectInfo effects[MAX_EFFECTS_PER_ENTITY];
  uint32_t active_count;
} EntityStatus;

static struct {
  EntityStatus entities[MAX_ENTITIES];
  uint32_t entity_count;
} status_context;

void status_sys_init(void) {
  status_context.entity_count = 0;
  LOG_INFO("Status Effect System Initialized");
}

void status_sys_update(float delta_time) {
  for (uint32_t i = 0; i < status_context.entity_count; i++) {
    EntityStatus *es = &status_context.entities[i];
    for (int j = 0; j < (int)es->active_count; j++) {
      es->effects[j].duration_remaining -= delta_time;
      if (es->effects[j].duration_remaining <= 0) {
        // Remove effect (swap with last)
        es->effects[j] = es->effects[es->active_count - 1];
        es->active_count--;
        j--;
      }
    }
  }
}

static EntityStatus *get_or_create_entity(uint64_t entity_id) {
  for (uint32_t i = 0; i < status_context.entity_count; i++) {
    if (status_context.entities[i].entity_id == entity_id) {
      return &status_context.entities[i];
    }
  }

  if (status_context.entity_count < MAX_ENTITIES) {
    EntityStatus *es = &status_context.entities[status_context.entity_count++];
    es->entity_id = entity_id;
    es->active_count = 0;
    return es;
  }
  return NULL;
}

void status_sys_apply_effect(uint64_t entity_id, StatusEffectType type,
                             float duration, float magnitude) {
  EntityStatus *es = get_or_create_entity(entity_id);
  if (!es) {
    LOG_WARN("Status System: Max active entities reached, cannot apply effect "
             "to %llu",
             (unsigned long long)entity_id);
    return;
  }

  // Check if already exists (renew)
  for (uint32_t i = 0; i < es->active_count; i++) {
    if (es->effects[i].type == type) {
      es->effects[i].duration_remaining = duration;
      es->effects[i].magnitude = magnitude;
      LOG_INFO("Refreshed Effect %d on Entity %llu", type, (unsigned long long)entity_id);
      return;
    }
  }

  // Add new
  if (es->active_count < MAX_EFFECTS_PER_ENTITY) {
    ActiveEffectInfo *info = &es->effects[es->active_count++];
    info->type = type;
    info->duration_remaining = duration;
    info->magnitude = magnitude;
    LOG_INFO("Applied Effect %d to Entity %llu (Duration: %.1fs)", type,
             (unsigned long long)entity_id, duration);
  }
}

void status_sys_remove_effect(uint64_t entity_id, StatusEffectType type) {
  EntityStatus *es = get_or_create_entity(entity_id);
  if (!es)
    return;

  for (uint32_t i = 0; i < es->active_count; i++) {
    if (es->effects[i].type == type) {
      es->effects[i] = es->effects[es->active_count - 1];
      es->active_count--;
      LOG_INFO("Removed Effect %d from Entity %llu", type, (unsigned long long)entity_id);
      return;
    }
  }
}

void status_sys_clear_all_effects(uint64_t entity_id) {
  EntityStatus *es = get_or_create_entity(entity_id);
  if (es) {
    es->active_count = 0;
    LOG_INFO("Cleared all effects for Entity %llu", (unsigned long long)entity_id);
  }
}

uint32_t status_sys_get_active_effects(uint64_t entity_id,
                                       ActiveEffectInfo *out_effects,
                                       uint32_t max_count) {
  EntityStatus *es = get_or_create_entity(entity_id);
  if (!es)
    return 0;

  uint32_t count = es->active_count < max_count ? es->active_count : max_count;
  memcpy(out_effects, es->effects, count * sizeof(ActiveEffectInfo));
  return count;
}
