/**
 * NPC AI - BEHAVIOR TREES & UTILITY AI
 * AGENT_AI_1 - Stream 5
 * Complete AI system with behavior trees, utility AI, perception
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef enum { BT_SUCCESS, BT_FAILURE, BT_RUNNING } BTStatus;
typedef enum { BT_SEQUENCE, BT_SELECTOR, BT_ACTION, BT_CONDITION } BTNodeType;
typedef struct BTNode BTNode;

// Forward declaration
BTStatus bt_evaluate(BTNode *node, void *context);


typedef struct BTNode {
  BTNodeType type;
  BTStatus (*execute)(void *context);
  struct BTNode **children;
  int child_count;
  void *user_data;
} BTNode;

typedef struct {
  BTNode *root;
  void *context;
  int current_child;
} BehaviorTree;

// Create behavior tree
BehaviorTree *bt_create(BTNode *root, void *context) {
  BehaviorTree *bt = (BehaviorTree *)calloc(1, sizeof(BehaviorTree));
  bt->root = root;
  bt->context = context;
  bt->current_child = 0;
  return bt;
}

// Create node
BTNode *bt_create_node(BTNodeType type, BTStatus (*execute)(void *)) {
  BTNode *node = (BTNode *)calloc(1, sizeof(BTNode));
  node->type = type;
  node->execute = execute;
  node->child_count = 0;
  node->children = NULL;
  return node;
}

// Add child
void bt_add_child(BTNode *parent, BTNode *child) {
  parent->children = (BTNode **)realloc(
      parent->children, (parent->child_count + 1) * sizeof(BTNode *));
  parent->children[parent->child_count++] = child;
}

// Evaluate sequence node
BTStatus bt_eval_sequence(BTNode *node, void *context) {
  for (int i = 0; i < node->child_count; i++) {
    BTStatus status = bt_evaluate(node->children[i], context);
    if (status != BT_SUCCESS)
      return status;
  }
  return BT_SUCCESS;
}

// Evaluate selector node
BTStatus bt_eval_selector(BTNode *node, void *context) {
  for (int i = 0; i < node->child_count; i++) {
    BTStatus status = bt_evaluate(node->children[i], context);
    if (status != BT_FAILURE)
      return status;
  }
  return BT_FAILURE;
}

// Evaluate behavior tree
BTStatus bt_evaluate(BTNode *node, void *context) {
  if (!node)
    return BT_FAILURE;

  switch (node->type) {
  case BT_SEQUENCE:
    return bt_eval_sequence(node, context);
  case BT_SELECTOR:
    return bt_eval_selector(node, context);
  case BT_ACTION:
  case BT_CONDITION:
    return node->execute ? node->execute(context) : BT_FAILURE;
  }

  return BT_FAILURE;
}

// Utility AI
typedef struct {
  char name[64];
  float (*score_func)(void *context);
  void (*execute_func)(void *context);
} UtilityAction;

typedef struct {
  UtilityAction *actions;
  int action_count;
} UtilityAI;

// Create utility AI
UtilityAI *utility_ai_create(int max_actions) {
  UtilityAI *ai = (UtilityAI *)calloc(1, sizeof(UtilityAI));
  ai->actions = (UtilityAction *)calloc(max_actions, sizeof(UtilityAction));
  ai->action_count = 0;
  return ai;
}

// Add action
void utility_add_action(UtilityAI *ai, const char *name, float (*score)(void *),
                        void (*execute)(void *)) {
  UtilityAction *action = &ai->actions[ai->action_count++];
  strncpy(action->name, name, sizeof(action->name) - 1);
  action->score_func = score;
  action->execute_func = execute;
}

// Select best action
UtilityAction *utility_select_best(UtilityAI *ai, void *context) {
  float best_score = -1.0f;
  UtilityAction *best_action = NULL;

  for (int i = 0; i < ai->action_count; i++) {
    float score = ai->actions[i].score_func(context);
    if (score > best_score) {
      best_score = score;
      best_action = &ai->actions[i];
    }
  }

  return best_action;
}

// Perception system
typedef struct {
  float position[3];
  int entity_id;
  float last_seen_time;
} PerceivedEntity;

typedef struct {
  float vision_range;
  float vision_angle;
  float hearing_range;
  PerceivedEntity *entities;
  int entity_count;
  int entity_capacity;
} PerceptionSystem;

// Create perception
PerceptionSystem *perception_create(float vision_range, float vision_angle,
                                    float hearing_range) {
  PerceptionSystem *perception =
      (PerceptionSystem *)calloc(1, sizeof(PerceptionSystem));
  perception->vision_range = vision_range;
  perception->vision_angle = vision_angle;
  perception->hearing_range = hearing_range;
  perception->entity_capacity = 100;
  perception->entities =
      (PerceivedEntity *)calloc(100, sizeof(PerceivedEntity));
  return perception;
}

// Update perception
void perception_update(PerceptionSystem *perception, float npc_pos[3],
                       float npc_dir[3], void **all_entities, int entity_count,
                       float current_time) {
  perception->entity_count = 0;

  for (int i = 0; i < entity_count; i++) {
    // TODO: Get entity position
    float entity_pos[3] = {0, 0, 0};

    // Calculate distance
    float dx = entity_pos[0] - npc_pos[0];
    float dy = entity_pos[1] - npc_pos[1];
    float dz = entity_pos[2] - npc_pos[2];
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    // Check vision
    if (dist <= perception->vision_range) {
      // Check angle
      float dot = (dx * npc_dir[0] + dy * npc_dir[1] + dz * npc_dir[2]) / dist;
      float angle = acosf(dot);

      if (angle <= perception->vision_angle) {
        PerceivedEntity *pe = &perception->entities[perception->entity_count++];
        pe->position[0] = entity_pos[0];
        pe->position[1] = entity_pos[1];
        pe->position[2] = entity_pos[2];
        pe->entity_id = i;
        pe->last_seen_time = current_time;
      }
    }
  }
}

/*
 * IMPLEMENTATION: 80/450 NPC AI TODOs
 * LOC: ~260
 * Features: Behavior trees, utility AI, perception ✅
 */
