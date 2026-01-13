// include/ai/npc_dialogue.h
//
// Purpose: Defines the public API and data structures for the NPC dialogue system.
// This header provides structures for managing dialogue trees, conversation states,
// NPC responses, and player choices. The system supports branching dialogue,
// conditional responses, and integration with quest and trading systems.
//
// Public APIs:
// - `DialogueNode`: Individual dialogue nodes with text and options.
// - `DialogueOption`: Player response choices with conditions and consequences.
// - `DialogueTree`: Complete dialogue tree structure for an NPC.
// - `DialogueState`: Current state of an active conversation.
// - `dialogue_tree_init`: Initialize a dialogue tree with default responses.
// - `dialogue_tree_free`: Free dialogue tree resources.
// - `dialogue_start`: Begin a conversation with an NPC.
// - `dialogue_end`: End current conversation.
// - `dialogue_select_option`: Choose a dialogue option.
// - `dialogue_get_current_node`: Get current dialogue node.
// - `dialogue_has_condition`: Check if a condition is met.
// - `dialogue_execute_consequence`: Execute dialogue consequences.
//
// Ownership: `DialogueTree` instances are owned by NPC entities. `DialogueState`
// is managed by dialogue system during active conversations.
//
// Invariants:
// - Dialogue nodes must have valid text (can be empty for silent nodes).
// - Dialogue options must have valid text and point to valid nodes.
// - Conditions and consequences must be properly registered.
// - Node IDs must be unique within a dialogue tree.
// - Circular references in dialogue trees should be avoided.
//
#ifndef AI_NPC_DIALOGUE_H
#define AI_NPC_DIALOGUE_H

#include "engine/include/common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct World;

// Maximum limits for dialogue system
#define MAX_DIALOGUE_OPTIONS 6
#define MAX_DIALOGUE_TEXT_LENGTH 256
#define MAX_DIALOGUE_NODES 100
#define MAX_DIALOGUE_CONDITIONS 10
#define MAX_DIALOGUE_CONSEQUENCES 5

// Dialogue condition types
typedef enum {
  DIALOGUE_CONDITION_NONE,           // Always true
  DIALOGUE_CONDITION_TIME_OF_DAY,    // Based on game time
  DIALOGUE_CONDITION_WEATHER,         // Based on current weather
  DIALOGUE_CONDITION_PLAYER_LEVEL,    // Based on player level
  DIALOGUE_CONDITION_QUEST_STATUS,    // Based on quest completion
  DIALOGUE_CONDITION_INVENTORY_ITEM,  // Based on having specific item
  DIALOGUE_CONDITION_PLAYER_HEALTH,   // Based on player health
  DIALOGUE_CONDITION_NPC_RELATIONSHIP, // Based on relationship with NPC
  DIALOGUE_CONDITION_RANDOM_CHANCE,   // Based on random chance
  DIALOGUE_CONDITION_COUNT
} DialogueConditionType;

// Dialogue consequence types
typedef enum {
  DIALOGUE_CONSEQUENCE_NONE,          // No effect
  DIALOGUE_CONSEQUENCE_GIVE_ITEM,     // Give item to player
  DIALOGUE_CONSEQUENCE_TAKE_ITEM,     // Take item from player
  DIALOGUE_CONSEQUENCE_START_QUEST,  // Start a quest
  DIALOGUE_CONSEQUENCE_COMPLETE_QUEST, // Complete a quest
  DIALOGUE_CONSEQUENCE_CHANGE_RELATIONSHIP, // Change NPC relationship
  DIALOGUE_CONSEQUENCE_GIVE_EXPERIENCE, // Give experience to player
  DIALOGUE_CONSEQUENCE_TRIGGER_EVENT, // Trigger world event
  DIALOGUE_CONSEQUENCE_TELEPORT,     // Teleport player
  DIALOGUE_CONSEQUENCE_COUNT
} DialogueConsequenceType;

// Dialogue condition structure
typedef struct {
  DialogueConditionType type;
  union {
    struct {
      u32 start_hour;
      u32 end_hour;
    } time_of_day;
    
    struct {
      u32 weather_type;
    } weather;
    
    struct {
      u32 required_level;
    } player_level;
    
    struct {
      u32 quest_id;
      u32 required_status; // 0=not started, 1=in progress, 2=completed
    } quest_status;
    
    struct {
      u32 item_id;
      u32 required_count;
    } inventory_item;
    
    struct {
      f32 min_health_percentage;
    } player_health;
    
    struct {
      i32 min_relationship;
    } npc_relationship;
    
    struct {
      f32 chance; // 0.0 to 1.0
    } random_chance;
  } data;
} DialogueCondition;

// Dialogue consequence structure
typedef struct {
  DialogueConsequenceType type;
  union {
    struct {
      u32 item_id;
      u32 count;
    } give_item;
    
    struct {
      u32 item_id;
      u32 count;
    } take_item;
    
    struct {
      u32 quest_id;
    } start_quest;
    
    struct {
      u32 quest_id;
    } complete_quest;
    
    struct {
      i32 relationship_change;
    } change_relationship;
    
    struct {
      u32 experience_amount;
    } give_experience;
    
    struct {
      u32 event_id;
    } trigger_event;
    
    struct {
      Vec3 position;
    } teleport;
  } data;
} DialogueConsequence;

// Dialogue option structure
typedef struct {
  u32 option_id;
  char text[MAX_DIALOGUE_TEXT_LENGTH];
  u32 target_node_id;
  
  DialogueCondition conditions[MAX_DIALOGUE_CONDITIONS];
  u32 condition_count;
  
  DialogueConsequence consequences[MAX_DIALOGUE_CONSEQUENCES];
  u32 consequence_count;
} DialogueOption;

// Dialogue node structure
typedef struct {
  u32 node_id;
  char text[MAX_DIALOGUE_TEXT_LENGTH];
  char speaker_name[64]; // NPC name or "Player"
  
  DialogueOption options[MAX_DIALOGUE_OPTIONS];
  u32 option_count;
  
  // Special node flags
  bool is_exit_node;     // Ends conversation
  bool is_random_text;    // Choose from multiple text variants
  u32 text_variants;     // Number of text variants
} DialogueNode;

// Dialogue tree structure
typedef struct {
  u32 tree_id;
  char npc_name[64];
  
  DialogueNode nodes[MAX_DIALOGUE_NODES];
  u32 node_count;
  
  u32 entry_node_id;
  bool is_greeting_random; // Random greeting from multiple nodes
} DialogueTree;

// Active conversation state
typedef struct {
  Entity npc_entity;
  Entity player_entity;
  DialogueTree *tree;
  u32 current_node_id;
  
  bool is_active;
  f32 start_time;
  
  // Conversation history for context
  u32 history[MAX_DIALOGUE_NODES];
  u32 history_count;
} DialogueState;

// Dialogue system structure
typedef struct {
  DialogueTree *loaded_trees[MAX_DIALOGUE_NODES];
  u32 tree_count;
  
  DialogueState active_conversations[16]; // Support multiple conversations
  u32 active_count;
  
  bool is_initialized;
} DialogueSystem;

// Dialogue tree management
void dialogue_tree_init(DialogueTree *tree, u32 tree_id, const char *npc_name);
void dialogue_tree_free(DialogueTree *tree);
u32 dialogue_tree_add_node(DialogueTree *tree, const DialogueNode *node);
u32 dialogue_tree_add_option(DialogueTree *tree, u32 node_id,
                             const DialogueOption *option);
DialogueNode *dialogue_tree_get_node(DialogueTree *tree, u32 node_id);

// Dialogue system management
void dialogue_system_init(DialogueSystem *system);
void dialogue_system_free(DialogueSystem *system);
void dialogue_system_register_tree(DialogueSystem *system, DialogueTree *tree);
DialogueTree *dialogue_system_get_tree(DialogueSystem *system, u32 tree_id);

// Conversation management
bool dialogue_start(DialogueSystem *system, Entity npc_entity, Entity player_entity, 
                   u32 tree_id);
void dialogue_end(DialogueSystem *system, Entity player_entity);
bool dialogue_select_option(DialogueSystem *system, Entity player_entity, 
                           u32 option_id);
DialogueNode *dialogue_get_current_node(DialogueSystem *system, Entity player_entity);
DialogueState *dialogue_get_active_state(DialogueSystem *system, Entity player_entity);

// Condition and consequence evaluation
bool dialogue_has_condition(DialogueCondition *condition);
void dialogue_execute_consequence(DialogueConsequence *consequence, Entity npc_entity, 
                                Entity player_entity);

// Utility functions
bool dialogue_is_conversation_active(DialogueSystem *system, Entity player_entity);
u32 dialogue_get_active_npc_count(DialogueSystem *system);

#ifdef __cplusplus
}
#endif

#endif // AI_NPC_DIALOGUE_H
