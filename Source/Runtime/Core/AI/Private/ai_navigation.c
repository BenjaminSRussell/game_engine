#include "../Public/unified_ai.h"
#include "ai_types.h"
#include <unified_logger.h>

void ai_npc_move_to(AISystem *system, NPCId id, Vec3 target) {
  if (!system || id == 0)
    return;

  pthread_mutex_lock(&system->lock);

  for (u32 i = 0; i < system->npc_count; i++) {
    if (system->npcs[i].id == id && system->npcs[i].active) {
      system->npcs[i].target_position = target;
      system->npcs[i].has_target = true;
      system->npcs[i].state = AI_STATE_PATROL; // Or MOVE
      LOG_INFO(LOG_CAT_AI, "NPC %u ordered to move to (%.1f, %.1f, %.1f)", id,
               target.x, target.y, target.z);
      break;
    }
  }

  pthread_mutex_unlock(&system->lock);
}

void ai_npc_stop(AISystem *system, NPCId id) {
  if (!system || id == 0)
    return;

  pthread_mutex_lock(&system->lock);
  for (u32 i = 0; i < system->npc_count; i++) {
    if (system->npcs[i].id == id && system->npcs[i].active) {
      system->npcs[i].has_target = false;
      system->npcs[i].velocity = (Vec3){0};
      system->npcs[i].state = AI_STATE_IDLE;
      break;
    }
  }
  pthread_mutex_unlock(&system->lock);
}
