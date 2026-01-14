#pragma once

#include <common.h>

// Forward declarations
typedef struct GuardData GuardData;

// API functions
bool guard_can_see_player(GuardData* data, float guard_pos[3], float guard_forward[3], float player_pos[3]);
void guard_update_suspicion(GuardData* data, float dt, bool saw_player_peripheral);
void guard_set_last_known_position(GuardData* data, float pos[3]);
void guard_investigate_last_position(GuardData* data, float guard_pos[3]);
void guard_trigger_alarm(GuardData* data, float alarm_pos[3]);
