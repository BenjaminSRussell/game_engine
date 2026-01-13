#ifndef NPC_CIVILIAN_H
#define NPC_CIVILIAN_H

#include <core/types.h>

typedef struct CivilianData CivilianData;

void civilian_update_flee(CivilianData *data, float self_pos[3], float danger_pos[3], float dt);
void civilian_update_daily_routine(CivilianData *data, float time_of_day);
void civilian_interact_with_player(CivilianData *data, float player_reputation);

#endif
