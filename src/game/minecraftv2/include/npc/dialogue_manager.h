// Dialogue manager API for Milestone 2 scaffolding.
#ifndef DIALOGUE_MANAGER_H
#define DIALOGUE_MANAGER_H

#include "../game_common.h"
#include "../npc/npc.h"

#ifdef __cplusplus
extern "C" {
#endif

void dialogue_manager_update(NPCSystem *system, f32 delta_time);
void dialogue_start(NPCSystem *system, EntityID npc, EntityID player);
void dialogue_start_tree(NPCSystem *system, EntityID npc, EntityID player);

#ifdef __cplusplus
}
#endif

#endif // DIALOGUE_MANAGER_H
