#ifndef NPC_BOSS_H
#define NPC_BOSS_H

#include <core/types.h>

typedef struct BossData BossData;

void boss_update(BossData *data, float delta_time);
void boss_on_damage(BossData *data, float damage, uint64_t attacker_id);

#endif
