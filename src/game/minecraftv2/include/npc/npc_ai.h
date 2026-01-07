#ifndef MINECRAFT_NPC_AI_H
#define MINECRAFT_NPC_AI_H

#include "npc_types.h"
#include <common.h>

void npc_ai_update(struct NPCSystem *system, EntityID entity, NPCComponent *npc,
                   f32 delta_time);

#endif // MINECRAFT_NPC_AI_H
