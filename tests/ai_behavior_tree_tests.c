#include "ai_behavior_tree_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

BehaviorTreeTestConfig g_behavior_tree_test_config;
BehaviorTreeTestResults g_behavior_tree_test_results;

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t get_time_ms(void) {
    return get_time_ns() / 1000000ULL;
}

bool behavior_tree_tests_init(const BehaviorTreeTestConfig *config) {
    if (config) {
        g_behavior_tree_test_config = *config;
    } else {
        g_behavior_tree_test_config = (BehaviorTreeTestConfig){
            .enable_verbose_output = false,
            .enable_performance_tests = true,
            .enable_stress_tests = false,
            .enable_debug_logging = false,
            .max_tree_depth = 10,
            .max_nodes_per_tree = 100,
            .test_iterations = 100,
            .execution_timeout_seconds = 5.0f,
            .output_file = NULL
        };
    }
    
    memset(&g_behavior_tree_test_results, 0, sizeof(g_behavior_tree_test_results));
    
    srand((unsigned int)time(NULL));
    return true;
}

void behavior_tree_tests_shutdown(bool generate_report) {
    if (generate_report) {
        behavior_tree_print_test_summary();
        if (g_behavior_tree_test_config.output_file) {
            behavior_tree_export_results(g_behavior_tree_test_config.output_file);
        }
    }
}

BehaviorNode* behavior_tree_create_node(BehaviorNodeType type, const char *name) {
    BehaviorNode *node = malloc(sizeof(BehaviorNode));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(BehaviorNode));
    
    node->id = (uint32_t)(rand() % 1000000);
    node->type = type;
    node->status = STATUS_INVALID;
    node->is_enabled = true;
    node->is_debug = false;
    
    if (name) {
        strncpy(node->name, name, sizeof(node->name) - 1);
    }
    
    return node;
}

void behavior_tree_destroy_node(BehaviorNode *node) {
    if (!node) return;
    
    // Destroy children first
    if (node->children) {
        for (uint32_t i = 0; i < node->child_count; i++) {
            behavior_tree_destroy_node(node->children[i]);
        }
        free(node->children);
    }
    
    free(node);
}

bool behavior_tree_add_child(BehaviorNode *parent, BehaviorNode *child) {
    if (!parent || !child) return false;
    
    // Expand children array if needed
    if (parent->child_count >= parent->max_children) {
        uint32_t new_capacity = parent->max_children ? parent->max_children * 2 : 4;
        BehaviorNode **new_children = realloc(parent->children, 
                                           new_capacity * sizeof(BehaviorNode*));
        if (!new_children) return false;
        
        parent->children = new_children;
        parent->max_children = new_capacity;
    }
    
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    
    return true;
}

BehaviorTree* behavior_tree_create(const char *name, uint32_t max_nodes) {
    BehaviorTree *tree = malloc(sizeof(BehaviorTree));
    if (!tree) return NULL;
    
    memset(tree, 0, sizeof(BehaviorTree));
    
    tree->id = (uint32_t)(rand() % 1000000);
    tree->max_nodes = max_nodes;
    tree->all_nodes = malloc(max_nodes * sizeof(BehaviorNode*));
    if (!tree->all_nodes) {
        free(tree);
        return NULL;
    }
    
    if (name) {
        strncpy(tree->name, name, sizeof(tree->name) - 1);
    }
    
    tree->is_active = true;
    
    return tree;
}

void behavior_tree_destroy(BehaviorTree *tree) {
    if (!tree) return;
    
    if (tree->root) {
        behavior_tree_destroy_node(tree->root);
    }
    
    if (tree->all_nodes) {
        free(tree->all_nodes);
    }
    
    if (tree->blackboard) {
        free(tree->blackboard);
    }
    
    free(tree);
}

bool behavior_tree_set_root(BehaviorTree *tree, BehaviorNode *root) {
    if (!tree || !root) return false;
    
    tree->root = root;
    return true;
}

bool behavior_tree_add_node(BehaviorTree *tree, BehaviorNode *node) {
    if (!tree || !node || tree->node_count >= tree->max_nodes) {
        return false;
    }
    
    tree->all_nodes[tree->node_count++] = node;
    return true;
}

static BehaviorNodeStatus execute_node(BehaviorNode *node, float dt) {
    if (!node || !node->is_enabled) {
        return STATUS_FAILURE;
    }
    
    uint64_t start_time = get_time_ns();
    node->execution_count++;
    
    BehaviorNodeStatus result = STATUS_SUCCESS;
    
    switch (node->type) {
        case NODE_TYPE_ACTION:
            // Simulate action execution
            result = (rand() % 100 > 10) ? STATUS_SUCCESS : STATUS_FAILURE;
            break;
            
        case NODE_TYPE_CONDITION:
            // Simulate condition check
            result = (rand() % 100 > 30) ? STATUS_SUCCESS : STATUS_FAILURE;
            break;
            
        case NODE_TYPE_SEQUENCE:
            result = STATUS_SUCCESS;
            for (uint32_t i = 0; i < node->child_count; i++) {
                BehaviorNodeStatus child_result = execute_node(node->children[i], dt);
                if (child_result != STATUS_SUCCESS) {
                    result = child_result;
                    break;
                }
            }
            break;
            
        case NODE_TYPE_SELECTOR:
            result = STATUS_FAILURE;
            for (uint32_t i = 0; i < node->child_count; i++) {
                BehaviorNodeStatus child_result = execute_node(node->children[i], dt);
                if (child_result != STATUS_FAILURE) {
                    result = child_result;
                    break;
                }
            }
            break;
            
        case NODE_TYPE_PARALLEL:
            // Execute all children, return success if any succeed
            result = STATUS_FAILURE;
            for (uint32_t i = 0; i < node->child_count; i++) {
                BehaviorNodeStatus child_result = execute_node(node->children[i], dt);
                if (child_result == STATUS_SUCCESS) {
                    result = STATUS_SUCCESS;
                }
            }
            break;
            
        case NODE_TYPE_INVERTER:
            if (node->child_count > 0) {
                BehaviorNodeStatus child_result = execute_node(node->children[0], dt);
                result = (child_result == STATUS_SUCCESS) ? STATUS_FAILURE : STATUS_SUCCESS;
            }
            break;
            
        case NODE_TYPE_REPEATER:
            if (node->child_count > 0) {
                // Execute child multiple times
                for (int i = 0; i < 3; i++) {
                    execute_node(node->children[0], dt);
                }
                result = STATUS_SUCCESS;
            }
            break;
            
        default:
            result = STATUS_FAILURE;
            break;
    }
    
    uint64_t end_time = get_time_ns();
    node->last_execution_time_ns = end_time - start_time;
    node->total_execution_time_ns += node->last_execution_time_ns;
    node->status = result;
    
    return result;
}

BehaviorNodeStatus behavior_tree_execute(BehaviorTree *tree, float dt) {
    if (!tree || !tree->root || !tree->is_active) {
        return STATUS_FAILURE;
    }
    
    uint64_t start_time = get_time_ns();
    tree->execution_count++;
    tree->current_depth = 0;
    
    BehaviorNodeStatus result = execute_node(tree->root, dt);
    
    uint64_t end_time = get_time_ns();
    tree->total_execution_time_ns += (end_time - start_time);
    
    g_behavior_tree_test_results.nodes_executed++;
    
    return result;
}

void behavior_tree_reset(BehaviorTree *tree) {
    if (!tree) return;
    
    // Reset all nodes
    for (uint32_t i = 0; i < tree->node_count; i++) {
        BehaviorNode *node = tree->all_nodes[i];
        if (node) {
            node->status = STATUS_INVALID;
            node->execution_count = 0;
            node->total_execution_time_ns = 0;
            node->last_execution_time_ns = 0;
        }
    }
    
    tree->current_depth = 0;
    tree->max_depth_reached = 0;
    tree->total_execution_time_ns = 0;
    tree->execution_count = 0;
}

bool test_node_action(void) {
    BehaviorNode *action = behavior_tree_create_node(NODE_TYPE_ACTION, "TestAction");
    if (!action) return false;
    
    // Execute action multiple times
    int success_count = 0;
    for (int i = 0; i < 100; i++) {
        BehaviorNodeStatus result = execute_node(action, 0.016f);
        if (result == STATUS_SUCCESS) {
            success_count++;
        }
    }
    
    // Action should succeed most of the time (90% success rate)
    bool success = (success_count >= 80) && (success_count <= 95);
    
    behavior_tree_destroy_node(action);
    return success;
}

bool test_node_condition(void) {
    BehaviorNode *condition = behavior_tree_create_node(NODE_TYPE_CONDITION, "TestCondition");
    if (!condition) return false;
    
    // Execute condition multiple times
    int success_count = 0;
    for (int i = 0; i < 100; i++) {
        BehaviorNodeStatus result = execute_node(condition, 0.016f);
        if (result == STATUS_SUCCESS) {
            success_count++;
        }
    }
    
    // Condition should succeed about 70% of the time
    bool success = (success_count >= 60) && (success_count <= 80);
    
    behavior_tree_destroy_node(condition);
    return success;
}

bool test_node_sequence(void) {
    BehaviorTree *tree = behavior_tree_create("SequenceTest", 10);
    if (!tree) return false;
    
    // Create sequence with action and condition
    BehaviorNode *sequence = behavior_tree_create_node(NODE_TYPE_SEQUENCE, "TestSequence");
    BehaviorNode *action1 = behavior_tree_create_node(NODE_TYPE_ACTION, "Action1");
    BehaviorNode *condition1 = behavior_tree_create_node(NODE_TYPE_CONDITION, "Condition1");
    BehaviorNode *action2 = behavior_tree_create_node(NODE_TYPE_ACTION, "Action2");
    
    if (!sequence || !action1 || !condition1 || !action2) {
        behavior_tree_destroy(tree);
        return false;
    }
    
    behavior_tree_add_child(sequence, action1);
    behavior_tree_add_child(sequence, condition1);
    behavior_tree_add_child(sequence, action2);
    
    behavior_tree_set_root(tree, sequence);
    behavior_tree_add_node(tree, sequence);
    behavior_tree_add_node(tree, action1);
    behavior_tree_add_node(tree, condition1);
    behavior_tree_add_node(tree, action2);
    
    // Execute sequence
    BehaviorNodeStatus result = behavior_tree_execute(tree, 0.016f);
    
    // Sequence should execute all children
    bool success = (action1->execution_count > 0) && 
                  (condition1->execution_count > 0) && 
                  (action2->execution_count > 0);
    
    behavior_tree_destroy(tree);
    return success;
}

bool test_node_selector(void) {
    BehaviorTree *tree = behavior_tree_create("SelectorTest", 10);
    if (!tree) return false;
    
    // Create selector with multiple actions
    BehaviorNode *selector = behavior_tree_create_node(NODE_TYPE_SELECTOR, "TestSelector");
    BehaviorNode *action1 = behavior_tree_create_node(NODE_TYPE_ACTION, "Action1");
    BehaviorNode *action2 = behavior_tree_create_node(NODE_TYPE_ACTION, "Action2");
    BehaviorNode *action3 = behavior_tree_create_node(NODE_TYPE_ACTION, "Action3");
    
    if (!selector || !action1 || !action2 || !action3) {
        behavior_tree_destroy(tree);
        return false;
    }
    
    behavior_tree_add_child(selector, action1);
    behavior_tree_add_child(selector, action2);
    behavior_tree_add_child(selector, action3);
    
    behavior_tree_set_root(tree, selector);
    behavior_tree_add_node(tree, selector);
    behavior_tree_add_node(tree, action1);
    behavior_tree_add_node(tree, action2);
    behavior_tree_add_node(tree, action3);
    
    // Execute selector
    BehaviorNodeStatus result = behavior_tree_execute(tree, 0.016f);
    
    // At least one action should have been executed
    bool success = (action1->execution_count > 0) || 
                  (action2->execution_count > 0) || 
                  (action3->execution_count > 0);
    
    behavior_tree_destroy(tree);
    return success;
}

bool test_decorator_inverter(void) {
    BehaviorTree *tree = behavior_tree_create("InverterTest", 10);
    if (!tree) return false;
    
    // Create inverter with condition
    BehaviorNode *inverter = behavior_tree_create_node(NODE_TYPE_INVERTER, "TestInverter");
    BehaviorNode *condition = behavior_tree_create_node(NODE_TYPE_CONDITION, "TestCondition");
    
    if (!inverter || !condition) {
        behavior_tree_destroy(tree);
        return false;
    }
    
    behavior_tree_add_child(inverter, condition);
    behavior_tree_set_root(tree, inverter);
    behavior_tree_add_node(tree, inverter);
    behavior_tree_add_node(tree, condition);
    
    // Execute inverter multiple times
    int success_count = 0;
    for (int i = 0; i < 50; i++) {
        behavior_tree_reset(tree);
        BehaviorNodeStatus result = behavior_tree_execute(tree, 0.016f);
        if (result == STATUS_SUCCESS) {
            success_count++;
        }
    }
    
    // Inverter should invert the condition's success rate
    // If condition succeeds 70% of time, inverter should succeed 30% of time
    bool success = (success_count >= 10) && (success_count <= 25);
    
    behavior_tree_destroy(tree);
    return success;
}

bool test_tree_simple_execution(void) {
    BehaviorTree *tree = behavior_tree_create("SimpleTest", 20);
    if (!tree) return false;
    
    // Create simple tree: Sequence(Action, Condition)
    BehaviorNode *root = behavior_tree_create_node(NODE_TYPE_SEQUENCE, "RootSequence");
    BehaviorNode *action = behavior_tree_create_node(NODE_TYPE_ACTION, "MainAction");
    BehaviorNode *condition = behavior_tree_create_node(NODE_TYPE_CONDITION, "MainCondition");
    
    if (!root || !action || !condition) {
        behavior_tree_destroy(tree);
        return false;
    }
    
    behavior_tree_add_child(root, action);
    behavior_tree_add_child(root, condition);
    
    behavior_tree_set_root(tree, root);
    behavior_tree_add_node(tree, root);
    behavior_tree_add_node(tree, action);
    behavior_tree_add_node(tree, condition);
    
    // Execute tree multiple times
    for (int i = 0; i < 10; i++) {
        behavior_tree_execute(tree, 0.016f);
    }
    
    // Verify execution
    bool success = (action->execution_count == 10) && 
                  (condition->execution_count == 10) &&
                  (tree->execution_count == 10);
    
    behavior_tree_destroy(tree);
    return success;
}

bool test_behavior_patrol(void) {
    BehaviorTree *tree = behavior_tree_create("PatrolBehavior", 30);
    if (!tree) return false;
    
    // Create patrol behavior tree
    BehaviorNode *selector = behavior_tree_create_node(NODE_TYPE_SELECTOR, "PatrolSelector");
    BehaviorNode *patrol_sequence = behavior_tree_create_node(NODE_TYPE_SEQUENCE, "PatrolSequence");
    BehaviorNode *check_enemy = behavior_tree_create_node(NODE_TYPE_CONDITION, "CheckEnemy");
    BehaviorNode *move_to_point = behavior_tree_create_node(NODE_TYPE_ACTION, "MoveToPoint");
    BehaviorNode *wait_action = behavior_tree_create_node(NODE_TYPE_ACTION, "WaitAction");
    
    if (!selector || !patrol_sequence || !check_enemy || !move_to_point || !wait_action) {
        behavior_tree_destroy(tree);
        return false;
    }
    
    behavior_tree_add_child(selector, check_enemy);
    behavior_tree_add_child(selector, patrol_sequence);
    behavior_tree_add_child(patrol_sequence, move_to_point);
    behavior_tree_add_child(patrol_sequence, wait_action);
    
    behavior_tree_set_root(tree, selector);
    behavior_tree_add_node(tree, selector);
    behavior_tree_add_node(tree, patrol_sequence);
    behavior_tree_add_node(tree, check_enemy);
    behavior_tree_add_node(tree, move_to_point);
    behavior_tree_add_node(tree, wait_action);
    
    // Execute patrol behavior
    for (int i = 0; i < 20; i++) {
        behavior_tree_execute(tree, 0.016f);
    }
    
    // Verify behavior execution
    bool success = (check_enemy->execution_count == 20) && 
                  (move_to_point->execution_count > 0) &&
                  (wait_action->execution_count > 0);
    
    behavior_tree_destroy(tree);
    return success;
}

bool test_performance_large_trees(void) {
    BehaviorTree *tree = behavior_tree_create("LargeTreeTest", 1000);
    if (!tree) return false;
    
    // Create large tree with many nodes
    BehaviorNode *root = behavior_tree_create_node(NODE_TYPE_SEQUENCE, "Root");
    behavior_tree_set_root(tree, root);
    behavior_tree_add_node(tree, root);
    
    // Add many children to root
    for (int i = 0; i < 50; i++) {
        BehaviorNode *sequence = behavior_tree_create_node(NODE_TYPE_SEQUENCE, "Sequence");
        behavior_tree_add_child(root, sequence);
        behavior_tree_add_node(tree, sequence);
        
        // Add actions to sequence
        for (int j = 0; j < 10; j++) {
            BehaviorNode *action = behavior_tree_create_node(NODE_TYPE_ACTION, "Action");
            behavior_tree_add_child(sequence, action);
            behavior_tree_add_node(tree, action);
        }
    }
    
    uint64_t start_time = get_time_ms();
    
    // Execute tree multiple times
    for (int i = 0; i < 100; i++) {
        behavior_tree_execute(tree, 0.016f);
    }
    
    uint64_t end_time = get_time_ms();
    double execution_time = (double)(end_time - start_time);
    
    g_behavior_tree_test_results.average_execution_time_ms = execution_time / 100.0;
    g_behavior_tree_test_results.max_execution_time_ms = execution_time;
    g_behavior_tree_test_results.min_execution_time_ms = execution_time / 100.0;
    
    behavior_tree_destroy(tree);
    return true;
}

bool behavior_tree_run_all_tests(void) {
    uint64_t start_time = get_time_ms();
    
    bool (*tests[])(void) = {
        test_node_action,
        test_node_condition,
        test_node_sequence,
        test_node_selector,
        test_decorator_inverter,
        test_tree_simple_execution,
        test_behavior_patrol,
        test_performance_large_trees
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < num_tests; i++) {
        g_behavior_tree_test_results.total_tests++;
        
        uint64_t test_start = get_time_ms();
        bool passed = tests[i]();
        uint64_t test_end = get_time_ms();
        
        if (passed) {
            g_behavior_tree_test_results.passed_tests++;
        } else {
            g_behavior_tree_test_results.failed_tests++;
        }
        
        // Update execution time statistics
        double test_time = (double)(test_end - test_start);
        if (g_behavior_tree_test_results.min_execution_time_ms == 0.0) {
            g_behavior_tree_test_results.min_execution_time_ms = test_time;
        }
        g_behavior_tree_test_results.min_execution_time_ms = 
            fmin(g_behavior_tree_test_results.min_execution_time_ms, test_time);
        g_behavior_tree_test_results.max_execution_time_ms = 
            fmax(g_behavior_tree_test_results.max_execution_time_ms, test_time);
    }
    
    g_behavior_tree_test_results.total_time_ms = get_time_ms() - start_time;
    g_behavior_tree_test_results.average_execution_time_ms = 
        g_behavior_tree_test_results.total_time_ms / g_behavior_tree_test_results.total_tests;
    
    return g_behavior_tree_test_results.failed_tests == 0;
}

void behavior_tree_print_test_summary(void) {
    printf("\n=== AI Behavior Tree Test Summary ===\n");
    printf("Total Tests: %u\n", g_behavior_tree_test_results.total_tests);
    printf("Passed: %u\n", g_behavior_tree_test_results.passed_tests);
    printf("Failed: %u\n", g_behavior_tree_test_results.failed_tests);
    printf("Skipped: %u\n", g_behavior_tree_test_results.skipped_tests);
    printf("Total Time: %.2f ms\n", g_behavior_tree_test_results.total_time_ms);
    printf("Average Execution Time: %.3f ms\n", g_behavior_tree_test_results.average_execution_time_ms);
    printf("Max Execution Time: %.3f ms\n", g_behavior_tree_test_results.max_execution_time_ms);
    printf("Min Execution Time: %.3f ms\n", g_behavior_tree_test_results.min_execution_time_ms);
    printf("Nodes Executed: %u\n", g_behavior_tree_test_results.nodes_executed);
    printf("Trees Created: %u\n", g_behavior_tree_test_results.trees_created);
    printf("Behaviors Tested: %u\n", g_behavior_tree_test_results.behaviors_tested);
    
    if (g_behavior_tree_test_results.failed_tests > 0) {
        printf("\nFailed Tests:\n%s\n", g_behavior_tree_test_results.error_messages);
    }
    
    printf("===================================\n");
}

BehaviorTreeTestResults behavior_tree_get_test_results(void) {
    return g_behavior_tree_test_results;
}

bool behavior_tree_export_results(const char *filename) {
    if (!filename) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    fprintf(file, "AI Behavior Tree Test Results\n");
    fprintf(file, "=============================\n\n");
    
    fprintf(file, "Configuration:\n");
    fprintf(file, "  Max Tree Depth: %u\n", g_behavior_tree_test_config.max_tree_depth);
    fprintf(file, "  Max Nodes per Tree: %u\n", g_behavior_tree_test_config.max_nodes_per_tree);
    fprintf(file, "  Test Iterations: %u\n", g_behavior_tree_test_config.test_iterations);
    fprintf(file, "  Execution Timeout: %.2f seconds\n\n", g_behavior_tree_test_config.execution_timeout_seconds);
    
    fprintf(file, "Results:\n");
    fprintf(file, "  Total Tests: %u\n", g_behavior_tree_test_results.total_tests);
    fprintf(file, "  Passed: %u\n", g_behavior_tree_test_results.passed_tests);
    fprintf(file, "  Failed: %u\n", g_behavior_tree_test_results.failed_tests);
    fprintf(file, "  Skipped: %u\n", g_behavior_tree_test_results.skipped_tests);
    fprintf(file, "  Total Time: %.2f ms\n", g_behavior_tree_test_results.total_time_ms);
    fprintf(file, "  Average Execution Time: %.3f ms\n", g_behavior_tree_test_results.average_execution_time_ms);
    fprintf(file, "  Max Execution Time: %.3f ms\n", g_behavior_tree_test_results.max_execution_time_ms);
    fprintf(file, "  Min Execution Time: %.3f ms\n", g_behavior_tree_test_results.min_execution_time_ms);
    fprintf(file, "  Nodes Executed: %u\n", g_behavior_tree_test_results.nodes_executed);
    fprintf(file, "  Trees Created: %u\n", g_behavior_tree_test_results.trees_created);
    fprintf(file, "  Behaviors Tested: %u\n\n", g_behavior_tree_test_results.behaviors_tested);
    
    if (g_behavior_tree_test_results.failed_tests > 0) {
        fprintf(file, "Failed Tests:\n%s\n", g_behavior_tree_test_results.error_messages);
    }
    
    fprintf(file, "Performance Report:\n%s\n", g_behavior_tree_test_results.performance_report);
    
    fclose(file);
    return true;
}
