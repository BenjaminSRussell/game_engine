// Progression and tech unlock system.
#include <core/systems/progression.h>
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

PlayerProgression *progression_create(void) {
    PlayerProgression *prog = (PlayerProgression *)calloc(1, sizeof(PlayerProgression));
    if (prog) {
        prog->level = 0;
        prog->experience = 0;
        prog->experience_for_next_level = 100;
        prog->total_blocks_broken = 0;
        prog->total_blocks_placed = 0;
        prog->total_kills = 0;
        prog->total_deaths = 0;
        prog->total_miles_walked = 0;
        prog->total_blocks_mined = 0;
    }
    return prog;
}

void progression_free(PlayerProgression *prog) {
    if (prog) free(prog);
}

void progression_add_experience(PlayerProgression *prog, u32 xp) {
    if (!prog) return;
    
    prog->experience += xp;
    LOG_DEBUG("Player gained %u XP (total: %u/%u)", xp, prog->experience, prog->experience_for_next_level);
    
    while (prog->experience >= prog->experience_for_next_level) {
        progression_level_up(prog);
    }
}

void progression_level_up(PlayerProgression *prog) {
    if (!prog) return;
    
    prog->experience -= prog->experience_for_next_level;
    prog->level++;
    prog->experience_for_next_level = 100 + (prog->level * 50);
    
    LOG_INFO("Player leveled up to level %u!", prog->level);
}

u32 progression_get_level(PlayerProgression *prog) {
    return prog ? prog->level : 0;
}

f32 progression_get_experience_percent(PlayerProgression *prog) {
    if (!prog || prog->experience_for_next_level == 0) return 0.0f;
    return (f32)prog->experience / (f32)prog->experience_for_next_level;
}

AchievementSystem *achievement_system_create(u32 initial_capacity) {
    AchievementSystem *sys = (AchievementSystem *)malloc(sizeof(AchievementSystem));
    if (!sys) return NULL;
    
    sys->achievement_capacity = initial_capacity;
    sys->achievement_count = 0;
    sys->achievements = (Achievement *)calloc(initial_capacity, sizeof(Achievement));
    
    if (!sys->achievements) {
        free(sys);
        return NULL;
    }
    
    return sys;
}

void achievement_system_free(AchievementSystem *sys) {
    if (!sys) return;
    
    for (u32 i = 0; i < sys->achievement_count; i++) {
        if (sys->achievements[i].id) free((void *)sys->achievements[i].id);
        if (sys->achievements[i].name) free((void *)sys->achievements[i].name);
        if (sys->achievements[i].description) free((void *)sys->achievements[i].description);
    }
    
    free(sys->achievements);
    free(sys);
}

void achievement_system_add_achievement(AchievementSystem *sys, const char *id, const char *name, const char *description) {
    if (!sys || !id || sys->achievement_count >= sys->achievement_capacity) return;
    
    Achievement *ach = &sys->achievements[sys->achievement_count++];
    ach->id = (const char *)malloc(strlen(id) + 1);
    ach->name = (const char *)malloc(strlen(name) + 1);
    ach->description = (const char *)malloc(strlen(description) + 1);
    
    if (ach->id && ach->name && ach->description) {
        strcpy((char *)ach->id, id);
        strcpy((char *)ach->name, name);
        strcpy((char *)ach->description, description);
        ach->unlocked = false;
        ach->progress = 0;
        ach->max_progress = 1;
        
        LOG_DEBUG("Achievement added: %s", name);
    }
}

void achievement_system_unlock(AchievementSystem *sys, const char *id) {
    if (!sys || !id) return;
    
    for (u32 i = 0; i < sys->achievement_count; i++) {
        if (sys->achievements[i].id && strcmp(sys->achievements[i].id, id) == 0) {
            if (!sys->achievements[i].unlocked) {
                sys->achievements[i].unlocked = true;
                LOG_INFO("ACHIEVEMENT UNLOCKED: %s", sys->achievements[i].name);
            }
            return;
        }
    }
}

bool achievement_system_is_unlocked(AchievementSystem *sys, const char *id) {
    if (!sys || !id) return false;
    
    for (u32 i = 0; i < sys->achievement_count; i++) {
        if (sys->achievements[i].id && strcmp(sys->achievements[i].id, id) == 0) {
            return sys->achievements[i].unlocked;
        }
    }
    
    return false;
}

u32 achievement_system_get_count(AchievementSystem *sys) {
    return sys ? sys->achievement_count : 0;
}
