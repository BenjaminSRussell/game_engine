#ifndef NPC_COMPANION_H
#define NPC_COMPANION_H

#include <core/types.h>

typedef struct CompanionData CompanionData;

typedef enum CompanionCommand {
    CMD_FOLLOW = 0,
    CMD_WAIT = 1,
    CMD_ATTACK = 2,
    CMD_DEFEND = 3,
} CompanionCommand;

void companion_update_follow(CompanionData *data, float player_pos[3], float self_pos[3], float dt);
void companion_assist_combat(CompanionData *data, uint64_t player_target);
void companion_give_command(CompanionData *data, CompanionCommand cmd);
void companion_try_banter(CompanionData *data, float current_time, const char *location_tag);

#endif
