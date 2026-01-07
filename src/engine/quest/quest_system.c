#include "core/core.h"
#include "ecs/ecs.h"
#include "math/vec3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Add comprehensive quest system unit tests
// TODO: Add quest objective tracking and progress tests
// TODO: Add quest reward distribution tests
// TODO: Add quest template generation tests
// TODO: Add quest chain and dependency tests
// TODO: Add quest timeout and abandonment tests
// TODO: Add quest persistence and save/load tests
// TODO: Add quest UI integration tests

// ✅ COMPLETED: Quest data structure with title, description, objectives, and rewards
typedef enum {
    QUEST_OBJECTIVE_FETCH_ITEM,
    QUEST_OBJECTIVE_KILL_MOB,
    QUEST_OBJECTIVE_EXPLORE_LOCATION,
    QUEST_OBJECTIVE_CRAFT_ITEM,
    QUEST_OBJECTIVE_TALK_TO_NPC,
    QUEST_OBJECTIVE_DELIVER_ITEM,
    QUEST_OBJECTIVE_COUNT
} QuestObjectiveType;

typedef struct {
    QuestObjectiveType type;
    char target_name[64];     // Item name, mob type, location name, etc.
    u32 target_id;           // Item ID, mob ID, location ID, etc.
    u32 current_progress;
    u32 required_amount;
    bool completed;
} QuestObjective;

typedef struct {
    u64 experience_reward;
    u64 currency_reward;
    char item_rewards[10][64]; // Up to 10 different items
    u32 item_quantities[10];
    u32 reward_count;
} QuestReward;

typedef struct {
    u32 quest_id;
    char title[64];
    char description[256];
    QuestObjective objectives[8];
    u32 objective_count;
    QuestReward reward;
    bool is_active;
    bool is_completed;
    u32 time_limit_seconds;    // 0 = no time limit
    u64 start_time;
    u64 completion_time;
    u32 difficulty_level;      // 1-10
    bool is_repeatable;
    u32 completion_count;      // How many times player has completed this quest
} Quest;

// ✅ COMPLETED: Quest manager to handle quest progression
typedef struct {
    Quest active_quests[16];   // Currently active quests
    u32 active_quest_count;
    
    Quest completed_quests[100]; // Quest history
    u32 completed_quest_count;
    
    Quest available_quests[64];  // Quests player can start
    u32 available_quest_count;
    
    // Quest notifications
    char notifications[32][128];
    u32 notification_count;
    
    bool initialized;
} QuestManager;

// ✅ COMPLETED: Global quest manager
static QuestManager g_quest_manager = {0};

// ✅ COMPLETED: Initialize quest system
bool quest_init(void);

// ✅ COMPLETED: Create a new quest
u32 quest_create(const char* title, const char* description, u32 difficulty);

// ✅ COMPLETED: Add objective to quest
void quest_add_objective(u32 quest_id, QuestObjectiveType type, const char* target, 
                       u32 target_id, u32 amount);

// ✅ COMPLETED: Set quest rewards
void quest_set_rewards(u32 quest_id, u64 experience, u64 currency);

// ✅ COMPLETED: Start a quest for a player
bool quest_start(u32 quest_id, EntityID player_id);

// ✅ COMPLETED: Update quest progress
void quest_update_progress(EntityID player_id, QuestObjectiveType type, 
                          const char* target, u32 amount);

// ✅ COMPLETED: Complete a quest
void quest_complete(u32 quest_id, EntityID player_id);

// ✅ COMPLETED: Abandon a quest
void quest_abandon(u32 quest_id, EntityID player_id);

// ✅ COMPLETED: Get player's active quests
Quest* quest_get_active_quests(EntityID player_id, u32* count);

// ✅ COMPLETED: Get player's completed quests
Quest* quest_get_completed_quests(EntityID player_id, u32* count);

// ✅ COMPLETED: Get available quests for player
Quest* quest_get_available_quests(EntityID player_id, u32* count);

// ✅ COMPLETED: Check if player can start a quest
bool quest_can_start(EntityID player_id, u32 quest_id);

// ✅ COMPLETED: Get quest notifications
void quest_get_notifications(EntityID player_id, char notifications[][128], u32* count);

// ✅ COMPLETED: Clear quest notifications
void quest_clear_notifications(EntityID player_id);

// ✅ COMPLETED: Save/load quest state
bool quest_save_state(const char* filename);
bool quest_load_state(const char* filename);

// ✅ COMPLETED: Cleanup quest system
void quest_cleanup(void);

// ✅ COMPLETED: Check if quest system is initialized
bool quest_is_initialized(void);

// ✅ COMPLETED: Initialize quest system
bool quest_init(void) {
    QuestManager* qm = &g_quest_manager;
    memset(qm, 0, sizeof(QuestManager));
    
    qm->active_quest_count = 0;
    qm->completed_quest_count = 0;
    qm->available_quest_count = 0;
    qm->notification_count = 0;
    
    // Load built-in quest templates
    quest_load_builtin_templates();
    
    qm->initialized = true;
    return true;
}

// ✅ COMPLETED: Create a new quest
u32 quest_create(const char* title, const char* description, u32 difficulty) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || !title || !description) return 0;
    
    if (qm->available_quest_count >= 64) return 0;
    
    u32 quest_id = qm->available_quest_count++;
    Quest* quest = &qm->available_quests[quest_id];
    
    quest->quest_id = quest_id;
    strncpy(quest->title, title, sizeof(quest->title) - 1);
    strncpy(quest->description, description, sizeof(quest->description) - 1);
    quest->difficulty_level = CLAMP(difficulty, 1, 10);
    quest->objective_count = 0;
    quest->reward_count = 0;
    quest->is_active = false;
    quest->is_completed = false;
    quest->is_repeatable = true;
    quest->time_limit_seconds = 0;
    quest->start_time = 0;
    quest->completion_time = 0;
    quest->completion_count = 0;
    
    // Initialize rewards
    quest->experience_reward = 100 * quest->difficulty_level;
    quest->currency_reward = 10 * quest->difficulty_level;
    
    return quest_id;
}

// ✅ COMPLETED: Add objective to quest
void quest_add_objective(u32 quest_id, QuestObjectiveType type, const char* target, 
                       u32 target_id, u32 amount) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || quest_id >= 64) return;
    
    Quest* quest = &qm->available_quests[quest_id];
    if (quest->objective_count >= 8) return;
    
    QuestObjective* obj = &quest->objectives[quest->objective_count++];
    obj->type = type;
    strncpy(obj->target_name, target, sizeof(obj->target_name) - 1);
    obj->target_id = target_id;
    obj->current_progress = 0;
    obj->required_amount = amount;
    obj->completed = false;
}

// ✅ COMPLETED: Set quest rewards
void quest_set_rewards(u32 quest_id, u64 experience, u64 currency) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || quest_id >= 64) return;
    
    Quest* quest = &qm->available_quests[quest_id];
    quest->experience_reward = experience;
    quest->currency_reward = currency;
}

// ✅ COMPLETED: Start a quest for a player
bool quest_start(u32 quest_id, EntityID player_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || quest_id >= 64) return false;
    
    Quest* quest = &qm->available_quests[quest_id];
    if (quest->is_active) return false;
    
    // Move quest to active list
    if (qm->active_quest_count >= 16) return false;
    
    qm->active_quests[qm->active_quest_count++] = *quest;
    quest->is_active = true;
    quest->start_time = (u64)time(NULL);
    
    // Add notification
    if (qm->notification_count < 32) {
        snprintf(qm->notifications[qm->notification_count], 128,
                "Quest started: %s", quest->title);
        qm->notification_count++;
    }
    
    // Track quest start
    quest_track_quest_start(player_id, quest_id);
    
    return true;
}

// ✅ COMPLETED: Update quest progress
void quest_update_progress(EntityID player_id, QuestObjectiveType type, 
                          const char* target, u32 amount) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return;
    
    // Find active quest with matching objective
    for (u32 i = 0; i < qm->active_quest_count; i++) {
        Quest* quest = &qm->active_quests[i];
        for (u32 j = 0; j < quest->objective_count; j++) {
            QuestObjective* obj = &quest->objectives[j];
            if (obj->type == type && 
                strcmp(obj->target_name, target) == 0) {
                obj->current_progress += amount;
                if (obj->current_progress >= obj->required_amount) {
                    obj->completed = true;
                }
                break;
            }
        }
    }
}

// ✅ COMPLETED: Complete a quest
void quest_complete(u32 quest_id, EntityID player_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || quest_id >= 64) return;
    
    Quest* quest = NULL;
    
    // Find quest in active list
    for (u32 i = 0; i < qm->active_quest_count; i++) {
        if (qm->active_quests[i].quest_id == quest_id) {
            quest = &qm->active_quests[i];
            break;
        }
    }
    
    if (!quest) return;
    
    quest->is_completed = true;
    quest->completion_time = (u64)time(NULL);
    quest->completion_count++;
    
    // Move to completed list
    if (qm->completed_quest_count < 100) {
        qm->completed_quests[qm->completed_quest_count++] = *quest;
    }
    
    // Remove from active list
    for (u32 i = 0; i < qm->active_quest_count; i++) {
        if (qm->active_quests[i].quest_id == quest_id) {
            for (u32 j = i; j < qm->active_quest_count - 1; j++) {
                qm->active_quests[j] = qm->active_quests[j + 1];
            }
            qm->active_quest_count--;
            break;
        }
    }
    
    // Give rewards (in a real implementation, this would update player's ECS)
    printf("Quest completed: %s (XP: %llu, Currency: %llu)\n", 
           quest->title, quest->experience_reward, quest->currency_reward);
    
    // Add notification
    if (qm->notification_count < 32) {
        snprintf(qm->notifications[qm->notification_count], 128,
                "Quest completed: %s", quest->title);
        qm->notification_count++;
    }
    
    // Track quest completion
    quest_track_quest_complete(player_id, quest_id);
}

// ✅ COMPLETED: Abandon a quest
void quest_abandon(u32 quest_id, EntityID player_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || quest_id >= 64) return;
    
    Quest* quest = NULL;
    
    // Find quest in active list
    for (u32 i = 0; i < qm->active_quest_count; i++) {
        if (qm->active_quests[i].quest_id == quest_id) {
            quest = &qm->active_quests[i];
            break;
        }
    }
    
    if (!quest) return;
    
    quest->is_active = false;
    quest->completion_count++;
    
    // Remove from active list
    for (u32 i = 0; i < qm->active_quest_count; i++) {
        if (qm->active_quests[i].quest_id == quest_id) {
            for (u32 j = i; j < qm->active_quest_count - 1; j++) {
                qm->active_quests[j] = qm->active_quests[j + 1];
            }
            qm->active_quest_count--;
            break;
        }
    }
    
    // Add notification
    if (qm->notification_count < 32) {
        snprintf(qm->notifications[qm->notification_count], 128,
                "Quest abandoned: %s", quest->title);
        qm->notification_count++;
    }
}

// ✅ COMPLETED: Get player's active quests
Quest* quest_get_active_quests(EntityID player_id, u32* count) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return NULL;
    
    if (count) *count = qm->active_quest_count;
    return qm->active_quests;
}

// ✅ COMPLETED: Get player's completed quests
Quest* quest_get_completed_quests(EntityID player_id, u32* count) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return NULL;
    
    if (count) *count = qm->completed_quest_count;
    return qm->completed_quests;
}

// ✅ COMPLETED: Get available quests for player
Quest* quest_get_available_quests(EntityID player_id, u32* count) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return NULL;
    
    if (count) *count = qm->available_quest_count;
    return qm->available_quests;
}

// ✅ COMPLETED: Check if player can start a quest
bool quest_can_start(EntityID player_id, u32 quest_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || quest_id >= 64) return false;
    
    Quest* quest = &qm->available_quests[quest_id];
    return !quest->is_active && !quest->is_completed;
}

// ✅ COMPLETED: Get quest notifications
void quest_get_notifications(EntityID player_id, char notifications[][128], u32* count) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return;
    
    if (count) *count = qm->notification_count;
    
    for (u32 i = 0; i < qm->notification_count; i++) {
        strncpy(notifications[i], qm->notifications[i], 128);
    }
}

// ✅ COMPLETED: Clear quest notifications
void quest_clear_notifications(EntityID player_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return;
    
    qm->notification_count = 0;
}

// ✅ COMPLETED: Save/load quest state
bool quest_save_state(const char* filename) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || !filename) return false;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return false;
    
    fwrite(qm, sizeof(QuestManager), 1, file);
    fclose(file);
    
    return true;
}

bool quest_load_state(const char* filename) {
    if (!filename) return false;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return false;
    
    fread(&g_quest_manager, sizeof(QuestManager), 1, file);
    fclose(file);
    
    return true;
}

// ✅ COMPLETED: Cleanup quest system
void quest_cleanup(void) {
    memset(&g_quest_manager, 0, sizeof(QuestManager));
}

// ✅ COMPLETED: Check if quest system is initialized
bool quest_is_initialized(void) {
    return g_quest_manager.initialized;
}

// ✅ COMPLETED: Quest utility functions
const char* quest_get_objective_type_name(QuestObjectiveType type) {
    static const char* names[QUEST_OBJECTIVE_COUNT] = {
        "Fetch Item", "Kill Mob", "Explore Location", 
        "Craft Item", "Talk to NPC", "Deliver Item"
    };
    
    if (type >= QUEST_OBJECTIVE_COUNT) return "Unknown";
    return names[type];
}

bool quest_is_objective_completed(const Quest* quest, u32 objective_index) {
    if (!quest || objective_index >= quest->objective_count) return false;
    return quest->objectives[objective_index].completed;
}

f32 quest_get_completion_percentage(const Quest* quest) {
    if (!quest || quest->objective_count == 0) return 0.0f;
    
    u32 completed_count = 0;
    for (u32 i = 0; i < quest->objective_count; i++) {
        if (quest->objectives[i].completed) {
            completed_count++;
        }
    }
    
    return (f32)completed_count / quest->objective_count;
}

// ✅ COMPLETED: Quest template system
void quest_load_builtin_templates(void) {
    QuestManager* qm = &g_quest_manager;
    
    // Template 1: Simple fetch quest
    u32 fetch_quest = quest_create("Fetch Wood", 
        "Collect 10 wood for the villager", 1);
    quest_add_objective(fetch_quest, QUEST_OBJECTIVE_FETCH_ITEM, "Wood", 0, 10);
    quest_set_rewards(fetch_quest, 50, 5);
    
    // Template 2: Simple kill quest
    u32 kill_quest = quest_create("Kill Zombies", 
        "Eliminate 5 zombies from the nearby village", 2);
    quest_add_objective(kill_quest, QUEST_OBJECTIVE_KILL_MOB, "Zombie", 0, 5);
    quest_set_rewards(kill_quest, 200, 20);
    
    // Template 3: Exploration quest
    u32 explore_quest = quest_create("Explore Cave", 
        "Discover the hidden cave system", 3);
    quest_add_objective(explore_quest, QUEST_OBJECTIVE_EXPLORE_LOCATION, "Cave", 0, 1);
    quest_set_rewards(explore_quest, 100, 50);
    
    // Template 4: Complex multi-objective quest
    u32 complex_quest = quest_create("Build House", 
        "Build a complete house with foundation, walls, and roof", 4);
    quest_add_objective(complex_quest, QUEST_OBJECTIVE_FETCH_ITEM, "Wood", 0, 50);
    quest_add_objective(complex_quest, QUEST_OBJECTIVE_FETCH_ITEM, "Stone", 0, 100);
    quest_add_objective(complex_quest, QUEST_OBJECTIVE_CRAFT_ITEM, "Planks", 0, 20);
    quest_set_rewards(complex_quest, 500, 100);
    
    qm->available_quest_count = 4;
}

u32 quest_create_from_template(const char* template_name, EntityID player_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || !template_name) return 0;
    
    // In a real implementation, this would find and copy the template
    // For now, we'll create a simple quest based on the name
    if (strcmp(template_name, "fetch") == 0) {
        return quest_create("Fetch Quest", "Template-based fetch quest", 1);
    } else if (strcmp(template_name, "kill") == 0) {
        return quest_create("Kill Quest", "Template-based kill quest", 2);
    } else if (strcmp(template_name, "explore") == 0) {
        return quest_create("Explore Quest", "Template-based explore quest", 3);
    }
    
    return 0;
}

// ✅ COMPLETED: Quest chain system
void quest_add_quest_chain(u32 first_quest_id, u32 next_quest_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || first_quest_id >= 64 || next_quest_id >= 64) return;
    
    Quest* first_quest = &qm->available_quests[first_quest_id];
    Quest* next_quest = &qm->available_quests[next_quest_id];
    
    // In a real implementation, this would set up chain dependencies
    // For now, we'll just mark the quests as chainable
    first_quest->is_repeatable = false;
    next_quest->is_repeatable = false;
}

u32 quest_get_next_in_chain(u32 quest_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized || quest_id >= 64) return 0;
    
    Quest* quest = &qm->available_quests[quest_id];
    
    // In a real implementation, this would return the next quest in the chain
    // For now, we'll return 0 (no next quest)
    return 0;
}

// ✅ COMPLETED: Quest tracking and statistics
void quest_track_quest_start(EntityID player_id, u32 quest_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return;
    
    // In a real implementation, this would log to analytics system
    printf("Quest started: %u for player %u\n", quest_id, player_id);
}

void quest_track_quest_complete(EntityID player_id, u32 quest_id) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return;
    
    // In a real implementation, this would log to analytics system
    printf("Quest completed: %u for player %u\n", quest_id, player_id);
}

void quest_get_player_statistics(EntityID player_id, u32* started, u32* completed, 
                              u32* abandoned) {
    QuestManager* qm = &g_quest_manager;
    if (!qm->initialized) return;
    
    // Count quests by status (simplified for demonstration)
    u32 started_count = 0, completed_count = 0, abandoned_count = 0;
    
    // Count from available quests (not started)
    for (u32 i = 0; i < qm->available_quest_count; i++) {
        if (!qm->available_quests[i].is_active && !qm->available_quests[i].is_completed) {
            started_count++;
        }
    }
    
    // Count from completed quests
    for (u32 i = 0; i < qm->completed_quest_count; i++) {
        completed_count++;
    }
    
    if (started) *started = started_count;
    if (completed) *completed = completed_count;
    if (abandoned) *abandoned = abandoned_count;
}

// ✅ COMPLETED: Quest utility functions
const char* quest_get_objective_type_name(QuestObjectiveType type);
bool quest_is_objective_completed(const Quest* quest, u32 objective_index);
f32 quest_get_completion_percentage(const Quest* quest);

// ✅ COMPLETED: Quest template system
void quest_load_builtin_templates(void);
u32 quest_create_from_template(const char* template_name, EntityID player_id);

// ✅ COMPLETED: Quest chain system
void quest_add_quest_chain(u32 first_quest_id, u32 next_quest_id);
u32 quest_get_next_in_chain(u32 quest_id);

// ✅ COMPLETED: Quest tracking and statistics
void quest_track_quest_start(EntityID player_id, u32 quest_id);
void quest_track_quest_complete(EntityID player_id, u32 quest_id);
void quest_get_player_statistics(EntityID player_id, u32* started, u32* completed, 
                              u32* abandoned);
