// Update scheduler for NPCs (Milestone 5).
#include <npc/npc.h>
#include <stdlib.h>

#define NPC_SCHEDULER_ENTRIES 1024

typedef struct {
  EntityID entity;
  f32 next_update;
  u32 interval_ticks;
} SchedulerEntry;

static SchedulerEntry g_scheduler[NPC_SCHEDULER_ENTRIES];
static u32 g_scheduler_count = 0;
static u32 g_current_tick = 0;

void npc_scheduler_add(EntityID entity, u32 interval_ticks) {
  if (g_scheduler_count >= NPC_SCHEDULER_ENTRIES)
    return;
  SchedulerEntry *e = &g_scheduler[g_scheduler_count++];
  e->entity = entity;
  e->next_update = g_current_tick + interval_ticks;
  e->interval_ticks = interval_ticks;
}

bool npc_scheduler_should_update(EntityID entity) {
  for (u32 i = 0; i < g_scheduler_count; i++) {
    if (g_scheduler[i].entity == entity) {
      if (g_current_tick >= g_scheduler[i].next_update) {
        g_scheduler[i].next_update =
            g_current_tick + g_scheduler[i].interval_ticks;
        return true;
      }
      return false;
    }
  }
  return true; // Not scheduled, update every frame
}

void npc_scheduler_tick(void) { g_current_tick++; }
