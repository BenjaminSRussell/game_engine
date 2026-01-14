#ifndef NPC_FACTION_SYSTEM_H
#define NPC_FACTION_SYSTEM_H

#include <core/types.h>

typedef enum FactionID {
    FACTION_NONE = 0,
    FACTION_PLAYER = 1,
    FACTION_VILLAGERS = 2,
    FACTION_GUARDS = 3,
    FACTION_BANDITS = 4,
    FACTION_MONSTERS = 5
} FactionID;

typedef enum {
    RELATION_HOSTILE = -1,
    RELATION_NEUTRAL = 0,
    RELATION_FRIENDLY = 1
} FactionRelation;

void faction_system_init(void);
int faction_get_relation(FactionID faction_a, FactionID faction_b);
void faction_set_relation(FactionID faction_a, FactionID faction_b, int relation);
void faction_modify_reputation(FactionID faction, float amount);

#endif
