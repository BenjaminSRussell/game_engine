#ifndef QUEST_SYSTEM_H
#define QUEST_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "math/vec3.h"
#include "core/utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// Quest System Limits
#define MAX_QUESTS 1024
#define MAX_OBJECTIVES_PER_QUEST 16
#define MAX_QUEST_NAME_LENGTH 64
#define MAX_QUEST_DESC_LENGTH 512
#define MAX_QUEST_REWARDS 8
#define MAX_QUEST_PREREQUISITES 8
#define MAX_QUEST_CHOICES 4

// Quest States
typedef enum {
    QUEST_STATE_LOCKED = 0,
    QUEST_STATE_AVAILABLE,
    QUEST_STATE_ACTIVE,
    QUEST_STATE_COMPLETED,
    QUEST_STATE_FAILED,
    QUEST_STATE_ABANDONED
} QuestState;

// Objective Types
typedef enum {
    OBJECTIVE_KILL = 0,
    OBJECTIVE_COLLECT,
    OBJECTIVE_TALK_TO,
    OBJECTIVE_GO_TO,
    OBJECTIVE_USE_ITEM,
    OBJECTIVE_ESCORT,
    OBJECTIVE_TIMED,
    OBJECTIVE_CUSTOM
} ObjectiveType;

// Objective Progress
typedef struct {
    ObjectiveType type;
    uint32_t target_id;        // NPC ID, Item ID, or Location ID
    uint32_t current_count;
    uint32_t required_count;
    char description[128];
    bool completed;
    bool optional;
    Vec3 location;            // For GO_TO objectives
    float radius;             // Radius for location objectives
} QuestObjective;

// Quest Rewards
typedef struct {
    enum {
        REWARD_XP,
        REWARD_GOLD,
        REWARD_ITEM,
        REWARD_REPUTATION,
        REWARD_ABILITY
    } type;
    union {
        uint32_t xp_amount;
        uint32_t gold_amount;
        struct {
            uint32_t item_id;
            uint32_t count;
        } item;
        struct {
            uint32_t faction_id;
            int32_t amount;
        } reputation;
        uint32_t ability_id;
    } data;
} QuestReward;

// Quest Definition
typedef struct {
    uint32_t id;
    char name[MAX_QUEST_NAME_LENGTH];
    char description[MAX_QUEST_DESC_LENGTH];
    QuestState state;
    
    // Objectives
    QuestObjective objectives[MAX_OBJECTIVES_PER_QUEST];
    uint8_t objective_count;
    
    // Prerequisites
    uint32_t prerequisites[MAX_QUEST_PREREQUISITES];
    uint8_t prerequisite_count;
    
    // Rewards
    QuestReward rewards[MAX_QUEST_REWARDS];
    uint8_t reward_count;
    
    // Timing
    bool is_timed;
    float time_limit;
    float time_remaining;
    
    // Choices (for branching quests)
    uint8_t choice_count;
    uint32_t next_quests[MAX_QUEST_CHOICES];
    
    // Metadata
    uint32_t quest_giver_id;
    uint32_t level_requirement;
    bool is_repeatable;
    float auto_accept_distance;
} Quest;

// Quest Manager
typedef struct {
    Quest quests[MAX_QUESTS];
    uint32_t quest_count;
    
    // Active quests for player
    uint32_t active_quests[MAX_QUESTS];
    uint32_t active_quest_count;
    
    // Completed quests
    uint32_t completed_quests[MAX_QUESTS];
    uint32_t completed_quest_count;
    
    // Quest events
    bool event_listeners_enabled;
} QuestManager;

// Quest Events
typedef enum {
    QUEST_EVENT_KILL,
    QUEST_EVENT_COLLECT,
    QUEST_EVENT_TALK,
    QUEST_EVENT_LOCATION,
    QUEST_EVENT_TIMER
} QuestEventType;

typedef struct {
    QuestEventType type;
    uint32_t entity_id;
    uint32_t count;
    Vec3 location;
} QuestEvent;

// Quest Log Entry
typedef struct {
    uint32_t quest_id;
    uint32_t timestamp;
    char message[256];
    QuestState old_state;
    QuestState new_state;
} QuestLogEntry;

// Quest Marker (for HUD/Map)
typedef struct {
    uint32_t quest_id;
    uint32_t objective_index;
    Vec3 world_position;
    bool is_active;
    char icon_name[32];
} QuestMarker;

// Quest Area Highlight
typedef struct {
    uint32_t quest_id;
    Vec3 center;
    float radius;
    bool is_active;
    uint32_t color;
} QuestArea;

// Quest Chain (sequential quests)
typedef struct {
    uint32_t chain_id;
    char name[MAX_QUEST_NAME_LENGTH];
    uint32_t quest_ids[MAX_QUEST_CHOICES];
    uint8_t quest_count;
    uint8_t current_index;
} QuestChain;

// Quest Save Data
typedef struct {
    uint32_t version;
    uint32_t active_quest_count;
    uint32_t completed_quest_count;
    
    // Active quest states
    struct {
        uint32_t quest_id;
        QuestState state;
        uint32_t objective_progress[MAX_OBJECTIVES_PER_QUEST];
        float time_remaining;
    } active_quests[MAX_QUESTS];
    
    // Completed quest list
    uint32_t completed_quest_ids[MAX_QUESTS];
    
    // Quest log
    QuestLogEntry log_entries[256];
    uint32_t log_count;
} QuestSaveData;

// Core Functions
bool quest_manager_init(QuestManager* manager);
void quest_manager_shutdown(QuestManager* manager);
QuestManager* quest_manager_create(void);
void quest_manager_destroy(QuestManager* manager);

// Quest Management
uint32_t quest_create(QuestManager* manager, const char* name, const char* description);
bool quest_add_objective(QuestManager* manager, uint32_t quest_id, ObjectiveType type, 
                        uint32_t target_id, uint32_t count, const char* description);
bool quest_add_prerequisite(QuestManager* manager, uint32_t quest_id, uint32_t prerequisite_id);
bool quest_add_reward(QuestManager* manager, uint32_t quest_id, const QuestReward* reward);
bool quest_set_timed(QuestManager* manager, uint32_t quest_id, float time_limit);

// Quest State Management
bool quest_accept(QuestManager* manager, uint32_t quest_id, uint32_t player_id);
bool quest_abandon(QuestManager* manager, uint32_t quest_id, uint32_t player_id);
bool quest_complete(QuestManager* manager, uint32_t quest_id, uint32_t player_id);
bool quest_fail(QuestManager* manager, uint32_t quest_id, uint32_t player_id);

// Objective Progress
bool quest_update_objective(QuestManager* manager, uint32_t quest_id, uint32_t objective_index, 
                           uint32_t current_count);
bool quest_advance_objective(QuestManager* manager, uint32_t quest_id, uint32_t objective_index, 
                             uint32_t amount);

// Event System
void quest_process_event(QuestManager* manager, const QuestEvent* event);
void quest_register_event_listener(QuestManager* manager, QuestEventType type);
void quest_unregister_event_listener(QuestManager* manager, QuestEventType type);

// Quest Queries
Quest* quest_get_quest(QuestManager* manager, uint32_t quest_id);
Quest* quest_find_quest_by_name(QuestManager* manager, const char* name);
bool quest_is_available(QuestManager* manager, uint32_t quest_id, uint32_t player_id);
bool quest_meets_prerequisites(QuestManager* manager, uint32_t quest_id, uint32_t player_id);

// Quest Chains
uint32_t quest_chain_create(QuestManager* manager, const char* name);
bool quest_chain_add_quest(QuestManager* manager, uint32_t chain_id, uint32_t quest_id);
QuestChain* quest_chain_get(QuestManager* manager, uint32_t chain_id);

// Quest Log
void quest_log_add_entry(QuestManager* manager, uint32_t quest_id, const char* message, 
                        QuestState old_state, QuestState new_state);
QuestLogEntry* quest_log_get_entries(QuestManager* manager, uint32_t* count);

// Quest Markers & HUD
QuestMarker* quest_get_markers(QuestManager* manager, uint32_t* count);
QuestArea* quest_get_areas(QuestManager* manager, uint32_t* count);
void quest_update_markers(QuestManager* manager);

// Save/Load
bool quest_save_state(QuestManager* manager, QuestSaveData* save_data);
bool quest_load_state(QuestManager* manager, const QuestSaveData* save_data);

// Utility Functions
const char* quest_state_to_string(QuestState state);
const char* objective_type_to_string(ObjectiveType type);
float quest_get_progress_percentage(QuestManager* manager, uint32_t quest_id);
bool quest_has_completed_objectives(QuestManager* manager, uint32_t quest_id);

// Quest Givers (NPCs)
typedef struct {
    uint32_t npc_id;
    uint32_t available_quests[MAX_QUESTS];
    uint32_t quest_count;
    bool show_exclamation_mark;
    Vec3 position;
    float interaction_radius;
} QuestGiver;

bool quest_giver_add_quest(QuestManager* manager, uint32_t npc_id, uint32_t quest_id);
QuestGiver* quest_giver_get(QuestManager* manager, uint32_t npc_id);
void quest_giver_update_visibility(QuestManager* manager, uint32_t player_id);

// Dynamic Quest Triggers
typedef struct {
    enum {
        TRIGGER_ZONE_ENTER,
        TRIGGER_ZONE_EXIT,
        TRIGGER_ITEM_PICKUP,
        TRIGGER_ENEMY_KILL,
        TRIGGER_TIME_ELAPSED,
        TRIGGER_CUSTOM_EVENT
    } type;
    
    uint32_t quest_id;
    Vec3 zone_center;
    float zone_radius;
    uint32_t trigger_data;
    bool one_time;
    bool triggered;
} QuestTrigger;

bool quest_trigger_create(QuestManager* manager, QuestTrigger* trigger);
void quest_trigger_update(QuestManager* manager, uint32_t player_id);

// World State Changes
typedef struct {
    uint32_t quest_id;
    enum {
        WORLD_CHANGE_SPAWN_NPC,
        WORLD_CHANGE_REMOVE_NPC,
        WORLD_CHANGE_ENABLE_OBJECT,
        WORLD_CHANGE_DISABLE_OBJECT,
        WORLD_CHANGE_WEATHER,
        WORLD_CHANGE_TIME_OF_DAY
    } type;
    union {
        struct {
            uint32_t npc_id;
            Vec3 position;
        } npc_spawn;
        struct {
            uint32_t object_id;
        } object_toggle;
        struct {
            uint32_t weather_type;
        } weather;
        struct {
            float time_of_day;
        } time;
    } data;
} WorldStateChange;

void quest_apply_world_changes(QuestManager* manager, uint32_t quest_id);
void quest_revert_world_changes(QuestManager* manager, uint32_t quest_id);

#ifdef __cplusplus
}
#endif

#endif // QUEST_SYSTEM_H
