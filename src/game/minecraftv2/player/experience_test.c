#include "player/experience_test.h>
#include <stdlib.h>
#include <string.h>

static ExperiencePlayerEntry *g_entries = NULL;
static size_t g_count = 0;

static float experience_required_for_level(int level) {
    if (level <= 0) return 0.0f;
    if (level <= 16) {
        return level * level + 6 * level;
    } else if (level <= 31) {
        return 2.5f * level * level - 40.5f * level + 360.0f;
    } else {
        return 4.5f * level * level - 162.5f * level + 2220.0f;
    }
}

static int level_for_experience(float experience) {
    if (experience < 0.0f) return 0;
    int level = 0;
    float total_exp = 0.0f;
    while (total_exp <= experience) {
        level++;
        total_exp += experience_required_for_level(level);
        if (level > 1000) break;
    }
    return level - 1;
}

static ExperiencePlayerEntry *get_or_create(EntityID e) {
    for (size_t i = 0; i < g_count; i++) if (g_entries[i].entity == e) return &g_entries[i];
    ExperiencePlayerEntry *tmp = (ExperiencePlayerEntry*)realloc(g_entries, (g_count + 1) * sizeof(ExperiencePlayerEntry));
    if (!tmp) return NULL;
    g_entries = tmp;
    g_entries[g_count].entity = e;
    g_entries[g_count].experience = 0.0f;
    g_entries[g_count].level = 0;
    g_entries[g_count].skill_points = 0;
    g_count++;
    return &g_entries[g_count - 1];
}

bool exp_test_add_experience_to_player(EntityID entity, float amount) {
    if (amount <= 0.0f) return false;
    ExperiencePlayerEntry *e = get_or_create(entity);
    if (!e) return false;
    e->experience += amount;
    int new_level = level_for_experience(e->experience);
    if (new_level > e->level) {
        int gained = new_level - e->level;
        e->skill_points += gained;
        e->level = new_level;
    }
    return true;
}

ExperiencePlayerEntry* exp_test_get_player_entry(EntityID entity) {
    for (size_t i = 0; i < g_count; i++) if (g_entries[i].entity == entity) return &g_entries[i];
    return NULL;
}

void exp_test_reset(void) {
    free(g_entries);
    g_entries = NULL;
    g_count = 0;
}
