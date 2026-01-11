// dialogue_system.c - Implementation
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_DIALOGUE_NODES 512
#define MAX_DIALOGUE_CHOICES 8
#define MAX_DIALOGUE_TEXT 1024
#define MAX_DIALOGUE_CONDITIONS 4

typedef enum {
  DIALOGUE_CONDITION_QUEST_COMPLETED,
  DIALOGUE_CONDITION_QUEST_ACTIVE,
  DIALOGUE_CONDITION_ITEM_HAS,
  DIALOGUE_CONDITION_SKILL_LEVEL,
  DIALOGUE_CONDITION_REPUTATION,
  DIALOGUE_CONDITION_CUSTOM
} DialogueConditionType;

typedef struct {
  DialogueConditionType type;
  u32 parameter1;
  u32 parameter2;
  bool required;
} DialogueCondition;

typedef struct {
  char text[MAX_DIALOGUE_TEXT];
  char speaker[64];
  u32 choice_count;
  u32 choice_indices[MAX_DIALOGUE_CHOICES];
  u32 condition_count;
  DialogueCondition conditions[MAX_DIALOGUE_CONDITIONS];
  u32 next_node_default;
  bool is_end_node;
} DialogueNode;

typedef struct {
  DialogueNode nodes[MAX_DIALOGUE_NODES];
  u32 node_count;
  u32 start_node;
  char dialogue_id[64];
  u32 npc_id;
} DialogueTree;

typedef struct {
  DialogueTree *dialogue;
  u32 current_node;
  bool is_active;
  u32 participant_entity;
} DialogueSession;

static DialogueTree g_dialogue_database[MAX_DIALOGUE_NODES];
static u32 g_dialogue_count = 0;
static DialogueSession g_current_session = {0};

static bool dialogue_check_node_conditions(u32 node_index);

bool dialogue_init(void) {
  memset(g_dialogue_database, 0, sizeof(g_dialogue_database));
  memset(&g_current_session, 0, sizeof(g_current_session));
  g_dialogue_count = 0;
  
  LOG_INFO("Dialogue system initialized");
  return true;
}

void dialogue_shutdown(void) {
  memset(g_dialogue_database, 0, sizeof(g_dialogue_database));
  memset(&g_current_session, 0, sizeof(g_current_session));
  g_dialogue_count = 0;
  
  LOG_INFO("Dialogue system shutdown");
}

u32 dialogue_create_tree(const char *dialogue_id, u32 npc_id) {
  if (!dialogue_id || g_dialogue_count >= MAX_DIALOGUE_NODES) {
    return 0;
  }
  
  DialogueTree *tree = &g_dialogue_database[g_dialogue_count];
  memset(tree, 0, sizeof(DialogueTree));
  
  strncpy(tree->dialogue_id, dialogue_id, 63);
  tree->dialogue_id[63] = '\0';
  tree->npc_id = npc_id;
  tree->start_node = 0;
  
  return ++g_dialogue_count;
}

bool dialogue_add_node(u32 dialogue_index, const char *text, const char *speaker) {
  if (dialogue_index == 0 || dialogue_index > g_dialogue_count) {
    return false;
  }
  
  DialogueTree *tree = &g_dialogue_database[dialogue_index - 1];
  if (tree->node_count >= MAX_DIALOGUE_NODES) {
    return false;
  }
  
  DialogueNode *node = &tree->nodes[tree->node_count];
  memset(node, 0, sizeof(DialogueNode));
  
  if (text) strncpy(node->text, text, MAX_DIALOGUE_TEXT - 1);
  if (speaker) strncpy(node->speaker, speaker, 63);
  node->speaker[63] = '\0';
  node->next_node_default = tree->node_count + 1;
  
  tree->node_count++;
  return true;
}

bool dialogue_add_choice(u32 dialogue_index, u32 node_index, u32 choice_node_index) {
  if (dialogue_index == 0 || dialogue_index > g_dialogue_count) {
    return false;
  }
  
  DialogueTree *tree = &g_dialogue_database[dialogue_index - 1];
  if (node_index >= tree->node_count || choice_node_index >= tree->node_count) {
    return false;
  }
  
  DialogueNode *node = &tree->nodes[node_index];
  if (node->choice_count >= MAX_DIALOGUE_CHOICES) {
    return false;
  }
  
  node->choice_indices[node->choice_count++] = choice_node_index;
  return true;
}

bool dialogue_add_condition(u32 dialogue_index, u32 node_index, 
                           DialogueConditionType type, u32 param1, u32 param2) {
  if (dialogue_index == 0 || dialogue_index > g_dialogue_count) {
    return false;
  }
  
  DialogueTree *tree = &g_dialogue_database[dialogue_index - 1];
  if (node_index >= tree->node_count) {
    return false;
  }
  
  DialogueNode *node = &tree->nodes[node_index];
  if (node->condition_count >= MAX_DIALOGUE_CONDITIONS) {
    return false;
  }
  
  DialogueCondition *cond = &node->conditions[node->condition_count++];
  cond->type = type;
  cond->parameter1 = param1;
  cond->parameter2 = param2;
  cond->required = true;
  
  return true;
}

bool dialogue_start_conversation(u32 dialogue_index, u32 participant_entity) {
  if (dialogue_index == 0 || dialogue_index > g_dialogue_count) {
    LOG_ERROR("Invalid dialogue index: %d", dialogue_index);
    return false;
  }
  
  DialogueTree *tree = &g_dialogue_database[dialogue_index - 1];
  if (tree->node_count == 0) {
    LOG_ERROR("Dialogue tree has no nodes");
    return false;
  }
  
  g_current_session.dialogue = tree;
  g_current_session.current_node = tree->start_node;
  g_current_session.is_active = true;
  g_current_session.participant_entity = participant_entity;
  
  LOG_INFO("Started dialogue conversation: %s", tree->dialogue_id);
  return true;
}

void dialogue_end_conversation(void) {
  memset(&g_current_session, 0, sizeof(g_current_session));
  LOG_INFO("Ended dialogue conversation");
}

bool dialogue_is_active(void) {
  return g_current_session.is_active;
}

const char* dialogue_get_current_text(void) {
  if (!g_current_session.is_active || !g_current_session.dialogue) {
    return NULL;
  }
  
  DialogueNode *node = &g_current_session.dialogue->nodes[g_current_session.current_node];
  return node->text;
}

const char* dialogue_get_current_speaker(void) {
  if (!g_current_session.is_active || !g_current_session.dialogue) {
    return NULL;
  }
  
  DialogueNode *node = &g_current_session.dialogue->nodes[g_current_session.current_node];
  return node->speaker;
}

u32 dialogue_get_choice_count(void) {
  if (!g_current_session.is_active || !g_current_session.dialogue) {
    return 0;
  }
  
  DialogueNode *node = &g_current_session.dialogue->nodes[g_current_session.current_node];
  
  // Filter choices by conditions
  u32 valid_choices = 0;
  for (u32 i = 0; i < node->choice_count; i++) {
    u32 choice_node = node->choice_indices[i];
    if (dialogue_check_node_conditions(choice_node)) {
      valid_choices++;
    }
  }
  
  return valid_choices;
}

bool dialogue_get_choice_text(u32 choice_index, char *out_text, u32 max_length) {
  if (!g_current_session.is_active || !g_current_session.dialogue || !out_text) {
    return false;
  }
  
  DialogueNode *node = &g_current_session.dialogue->nodes[g_current_session.current_node];
  
  // Find the Nth valid choice
  u32 valid_count = 0;
  for (u32 i = 0; i < node->choice_count; i++) {
    u32 choice_node = node->choice_indices[i];
    if (dialogue_check_node_conditions(choice_node)) {
      if (valid_count == choice_index) {
        DialogueNode *choice = &g_current_session.dialogue->nodes[choice_node];
        strncpy(out_text, choice->text, max_length - 1);
        out_text[max_length - 1] = '\0';
        return true;
      }
      valid_count++;
    }
  }
  
  return false;
}

bool dialogue_select_choice(u32 choice_index) {
  if (!g_current_session.is_active || !g_current_session.dialogue) {
    return false;
  }
  
  DialogueNode *node = &g_current_session.dialogue->nodes[g_current_session.current_node];
  
  // Find the Nth valid choice
  u32 valid_count = 0;
  for (u32 i = 0; i < node->choice_count; i++) {
    u32 choice_node = node->choice_indices[i];
    if (dialogue_check_node_conditions(choice_node)) {
      if (valid_count == choice_index) {
        g_current_session.current_node = choice_node;
        
        // Check if this is an end node
        DialogueNode *next_node = &g_current_session.dialogue->nodes[choice_node];
        if (next_node->is_end_node) {
          dialogue_end_conversation();
        }
        
        return true;
      }
      valid_count++;
    }
  }
  
  return false;
}

bool dialogue_advance_to_next(void) {
  if (!g_current_session.is_active || !g_current_session.dialogue) {
    return false;
  }
  
  DialogueNode *node = &g_current_session.dialogue->nodes[g_current_session.current_node];
  
  if (node->is_end_node) {
    dialogue_end_conversation();
    return false;
  }
  
  if (node->choice_count > 0) {
    // Has choices - need to select one
    return false;
  }
  
  g_current_session.current_node = node->next_node_default;
  
  // Check if next node is end node
  DialogueNode *next_node = &g_current_session.dialogue->nodes[g_current_session.current_node];
  if (next_node->is_end_node) {
    dialogue_end_conversation();
  }
  
  return true;
}

bool dialogue_check_node_conditions(u32 node_index) {
  if (!g_current_session.dialogue || node_index >= g_current_session.dialogue->node_count) {
    return false;
  }
  
  DialogueNode *node = &g_current_session.dialogue->nodes[node_index];
  
  for (u32 i = 0; i < node->condition_count; i++) {
    DialogueCondition *cond = &node->conditions[i];
    
    bool condition_met = false;
    switch (cond->type) {
      case DIALOGUE_CONDITION_QUEST_COMPLETED:
        // Check if quest is completed
        condition_met = true; // Placeholder
        break;
        
      case DIALOGUE_CONDITION_QUEST_ACTIVE:
        // Check if quest is active
        condition_met = true; // Placeholder
        break;
        
      case DIALOGUE_CONDITION_ITEM_HAS:
        // Check if player has item
        condition_met = true; // Placeholder
        break;
        
      case DIALOGUE_CONDITION_SKILL_LEVEL:
        // Check skill level
        condition_met = true; // Placeholder
        break;
        
      case DIALOGUE_CONDITION_REPUTATION:
        // Check reputation
        condition_met = true; // Placeholder
        break;
        
      case DIALOGUE_CONDITION_CUSTOM:
        // Custom condition check
        condition_met = true; // Placeholder
        break;
        
      default:
        condition_met = false;
        break;
    }
    
    if (cond->required && !condition_met) {
      return false;
    }
  }
  
  return true;
}

u32 dialogue_find_by_npc(u32 npc_id) {
  for (u32 i = 0; i < g_dialogue_count; i++) {
    if (g_dialogue_database[i].npc_id == npc_id) {
      return i + 1; // Return 1-based index
    }
  }
  return 0;
}

u32 dialogue_find_by_id(const char *dialogue_id) {
  if (!dialogue_id) return 0;
  
  for (u32 i = 0; i < g_dialogue_count; i++) {
    if (strcmp(g_dialogue_database[i].dialogue_id, dialogue_id) == 0) {
      return i + 1; // Return 1-based index
    }
  }
  return 0;
}

bool dialogue_set_start_node(u32 dialogue_index, u32 node_index) {
  if (dialogue_index == 0 || dialogue_index > g_dialogue_count) {
    return false;
  }
  
  DialogueTree *tree = &g_dialogue_database[dialogue_index - 1];
  if (node_index >= tree->node_count) {
    return false;
  }
  
  tree->start_node = node_index;
  return true;
}

bool dialogue_set_end_node(u32 dialogue_index, u32 node_index) {
  if (dialogue_index == 0 || dialogue_index > g_dialogue_count) {
    return false;
  }
  
  DialogueTree *tree = &g_dialogue_database[dialogue_index - 1];
  if (node_index >= tree->node_count) {
    return false;
  }
  
  tree->nodes[node_index].is_end_node = true;
  return true;
}

void dialogue_get_session_info(char *out_info, u32 max_length) {
  if (!out_info || max_length == 0) return;
  
  if (!g_current_session.is_active || !g_current_session.dialogue) {
    snprintf(out_info, max_length, "No active dialogue");
    return;
  }
  
  snprintf(out_info, max_length, 
           "Dialogue: %s, Node: %d/%d, Participant: %d",
           g_current_session.dialogue->dialogue_id,
           g_current_session.current_node + 1,
           g_current_session.dialogue->node_count,
           g_current_session.participant_entity);
}

bool dialogue_export_tree(u32 dialogue_index, const char *filename) {
  if (dialogue_index == 0 || dialogue_index > g_dialogue_count || !filename) {
    return false;
  }
  
  DialogueTree *tree = &g_dialogue_database[dialogue_index - 1];
  
  FILE *file = fopen(filename, "w");
  if (!file) {
    LOG_ERROR("Failed to open dialogue export file: %s", filename);
    return false;
  }
  
  fprintf(file, "Dialogue Tree: %s\n", tree->dialogue_id);
  fprintf(file, "NPC ID: %d\n", tree->npc_id);
  fprintf(file, "Start Node: %d\n", tree->start_node);
  fprintf(file, "Node Count: %d\n\n", tree->node_count);
  
  for (u32 i = 0; i < tree->node_count; i++) {
    DialogueNode *node = &tree->nodes[i];
    fprintf(file, "Node %d:\n", i);
    fprintf(file, "  Speaker: %s\n", node->speaker);
    fprintf(file, "  Text: %s\n", node->text);
    fprintf(file, "  Choices: %d\n", node->choice_count);
    
    for (u32 j = 0; j < node->choice_count; j++) {
      fprintf(file, "    -> Node %d\n", node->choice_indices[j]);
    }
    
    fprintf(file, "  End Node: %s\n", node->is_end_node ? "Yes" : "No");
    fprintf(file, "\n");
  }
  
  fclose(file);
  LOG_INFO("Dialogue tree exported to: %s", filename);
  return true;
}
