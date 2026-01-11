#ifndef EXPERIENCE_TEST_H
#define EXPERIENCE_TEST_H

#include "../game_common.h"

typedef struct {
    EntityID entity;
    float experience;
    int level;
    int skill_points;
} ExperiencePlayerEntry;

bool exp_test_add_experience_to_player(EntityID entity, float amount);
ExperiencePlayerEntry* exp_test_get_player_entry(EntityID entity);

void exp_test_reset(void);

#endif
