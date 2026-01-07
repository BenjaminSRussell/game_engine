#ifndef NPC_SCHEDULING_H
#define NPC_SCHEDULING_H

#include <core/types.h>

typedef struct Schedule Schedule;

void npc_schedule_update(Schedule *schedule, float time_of_day);

#endif
