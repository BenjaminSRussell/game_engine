/**
 * =================================================================================================
 *                              NPC BEHAVIOR SYSTEM
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Pre-built NPC behaviors with schedules, routines, and social interactions.
 *
 * =================================================================================================
 */

#ifndef NPC_BEHAVIOR_H
#define NPC_BEHAVIOR_H

// #include "../../../engine/common.h"
#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    NPC TYPES
 * =================================================================================================
 */

typedef enum NPCRole {
  NPC_ROLE_CIVILIAN,
  NPC_ROLE_GUARD,
  NPC_ROLE_MERCHANT,
  NPC_ROLE_BLACKSMITH,
  NPC_ROLE_INNKEEPER,
  NPC_ROLE_FARMER,
  NPC_ROLE_MINER,
  NPC_ROLE_PRIEST,
  NPC_ROLE_NOBLE,
  NPC_ROLE_BEGGAR,
  NPC_ROLE_ENTERTAINER,
  NPC_ROLE_SAILOR,
  NPC_ROLE_SOLDIER,
  NPC_ROLE_QUEST_GIVER,
  NPC_ROLE_COMPANION,
  NPC_ROLE_ENEMY,
} NPCRole;

typedef enum NPCState {
  NPC_STATE_IDLE,
  NPC_STATE_WALKING,
  NPC_STATE_RUNNING,
  NPC_STATE_WORKING,
  NPC_STATE_RESTING,
  NPC_STATE_EATING,
  NPC_STATE_SLEEPING,
  NPC_STATE_TALKING,
  NPC_STATE_SHOPPING,
  NPC_STATE_FIGHTING,
  NPC_STATE_FLEEING,
  NPC_STATE_PATROLLING,
  NPC_STATE_GUARDING,
  NPC_STATE_FOLLOWING,
  NPC_STATE_DEAD,
} NPCState;

/* =================================================================================================
 *                                    SCHEDULE SYSTEM
 * =================================================================================================
 */

typedef struct ScheduleEntry {
  float start_hour; // 0-24
  float end_hour;
  char activity[32];
  char location[64];
  float position[3];
  char animation[64];
  uint32_t priority;
  bool interruptible;
  char condition[128]; // Lua condition
} ScheduleEntry;

typedef struct NPCSchedule {
  char name[64];
  ScheduleEntry *entries;
  uint32_t entry_count;
  uint32_t entry_capacity;

  // Per-day overrides
  ScheduleEntry *day_overrides[7];
  uint32_t day_override_counts[7];

  // Weather overrides
  ScheduleEntry *rain_overrides;
  uint32_t rain_override_count;
} NPCSchedule;

NPCSchedule *schedule_create(const char *name);
void schedule_add_entry(NPCSchedule *schedule, const ScheduleEntry *entry);
void schedule_remove_entry(NPCSchedule *schedule, int index);
const ScheduleEntry *schedule_get_current_entry(NPCSchedule *schedule,
                                                float time);
const ScheduleEntry *schedule_get_next_entry(NPCSchedule *schedule, float time);
bool schedule_evaluate_conditions(const ScheduleEntry *entry, void *context);
void schedule_add_day_override(NPCSchedule *schedule, int day,
                               const ScheduleEntry *entry);
void schedule_add_weather_override(NPCSchedule *schedule, const char *weather,
                                   const ScheduleEntry *entry);
void schedule_serialize(NPCSchedule *schedule, const char *path);
void schedule_deserialize(NPCSchedule *schedule, const char *path);
void schedule_load_preset(NPCSchedule *schedule, const char *name);

/* =================================================================================================
 *                                    NPC NEEDS
 * =================================================================================================
 */

typedef struct NPCNeeds {
  float hunger; // 0-100
  float thirst;
  float energy;
  float social;
  float entertainment;
  float safety;
  float comfort;
  float hygiene;

  // Decay rates per hour
  float hunger_decay;
  float thirst_decay;
  float energy_decay;
  float social_decay;

  // Thresholds
  float critical_threshold;
  float low_threshold;
  float satisfied_threshold;
} NPCNeeds;

void needs_init(NPCNeeds *needs);
void needs_update(NPCNeeds *needs, float dt);
void needs_satisfy(NPCNeeds *needs, const char *need, float amount);
float needs_get_priority(NPCNeeds *needs, const char *need);
bool needs_get_urgent(NPCNeeds *needs);
void needs_affect_mood(NPCNeeds *needs, float *mood);
void needs_affect_behavior(NPCNeeds *needs, void *behavior);

/* =================================================================================================
 *                                    NPC RELATIONSHIPS
 * =================================================================================================
 */

typedef struct Relationship {
  uint32_t other_npc_id;
  float friendship;           // -100 to 100
  float romance;              // 0 to 100
  float respect;              // -100 to 100
  float fear;                 // 0 to 100
  float trust;                // -100 to 100
  char relationship_type[32]; // "friend", "rival", "spouse", etc.
  uint64_t first_met;
  uint64_t last_interaction;
} Relationship;

typedef struct RelationshipManager {
  uint32_t npc_id;
  Relationship *relationships;
  uint32_t relationship_count;
  uint32_t relationship_capacity;

  float default_disposition;
  float disposition_decay;
} RelationshipManager;

void relationship_init(RelationshipManager *mgr, uint32_t npc_id);
void relationship_add(RelationshipManager *mgr, uint32_t other_id,
                      float initial);
void relationship_remove(RelationshipManager *mgr, uint32_t other_id);
Relationship *relationship_get(RelationshipManager *mgr, uint32_t other_id);
void relationship_modify(RelationshipManager *mgr, uint32_t other_id,
                         float amount);
void relationship_update_from_action(RelationshipManager *mgr, void *action);
float relationship_get_disposition(RelationshipManager *mgr, uint32_t other_id);
bool relationship_check_threshold(RelationshipManager *mgr, uint32_t other_id,
                                  float threshold);
void relationship_gossip_spread(RelationshipManager *mgr, void *gossip);
void relationship_faction_modifier(RelationshipManager *mgr,
                                   const char *faction, float mod);
void relationship_serialize(RelationshipManager *mgr, const char *path);

/* =================================================================================================
 *                                    NPC MEMORY
 * =================================================================================================
 */

typedef enum MemoryType {
  MEMORY_SAW_PLAYER,
  MEMORY_SAW_CRIME,
  MEMORY_WAS_ATTACKED,
  MEMORY_WAS_HELPED,
  MEMORY_RECEIVED_GIFT,
  MEMORY_TRADED,
  MEMORY_CONVERSATION,
  MEMORY_WITNESSED_EVENT,
  MEMORY_DISCOVERED_LOCATION,
} MemoryType;

typedef struct NPCMemory {
  MemoryType type;
  uint32_t subject_id;
  float position[3];
  uint64_t timestamp;
  float importance;
  float decay_rate;
  char details[256];
  bool is_rumor;
} NPCMemory;

typedef struct MemoryBank {
  NPCMemory *memories;
  uint32_t memory_count;
  uint32_t memory_capacity;
  uint32_t max_memories;
  float memory_decay_rate;
} MemoryBank;

void memory_bank_init(MemoryBank *bank);
void memory_add(MemoryBank *bank, MemoryType type, uint32_t subject,
                const char *details);
void memory_forget(MemoryBank *bank, uint32_t subject);
void memory_decay(MemoryBank *bank, float dt);
NPCMemory *memory_recall(MemoryBank *bank, MemoryType type);
NPCMemory *memory_search(MemoryBank *bank, const char *query);
void memory_consolidate(MemoryBank *bank);
void memory_share_as_rumor(MemoryBank *bank, uint32_t target_npc);
void memory_receive_rumor(MemoryBank *bank, const NPCMemory *rumor);
void memory_serialize(MemoryBank *bank, const char *path);

/* =================================================================================================
 *                                    NPC DEFINITION
 * =================================================================================================
 */

typedef struct NPCDefinition {
  uint32_t id;
  char name[64];
  char display_name[64];
  char description[256];

  NPCRole role;
  char faction[32];

  // Visual
  char model_path[128];
  char portrait_path[128];
  uint32_t customization_preset;

  // Stats
  int32_t level;
  float health;
  float stamina;
  float damage;
  float armor;

  // AI
  char ai_profile[64];
  float aggression;
  float bravery;
  float sociability;
  float curiosity;

  // Dialogue
  uint32_t dialogue_tree_id;
  uint32_t bark_collection_id;

  // Schedule
  NPCSchedule *schedule;

  // Needs
  NPCNeeds needs;

  // Home
  char home_location[64];
  float home_position[3];
  char work_location[64];
  float work_position[3];

  // Inventory
  uint32_t
      inventory_items[16]; // Changed ItemID to uint32_t to avoid dependency
  uint32_t inventory_count;
  int32_t gold;

  // Merchant
  bool is_merchant;
  uint32_t shop_inventory_id;
  float buy_price_modifier;
  float sell_price_modifier;

  // Essential
  bool is_essential;
  bool is_unique;
  bool respawns;
  float respawn_time;
} NPCDefinition;

NPCDefinition *npc_definition_create(const char *name);
NPCDefinition *npc_definition_load(const char *path);
void npc_definition_save(NPCDefinition *def, const char *path);
bool npc_definition_validate(NPCDefinition *def);
NPCDefinition *npc_definition_from_template(const char *template_name);
void npc_definition_randomize(NPCDefinition *def, uint32_t seed);

/* =================================================================================================
 *                                    NPC INSTANCE
 * =================================================================================================
 */

typedef struct NPCInstance {
  uint32_t instance_id;
  uint32_t definition_id;

  // Transform
  float position[3];
  float rotation[4];

  // State
  NPCState state;
  NPCState previous_state;
  float state_timer;

  // Runtime
  NPCNeeds current_needs;
  RelationshipManager relationships;
  MemoryBank memories;

  // AI
  uint32_t current_goal;
  float goal_progress;
  uint32_t target_entity;
  float path[32][3];
  uint32_t path_length;
  uint32_t path_index;

  // Combat
  float current_health;
  uint32_t attack_target;
  float attack_cooldown;

  // Animation
  char current_animation[64];
  float animation_time;

  // Dialogue
  bool in_conversation;
  uint32_t conversation_partner;

  // Schedule
  uint32_t current_schedule_index;
  float schedule_check_timer;

  // Flags
  bool is_spawned;
  bool is_dead;
  bool is_hostile;
  bool is_alerted;
  bool is_searching;
} NPCInstance;

NPCInstance *npc_spawn(uint32_t def_id, float x, float y, float z);
void npc_despawn(NPCInstance *npc);
void npc_update(NPCInstance *npc, float dt);
void npc_update_needs(NPCInstance *npc, float dt);
void npc_update_schedule(NPCInstance *npc, float time);
void npc_update_ai(NPCInstance *npc, float dt);
void npc_update_combat(NPCInstance *npc, float dt);
void npc_update_movement(NPCInstance *npc, float dt);
void npc_update_animation(NPCInstance *npc, float dt);
void npc_select_behavior(NPCInstance *npc);
void npc_execute_behavior(NPCInstance *npc, float dt);
void npc_react_to_event(NPCInstance *npc, void *event);
void npc_take_damage(NPCInstance *npc, float amount, uint32_t attacker);
void npc_die(NPCInstance *npc);
void npc_respawn(NPCInstance *npc);
void npc_start_conversation(NPCInstance *npc, uint32_t partner);
void npc_end_conversation(NPCInstance *npc);
void npc_follow_path(NPCInstance *npc, float speed);
bool npc_find_path_to(NPCInstance *npc, float x, float y, float z);
void npc_interact_with(NPCInstance *npc, uint32_t entity);
void npc_serialize(NPCInstance *npc, const char *path);
void npc_deserialize(NPCInstance *npc, const char *path);

/* =================================================================================================
 *                                    NPC MANAGER
 * =================================================================================================
 */

typedef struct NPCManager {
  NPCDefinition *definitions;
  uint32_t definition_count;

  NPCInstance *instances;
  uint32_t instance_count;
  uint32_t instance_capacity;

  // Pooling
  uint32_t *free_instance_ids;
  uint32_t free_instance_count;

  // Spatial
  void *spatial_hash;

  // Settings
  float update_radius;
  float spawn_radius;
  float despawn_radius;
  uint32_t max_active_npcs;
} NPCManager;

void npc_manager_init(NPCManager *mgr);
void npc_manager_shutdown(NPCManager *mgr);
void npc_manager_update(NPCManager *mgr, float dt);
void npc_manager_spawn_in_area(NPCManager *mgr, float x, float y, float radius,
                               int count);
void npc_manager_despawn_out_of_range(NPCManager *mgr, float x, float y,
                                      float radius);
NPCInstance *npc_manager_find_nearby(NPCManager *mgr, float x, float y,
                                     float radius);
NPCInstance *npc_manager_find_by_role(NPCManager *mgr, NPCRole role);
NPCInstance *npc_manager_find_by_faction(NPCManager *mgr, const char *faction);
void npc_manager_broadcast_event(NPCManager *mgr, void *event);
void npc_manager_save(NPCManager *mgr, const char *path);
void npc_manager_load(NPCManager *mgr, const char *path);

#endif // NPC_BEHAVIOR_H
