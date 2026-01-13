// src/engine/ai/npc_dialogue.c
//
// Purpose: Implements the NPC dialogue system with support for branching conversations,
// conditional responses, and consequence execution. This system manages dialogue trees,
// active conversations, and integrates with other game systems like quests, inventory,
// and relationships.
//
// Key Features:
// - Branching dialogue trees with multiple choice options
// - Conditional responses based on game state, time, weather, player stats
// - Consequence system for item transfers, quest triggers, relationship changes
// - Support for multiple simultaneous conversations
// - Randomized greetings and text variants
// - Conversation history tracking
//
#include <ai/npc_dialogue.h>
#include <ai/npc.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// External system references
extern struct World *g_world;
extern f32 g_game_time;

// Dialogue tree management
void dialogue_tree_init(DialogueTree *tree, u32 tree_id, const char *npc_name) {
  if (!tree || !npc_name)
    return;

  memset(tree, 0, sizeof(DialogueTree));
  tree->tree_id = tree_id;
  strncpy(tree->npc_name, npc_name, sizeof(tree->npc_name) - 1);
  tree->entry_node_id = 0;
  tree->is_greeting_random = false;

  LOG_DEBUG("Dialogue tree initialized for NPC: %s", npc_name);
}

void dialogue_tree_free(DialogueTree *tree) {
  if (!tree)
    return;

  memset(tree, 0, sizeof(DialogueTree));
  LOG_DEBUG("Dialogue tree freed");
}

u32 dialogue_tree_add_node(DialogueTree *tree, const DialogueNode *node) {
  if (!tree || !node || tree->node_count >= MAX_DIALOGUE_NODES) {
    return 0;
  }

  tree->nodes[tree->node_count] = *node;
  tree->nodes[tree->node_count].node_id = tree->node_count;
  tree->node_count++;

  LOG_DEBUG("Added dialogue node %u to tree %s", tree->node_count - 1,
            tree->npc_name);
  return tree->node_count - 1;
}

u32 dialogue_tree_add_option(DialogueTree *tree, u32 node_id,
                             const DialogueOption *option) {
  if (!tree || !option || node_id >= tree->node_count) {
    return 0;
  }

  DialogueNode *node = &tree->nodes[node_id];
  if (node->option_count >= MAX_DIALOGUE_OPTIONS) {
    return 0;
  }

  node->options[node->option_count] = *option;
  node->options[node->option_count].option_id = node->option_count;
  node->option_count++;

  LOG_DEBUG("Added dialogue option %u to node %u in tree %s",
            node->option_count - 1, node_id, tree->npc_name);
  return node->option_count - 1;
}

DialogueNode *dialogue_tree_get_node(DialogueTree *tree, u32 node_id) {
  if (!tree || node_id >= tree->node_count) {
    return NULL;
  }

  return &tree->nodes[node_id];
}

// Dialogue system management
void dialogue_system_init(DialogueSystem *system) {
  if (!system)
    return;

  memset(system, 0, sizeof(DialogueSystem));
  system->is_initialized = true;

  LOG_DEBUG("Dialogue system initialized");
}

void dialogue_system_free(DialogueSystem *system) {
  if (!system)
    return;

  // Free all loaded trees
  for (u32 i = 0; i < system->tree_count; i++) {
    if (system->loaded_trees[i]) {
      dialogue_tree_free(system->loaded_trees[i]);
      free(system->loaded_trees[i]);
      system->loaded_trees[i] = NULL;
    }
  }

  memset(system, 0, sizeof(DialogueSystem));
  LOG_DEBUG("Dialogue system freed");
}

void dialogue_system_register_tree(DialogueSystem *system, DialogueTree *tree) {
  if (!system || !tree || system->tree_count >= MAX_DIALOGUE_NODES) {
    return;
  }

  system->loaded_trees[system->tree_count++] = tree;
  LOG_DEBUG("Registered dialogue tree %u for NPC %s", tree->tree_id, tree->npc_name);
}

DialogueTree *dialogue_system_get_tree(DialogueSystem *system, u32 tree_id) {
  if (!system)
    return NULL;

  for (u32 i = 0; i < system->tree_count; i++) {
    if (system->loaded_trees[i] && system->loaded_trees[i]->tree_id == tree_id) {
      return system->loaded_trees[i];
    }
  }

  return NULL;
}

// Find an active conversation state for a player
static DialogueState *find_conversation_state(DialogueSystem *system, Entity player_entity) {
  for (u32 i = 0; i < system->active_count; i++) {
    if (system->active_conversations[i].player_entity.id == player_entity.id) {
      return &system->active_conversations[i];
    }
  }
  return NULL;
}

// Start a conversation with an NPC
bool dialogue_start(DialogueSystem *system, Entity npc_entity, Entity player_entity, 
                   u32 tree_id) {
  if (!system || !g_world)
    return false;

  // Check if player is already in a conversation
  if (dialogue_is_conversation_active(system, player_entity)) {
    LOG_WARN("Player %u is already in a conversation", player_entity.id);
    return false;
  }

  // Get the dialogue tree
  DialogueTree *tree = dialogue_system_get_tree(system, tree_id);
  if (!tree) {
    LOG_ERROR("Dialogue tree %u not found", tree_id);
    return false;
  }

  // Check if we have room for another conversation
  if (system->active_count >= 16) {
    LOG_ERROR("Too many active conversations");
    return false;
  }

  // Create new conversation state
  DialogueState *state = &system->active_conversations[system->active_count++];
  memset(state, 0, sizeof(DialogueState));
  
  state->npc_entity = npc_entity;
  state->player_entity = player_entity;
  state->tree = tree;
  state->current_node_id = tree->entry_node_id;
  state->is_active = true;
  state->start_time = g_game_time;

  // Set NPC to dialogue mode
  NPCSystem *npc_system = NULL; // TODO: Get NPC system reference
  if (npc_system) {
    npc_begin_dialogue(npc_system, npc_entity, player_entity);
  }

  LOG_INFO("Started dialogue between NPC %u and player %u using tree %u", 
           npc_entity.id, player_entity.id, tree_id);
  return true;
}

// End a conversation
void dialogue_end(DialogueSystem *system, Entity player_entity) {
  if (!system)
    return;

  DialogueState *state = find_conversation_state(system, player_entity);
  if (!state) {
    LOG_WARN("No active conversation found for player %u", player_entity.id);
    return;
  }

  // Remove NPC from dialogue mode
  NPCSystem *npc_system = NULL; // TODO: Get NPC system reference
  if (npc_system) {
    npc_end_dialogue(npc_system, state->npc_entity);
  }

  // Remove conversation state by shifting remaining conversations
  u32 index = state - system->active_conversations;
  for (u32 i = index; i < system->active_count - 1; i++) {
    system->active_conversations[i] = system->active_conversations[i + 1];
  }
  system->active_count--;

  LOG_INFO("Ended dialogue between NPC %u and player %u", 
           state->npc_entity.id, player_entity.id);
}

// Select a dialogue option
bool dialogue_select_option(DialogueSystem *system, Entity player_entity, 
                           u32 option_id) {
  if (!system)
    return false;

  DialogueState *state = find_conversation_state(system, player_entity);
  if (!state) {
    LOG_WARN("No active conversation found for player %u", player_entity.id);
    return false;
  }

  DialogueNode *current_node = dialogue_tree_get_node(state->tree, state->current_node_id);
  if (!current_node) {
    LOG_ERROR("Current dialogue node %u not found", state->current_node_id);
    return false;
  }

  // Find the selected option
  DialogueOption *selected_option = NULL;
  for (u32 i = 0; i < current_node->option_count; i++) {
    if (current_node->options[i].option_id == option_id) {
      selected_option = &current_node->options[i];
      break;
    }
  }

  if (!selected_option) {
    LOG_WARN("Dialogue option %u not found in node %u", option_id, state->current_node_id);
    return false;
  }

  // Check if all conditions are met
  for (u32 i = 0; i < selected_option->condition_count; i++) {
    if (!dialogue_has_condition(&selected_option->conditions[i])) {
      LOG_DEBUG("Dialogue condition %u not met", i);
      return false;
    }
  }

  // Execute consequences
  for (u32 i = 0; i < selected_option->consequence_count; i++) {
    dialogue_execute_consequence(&selected_option->consequences[i], 
                                state->npc_entity, player_entity);
  }

  // Add to history
  if (state->history_count < MAX_DIALOGUE_NODES) {
    state->history[state->history_count++] = state->current_node_id;
  }

  // Move to target node or end conversation
  if (selected_option->target_node_id < state->tree->node_count) {
    state->current_node_id = selected_option->target_node_id;
    
    // Check if new node is an exit node
    DialogueNode *new_node = dialogue_tree_get_node(state->tree, state->current_node_id);
    if (new_node && new_node->is_exit_node) {
      dialogue_end(system, player_entity);
    }
  } else {
    // Invalid target node, end conversation
    dialogue_end(system, player_entity);
  }

  LOG_DEBUG("Selected dialogue option %u, moved to node %u", 
            option_id, state->current_node_id);
  return true;
}

// Get current dialogue node for a player
DialogueNode *dialogue_get_current_node(DialogueSystem *system, Entity player_entity) {
  DialogueState *state = find_conversation_state(system, player_entity);
  if (!state) {
    return NULL;
  }

  return dialogue_tree_get_node(state->tree, state->current_node_id);
}

// Get active conversation state
DialogueState *dialogue_get_active_state(DialogueSystem *system, Entity player_entity) {
  return find_conversation_state(system, player_entity);
}

// Check if a condition is met
bool dialogue_has_condition(DialogueCondition *condition) {
  if (!condition || condition->type == DIALOGUE_CONDITION_NONE) {
    return true;
  }

  switch (condition->type) {
  case DIALOGUE_CONDITION_TIME_OF_DAY: {
    // TODO: Get current game time
    u32 current_hour = (u32)(g_game_time / 3600.0f) % 24;
    return (current_hour >= condition->data.time_of_day.start_hour && 
            current_hour <= condition->data.time_of_day.end_hour);
  }
  
  case DIALOGUE_CONDITION_RANDOM_CHANCE: {
    f32 roll = (float)rand() / (float)RAND_MAX;
    return roll <= condition->data.random_chance.chance;
  }
  
  case DIALOGUE_CONDITION_PLAYER_HEALTH: {
    // TODO: Get player health component
    return true; // Placeholder
  }
  
  case DIALOGUE_CONDITION_NPC_RELATIONSHIP: {
    // TODO: Get NPC relationship with player
    return true; // Placeholder
  }
  
  case DIALOGUE_CONDITION_INVENTORY_ITEM: {
    // TODO: Check player inventory
    return true; // Placeholder
  }
  
  case DIALOGUE_CONDITION_QUEST_STATUS: {
    // TODO: Check quest system
    return true; // Placeholder
  }
  
  case DIALOGUE_CONDITION_WEATHER: {
    // TODO: Get weather system
    return true; // Placeholder
  }
  
  case DIALOGUE_CONDITION_PLAYER_LEVEL: {
    // TODO: Get player level/experience
    return true; // Placeholder
  }
  
  default:
    return true;
  }
}

// Execute a dialogue consequence
void dialogue_execute_consequence(DialogueConsequence *consequence, Entity npc_entity, 
                                Entity player_entity) {
  if (!consequence || consequence->type == DIALOGUE_CONSEQUENCE_NONE) {
    return;
  }

  switch (consequence->type) {
  case DIALOGUE_CONSEQUENCE_GIVE_ITEM: {
    // TODO: Add item to player inventory
    LOG_DEBUG("Give item %u (count %u) to player %u", 
              consequence->data.give_item.item_id, 
              consequence->data.give_item.count, player_entity.id);
    break;
  }
  
  case DIALOGUE_CONSEQUENCE_TAKE_ITEM: {
    // TODO: Remove item from player inventory
    LOG_DEBUG("Take item %u (count %u) from player %u", 
              consequence->data.take_item.item_id, 
              consequence->data.take_item.count, player_entity.id);
    break;
  }
  
  case DIALOGUE_CONSEQUENCE_START_QUEST: {
    // TODO: Start quest for player
    LOG_DEBUG("Start quest %u for player %u", 
              consequence->data.start_quest.quest_id, player_entity.id);
    break;
  }
  
  case DIALOGUE_CONSEQUENCE_COMPLETE_QUEST: {
    // TODO: Complete quest for player
    LOG_DEBUG("Complete quest %u for player %u", 
              consequence->data.complete_quest.quest_id, player_entity.id);
    break;
  }
  
  case DIALOGUE_CONSEQUENCE_CHANGE_RELATIONSHIP: {
    // TODO: Change NPC relationship with player
    LOG_DEBUG("Change relationship by %d between NPC %u and player %u", 
              consequence->data.change_relationship.relationship_change,
              npc_entity.id, player_entity.id);
    break;
  }
  
  case DIALOGUE_CONSEQUENCE_GIVE_EXPERIENCE: {
    // TODO: Give experience to player
    LOG_DEBUG("Give %u experience to player %u", 
              consequence->data.give_experience.experience_amount, player_entity.id);
    break;
  }
  
  case DIALOGUE_CONSEQUENCE_TRIGGER_EVENT: {
    // TODO: Trigger world event
    LOG_DEBUG("Trigger event %u", consequence->data.trigger_event.event_id);
    break;
  }
  
  case DIALOGUE_CONSEQUENCE_TELEPORT: {
    // TODO: Teleport player
    LOG_DEBUG("Teleport player %u to (%.1f, %.1f, %.1f)", 
              player_entity.id, 
              consequence->data.teleport.position.x,
              consequence->data.teleport.position.y,
              consequence->data.teleport.position.z);
    break;
  }
  
  default:
    break;
  }
}

// Utility functions
bool dialogue_is_conversation_active(DialogueSystem *system, Entity player_entity) {
  return find_conversation_state(system, player_entity) != NULL;
}

u32 dialogue_get_active_npc_count(DialogueSystem *system) {
  return system->active_count;
}
