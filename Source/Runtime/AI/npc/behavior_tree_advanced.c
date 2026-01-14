#include "ai/ai_types.h"
#include "core/core.h"
#include <stdlib.h>

typedef enum { BT_SUCCESS, BT_FAILURE, BT_RUNNING } BTStatus;
typedef enum { BT_SEQUENCE, BT_SELECTOR, BT_ACTION, BT_CONDITION, BT_INVERTER, BT_REPEATER } BTNodeType;

typedef struct BTNode {
    BTNodeType type;
    BTStatus (*tick)(struct BTNode*, void*);
    struct BTNode **children;
    int child_count;
    void *data;
} BTNode;

typedef struct {
    const char *key;
    void *value;
} BlackboardEntry;

typedef struct {
    BlackboardEntry entries[32];
    int count;
} Blackboard;

BTStatus bt_sequence_tick(BTNode *node, void *context) {
    for (int i = 0; i < node->child_count; i++) {
        BTStatus status = node->children[i]->tick(node->children[i], context);
        if (status != BT_SUCCESS) return status;
    }
    return BT_SUCCESS;
}

BTStatus bt_selector_tick(BTNode *node, void *context) {
    for (int i = 0; i < node->child_count; i++) {
        BTStatus status = node->children[i]->tick(node->children[i], context);
        if (status != BT_FAILURE) return status;
    }
    return BT_FAILURE;
}

BTNode* bt_create_sequence(BTNode **children, int count) {
    BTNode *node = calloc(1, sizeof(BTNode));
    node->type = BT_SEQUENCE;
    node->tick = bt_sequence_tick;
    node->children = children;
    node->child_count = count;
    return node;
}

BTNode* bt_create_selector(BTNode **children, int count) {
    BTNode *node = calloc(1, sizeof(BTNode));
    node->type = BT_SELECTOR;
    node->tick = bt_selector_tick;
    node->children = children;
    node->child_count = count;
    return node;
}

BTNode* bt_create_action(BTStatus (*action)(BTNode*, void*)) {
    BTNode *node = calloc(1, sizeof(BTNode));
    node->type = BT_ACTION;
    node->tick = action;
    return node;
}

void blackboard_set(Blackboard *bb, const char *key, void *value) {
    for (int i = 0; i < bb->count; i++) {
        if (strcmp(bb->entries[i].key, key) == 0) {
            bb->entries[i].value = value;
            return;
        }
    }
    if (bb->count < 32) {
        bb->entries[bb->count].key = key;
        bb->entries[bb->count].value = value;
        bb->count++;
    }
}

void* blackboard_get(Blackboard *bb, const char *key) {
    for (int i = 0; i < bb->count; i++) {
        if (strcmp(bb->entries[i].key, key) == 0) {
            return bb->entries[i].value;
        }
    }
    return NULL;
}
