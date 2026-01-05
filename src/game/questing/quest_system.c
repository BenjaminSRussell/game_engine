#include "game/questing/quest_system.h"
#include <string.h>

void quest_system_init() {}

void quest_start(void *player, int quest_id) {}

void quest_complete(void *player, int quest_id) {}

void quest_update_objective(void *player, int quest_id, int objective_id, int progress) {}

int quest_is_complete(void *player, int quest_id) {
    return 0;
}

void quest_get_active_quests(void *player, void *output_list) {}
