#include "../Public/unified_ai.h"
#include "ai_types.h"
#include <stdlib.h>
#include <string.h>
#include <unified_logger.h>

NPCId ai_npc_create(AISystem *system, Vec3 position) {
  if (!system)
    return 0;

  pthread_mutex_lock(&system->lock);

  if (system->npc_count >= system->max_npcs) {
    LOG_WARN(LOG_CAT_AI, "Cannot create NPC: Max limit reached");
    pthread_mutex_unlock(&system->lock);
    return 0;
  }

  NPCId id = system->npc_count + 1; // 1-based ID
  AINPC *npc = &system->npcs[system->npc_count];

  memset(npc, 0, sizeof(AINPC));
  npc->id = id;
  npc->active = true;
  npc->position = position;
  npc->state = AI_STATE_IDLE;
  npc->health = 100.0f;
  npc->morale = 1.0f;

  system->npc_count++;

  LOG_INFO(LOG_CAT_AI, "NPC %u created at (%.1f, %.1f, %.1f)", id, position.x,
           position.y, position.z);

  pthread_mutex_unlock(&system->lock);
  return id;
}

void ai_npc_destroy(AISystem *system, NPCId id) {
  if (!system || id == 0)
    return;

  pthread_mutex_lock(&system->lock);

  for (u32 i = 0; i < system->npc_count; i++) {
    if (system->npcs[i].id == id) {
      system->npcs[i].active = false;
      // In a real system we might compact the array or use a free list
      LOG_INFO(LOG_CAT_AI, "NPC %u destroyed", id);
      break;
    }
  }

  pthread_mutex_unlock(&system->lock);
}

bool ai_npc_get_info(AISystem *system, NPCId id, AINPCInfo *out_info) {
  if (!system || !out_info || id == 0)
    return false;

  // Lockless read optimization could be applied here if needed
  // For now, simple linear search
  for (u32 i = 0; i < system->npc_count; i++) {
    if (system->npcs[i].id == id && system->npcs[i].active) {
      out_info->id = id;
      out_info->state = system->npcs[i].state;
      out_info->position = system->npcs[i].position;
      out_info->health = system->npcs[i].health;
      out_info->morale = system->npcs[i].morale;
      return true;
    }
  }
  return false;
}

void ai_npc_set_state(AISystem *system, NPCId id, AIState state) {
  if (!system || id == 0)
    return;

  pthread_mutex_lock(&system->lock);
  for (u32 i = 0; i < system->npc_count; i++) {
    if (system->npcs[i].id == id && system->npcs[i].active) {
      system->npcs[i].state = state;
      break;
    }
  }
  pthread_mutex_unlock(&system->lock);
}
