#ifndef BEHAVIOR_TREE_UTILITY_H
#define BEHAVIOR_TREE_UTILITY_H

#include <ai/behavior_tree.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct UtilityAgent UtilityAgent;
typedef struct UtilityAction UtilityAction;

// AGENT_AI_1 Phase 4: Behavior Tree + Utility AI Integration

/**
 * Create a utility selector node.
 * This node evaluates all children using utility scoring and executes the best
 * one.
 *
 * @param name Node name
 * @param utility_agent The utility agent containing actions that match child
 * nodes
 * @return New utility selector node, or NULL on failure
 */
BTNode *bt_create_utility_selector(const char *name,
                                   UtilityAgent *utility_agent);

/**
 * Cleanup utility selector data.
 * Should be called before destroying the node.
 */
void bt_utility_selector_cleanup(BTNode *node);

/**
 * Add a child to a utility selector.
 * This registers both the BT child node AND the corresponding utility action.
 * The action's name will be set to match the child node's name.
 *
 * @param utility_selector The utility selector node
 * @param child The child BT node to add
 * @param action The utility action associated with this child
 * @return true on success, false on failure
 */
bool bt_utility_selector_add_action_child(BTNode *utility_selector,
                                          BTNode *child, UtilityAction *action);

#ifdef __cplusplus
}
#endif

#endif // BEHAVIOR_TREE_UTILITY_H
