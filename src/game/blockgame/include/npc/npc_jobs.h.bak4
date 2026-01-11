// Job assignment and housing for NPCs (Milestone 3).
#ifndef NPC_JOBS_H
#define NPC_JOBS_H

#include "../game_common.h"
#include "../npc/npc.h"
#include "../npc/npc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void npc_assign_job(NPCSystem *system, EntityID npc, NPCJob job);
void npc_assign_home(NPCSystem *system, EntityID npc, EntityID home_entity);
void npc_assign_workplace(NPCSystem *system, EntityID npc, EntityID workplace_entity);
EntityID npc_find_workplace_type(NPCSystem *system, EntityID npc, const char *workplace_type);
void npc_register_workplace(NPCJob job, EntityID workplace_entity);
void npc_unregister_workplace(EntityID workplace_entity);
void npc_jobs_update(NPCSystem *system, f32 delta_time);

#ifdef __cplusplus
}
#endif

#endif // NPC_JOBS_H
