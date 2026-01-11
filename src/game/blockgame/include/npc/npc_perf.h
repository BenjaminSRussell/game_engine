// Performance and tooling API for NPCs (Milestone 5).
#ifndef NPC_PERF_H
#define NPC_PERF_H

#include "../game_common.h"
#include "../npc/npc.h"
#include <math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

void npc_lod_update(NPCSystem *system, Vec3 player_pos);
bool npc_should_update(EntityID entity, f32 dt);

void npc_scheduler_add(EntityID entity, u32 interval_ticks);
bool npc_scheduler_should_update(EntityID entity);
void npc_scheduler_tick(void);

void npc_profile_begin(const char *name);
void npc_profile_end(const char *name);
void npc_profile_dump(void);

#ifdef __cplusplus
}
#endif

#endif // NPC_PERF_H
