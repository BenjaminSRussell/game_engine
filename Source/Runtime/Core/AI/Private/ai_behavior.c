#include "../Public/unified_ai.h"
#include "ai_types.h"
#include <stdlib.h>
#include <unified_memory.h>

static BTStatus bt_tick_sequence(AIBehaviorNode *node, void *context) {
  for (u32 i = 0; i < node->child_count; i++) {
    BTStatus status = node->children[i]->tick(node->children[i], context);
    if (status != BT_SUCCESS) {
      return status;
    }
  }
  return BT_SUCCESS;
}

static BTStatus bt_tick_selector(AIBehaviorNode *node, void *context) {
  for (u32 i = 0; i < node->child_count; i++) {
    BTStatus status = node->children[i]->tick(node->children[i], context);
    if (status == BT_SUCCESS) {
      return BT_SUCCESS;
    }
    if (status == BT_RUNNING) {
      return BT_RUNNING;
    }
  }
  return BT_FAILURE;
}

AIBehaviorNode *ai_bt_create_sequence(AIBehaviorNode **children, u32 count) {
  AIBehaviorNode *node = UNIFIED_ALLOC(sizeof(AIBehaviorNode));
  if (!node)
    return NULL;

  node->tick = bt_tick_sequence;
  node->children = children;
  node->child_count = count;
  return node;
}

AIBehaviorNode *ai_bt_create_selector(AIBehaviorNode **children, u32 count) {
  AIBehaviorNode *node = UNIFIED_ALLOC(sizeof(AIBehaviorNode));
  if (!node)
    return NULL;

  node->tick = bt_tick_selector;
  node->children = children;
  node->child_count = count;
  return node;
}

void ai_bt_destroy(AIBehaviorNode *node) {
  if (!node)
    return;

  // Assuming children are managed by caller or we need deep free flag
  // For now, shallow free of the node wrapper
  UNIFIED_FREE(node);
}
