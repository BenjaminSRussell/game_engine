#include "ai/npc_advanced/behavior_tree_utility.h"
#include <ai/behavior_tree.h>
#include <ai/npc_advanced/utility_ai.h>
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <string.h>

/**
 * =================================================================================================
 *                    BEHAVIOR TREE + UTILITY AI INTEGRATION - AGENT_AI_1 Phase
 * 4
 * =================================================================================================
 *
 * PURPOSE: Hybrid decision making combining BT structure with Utility AI
 * scoring.
 *
 * CONCEPT: BT_NODE_UTILITY_SELECTOR evaluates all child nodes using utility
 * scoring, then executes the highest-scoring child. This combines:
 *          - BT structural safety (fallbacks, sequences)
 *          - Utility AI context-awareness (smooth, weighted decisions)
 * =================================================================================================
 */

// Data for utility selector node
typedef struct {
  UtilityAgent *utility_agent;
  UtilityContext *utility_context;
  int last_selected_child;
} BTUtilitySelectorData;

// Tick function for BT_NODE_UTILITY_SELECTOR
BTNodeState bt_utility_selector_tick(BTNode *self, BTContext *context) {
  if (!self || !context || self->child_count == 0) {
    return BT_STATE_FAILURE;
  }

  BTUtilitySelectorData *data = (BTUtilitySelectorData *)self->data.node_data;
  if (!data || !data->utility_agent) {
    LOG_ERROR("BT Utility Selector: Missing utility agent data");
    return BT_STATE_FAILURE;
  }

  // Update utility context
  if (data->utility_context) {
    data->utility_context->agent_entity = context->agent;
    data->utility_context->delta_time = context->delta_time;
    data->utility_context->frame_number = context->frame_number;
    data->utility_context->blackboard = context->blackboard;
  }

  // Evaluate all children using utility scores
  float best_score = -1.0f;
  int best_child_idx = -1;

  for (int i = 0; i < self->child_count; i++) {
    BTNode *child = self->children[i];

    // Each child should have an associated utility action
    // We can use the child's name to find the matching action
    UtilityAction *action = NULL;
    for (int j = 0; j < data->utility_agent->action_count; j++) {
      if (strcmp(data->utility_agent->actions[j]->name, child->name) == 0) {
        action = data->utility_agent->actions[j];
        break;
      }
    }

    if (action) {
      float score = utility_evaluate_action(action, data->utility_context);

      // Apply inertia if this is the current child
      if (i == data->last_selected_child &&
          data->utility_agent->inertia.enabled) {
        score *= (1.0f + data->utility_agent->inertia.inertia_bonus);
      }

      if (score > best_score) {
        best_score = score;
        best_child_idx = i;
      }
    }
  }

  if (best_child_idx < 0) {
    return BT_STATE_FAILURE; // No valid child found
  }

  // Execute the best child
  data->last_selected_child = best_child_idx;
  BTNode *selected_child = self->children[best_child_idx];

  if (selected_child->on_execute) {
    return selected_child->on_execute(selected_child, context);
  }

  return BT_STATE_FAILURE;
}

// Create a utility selector node
BTNode *bt_create_utility_selector(const char *name,
                                   UtilityAgent *utility_agent) {
  BTNode *node = bt_create_node(name, BT_NODE_UTILITY_SELECTOR);
  if (!node)
    return NULL;

  // Allocate selector data
  BTUtilitySelectorData *data = (BTUtilitySelectorData *)MALLOC_TAGGED(
      sizeof(BTUtilitySelectorData), MEMORY_TAG_AI);
  if (!data) {
    bt_destroy_node(node);
    return NULL;
  }

  memset(data, 0, sizeof(BTUtilitySelectorData));
  data->utility_agent = utility_agent;
  data->utility_context = utility_context_create();
  data->last_selected_child = -1;

  node->data.node_data = data;
  node->on_execute = bt_utility_selector_tick;

  return node;
}

// Cleanup utility selector data
void bt_utility_selector_cleanup(BTNode *node) {
  if (!node || node->type != BT_NODE_UTILITY_SELECTOR)
    return;

  BTUtilitySelectorData *data = (BTUtilitySelectorData *)node->data.node_data;
  if (data) {
    if (data->utility_context) {
      utility_context_destroy(data->utility_context);
    }
    FREE(data);
    node->data.node_data = NULL;
  }
}

// Helper: Add a child action to the utility selector
// This registers both the BT node AND the corresponding utility action
bool bt_utility_selector_add_action_child(BTNode *utility_selector,
                                          BTNode *child,
                                          UtilityAction *action) {
  if (!utility_selector || !child || !action)
    return false;

  if (utility_selector->type != BT_NODE_UTILITY_SELECTOR) {
    LOG_ERROR("Node is not a utility selector");
    return false;
  }

  BTUtilitySelectorData *data =
      (BTUtilitySelectorData *)utility_selector->data.node_data;
  if (!data || !data->utility_agent)
    return false;

  // Add child to BT node
  if (!bt_add_child(utility_selector, child)) {
    return false;
  }

  // Add action to utility agent (ensure names match)
  strncpy(action->name, child->name, sizeof(action->name) - 1);
  if (!utility_agent_add_action(data->utility_agent, action)) {
    bt_remove_child(utility_selector, child);
    return false;
  }

  return true;
}
