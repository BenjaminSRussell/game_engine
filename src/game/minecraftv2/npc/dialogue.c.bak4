// NPC dialogue system implementation.
#include <core/logger.h>
#include <core/memory.h>
#include <ecs/component_ids.h>
#include <npc/dialogue.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

  system->loaded_trees[system->tree_count] = tree;
  system->tree_count++;

  LOG_DEBUG("Registered dialogue tree %u for NPC: %s", tree->tree_id,
            tree->npc_name);
}

DialogueTree *dialogue_system_get_tree(DialogueSystem *system, u32 tree_id) {
  if (!system)
    return NULL;

  for (u32 i = 0; i < system->tree_count; i++) {
    if (system->loaded_trees[i] &&
        system->loaded_trees[i]->tree_id == tree_id) {
      return system->loaded_trees[i];
    }
  }

  return NULL;
}

bool dialogue_start(DialogueSystem *system, struct ECSWorld *ecs,
                    EntityID npc_entity, EntityID player_entity) {
  if (!system || !ecs || system->current_conversation.is_active) {
    return false;
  }

  // Get NPC component to find dialogue tree
  // In a full implementation, this would query the NPC component
  // For now, we'll use a placeholder tree ID
  u32 tree_id = 0; // Would be retrieved from NPC component

  DialogueTree *tree = dialogue_system_get_tree(system, tree_id);
  if (!tree) {
    LOG_DEBUG("No dialogue tree found for NPC entity %u", npc_entity);
    return false;
  }

  // Initialize conversation state
  memset(&system->current_conversation, 0, sizeof(DialogueState));
  system->current_conversation.npc_entity = npc_entity;
  system->current_conversation.player_entity = player_entity;
  system->current_conversation.current_tree = tree;
  system->current_conversation.current_node_id = tree->entry_node_id;
  system->current_conversation.is_active = true;
  system->current_conversation.conversation_start_time =
      (f32)clock() / CLOCKS_PER_SEC;
  system->current_conversation.can_continue = true;

  // If greeting is random, select a random greeting node
  if (tree->is_greeting_random) {
    // Find all greeting nodes (nodes with ID < 10 are typically greetings)
    u32 greeting_count = 0;
    u32 greeting_nodes[10];

    for (u32 i = 0; i < tree->node_count && i < 10; i++) {
      greeting_nodes[greeting_count++] = i;
    }

    if (greeting_count > 0) {
      u32 random_index = rand() % greeting_count;
      system->current_conversation.current_node_id =
          greeting_nodes[random_index];
    }
  }

  LOG_DEBUG("Started dialogue with NPC entity %u, starting at node %u",
            npc_entity, system->current_conversation.current_node_id);

  return true;
}

void dialogue_end(DialogueSystem *system) {
  if (!system || !system->current_conversation.is_active) {
    return;
  }

  LOG_DEBUG("Ended dialogue with NPC entity %u",
            system->current_conversation.npc_entity);

  memset(&system->current_conversation, 0, sizeof(DialogueState));
}

bool dialogue_select_option(DialogueSystem *system, u32 option_id) {
  if (!system || !system->current_conversation.is_active ||
      !system->current_conversation.can_continue) {
    return false;
  }

  DialogueNode *current_node = dialogue_get_current_node(system);
  if (!current_node) {
    return false;
  }

  // Find the selected option
  DialogueOption *selected_option = NULL;
  for (u32 i = 0; i < current_node->option_count; i++) {
    if (current_node->options[i].option_id == option_id &&
        current_node->options[i].is_available) {
      selected_option = &current_node->options[i];
      break;
    }
  }

  if (!selected_option) {
    LOG_DEBUG("Invalid dialogue option %u selected", option_id);
    return false;
  }

  // Execute consequences
  for (u32 i = 0; i < selected_option->consequence_count; i++) {
    dialogue_execute_consequence(&selected_option->consequences[i],
                                 NULL, // ECS world would be passed here
                                 system->current_conversation.npc_entity,
                                 system->current_conversation.player_entity);
  }

  // Move to target node
  if (selected_option->target_node_id <
      system->current_conversation.current_tree->node_count) {
    system->current_conversation.current_node_id =
        selected_option->target_node_id;
    system->current_conversation.nodes_visited++;

    // Check if new node is an exit node
    DialogueNode *new_node = dialogue_get_current_node(system);
    if (new_node && new_node->is_exit_node) {
      system->current_conversation.can_continue = false;
      dialogue_end(system);
    }

    LOG_DEBUG("Moved to dialogue node %u",
              system->current_conversation.current_node_id);
    return true;
  }

  return false;
}

DialogueNode *dialogue_get_current_node(DialogueSystem *system) {
  if (!system || !system->current_conversation.is_active ||
      !system->current_conversation.current_tree) {
    return NULL;
  }

  return dialogue_tree_get_node(system->current_conversation.current_tree,
                                system->current_conversation.current_node_id);
}

DialogueState *dialogue_get_current_state(DialogueSystem *system) {
  if (!system || !system->current_conversation.is_active) {
    return NULL;
  }

  return &system->current_conversation;
}

bool dialogue_evaluate_condition(const DialogueCondition *condition,
                                 struct ECSWorld *ecs, EntityID npc_entity,
                                 EntityID player_entity) {
  if (!condition)
    return true;

  switch (condition->type) {
  case DIALOGUE_CONDITION_NONE:
    return true;

  case DIALOGUE_CONDITION_TIME_OF_DAY:
    // In a full implementation, this would check game time
    // For now, return true as placeholder
    return true;

  case DIALOGUE_CONDITION_WEATHER:
    // In a full implementation, this would check current weather
    return true;

  case DIALOGUE_CONDITION_PLAYER_LEVEL:
    // In a full implementation, this would check player level
    return true;

  case DIALOGUE_CONDITION_QUEST_STATUS:
    // In a full implementation, this would check quest status
    return true;

  case DIALOGUE_CONDITION_INVENTORY_ITEM:
    // In a full implementation, this would check player inventory
    return true;

  case DIALOGUE_CONDITION_PLAYER_HEALTH:
    // In a full implementation, this would check player health
    return true;

  case DIALOGUE_CONDITION_NPC_RELATIONSHIP:
    // In a full implementation, this would check NPC relationship
    return true;

  case DIALOGUE_CONDITION_RANDOM_CHANCE:
    return ((f32)rand() / RAND_MAX) <
           condition->data.random_chance.success_chance;

  default:
    return true;
  }
}

bool dialogue_evaluate_option_availability(const DialogueOption *option,
                                           struct ECSWorld *ecs,
                                           EntityID npc_entity,
                                           EntityID player_entity) {
  if (!option)
    return false;

  // All conditions must be met for option to be available
  for (u32 i = 0; i < option->condition_count; i++) {
    if (!dialogue_evaluate_condition(&option->conditions[i], ecs, npc_entity,
                                     player_entity)) {
      return false;
    }
  }

  return true;
}

void dialogue_execute_consequence(const DialogueConsequence *consequence,
                                  struct ECSWorld *ecs, EntityID npc_entity,
                                  EntityID player_entity) {
  if (!consequence)
    return;

  switch (consequence->type) {
  case DIALOGUE_CONSEQUENCE_NONE:
    break;

  case DIALOGUE_CONSEQUENCE_GIVE_ITEM:
    LOG_DEBUG("Give item %u (count: %u) to player",
              consequence->data.give_item.item_id,
              consequence->data.give_item.count);
    // In a full implementation, this would add item to player inventory
    break;

  case DIALOGUE_CONSEQUENCE_TAKE_ITEM:
    LOG_DEBUG("Take item %u (count: %u) from player",
              consequence->data.take_item.item_id,
              consequence->data.take_item.count);
    // In a full implementation, this would remove item from player inventory
    break;

  case DIALOGUE_CONSEQUENCE_START_QUEST:
    LOG_DEBUG("Start quest %u", consequence->data.quest.quest_id);
    // In a full implementation, this would start the specified quest
    break;

  case DIALOGUE_CONSEQUENCE_COMPLETE_QUEST:
    LOG_DEBUG("Complete quest %u", consequence->data.quest.quest_id);
    // In a full implementation, this would complete the specified quest
    break;

  case DIALOGUE_CONSEQUENCE_CHANGE_RELATIONSHIP:
    LOG_DEBUG("Change relationship by %.2f",
              consequence->data.relationship.relationship_change);
    // In a full implementation, this would modify NPC relationship value
    break;

  case DIALOGUE_CONSEQUENCE_GIVE_EXPERIENCE:
    LOG_DEBUG("Give %u experience to player",
              consequence->data.experience.experience_amount);
    // In a full implementation, this would add experience to player
    break;

  case DIALOGUE_CONSEQUENCE_TRIGGER_EVENT:
    LOG_DEBUG("Trigger world event %u",
              consequence->data.trigger_event.event_id);
    // In a full implementation, this would trigger a world event
    break;

  case DIALOGUE_CONSEQUENCE_TELEPORT:
    LOG_DEBUG("Teleport player to (%.2f, %.2f, %.2f)",
              consequence->data.teleport.position.x,
              consequence->data.teleport.position.y,
              consequence->data.teleport.position.z);
    // In a full implementation, this would teleport the player
    break;

  default:
    break;
  }
}

const char *dialogue_get_condition_name(DialogueConditionType type) {
  switch (type) {
  case DIALOGUE_CONDITION_NONE:
    return "None";
  case DIALOGUE_CONDITION_TIME_OF_DAY:
    return "Time of Day";
  case DIALOGUE_CONDITION_WEATHER:
    return "Weather";
  case DIALOGUE_CONDITION_PLAYER_LEVEL:
    return "Player Level";
  case DIALOGUE_CONDITION_QUEST_STATUS:
    return "Quest Status";
  case DIALOGUE_CONDITION_INVENTORY_ITEM:
    return "Inventory Item";
  case DIALOGUE_CONDITION_PLAYER_HEALTH:
    return "Player Health";
  case DIALOGUE_CONDITION_NPC_RELATIONSHIP:
    return "NPC Relationship";
  case DIALOGUE_CONDITION_RANDOM_CHANCE:
    return "Random Chance";
  default:
    return "Unknown";
  }
}

const char *dialogue_get_consequence_name(DialogueConsequenceType type) {
  switch (type) {
  case DIALOGUE_CONSEQUENCE_NONE:
    return "None";
  case DIALOGUE_CONSEQUENCE_GIVE_ITEM:
    return "Give Item";
  case DIALOGUE_CONSEQUENCE_TAKE_ITEM:
    return "Take Item";
  case DIALOGUE_CONSEQUENCE_START_QUEST:
    return "Start Quest";
  case DIALOGUE_CONSEQUENCE_COMPLETE_QUEST:
    return "Complete Quest";
  case DIALOGUE_CONSEQUENCE_CHANGE_RELATIONSHIP:
    return "Change Relationship";
  case DIALOGUE_CONSEQUENCE_GIVE_EXPERIENCE:
    return "Give Experience";
  case DIALOGUE_CONSEQUENCE_TRIGGER_EVENT:
    return "Trigger Event";
  case DIALOGUE_CONSEQUENCE_TELEPORT:
    return "Teleport";
  default:
    return "Unknown";
  }
}

void dialogue_debug_print_tree(const DialogueTree *tree) {
  if (!tree)
    return;

  LOG_DEBUG("Dialogue Tree: %s (ID: %u)", tree->npc_name, tree->tree_id);
  LOG_DEBUG("Entry Node: %u, Nodes: %u, Random Greeting: %s",
            tree->entry_node_id, tree->node_count,
            tree->is_greeting_random ? "Yes" : "No");

  for (u32 i = 0; i < tree->node_count; i++) {
    const DialogueNode *node = &tree->nodes[i];
    LOG_DEBUG("Node %u: %s (%s) - Exit: %s", node->node_id, node->text,
              node->speaker_name, node->is_exit_node ? "Yes" : "No");

    for (u32 j = 0; j < node->option_count; j++) {
      const DialogueOption *option = &node->options[j];
      LOG_DEBUG("  Option %u: %s -> Node %u", option->option_id, option->text,
                option->target_node_id);
    }
  }
}

void dialogue_debug_print_current_state(const DialogueSystem *system) {
  if (!system || !system->current_conversation.is_active) {
    LOG_DEBUG("No active dialogue conversation");
    return;
  }

  const DialogueState *state = &system->current_conversation;
  LOG_DEBUG("Active Dialogue State:");
  LOG_DEBUG("  NPC Entity: %u", state->npc_entity);
  LOG_DEBUG("  Player Entity: %u", state->player_entity);
  LOG_DEBUG("  Current Node: %u", state->current_node_id);
  LOG_DEBUG("  Nodes Visited: %u", state->nodes_visited);
  LOG_DEBUG("  Can Continue: %s", state->can_continue ? "Yes" : "No");

  f32 duration = (f32)clock() / CLOCKS_PER_SEC - state->conversation_start_time;
  LOG_DEBUG("  Duration: %.2f seconds", duration);
}

// Helper functions for creating common dialogue elements
DialogueCondition dialogue_create_time_condition(u32 start_hour, u32 end_hour) {
  DialogueCondition condition = {0};
  condition.type = DIALOGUE_CONDITION_TIME_OF_DAY;
  condition.data.time_of_day.start_hour = start_hour;
  condition.data.time_of_day.end_hour = end_hour;
  return condition;
}

DialogueCondition dialogue_create_weather_condition(u32 weather_type) {
  DialogueCondition condition = {0};
  condition.type = DIALOGUE_CONDITION_WEATHER;
  condition.data.weather.weather_type = weather_type;
  return condition;
}

DialogueCondition dialogue_create_level_condition(u32 required_level) {
  DialogueCondition condition = {0};
  condition.type = DIALOGUE_CONDITION_PLAYER_LEVEL;
  condition.data.player_level.required_level = required_level;
  return condition;
}

DialogueCondition dialogue_create_quest_condition(u32 quest_id,
                                                  u32 required_status) {
  DialogueCondition condition = {0};
  condition.type = DIALOGUE_CONDITION_QUEST_STATUS;
  condition.data.quest_status.quest_id = quest_id;
  condition.data.quest_status.required_status = required_status;
  return condition;
}

DialogueCondition dialogue_create_item_condition(u32 item_id,
                                                 u32 required_count) {
  DialogueCondition condition = {0};
  condition.type = DIALOGUE_CONDITION_INVENTORY_ITEM;
  condition.data.inventory_item.item_id = item_id;
  condition.data.inventory_item.required_count = required_count;
  return condition;
}

DialogueCondition dialogue_create_health_condition(f32 min_health_percentage) {
  DialogueCondition condition = {0};
  condition.type = DIALOGUE_CONDITION_PLAYER_HEALTH;
  condition.data.player_health.min_health_percentage = min_health_percentage;
  return condition;
}

DialogueCondition
dialogue_create_relationship_condition(f32 required_relationship) {
  DialogueCondition condition = {0};
  condition.type = DIALOGUE_CONDITION_NPC_RELATIONSHIP;
  condition.data.npc_relationship.required_relationship = required_relationship;
  return condition;
}

DialogueCondition dialogue_create_random_chance_condition(f32 success_chance) {
  DialogueCondition condition = {0};
  condition.type = DIALOGUE_CONDITION_RANDOM_CHANCE;
  condition.data.random_chance.success_chance =
      CLAMP(success_chance, 0.0f, 1.0f);
  return condition;
}

DialogueConsequence dialogue_create_give_item_consequence(u32 item_id,
                                                          u32 count) {
  DialogueConsequence consequence = {0};
  consequence.type = DIALOGUE_CONSEQUENCE_GIVE_ITEM;
  consequence.data.give_item.item_id = item_id;
  consequence.data.give_item.count = count;
  return consequence;
}

DialogueConsequence dialogue_create_take_item_consequence(u32 item_id,
                                                          u32 count) {
  DialogueConsequence consequence = {0};
  consequence.type = DIALOGUE_CONSEQUENCE_TAKE_ITEM;
  consequence.data.take_item.item_id = item_id;
  consequence.data.take_item.count = count;
  return consequence;
}

DialogueConsequence dialogue_create_start_quest_consequence(u32 quest_id) {
  DialogueConsequence consequence = {0};
  consequence.type = DIALOGUE_CONSEQUENCE_START_QUEST;
  consequence.data.quest.quest_id = quest_id;
  return consequence;
}

DialogueConsequence dialogue_create_complete_quest_consequence(u32 quest_id) {
  DialogueConsequence consequence = {0};
  consequence.type = DIALOGUE_CONSEQUENCE_COMPLETE_QUEST;
  consequence.data.quest.quest_id = quest_id;
  return consequence;
}

DialogueConsequence
dialogue_create_relationship_consequence(f32 relationship_change) {
  DialogueConsequence consequence = {0};
  consequence.type = DIALOGUE_CONSEQUENCE_CHANGE_RELATIONSHIP;
  consequence.data.relationship.relationship_change = relationship_change;
  return consequence;
}

DialogueConsequence
dialogue_create_experience_consequence(u32 experience_amount) {
  DialogueConsequence consequence = {0};
  consequence.type = DIALOGUE_CONSEQUENCE_GIVE_EXPERIENCE;
  consequence.data.experience.experience_amount = experience_amount;
  return consequence;
}

DialogueConsequence dialogue_create_teleport_consequence(Vec3 position) {
  DialogueConsequence consequence = {0};
  consequence.type = DIALOGUE_CONSEQUENCE_TELEPORT;
  consequence.data.teleport.position = position;
  return consequence;
}
