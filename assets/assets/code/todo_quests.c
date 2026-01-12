#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Quest objective types
typedef enum {
    QUEST_OBJECTIVE_KILL,
    QUEST_OBJECTIVE_COLLECT,
    QUEST_OBJECTIVE_EXPLORE,
    QUEST_OBJECTIVE_CRAFT,
    QUEST_OBJECTIVE_TALK,
    QUEST_OBJECTIVE_DELIVER
} quest_objective_type_t;

// Quest reward types
typedef enum {
    QUEST_REWARD_EXPERIENCE,
    QUEST_REWARD_ITEM,
    QUEST_REWARD_CURRENCY,
    QUEST_REWARD_REPUTATION
} quest_reward_type_t;

// Quest objective structure
typedef struct {
    quest_objective_type_t type;
    char target[256];
    int required_amount;
    int current_amount;
    bool completed;
    char description[512];
} quest_objective_t;

// Quest reward structure
typedef struct {
    quest_reward_type_t type;
    int amount;
    char item_id[128];
    char description[256];
} quest_reward_t;

// Quest data structure
typedef struct {
    int id;
    char title[256];
    char description[1024];
    quest_objective_t* objectives;
    int objective_count;
    quest_reward_t* rewards;
    int reward_count;
    int level_requirement;
    bool is_repeatable;
    int time_limit_seconds;
    bool auto_accept;
    char prerequisite_quest_ids[32];
    int prerequisite_count;
} quest_t;

// Player quest status
typedef enum {
    QUEST_STATUS unavailable,
    QUEST_STATUS available,
    QUEST_STATUS active,
    QUEST_STATUS completed,
    QUEST_STATUS failed,
    QUEST_STATUS abandoned
} quest_status_t;

// Player quest data
typedef struct {
    int quest_id;
    quest_status_t status;
    time_t start_time;
    time_t completion_time;
    int* objective_progress;
    bool is_tracked;
} player_quest_t;

// Quest manager structure
typedef struct {
    quest_t* quests;
    int quest_count;
    int quest_capacity;
    player_quest_t* player_quests;
    int player_quest_count;
    int player_quest_capacity;
    int next_quest_id;
} quest_manager_t;

// Quest manager functions
quest_manager_t* quest_manager_create(void);
void quest_manager_destroy(quest_manager_t* manager);
int quest_manager_add_quest(quest_manager_t* manager, const quest_t* quest);
quest_t* quest_manager_get_quest(quest_manager_t* manager, int quest_id);
bool quest_manager_start_quest(quest_manager_t* manager, int player_id, int quest_id);
bool quest_manager_complete_quest(quest_manager_t* manager, int player_id, int quest_id);
bool quest_manager_abandon_quest(quest_manager_t* manager, int player_id, int quest_id);
bool quest_manager_update_objective(quest_manager_t* manager, int player_id, int quest_id, int objective_index, int progress);
quest_t* quest_manager_get_available_quests(quest_manager_t* manager, int player_id, int* count);
player_quest_t* quest_manager_get_active_quests(quest_manager_t* manager, int player_id, int* count);
bool quest_manager_is_quest_available(quest_manager_t* manager, int player_id, int quest_id);
void quest_manager_save_progress(quest_manager_t* manager, int player_id, const char* filename);
void quest_manager_load_progress(quest_manager_t* manager, int player_id, const char* filename);

// Utility functions
const char* quest_objective_type_to_string(quest_objective_type_t type);
const char* quest_reward_type_to_string(quest_reward_type_t type);
const char* quest_status_to_string(quest_status_t status);
float quest_get_completion_percentage(const player_quest_t* player_quest, const quest_t* quest);
bool quest_is_completed(const player_quest_t* player_quest, const quest_t* quest);

// Quest event callbacks
typedef void (*quest_started_callback_t)(int player_id, int quest_id);
typedef void (*quest_completed_callback_t)(int player_id, int quest_id);
typedef void (*quest_objective_updated_callback_t)(int player_id, int quest_id, int objective_index, int progress);
typedef void (*quest_failed_callback_t)(int player_id, int quest_id);

// Set quest event callbacks
void quest_manager_set_callbacks(quest_manager_t* manager,
                                quest_started_callback_t started,
                                quest_completed_callback_t completed,
                                quest_objective_updated_callback_t objective_updated,
                                quest_failed_callback_t failed);

// Quest validation
bool quest_validate(const quest_t* quest);
bool quest_has_valid_prerequisites(const quest_manager_t* manager, int player_id, const quest_t* quest);

// Quest UI helpers
void quest_format_objective_progress(char* buffer, size_t buffer_size, const quest_objective_t* objective);
void quest_format_reward_description(char* buffer, size_t buffer_size, const quest_reward_t* reward);
int quest_get_total_objective_count(const quest_t* quest);
int quest_get_completed_objective_count(const quest_t* quest, const int* progress);

// Quest debugging
void quest_debug_print(const quest_t* quest);
void quest_manager_debug_print(const quest_manager_t* manager);
void player_quest_debug_print(const player_quest_t* player_quest, const quest_t* quest);

// Quest manager implementation
quest_manager_t* quest_manager_create(void) {
    quest_manager_t* manager = malloc(sizeof(quest_manager_t));
    if (!manager) return NULL;
    
    manager->quests = malloc(sizeof(quest_t) * 100);
    manager->quest_count = 0;
    manager->quest_capacity = 100;
    
    manager->player_quests = malloc(sizeof(player_quest_t) * 500);
    manager->player_quest_count = 0;
    manager->player_quest_capacity = 500;
    
    manager->next_quest_id = 1;
    
    return manager;
}

void quest_manager_destroy(quest_manager_t* manager) {
    if (!manager) return;
    
    for (int i = 0; i < manager->quest_count; i++) {
        free(manager->quests[i].objectives);
        free(manager->quests[i].rewards);
    }
    
    for (int i = 0; i < manager->player_quest_count; i++) {
        free(manager->player_quests[i].objective_progress);
    }
    
    free(manager->quests);
    free(manager->player_quests);
    free(manager);
}

int quest_manager_add_quest(quest_manager_t* manager, const quest_t* quest) {
    if (!manager || !quest || manager->quest_count >= manager->quest_capacity) {
        return -1;
    }
    
    quest_t new_quest = *quest;
    new_quest.id = manager->next_quest_id++;
    
    // Deep copy objectives
    if (quest->objectives && quest->objective_count > 0) {
        new_quest.objectives = malloc(sizeof(quest_objective_t) * quest->objective_count);
        memcpy(new_quest.objectives, quest->objectives, sizeof(quest_objective_t) * quest->objective_count);
    }
    
    // Deep copy rewards
    if (quest->rewards && quest->reward_count > 0) {
        new_quest.rewards = malloc(sizeof(quest_reward_t) * quest->reward_count);
        memcpy(new_quest.rewards, quest->rewards, sizeof(quest_reward_t) * quest->reward_count);
    }
    
    manager->quests[manager->quest_count++] = new_quest;
    return new_quest.id;
}

quest_t* quest_manager_get_quest(quest_manager_t* manager, int quest_id) {
    if (!manager) return NULL;
    
    for (int i = 0; i < manager->quest_count; i++) {
        if (manager->quests[i].id == quest_id) {
            return &manager->quests[i];
        }
    }
    return NULL;
}

bool quest_manager_start_quest(quest_manager_t* manager, int player_id, int quest_id) {
    if (!manager) return false;
    
    quest_t* quest = quest_manager_get_quest(manager, quest_id);
    if (!quest || !quest_manager_is_quest_available(manager, player_id, quest_id)) {
        return false;
    }
    
    // Check if player already has this quest
    for (int i = 0; i < manager->player_quest_count; i++) {
        if (manager->player_quests[i].quest_id == quest_id) {
            return false; // Already has quest
        }
    }
    
    if (manager->player_quest_count >= manager->player_quest_capacity) {
        return false;
    }
    
    player_quest_t player_quest = {0};
    player_quest.quest_id = quest_id;
    player_quest.status = QUEST_STATUS_active;
    player_quest.start_time = time(NULL);
    player_quest.is_tracked = true;
    
    // Initialize objective progress
    player_quest.objective_progress = malloc(sizeof(int) * quest->objective_count);
    for (int i = 0; i < quest->objective_count; i++) {
        player_quest.objective_progress[i] = 0;
    }
    
    manager->player_quests[manager->player_quest_count++] = player_quest;
    return true;
}

bool quest_manager_complete_quest(quest_manager_t* manager, int player_id, int quest_id) {
    if (!manager) return false;
    
    quest_t* quest = quest_manager_get_quest(manager, quest_id);
    if (!quest) return false;
    
    for (int i = 0; i < manager->player_quest_count; i++) {
        player_quest_t* player_quest = &manager->player_quests[i];
        if (player_quest->quest_id == quest_id && player_quest->status == QUEST_STATUS_active) {
            if (quest_is_completed(player_quest, quest)) {
                player_quest->status = QUEST_STATUS_completed;
                player_quest->completion_time = time(NULL);
                return true;
            }
        }
    }
    return false;
}

bool quest_manager_abandon_quest(quest_manager_t* manager, int player_id, int quest_id) {
    if (!manager) return false;
    
    for (int i = 0; i < manager->player_quest_count; i++) {
        if (manager->player_quests[i].quest_id == quest_id) {
            manager->player_quests[i].status = QUEST_STATUS_abandoned;
            return true;
        }
    }
    return false;
}

bool quest_manager_update_objective(quest_manager_t* manager, int player_id, int quest_id, int objective_index, int progress) {
    if (!manager) return false;
    
    quest_t* quest = quest_manager_get_quest(manager, quest_id);
    if (!quest || objective_index < 0 || objective_index >= quest->objective_count) {
        return false;
    }
    
    for (int i = 0; i < manager->player_quest_count; i++) {
        player_quest_t* player_quest = &manager->player_quests[i];
        if (player_quest->quest_id == quest_id && player_quest->status == QUEST_STATUS_active) {
            player_quest->objective_progress[objective_index] = progress;
            
            // Check if objective is completed
            if (progress >= quest->objectives[objective_index].required_amount) {
                quest->objectives[objective_index].completed = true;
            }
            
            return true;
        }
    }
    return false;
}

bool quest_manager_is_quest_available(quest_manager_t* manager, int player_id, int quest_id) {
    if (!manager) return false;
    
    quest_t* quest = quest_manager_get_quest(manager, quest_id);
    if (!quest) return false;
    
    // Check level requirement (simplified)
    // Check prerequisites
    return quest_has_valid_prerequisites(manager, player_id, quest);
}

bool quest_has_valid_prerequisites(const quest_manager_t* manager, int player_id, const quest_t* quest) {
    for (int i = 0; i < quest->prerequisite_count; i++) {
        int prereq_id = quest->prerequisite_quest_ids[i];
        bool found_completed = false;
        
        for (int j = 0; j < manager->player_quest_count; j++) {
            if (manager->player_quests[j].quest_id == prereq_id && 
                manager->player_quests[j].status == QUEST_STATUS_completed) {
                found_completed = true;
                break;
            }
        }
        
        if (!found_completed) {
            return false;
        }
    }
    return true;
}

bool quest_is_completed(const player_quest_t* player_quest, const quest_t* quest) {
    if (!player_quest || !quest) return false;
    
    for (int i = 0; i < quest->objective_count; i++) {
        if (player_quest->objective_progress[i] < quest->objectives[i].required_amount) {
            return false;
        }
    }
    return true;
}

float quest_get_completion_percentage(const player_quest_t* player_quest, const quest_t* quest) {
    if (!player_quest || !quest || quest->objective_count == 0) return 0.0f;
    
    int total_required = 0;
    int total_current = 0;
    
    for (int i = 0; i < quest->objective_count; i++) {
        total_required += quest->objectives[i].required_amount;
        total_current += player_quest->objective_progress[i];
    }
    
    return (float)total_current / (float)total_required;
}

const char* quest_status_to_string(quest_status_t status) {
    switch (status) {
        case QUEST_STATUS_unavailable: return "Unavailable";
        case QUEST_STATUS_available: return "Available";
        case QUEST_STATUS_active: return "Active";
        case QUEST_STATUS_completed: return "Completed";
        case QUEST_STATUS_failed: return "Failed";
        case QUEST_STATUS_abandoned: return "Abandoned";
        default: return "Unknown";
    }
}

void quest_debug_print(const quest_t* quest) {
    if (!quest) return;
    printf("Quest ID: %d\n", quest->id);
    printf("Title: %s\n", quest->title);
    printf("Description: %s\n", quest->description);
    printf("Objectives (%d):\n", quest->objective_count);
    for (int i = 0; i < quest->objective_count; i++) {
        printf("  - %s: %d/%d\n", quest->objectives[i].description, 
               quest->objectives[i].current_amount, quest->objectives[i].required_amount);
    }
    printf("Rewards (%d):\n", quest->reward_count);
    for (int i = 0; i < quest->reward_count; i++) {
        printf("  - %s\n", quest->rewards[i].description);
    }
}

// TODO: High - Create a UI for displaying and tracking quests, including a quest log and on-screen notifications.
// TODO: Medium - Implement a variety of quest objectives, such as fetching items, killing mobs, and exploring specific locations.
// TODO: Medium - Create a system for rewarding players for completing quests, such as giving them items, experience, or currency.
