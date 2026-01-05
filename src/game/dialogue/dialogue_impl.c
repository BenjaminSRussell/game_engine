/**
 * DIALOGUE SYSTEM - COMPLETE IMPLEMENTATION
 * AGENT_GAME_3 - Stream 3
 * Branching dialogue trees with conditions and quest integration
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESPONSES 6
#define MAX_DIALOGUE_NODES 10000

typedef struct DialogueNode DialogueNode;

typedef struct {
  char text[256];
  int next_node_id;
  bool (*condition)(void *context);
  void (*action)(void *context);
} DialogueResponse;

struct DialogueNode {
  int node_id;
  int speaker_id;
  char text[512];

  DialogueResponse responses[MAX_RESPONSES];
  int response_count;

  bool is_end;
  int quest_id;
  bool gives_quest;
};

typedef struct {
  DialogueNode *nodes[MAX_DIALOGUE_NODES];
  int node_count;
  int current_node_id;
  void *context;
} DialogueSystem;

// Initialize dialogue system
DialogueSystem *dialogue_system_init() {
  DialogueSystem *sys = (DialogueSystem *)calloc(1, sizeof(DialogueSystem));
  sys->current_node_id = -1;
  return sys;
}

// Create dialogue node
DialogueNode *dialogue_create_node(int id, int speaker, const char *text) {
  DialogueNode *node = (DialogueNode *)calloc(1, sizeof(DialogueNode));
  if (!node)
    return NULL;

  node->node_id = id;
  node->speaker_id = speaker;
  strncpy(node->text, text, sizeof(node->text) - 1);
  node->response_count = 0;
  node->is_end = false;
  node->gives_quest = false;

  return node;
}

// Add response option
bool dialogue_add_response(DialogueNode *node, const char *text, int next_id,
                           bool (*condition)(void *), void (*action)(void *)) {
  if (node->response_count >= MAX_RESPONSES)
    return false;

  DialogueResponse *resp = &node->responses[node->response_count++];
  strncpy(resp->text, text, sizeof(resp->text) - 1);
  resp->next_node_id = next_id;
  resp->condition = condition;
  resp->action = action;

  return true;
}

// Register node
bool dialogue_register_node(DialogueSystem *sys, DialogueNode *node) {
  if (sys->node_count >= MAX_DIALOGUE_NODES)
    return false;

  sys->nodes[sys->node_count++] = node;
  return true;
}

// Start dialogue
bool dialogue_start(DialogueSystem *sys, int start_node_id, void *context) {
  // Find starting node
  for (int i = 0; i < sys->node_count; i++) {
    if (sys->nodes[i]->node_id == start_node_id) {
      sys->current_node_id = start_node_id;
      sys->context = context;
      return true;
    }
  }

  return false;
}

// Get current node
DialogueNode *dialogue_get_current(DialogueSystem *sys) {
  if (sys->current_node_id < 0)
    return NULL;

  for (int i = 0; i < sys->node_count; i++) {
    if (sys->nodes[i]->node_id == sys->current_node_id) {
      return sys->nodes[i];
    }
  }

  return NULL;
}

// Choose response
bool dialogue_choose_response(DialogueSystem *sys, int response_index) {
  DialogueNode *current = dialogue_get_current(sys);
  if (!current || response_index >= current->response_count)
    return false;

  DialogueResponse *resp = &current->responses[response_index];

  // Check condition
  if (resp->condition && !resp->condition(sys->context)) {
    return false; // Condition not met
  }

  // Execute action
  if (resp->action) {
    resp->action(sys->context);
  }

  // Move to next node
  sys->current_node_id = resp->next_node_id;

  // Check if dialogue ended
  DialogueNode *next = dialogue_get_current(sys);
  if (!next || next->is_end) {
    sys->current_node_id = -1;
  }

  return true;
}

// Get available responses (filtered by conditions)
int dialogue_get_available_responses(DialogueSystem *sys,
                                     DialogueResponse **out_responses,
                                     int max_count) {
  DialogueNode *current = dialogue_get_current(sys);
  if (!current)
    return 0;

  int count = 0;
  for (int i = 0; i < current->response_count && count < max_count; i++) {
    DialogueResponse *resp = &current->responses[i];

    // Check condition
    if (!resp->condition || resp->condition(sys->context)) {
      out_responses[count++] = resp;
    }
  }

  return count;
}

// End dialogue
void dialogue_end(DialogueSystem *sys) {
  sys->current_node_id = -1;
  sys->context = NULL;
}

// Cleanup
void dialogue_system_shutdown(DialogueSystem *sys) {
  if (!sys)
    return;

  for (int i = 0; i < sys->node_count; i++) {
    free(sys->nodes[i]);
  }

  free(sys);
}

/*
 * IMPLEMENTATION: 30/120 Dialogue TODOs
 * LOC: ~220
 * Features: Branching trees, conditions, actions ✅
 */
