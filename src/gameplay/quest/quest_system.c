#include "quest_system.h"
#include "core/common/memory/allocator.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_QUESTS 2048
#define MAX_OBJECTIVES_PER_QUEST 16
#define MAX_REWARDS_PER_QUEST 8
#define MAX_PLAYERS 1024
#define QUEST_NAME_LENGTH 64
#define QUEST_DESCRIPTION_LENGTH 256
#define OBJECTIVE_TEXT_LENGTH 128

typedef enum quest_status {
    QUEST_STATUS_AVAILABLE = 0,
    QUEST_STATUS_ACTIVE,
    QUEST_STATUS_COMPLETED,
    QUEST_STATUS_FAILED,
    QUEST_STATUS_LOCKED
} quest_status_t;

typedef enum objective_type {
    OBJECTIVE_KILL = 0,
    OBJECTIVE_COLLECT,
    OBJECTIVE_DELIVER,
    OBJECTIVE_TALK,
    OBJECTIVE_EXPLORE,
    OBJECTIVE_CRAFT,
    OBJECTIVE_ESCORT,
    OBJECTIVE_TIMED
} objective_type_t;

typedef struct quest_objective {
    uint32_t objective_id;
    objective_type_t type;
    char description[OBJECTIVE_TEXT_LENGTH];
    
    uint32_t target_id;      // NPC to kill, item to collect, etc.
    uint32_t target_quantity;
    uint32_t current_quantity;
    
    uint32_t location_id;    // For explore/deliver objectives
    uint32_t time_limit_ms;  // For timed objectives
    uint32_t start_time_ms;  // When objective was started
    
    bool is_completed;
    bool is_optional;
    bool hidden;             // Hidden from player until discovered
} quest_objective_t;

typedef struct quest_reward {
    uint32_t item_id;
    uint32_t quantity;
    uint32_t gold_reward;
    float xp_reward;
    uint32_t faction_id;
    int faction_reputation_change;
} quest_reward_t;

typedef struct quest {
    uint32_t quest_id;
    char name[QUEST_NAME_LENGTH];
    char description[QUEST_DESCRIPTION_LENGTH];
    
    quest_objective_t objectives[MAX_OBJECTIVES_PER_QUEST];
    uint32_t objective_count;
    
    quest_reward_t rewards[MAX_REWARDS_PER_QUEST];
    uint32_t reward_count;
    
    uint32_t required_level;
    uint32_t prerequisite_quest_ids[8];  // Quests that must be completed first
    uint32_t prerequisite_count;
    
    uint32_t quest_giver_id;
    uint32_t quest_turn_in_id;
    
    bool is_repeatable;
    uint32_t repeat_cooldown_ms;
    uint32_t last_completion_time;
    
    bool auto_accept;
    bool shareable;
    uint32_t difficulty_level;
    
    bool enabled;
} quest_t;

typedef struct player_quest {
    uint32_t quest_id;
    quest_status_t status;
    uint32_t accepted_time_ms;
    uint32_t completion_time_ms;
    uint32_t progress[MAX_OBJECTIVES_PER_QUEST];  // Progress for each objective
    bool objectives_discovered[MAX_OBJECTIVES_PER_QUEST];
} player_quest_t;

typedef struct player_quest_data {
    uint32_t player_id;
    player_quest_t active_quests[32];  // Max concurrent quests
    uint32_t active_quest_count;
    
    uint32_t completed_quest_ids[MAX_QUESTS];
    uint32_t completed_quest_count;
    
    uint32_t failed_quest_ids[MAX_QUESTS];
    uint32_t failed_quest_count;
    
    uint32_t quest_journal[MAX_QUESTS];  // All known quests (completed or available)
    uint32_t journal_count;
} player_quest_data_t;

typedef struct quest_system {
    quest_t quests[MAX_QUESTS];
    uint32_t quest_count;
    
    player_quest_data_t players[MAX_PLAYERS];
    uint32_t player_count;
    
    uint32_t next_quest_id;
    uint32_t next_objective_id;
    
    bool initialized;
} quest_system_t;

static quest_system_t g_quest_system = {0};

// Forward declarations
static quest_t* get_quest(uint32_t quest_id);
static player_quest_data_t* get_player_data(uint32_t player_id);
static player_quest_t* get_player_quest(uint32_t player_id, uint32_t quest_id);
static bool check_quest_prerequisites(const quest_t* quest, uint32_t player_id);
static void update_quest_objectives(uint32_t player_id, objective_type_t type, 
                                   uint32_t target_id, uint32_t quantity);
static bool is_quest_complete(uint32_t player_id, uint32_t quest_id);
static void give_quest_rewards(uint32_t player_id, const quest_t* quest);

bool quest_init(void) {
    if (g_quest_system.initialized) {
        return true;
    }
    
    memset(&g_quest_system, 0, sizeof(g_quest_system));
    g_quest_system.next_quest_id = 1;
    g_quest_system.next_objective_id = 1;
    
    g_quest_system.initialized = true;
    log_info("Quest system initialized");
    return true;
}

void quest_shutdown(void) {
    if (!g_quest_system.initialized) {
        return;
    }
    
    memset(&g_quest_system, 0, sizeof(g_quest_system));
    log_info("Quest system shutdown");
}

uint32_t quest_add_quest(const char* name, const char* description, uint32_t required_level,
                        uint32_t quest_giver_id, uint32_t quest_turn_in_id) {
    if (!g_quest_system.initialized || !name || !description || 
        g_quest_system.quest_count >= MAX_QUESTS) {
        return 0;
    }
    
    quest_t* quest = &g_quest_system.quests[g_quest_system.quest_count];
    memset(quest, 0, sizeof(quest_t));
    
    quest->quest_id = g_quest_system.next_quest_id++;
    strncpy(quest->name, name, QUEST_NAME_LENGTH - 1);
    strncpy(quest->description, description, QUEST_DESCRIPTION_LENGTH - 1);
    quest->required_level = required_level;
    quest->quest_giver_id = quest_giver_id;
    quest->quest_turn_in_id = quest_turn_in_id;
    quest->enabled = true;
    
    g_quest_system.quest_count++;
    log_debug("Added quest: %s (ID: %u)", name, quest->quest_id);
    
    return quest->quest_id;
}

bool quest_add_prerequisite(uint32_t quest_id, uint32_t prerequisite_quest_id) {
    if (!g_quest_system.initialized) {
        return false;
    }
    
    quest_t* quest = get_quest(quest_id);
    if (!quest || quest->prerequisite_count >= 8) {
        return false;
    }
    
    quest->prerequisite_quest_ids[quest->prerequisite_count++] = prerequisite_quest_id;
    log_debug("Added prerequisite quest %u to quest %u", prerequisite_quest_id, quest_id);
    return true;
}

uint32_t quest_add_objective(uint32_t quest_id, objective_type_t type, const char* description,
                            uint32_t target_id, uint32_t target_quantity, bool optional, bool hidden) {
    if (!g_quest_system.initialized || !description) {
        return 0;
    }
    
    quest_t* quest = get_quest(quest_id);
    if (!quest || quest->objective_count >= MAX_OBJECTIVES_PER_QUEST) {
        return 0;
    }
    
    quest_objective_t* objective = &quest->objectives[quest->objective_count];
    memset(objective, 0, sizeof(quest_objective_t));
    
    objective->objective_id = g_quest_system.next_objective_id++;
    objective->type = type;
    strncpy(objective->description, description, OBJECTIVE_TEXT_LENGTH - 1);
    objective->target_id = target_id;
    objective->target_quantity = target_quantity;
    objective->current_quantity = 0;
    objective->is_optional = optional;
    objective->hidden = hidden;
    
    quest->objective_count++;
    log_debug("Added objective to quest %u: %s", quest_id, description);
    
    return objective->objective_id;
}

bool quest_add_reward(uint32_t quest_id, uint32_t item_id, uint32_t quantity,
                     uint32_t gold_reward, float xp_reward) {
    if (!g_quest_system.initialized) {
        return false;
    }
    
    quest_t* quest = get_quest(quest_id);
    if (!quest || quest->reward_count >= MAX_REWARDS_PER_QUEST) {
        return false;
    }
    
    quest_reward_t* reward = &quest->rewards[quest->reward_count];
    memset(reward, 0, sizeof(quest_reward_t));
    
    reward->item_id = item_id;
    reward->quantity = quantity;
    reward->gold_reward = gold_reward;
    reward->xp_reward = xp_reward;
    
    quest->reward_count++;
    log_debug("Added reward to quest %u: item %u x%u, gold %u, xp %.1f",
             quest_id, item_id, quantity, gold_reward, xp_reward);
    
    return true;
}

bool quest_accept_quest(uint32_t player_id, uint32_t quest_id) {
    if (!g_quest_system.initialized) {
        return false;
    }
    
    quest_t* quest = get_quest(quest_id);
    player_quest_data_t* player_data = get_player_data(player_id);
    
    if (!quest || !player_data || !quest->enabled) {
        return false;
    }
    
    // Check if player already has this quest
    if (get_player_quest(player_id, quest_id)) {
        log_warn("Player %u already has quest %u", player_id, quest_id);
        return false;
    }
    
    // Check prerequisites
    if (!check_quest_prerequisites(quest, player_id)) {
        log_warn("Player %u does not meet prerequisites for quest %u", player_id, quest_id);
        return false;
    }
    
    // Add to active quests
    if (player_data->active_quest_count >= 32) {
        log_error("Player %u has too many active quests", player_id);
        return false;
    }
    
    player_quest_t* player_quest = &player_data->active_quests[player_data->active_quest_count];
    player_quest->quest_id = quest_id;
    player_quest->status = QUEST_STATUS_ACTIVE;
    player_quest->accepted_time_ms = get_current_time_ms();
    player_quest->completion_time_ms = 0;
    
    // Initialize progress
    for (uint32_t i = 0; i < quest->objective_count; i++) {
        player_quest->progress[i] = 0;
        player_quest->objectives_discovered[i] = !quest->objectives[i].hidden;
    }
    
    player_data->active_quest_count++;
    
    // Add to journal if not already there
    bool in_journal = false;
    for (uint32_t i = 0; i < player_data->journal_count; i++) {
        if (player_data->quest_journal[i] == quest_id) {
            in_journal = true;
            break;
        }
    }
    
    if (!in_journal && player_data->journal_count < MAX_QUESTS) {
        player_data->quest_journal[player_data->journal_count++] = quest_id;
    }
    
    log_info("Player %u accepted quest %u: %s", player_id, quest_id, quest->name);
    return true;
}

bool quest_abandon_quest(uint32_t player_id, uint32_t quest_id) {
    if (!g_quest_system.initialized) {
        return false;
    }
    
    player_quest_data_t* player_data = get_player_data(player_id);
    if (!player_data) {
        return false;
    }
    
    // Find and remove from active quests
    for (uint32_t i = 0; i < player_data->active_quest_count; i++) {
        if (player_data->active_quests[i].quest_id == quest_id) {
            // Remove from active quests
            memmove(&player_data->active_quests[i],
                    &player_data->active_quests[i + 1],
                    (player_data->active_quest_count - i - 1) * sizeof(player_quest_t));
            player_data->active_quest_count--;
            
            log_info("Player %u abandoned quest %u", player_id, quest_id);
            return true;
        }
    }
    
    return false;
}

bool quest_complete_quest(uint32_t player_id, uint32_t quest_id) {
    if (!g_quest_system.initialized) {
        return false;
    }
    
    quest_t* quest = get_quest(quest_id);
    player_quest_t* player_quest = get_player_quest(player_id, quest_id);
    
    if (!quest || !player_quest || player_quest->status != QUEST_STATUS_ACTIVE) {
        return false;
    }
    
    // Check if all required objectives are complete
    if (!is_quest_complete(player_id, quest_id)) {
        log_warn("Quest %u is not yet complete for player %u", quest_id, player_id);
        return false;
    }
    
    // Mark as completed
    player_quest->status = QUEST_STATUS_COMPLETED;
    player_quest->completion_time_ms = get_current_time_ms();
    
    // Give rewards
    give_quest_rewards(player_id, quest);
    
    // Move from active to completed
    player_quest_data_t* player_data = get_player_data(player_id);
    if (player_data) {
        // Add to completed quests
        if (player_data->completed_quest_count < MAX_QUESTS) {
            player_data->completed_quest_ids[player_data->completed_quest_count++] = quest_id;
        }
        
        // Remove from active quests
        for (uint32_t i = 0; i < player_data->active_quest_count; i++) {
            if (player_data->active_quests[i].quest_id == quest_id) {
                memmove(&player_data->active_quests[i],
                        &player_data->active_quests[i + 1],
                        (player_data->active_quest_count - i - 1) * sizeof(player_quest_t));
                player_data->active_quest_count--;
                break;
            }
        }
    }
    
    log_info("Player %u completed quest %u: %s", player_id, quest_id, quest->name);
    return true;
}

bool quest_fail_quest(uint32_t player_id, uint32_t quest_id) {
    if (!g_quest_system.initialized) {
        return false;
    }
    
    player_quest_t* player_quest = get_player_quest(player_id, quest_id);
    if (!player_quest) {
        return false;
    }
    
    player_quest->status = QUEST_STATUS_FAILED;
    player_quest->completion_time_ms = get_current_time_ms();
    
    // Move from active to failed
    player_quest_data_t* player_data = get_player_data(player_id);
    if (player_data) {
        // Add to failed quests
        if (player_data->failed_quest_count < MAX_QUESTS) {
            player_data->failed_quest_ids[player_data->failed_quest_count++] = quest_id;
        }
        
        // Remove from active quests
        for (uint32_t i = 0; i < player_data->active_quest_count; i++) {
            if (player_data->active_quests[i].quest_id == quest_id) {
                memmove(&player_data->active_quests[i],
                        &player_data->active_quests[i + 1],
                        (player_data->active_quest_count - i - 1) * sizeof(player_quest_t));
                player_data->active_quest_count--;
                break;
            }
        }
    }
    
    log_info("Player %u failed quest %u", player_id, quest_id);
    return true;
}

void quest_on_kill_event(uint32_t player_id, uint32_t target_id, uint32_t quantity) {
    update_quest_objectives(player_id, OBJECTIVE_KILL, target_id, quantity);
}

void quest_on_collect_event(uint32_t player_id, uint32_t item_id, uint32_t quantity) {
    update_quest_objectives(player_id, OBJECTIVE_COLLECT, item_id, quantity);
}

void quest_on_talk_event(uint32_t player_id, uint32_t npc_id) {
    update_quest_objectives(player_id, OBJECTIVE_TALK, npc_id, 1);
}

void quest_on_explore_event(uint32_t player_id, uint32_t location_id) {
    update_quest_objectives(player_id, OBJECTIVE_EXPLORE, location_id, 1);
}

void quest_on_craft_event(uint32_t player_id, uint32_t item_id, uint32_t quantity) {
    update_quest_objectives(player_id, OBJECTIVE_CRAFT, item_id, quantity);
}

const quest_t* quest_get_quest(uint32_t quest_id) {
    return get_quest(quest_id);
}

void quest_get_available_quests(uint32_t player_id, const quest_t** quests, uint32_t* quest_count) {
    if (!g_quest_system.initialized || !quests || !quest_count) {
        return;
    }
    
    *quests = g_quest_system.quests;
    *quest_count = 0;
    
    player_quest_data_t* player_data = get_player_data(player_id);
    if (!player_data) {
        return;
    }
    
    for (uint32_t i = 0; i < g_quest_system.quest_count; i++) {
        const quest_t* quest = &g_quest_system.quests[i];
        
        if (!quest->enabled) {
            continue;
        }
        
        // Check if player already has this quest
        bool has_quest = false;
        if (get_player_quest(player_id, quest->quest_id)) {
            has_quest = true;
        }
        
        // Check if already completed
        bool completed = false;
        for (uint32_t j = 0; j < player_data->completed_quest_count; j++) {
            if (player_data->completed_quest_ids[j] == quest->quest_id) {
                completed = true;
                break;
            }
        }
        
        if (!has_quest && !completed && check_quest_prerequisites(quest, player_id)) {
            (*quest_count)++;
        }
    }
}

void quest_get_active_quests(uint32_t player_id, const player_quest_t** quests, uint32_t* quest_count) {
    if (!g_quest_system.initialized || !quests || !quest_count) {
        return;
    }
    
    player_quest_data_t* player_data = get_player_data(player_id);
    if (!player_data) {
        *quests = NULL;
        *quest_count = 0;
        return;
    }
    
    *quests = player_data->active_quests;
    *quest_count = player_data->active_quest_count;
}

bool quest_is_quest_available(uint32_t player_id, uint32_t quest_id) {
    if (!g_quest_system.initialized) {
        return false;
    }
    
    const quest_t* quest = get_quest(quest_id);
    if (!quest || !quest->enabled) {
        return false;
    }
    
    // Check if player already has or completed this quest
    if (get_player_quest(player_id, quest_id)) {
        return false;
    }
    
    player_quest_data_t* player_data = get_player_data(player_id);
    if (player_data) {
        for (uint32_t i = 0; i < player_data->completed_quest_count; i++) {
            if (player_data->completed_quest_ids[i] == quest_id) {
                return false;
            }
        }
    }
    
    return check_quest_prerequisites(quest, player_id);
}

// Static helper functions
static quest_t* get_quest(uint32_t quest_id) {
    for (uint32_t i = 0; i < g_quest_system.quest_count; i++) {
        if (g_quest_system.quests[i].quest_id == quest_id) {
            return &g_quest_system.quests[i];
        }
    }
    return NULL;
}

static player_quest_data_t* get_player_data(uint32_t player_id) {
    // Find existing player data
    for (uint32_t i = 0; i < g_quest_system.player_count; i++) {
        if (g_quest_system.players[i].player_id == player_id) {
            return &g_quest_system.players[i];
        }
    }
    
    // Create new player data if space allows
    if (g_quest_system.player_count < MAX_PLAYERS) {
        player_quest_data_t* player_data = &g_quest_system.players[g_quest_system.player_count];
        memset(player_data, 0, sizeof(player_quest_data_t));
        player_data->player_id = player_id;
        g_quest_system.player_count++;
        return player_data;
    }
    
    return NULL;
}

static player_quest_t* get_player_quest(uint32_t player_id, uint32_t quest_id) {
    player_quest_data_t* player_data = get_player_data(player_id);
    if (!player_data) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < player_data->active_quest_count; i++) {
        if (player_data->active_quests[i].quest_id == quest_id) {
            return &player_data->active_quests[i];
        }
    }
    
    return NULL;
}

static bool check_quest_prerequisites(const quest_t* quest, uint32_t player_id) {
    player_quest_data_t* player_data = get_player_data(player_id);
    if (!player_data) {
        return quest->prerequisite_count == 0;
    }
    
    for (uint32_t i = 0; i < quest->prerequisite_count; i++) {
        uint32_t prereq_id = quest->prerequisite_quest_ids[i];
        bool found = false;
        
        // Check if prerequisite quest is completed
        for (uint32_t j = 0; j < player_data->completed_quest_count; j++) {
            if (player_data->completed_quest_ids[j] == prereq_id) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            return false;
        }
    }
    
    return true;
}

static void update_quest_objectives(uint32_t player_id, objective_type_t type,
                                   uint32_t target_id, uint32_t quantity) {
    player_quest_data_t* player_data = get_player_data(player_id);
    if (!player_data) {
        return;
    }
    
    for (uint32_t i = 0; i < player_data->active_quest_count; i++) {
        player_quest_t* player_quest = &player_data->active_quests[i];
        quest_t* quest = get_quest(player_quest->quest_id);
        
        if (!quest || player_quest->status != QUEST_STATUS_ACTIVE) {
            continue;
        }
        
        for (uint32_t j = 0; j < quest->objective_count; j++) {
            quest_objective_t* objective = &quest->objectives[j];
            
            if (objective->type == type && objective->target_id == target_id &&
                !objective->is_completed) {
                
                // Update progress
                player_quest->progress[j] = (player_quest->progress[j] + quantity > objective->target_quantity) ?
                                           objective->target_quantity : player_quest->progress[j] + quantity;
                
                // Mark as complete if target reached
                if (player_quest->progress[j] >= objective->target_quantity) {
                    objective->is_completed = true;
                    log_debug("Player %u completed objective %u for quest %u", player_id, j, quest->quest_id);
                }
                
                // Discover hidden objectives if this was a prerequisite
                if (objective->hidden) {
                    objective->hidden = false;
                    player_quest->objectives_discovered[j] = true;
                }
            }
        }
    }
}

static bool is_quest_complete(uint32_t player_id, uint32_t quest_id) {
    quest_t* quest = get_quest(quest_id);
    player_quest_t* player_quest = get_player_quest(player_id, quest_id);
    
    if (!quest || !player_quest) {
        return false;
    }
    
    for (uint32_t i = 0; i < quest->objective_count; i++) {
        const quest_objective_t* objective = &quest->objectives[i];
        
        if (!objective->is_optional && !objective->is_completed) {
            return false;
        }
    }
    
    return true;
}

static void give_quest_rewards(uint32_t player_id, const quest_t* quest) {
    // This would interface with inventory, experience, and faction systems
    // Placeholder implementation
    
    for (uint32_t i = 0; i < quest->reward_count; i++) {
        const quest_reward_t* reward = &quest->rewards[i];
        log_debug("Giving reward to player %u: item %u x%u, gold %u, xp %.1f",
                 player_id, reward->item_id, reward->quantity, reward->gold_reward, reward->xp_reward);
    }
}

uint32_t get_current_time_ms(void) {
    // Placeholder - would typically use platform-specific time function
    static uint32_t counter = 0;
    return counter += 16;  // Simulate 60 FPS
}
