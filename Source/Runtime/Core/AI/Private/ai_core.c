#include "../Public/unified_ai.h"
#include "ai_types.h"
#include <stdlib.h>
#include <string.h>
#include <unified_logger.h>
#include <unified_memory.h>

AISystem *ai_system_create(AIConfig config) {
  AISystem *system = UNIFIED_ALLOC(sizeof(AISystem));
  if (!system) {
    LOG_ERROR(LOG_CAT_AI, "Failed to allocate AI System");
    return NULL;
  }

  memset(system, 0, sizeof(AISystem));

  system->max_npcs = config.max_npcs > 0 ? config.max_npcs : MAX_NPCS;
  system->update_rate = config.update_rate > 0 ? config.update_rate : 20;

  system->npcs = UNIFIED_ALLOC(sizeof(AINPC) * system->max_npcs);
  if (!system->npcs) {
    LOG_ERROR(LOG_CAT_AI, "Failed to allocate NPC storage");
    UNIFIED_FREE(system);
    return NULL;
  }
  memset(system->npcs, 0, sizeof(AINPC) * system->max_npcs);

  pthread_mutex_init(&system->lock, NULL);

  LOG_INFO(LOG_CAT_AI, "AI System created with %u max NPCs", system->max_npcs);
  return system;
}

void ai_system_destroy(AISystem *system) {
  if (!system)
    return;

  pthread_mutex_lock(&system->lock);

  for (u32 i = 0; i < system->max_npcs; i++) {
    if (system->npcs[i].current_path) {
      UNIFIED_FREE(system->npcs[i].current_path);
    }
  }

  UNIFIED_FREE(system->npcs);
  pthread_mutex_unlock(&system->lock);
  pthread_mutex_destroy(&system->lock);

  UNIFIED_FREE(system);
  LOG_INFO(LOG_CAT_AI, "AI System destroyed");
}

void ai_system_update(AISystem *system, f32 delta_time) {
  if (!system)
    return;

  system->time_since_last_update += delta_time;
  f32 update_interval = 1.0f / system->update_rate;

  if (system->time_since_last_update < update_interval) {
    return;
  }

  pthread_mutex_lock(&system->lock);

  for (u32 i = 0; i < system->npc_count; i++) {
    AINPC *npc = &system->npcs[i];
    if (!npc->active)
      continue;

    // Update perception decay
    for (u32 p = 0; p < npc->perception_count; p++) {
      npc->perceptions[p].age += system->time_since_last_update;
    }

    // Simple state machine for now
    switch (npc->state) {
    case AI_STATE_IDLE:
      // Do nothing
      break;
    case AI_STATE_PATROL:
      // Move towards target if assigned
      if (npc->has_target) {
        Vec3 diff = vec3_sub(npc->target_position, npc->position);
        f32 dist = vec3_length(diff);
        if (dist > 0.1f) {
          Vec3 dir = vec3_normalize(diff);
          f32 speed = 5.0f; // placeholder speed
          npc->velocity = vec3_mul(dir, speed);
          npc->position =
              vec3_add(npc->position,
                       vec3_mul(npc->velocity, system->time_since_last_update));
        } else {
          npc->velocity = (Vec3){0};
          npc->has_target = false;
          npc->state = AI_STATE_IDLE;
        }
      }
      break;
    default:
      break;
    }
  }

  system->time_since_last_update = 0.0f;
  pthread_mutex_unlock(&system->lock);
}
