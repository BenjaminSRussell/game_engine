#include "behavior_tree.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_CHILDREN 16

// Runtime data structures for stateful nodes
typedef struct {
    u32 repeat_count;
    u32 current_count;
} RepeaterData;

typedef struct {
    f32 duration;
    f32 elapsed;
} TimerData;

typedef struct {
    bool succeed_on_one;
    bool fail_on_one;
} ParallelData;

// Helper to create a generic node
static BehaviorNode* _create_node(const char* name, BTNodeType type) {
    BehaviorNode* node = (BehaviorNode*)calloc(1, sizeof(BehaviorNode));
    if (!node) return NULL;

    strncpy(node->name, name, sizeof(node->name) - 1);
    node->type = type;
    node->last_status = BT_STATUS_FAILURE; // Default?

    return node;
}

// MARK: - Tree Management

BehaviorTree* bt_create_tree(const char* name) {
    BehaviorTree* tree = (BehaviorTree*)calloc(1, sizeof(BehaviorTree));
    if (!tree) return NULL;

    strncpy(tree->name, name, sizeof(tree->name) - 1);
    tree->enabled = true;

    // Allocate blackboard
    tree->blackboard = calloc(1, sizeof(BTBlackboard));

    return tree;
}

static void _destroy_node_recursive(BehaviorNode* node) {
    if (!node) return;

    // Clean up type-specific data
    switch (node->type) {
        case BT_NODE_SELECTOR:
        case BT_NODE_SEQUENCE:
        case BT_NODE_PARALLEL:
            for (u32 i = 0; i < node->composite.child_count; ++i) {
                _destroy_node_recursive(node->composite.children[i]);
            }
            free(node->composite.children);
            break;

        case BT_NODE_DECORATOR:
            _destroy_node_recursive(node->decorator.child);
            break;

        default:
            break;
    }

    if (node->runtime_data) {
        free(node->runtime_data);
    }

    free(node);
}

void bt_destroy_tree(BehaviorTree* tree) {
    if (!tree) return;

    if (tree->root) {
        _destroy_node_recursive(tree->root);
    }

    if (tree->blackboard) {
        free(tree->blackboard);
    }

    free(tree);
}

void bt_set_root(BehaviorTree* tree, BehaviorNode* root) {
    if (!tree) return;
    tree->root = root;
}

void bt_set_enabled(BehaviorTree* tree, bool enabled) {
    if (!tree) return;
    tree->enabled = enabled;
}

// MARK: - Node Creation

BehaviorNode* bt_create_selector(const char* name) {
    BehaviorNode* node = _create_node(name, BT_NODE_SELECTOR);
    node->composite.children = (BehaviorNode**)calloc(MAX_CHILDREN, sizeof(BehaviorNode*));
    return node;
}

BehaviorNode* bt_create_sequence(const char* name) {
    BehaviorNode* node = _create_node(name, BT_NODE_SEQUENCE);
    node->composite.children = (BehaviorNode**)calloc(MAX_CHILDREN, sizeof(BehaviorNode*));
    return node;
}

BehaviorNode* bt_create_parallel(const char* name, bool succeed_on_one, bool fail_on_one) {
    BehaviorNode* node = _create_node(name, BT_NODE_PARALLEL);
    node->composite.children = (BehaviorNode**)calloc(MAX_CHILDREN, sizeof(BehaviorNode*));

    ParallelData* data = (ParallelData*)calloc(1, sizeof(ParallelData));
    data->succeed_on_one = succeed_on_one;
    data->fail_on_one = fail_on_one;
    node->runtime_data = data;

    return node;
}

BehaviorNode* bt_create_condition(const char* name, BTConditionFunc condition, void* user_data) {
    BehaviorNode* node = _create_node(name, BT_NODE_CONDITION);
    node->condition.condition = condition;
    node->condition.user_data = user_data;
    return node;
}

BehaviorNode* bt_create_action(const char* name, BTActionFunc action, void* user_data) {
    BehaviorNode* node = _create_node(name, BT_NODE_ACTION);
    node->action.action = action;
    node->action.user_data = user_data;
    return node;
}

// MARK: - Decorators

static BTNodeStatus _inverter_func(BTNodeStatus status) {
    if (status == BT_STATUS_SUCCESS) return BT_STATUS_FAILURE;
    if (status == BT_STATUS_FAILURE) return BT_STATUS_SUCCESS;
    return status;
}

BehaviorNode* bt_create_inverter(const char* name, BehaviorNode* child) {
    BehaviorNode* node = _create_node(name, BT_NODE_DECORATOR);
    node->decorator.child = child;
    node->decorator.decorate = _inverter_func;
    node->decorator.type = BT_DECORATOR_INVERTER;
    return node;
}

BehaviorNode* bt_create_repeater(const char* name, BehaviorNode* child, u32 repeat_count) {
     BehaviorNode* node = _create_node(name, BT_NODE_DECORATOR);
     node->decorator.child = child;
     node->decorator.type = BT_DECORATOR_REPEATER;

     RepeaterData* data = (RepeaterData*)calloc(1, sizeof(RepeaterData));
     data->repeat_count = repeat_count;
     data->current_count = 0;
     node->runtime_data = data;

     return node;
}

BehaviorNode* bt_create_timer(const char* name, BehaviorNode* child, f32 duration) {
    BehaviorNode* node = _create_node(name, BT_NODE_DECORATOR);
    node->decorator.child = child;
    node->decorator.type = BT_DECORATOR_TIMER;

    TimerData* data = (TimerData*)calloc(1, sizeof(TimerData));
    data->duration = duration;
    data->elapsed = 0.0f;
    node->runtime_data = data;

    return node;
}

BehaviorNode* bt_create_custom(const char* name, BTCustomUpdateFunc update_func, void* user_data) {
    BehaviorNode* node = _create_node(name, BT_NODE_CUSTOM);
    node->custom.update_func = update_func;
    node->custom.user_data = user_data;
    return node;
}

// MARK: - Node Composition

void bt_add_child(BehaviorNode* parent, BehaviorNode* child) {
    if (!parent || !child) return;

    if (parent->type == BT_NODE_SELECTOR ||
        parent->type == BT_NODE_SEQUENCE ||
        parent->type == BT_NODE_PARALLEL) {

        if (parent->composite.child_count < MAX_CHILDREN) {
            parent->composite.children[parent->composite.child_count++] = child;
        }
    } else if (parent->type == BT_NODE_DECORATOR) {
        parent->decorator.child = child;
    }
}

// MARK: - Tree Execution

static BTNodeStatus _update_node(BehaviorNode* node, Entity* entity, f32 delta_time) {
    if (!node) return BT_STATUS_FAILURE;

    BTNodeStatus status = BT_STATUS_FAILURE;

    switch (node->type) {
        case BT_NODE_SEQUENCE: {
            // Sequence: Run children until one fails or running
            if (node->last_status != BT_STATUS_RUNNING) {
                node->composite.current_child = 0;
            }

            status = BT_STATUS_SUCCESS;
            for (u32 i = node->composite.current_child; i < node->composite.child_count; ++i) {
                status = _update_node(node->composite.children[i], entity, delta_time);

                if (status == BT_STATUS_RUNNING) {
                    node->composite.current_child = i;
                    break;
                }

                if (status == BT_STATUS_FAILURE) {
                    node->composite.current_child = 0; // Reset
                    break;
                }
            }
            break;
        }

        case BT_NODE_SELECTOR: {
            // Selector: Run children until one succeeds or running
            if (node->last_status != BT_STATUS_RUNNING) {
                node->composite.current_child = 0;
            }

            status = BT_STATUS_FAILURE;
            for (u32 i = node->composite.current_child; i < node->composite.child_count; ++i) {
                status = _update_node(node->composite.children[i], entity, delta_time);

                if (status == BT_STATUS_RUNNING) {
                    node->composite.current_child = i;
                    break;
                }

                if (status == BT_STATUS_SUCCESS) {
                    node->composite.current_child = 0; // Reset
                    break;
                }
            }
            break;
        }

        case BT_NODE_PARALLEL: {
            ParallelData* data = (ParallelData*)node->runtime_data;
            bool succeed_on_one = data ? data->succeed_on_one : false;
            bool fail_on_one = data ? data->fail_on_one : false;

            bool any_running = false;
            bool any_failure = false;
            bool any_success = false;

            for (u32 i = 0; i < node->composite.child_count; ++i) {
                BTNodeStatus child_status = _update_node(node->composite.children[i], entity, delta_time);
                if (child_status == BT_STATUS_RUNNING) any_running = true;
                if (child_status == BT_STATUS_FAILURE) any_failure = true;
                if (child_status == BT_STATUS_SUCCESS) any_success = true;
            }

            if (fail_on_one && any_failure) status = BT_STATUS_FAILURE;
            else if (succeed_on_one && any_success) status = BT_STATUS_SUCCESS;
            else if (any_running) status = BT_STATUS_RUNNING;
            else if (any_failure) status = BT_STATUS_FAILURE;
            else status = BT_STATUS_SUCCESS;
            break;
        }

        case BT_NODE_CONDITION:
            if (node->condition.condition) {
                status = node->condition.condition(entity, node->condition.user_data);
            }
            break;

        case BT_NODE_ACTION:
            if (node->action.action) {
                status = node->action.action(entity, node->action.user_data, delta_time);
            }
            break;

        case BT_NODE_DECORATOR:
            if (node->decorator.child) {
                switch (node->decorator.type) {
                    case BT_DECORATOR_INVERTER:
                    case BT_DECORATOR_GENERIC:
                        status = _update_node(node->decorator.child, entity, delta_time);
                        if (node->decorator.decorate) {
                            status = node->decorator.decorate(status);
                        }
                        break;

                    case BT_DECORATOR_REPEATER: {
                        RepeaterData* data = (RepeaterData*)node->runtime_data;
                        if (data && data->repeat_count > 0) {
                             status = _update_node(node->decorator.child, entity, delta_time);
                             // If child is running, we return running
                             if (status == BT_STATUS_RUNNING) {
                                 status = BT_STATUS_RUNNING;
                             } else {
                                 // Child finished (Success or Failure)
                                 // We count it as one iteration.
                                 // Standard repeater often repeats on Success only or Failure only, or Always.
                                 // This implementation repeats Always.
                                 data->current_count++;
                                 if (data->current_count >= data->repeat_count) {
                                     data->current_count = 0; // Reset
                                     status = BT_STATUS_SUCCESS;
                                 } else {
                                     status = BT_STATUS_RUNNING;
                                 }
                             }
                        } else {
                            status = _update_node(node->decorator.child, entity, delta_time);
                        }
                        break;
                    }

                    case BT_DECORATOR_TIMER: {
                        TimerData* data = (TimerData*)node->runtime_data;
                        if (data && data->duration > 0) {
                            data->elapsed += delta_time;
                            if (data->elapsed >= data->duration) {
                                status = _update_node(node->decorator.child, entity, delta_time);
                            } else {
                                status = BT_STATUS_RUNNING;
                            }
                        } else {
                            status = _update_node(node->decorator.child, entity, delta_time);
                        }
                        break;
                    }
                }
            }
            break;

        case BT_NODE_CUSTOM:
            if (node->custom.update_func) {
                status = node->custom.update_func(node, entity, node->custom.user_data, delta_time);
            }
            break;
    }

    node->last_status = status;
    return status;
}

BTNodeStatus bt_update_tree(BehaviorTree* tree, Entity* entity, f32 delta_time) {
    if (!tree || !tree->enabled || !tree->root) return BT_STATUS_FAILURE;
    return _update_node(tree->root, entity, delta_time);
}

static void _reset_node_recursive(BehaviorNode* node) {
    if (!node) return;

    node->last_status = BT_STATUS_FAILURE; // Reset status

    if (node->type == BT_NODE_SELECTOR || node->type == BT_NODE_SEQUENCE) {
        node->composite.current_child = 0;
        for (u32 i = 0; i < node->composite.child_count; ++i) {
            _reset_node_recursive(node->composite.children[i]);
        }
    } else if (node->type == BT_NODE_PARALLEL) {
        for (u32 i = 0; i < node->composite.child_count; ++i) {
            _reset_node_recursive(node->composite.children[i]);
        }
    } else if (node->type == BT_NODE_DECORATOR) {
        _reset_node_recursive(node->decorator.child);

        // Reset runtime data if any
        if (node->runtime_data) {
             if (node->decorator.type == BT_DECORATOR_REPEATER) {
                 RepeaterData* data = (RepeaterData*)node->runtime_data;
                 data->current_count = 0;
             } else if (node->decorator.type == BT_DECORATOR_TIMER) {
                 TimerData* data = (TimerData*)node->runtime_data;
                 data->elapsed = 0;
             }
        }
    }
}

void bt_reset_tree(BehaviorTree* tree) {
    if (!tree || !tree->root) return;
    _reset_node_recursive(tree->root);
}

// MARK: - Blackboard Management

BTBlackboard* bt_get_blackboard(BehaviorTree* tree) {
    return tree ? tree->blackboard : NULL;
}

void bt_set_blackboard_data(BehaviorTree* tree, void* data, size_t size) {
    if (!tree || !tree->blackboard) return;
    tree->blackboard->custom_data = data;
    tree->blackboard->custom_data_size = size;
}

// MARK: - Built-in Condition/Action Stubs

BTNodeStatus bt_condition_has_target(Entity* entity, void* user_data) {
    return BT_STATUS_FAILURE;
}

BTNodeStatus bt_action_wander(Entity* entity, void* user_data, f32 delta_time) {
    return BT_STATUS_SUCCESS;
}

// ... other stubs as needed ...
const char* bt_get_node_status_string(BTNodeStatus status) {
    switch (status) {
        case BT_STATUS_SUCCESS: return "SUCCESS";
        case BT_STATUS_FAILURE: return "FAILURE";
        case BT_STATUS_RUNNING: return "RUNNING";
        default: return "UNKNOWN";
    }
}
