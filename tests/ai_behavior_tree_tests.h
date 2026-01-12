/**
 * @file ai_behavior_tree_tests.h
 * @brief AI behavior tree execution tests
 *
 * Comprehensive test suite for AI behavior tree systems including
 * node execution, tree traversal, and behavior validation.
 */

#ifndef AI_BEHAVIOR_TREE_TESTS_H
#define AI_BEHAVIOR_TREE_TESTS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Behavior Tree Node Types
// ========================================

typedef enum BehaviorNodeType {
    NODE_TYPE_ACTION,                  // Action node
    NODE_TYPE_CONDITION,                // Condition node
    NODE_TYPE_SEQUENCE,                 // Sequence node
    NODE_TYPE_SELECTOR,                 // Selector node
    NODE_TYPE_PARALLEL,                 // Parallel node
    NODE_TYPE_DECORATOR,                // Decorator node
    NODE_TYPE_INVERTER,                 // Inverter decorator
    NODE_TYPE_REPEATER,                 // Repeater decorator
    NODE_TYPE_UNTIL_SUCCESS,            // Until success decorator
    NODE_TYPE_UNTIL_FAILURE,            // Until failure decorator
    NODE_TYPE_TIMER,                    // Timer decorator
    NODE_TYPE_CUSTOM                    // Custom node type
} BehaviorNodeType;

// ========================================
// Behavior Node Status
// ========================================

typedef enum BehaviorNodeStatus {
    STATUS_SUCCESS,                     // Node executed successfully
    STATUS_FAILURE,                     // Node execution failed
    STATUS_RUNNING,                     // Node is currently running
    STATUS_INVALID,                     // Node is invalid
    STATUS_DISABLED                     // Node is disabled
} BehaviorNodeStatus;

// ========================================
// Behavior Tree Test Configuration
// ========================================

typedef struct BehaviorTreeTestConfig {
    bool enable_verbose_output;         // Enable verbose test output
    bool enable_performance_tests;       // Enable performance benchmarks
    bool enable_stress_tests;           // Enable stress tests
    bool enable_debug_logging;          // Enable debug logging
    uint32_t max_tree_depth;            // Maximum tree depth for tests
    uint32_t max_nodes_per_tree;        // Maximum nodes per tree
    uint32_t test_iterations;           // Number of test iterations
    float execution_timeout_seconds;      // Execution timeout per test
    const char *output_file;            // Output file for results
} BehaviorTreeTestConfig;

// ========================================
// Behavior Tree Test Results
// ========================================

typedef struct BehaviorTreeTestResults {
    uint32_t total_tests;               // Total tests run
    uint32_t passed_tests;              // Tests that passed
    uint32_t failed_tests;              // Tests that failed
    uint32_t skipped_tests;             // Tests that were skipped
    double total_time_ms;               // Total execution time
    double average_execution_time_ms;    // Average execution time
    double max_execution_time_ms;        // Maximum execution time
    double min_execution_time_ms;        // Minimum execution time
    uint32_t nodes_executed;            // Total nodes executed
    uint32_t trees_created;             // Total trees created
    uint32_t behaviors_tested;          // Total behaviors tested
    char error_messages[4096];          // Accumulated error messages
    char performance_report[2048];      // Performance report
} BehaviorTreeTestResults;

// ========================================
// Behavior Node Structure
// ========================================

typedef struct BehaviorNode {
    uint32_t id;                        // Unique node ID
    BehaviorNodeType type;                // Node type
    BehaviorNodeStatus status;            // Current status
    char name[64];                      // Node name
    void *user_data;                     // User-defined data
    struct BehaviorNode *parent;          // Parent node
    struct BehaviorNode **children;       // Child nodes
    uint32_t child_count;                // Number of children
    uint32_t max_children;               // Maximum children capacity
    uint32_t execution_count;            // Number of times executed
    uint64_t total_execution_time_ns;    // Total execution time
    uint64_t last_execution_time_ns;     // Last execution time
    bool is_enabled;                     // Whether node is enabled
    bool is_debug;                       // Whether node is in debug mode
} BehaviorNode;

// ========================================
// Behavior Tree Structure
// ========================================

typedef struct BehaviorTree {
    uint32_t id;                        // Tree ID
    char name[64];                      // Tree name
    BehaviorNode *root;                  // Root node
    BehaviorNode **all_nodes;            // All nodes in tree
    uint32_t node_count;                 // Number of nodes
    uint32_t max_nodes;                  // Maximum nodes capacity
    uint32_t current_depth;              // Current execution depth
    uint32_t max_depth_reached;          // Maximum depth reached
    uint64_t total_execution_time_ns;    // Total execution time
    uint32_t execution_count;            // Number of executions
    bool is_active;                      // Whether tree is active
    void *blackboard;                    // Blackboard data
} BehaviorTree;

// ========================================
// Blackboard Entry
// ========================================

typedef struct BlackboardEntry {
    char key[64];                       // Entry key
    void *value;                        // Entry value
    size_t value_size;                   // Value size
    uint32_t type_id;                   // Type identifier
    uint64_t last_access_time;           // Last access timestamp
    bool is_persistent;                  // Whether entry persists
} BlackboardEntry;

// ========================================
// Global Test State
// ========================================

extern BehaviorTreeTestConfig g_behavior_tree_test_config;
extern BehaviorTreeTestResults g_behavior_tree_test_results;

// ========================================
// Test Suite Functions
// ========================================

/**
 * Initialize behavior tree test suite
 * @param config Test configuration
 * @return True if initialization successful
 */
bool behavior_tree_tests_init(const BehaviorTreeTestConfig *config);

/**
 * Shutdown behavior tree test suite
 * @param generate_report Whether to generate final report
 */
void behavior_tree_tests_shutdown(bool generate_report);

/**
 * Run all behavior tree tests
 * @return True if all tests pass
 */
bool behavior_tree_run_all_tests(void);

/**
 * Get test results
 * @return Test results
 */
BehaviorTreeTestResults behavior_tree_get_test_results(void);

/**
 * Print test summary
 */
void behavior_tree_print_test_summary(void);

/**
 * Export test results to file
 * @param filename Output filename
 * @return True if export successful
 */
bool behavior_tree_export_results(const char *filename);

// ========================================
// Node Creation and Management
// ========================================

/**
 * Create behavior node
 * @param type Node type
 * @param name Node name
 * @return Created node or NULL on failure
 */
BehaviorNode* behavior_tree_create_node(BehaviorNodeType type, const char *name);

/**
 * Destroy behavior node
 * @param node Node to destroy
 */
void behavior_tree_destroy_node(BehaviorNode *node);

/**
 * Add child to node
 * @param parent Parent node
 * @param child Child node
 * @return True if addition successful
 */
bool behavior_tree_add_child(BehaviorNode *parent, BehaviorNode *child);

/**
 * Remove child from node
 * @param parent Parent node
 * @param child Child node
 * @return True if removal successful
 */
bool behavior_tree_remove_child(BehaviorNode *parent, BehaviorNode *child);

/**
 * Find node by ID
 * @param tree Behavior tree
 * @param node_id Node ID to find
 * @return Found node or NULL
 */
BehaviorNode* behavior_tree_find_node(const BehaviorTree *tree, uint32_t node_id);

/**
 * Find node by name
 * @param tree Behavior tree
 * @param name Node name to find
 * @return Found node or NULL
 */
BehaviorNode* behavior_tree_find_node_by_name(const BehaviorTree *tree, const char *name);

// ========================================
// Tree Creation and Management
// ========================================

/**
 * Create behavior tree
 * @param name Tree name
 * @param max_nodes Maximum number of nodes
 * @return Created tree or NULL on failure
 */
BehaviorTree* behavior_tree_create(const char *name, uint32_t max_nodes);

/**
 * Destroy behavior tree
 * @param tree Tree to destroy
 */
void behavior_tree_destroy(BehaviorTree *tree);

/**
 * Set root node of tree
 * @param tree Behavior tree
 * @param root Root node
 * @return True if setting successful
 */
bool behavior_tree_set_root(BehaviorTree *tree, BehaviorNode *root);

/**
 * Add node to tree
 * @param tree Behavior tree
 * @param node Node to add
 * @return True if addition successful
 */
bool behavior_tree_add_node(BehaviorTree *tree, BehaviorNode *node);

/**
 * Remove node from tree
 * @param tree Behavior tree
 * @param node Node to remove
 * @return True if removal successful
 */
bool behavior_tree_remove_node(BehaviorTree *tree, BehaviorNode *node);

// ========================================
// Tree Execution
// ========================================

/**
 * Execute behavior tree
 * @param tree Behavior tree
 * @param dt Time delta in seconds
 * @return Execution status
 */
BehaviorNodeStatus behavior_tree_execute(BehaviorTree *tree, float dt);

/**
 * Reset behavior tree
 * @param tree Behavior tree
 */
void behavior_tree_reset(BehaviorTree *tree);

/**
 * Pause behavior tree
 * @param tree Behavior tree
 */
void behavior_tree_pause(BehaviorTree *tree);

/**
 * Resume behavior tree
 * @param tree Behavior tree
 */
void behavior_tree_resume(BehaviorTree *tree);

/**
 * Stop behavior tree
 * @param tree Behavior tree
 */
void behavior_tree_stop(BehaviorTree *tree);

// ========================================
// Blackboard Management
// ========================================

/**
 * Set blackboard value
 * @param tree Behavior tree
 * @param key Value key
 * @param value Value data
 * @param size Value size
 * @param type_id Type identifier
 * @return True if set successful
 */
bool behavior_tree_set_blackboard(BehaviorTree *tree, const char *key, 
                                const void *value, size_t size, uint32_t type_id);

/**
 * Get blackboard value
 * @param tree Behavior tree
 * @param key Value key
 * @param value Output value buffer
 * @param size Buffer size
 * @param type_id Expected type ID
 * @return True if get successful
 */
bool behavior_tree_get_blackboard(const BehaviorTree *tree, const char *key,
                                void *value, size_t size, uint32_t type_id);

/**
 * Remove blackboard entry
 * @param tree Behavior tree
 * @param key Entry key
 * @return True if removal successful
 */
bool behavior_tree_remove_blackboard(BehaviorTree *tree, const char *key);

/**
 * Clear all blackboard entries
 * @param tree Behavior tree
 */
void behavior_tree_clear_blackboard(BehaviorTree *tree);

// ========================================
// Basic Node Tests
// ========================================

/**
 * Test action node execution
 * @return True if test passes
 */
bool test_node_action(void);

/**
 * Test condition node execution
 * @return True if test passes
 */
bool test_node_condition(void);

/**
 * Test sequence node execution
 * @return True if test passes
 */
bool test_node_sequence(void);

/**
 * Test selector node execution
 * @return True if test passes
 */
bool test_node_selector(void);

/**
 * Test parallel node execution
 * @return True if test passes
 */
bool test_node_parallel(void);

// ========================================
// Decorator Node Tests
// ========================================

/**
 * Test inverter decorator
 * @return True if test passes
 */
bool test_decorator_inverter(void);

/**
 * Test repeater decorator
 * @return True if test passes
 */
bool test_decorator_repeater(void);

/**
 * Test until success decorator
 * @return True if test passes
 */
bool test_decorator_until_success(void);

/**
 * Test until failure decorator
 * @return True if test passes
 */
bool test_decorator_until_failure(void);

/**
 * Test timer decorator
 * @return True if test passes
 */
bool test_decorator_timer(void);

// ========================================
// Tree Execution Tests
// ========================================

/**
 * Test simple tree execution
 * @return True if test passes
 */
bool test_tree_simple_execution(void);

/**
 * Test complex tree execution
 * @return True if test passes
 */
bool test_tree_complex_execution(void);

/**
 * Test tree with blackboard
 * @return True if test passes
 */
bool test_tree_with_blackboard(void);

/**
 * Test tree state persistence
 * @return True if test passes
 */
bool test_tree_state_persistence(void);

/**
 * Test tree interruption and resumption
 * @return True if test passes
 */
bool test_tree_interruption_resumption(void);

// ========================================
// Behavior Tests
// ========================================

/**
 * Test patrol behavior
 * @return True if test passes
 */
bool test_behavior_patrol(void);

/**
 * Test chase behavior
 * @return True if test passes
 */
bool test_behavior_chase(void);

/**
 * Test flee behavior
 * @return True if test passes
 */
bool test_behavior_flee(void);

/**
 * Test attack behavior
 * @return True if test passes
 */
bool test_behavior_attack(void);

/**
 * Test idle behavior
 * @return True if test passes
 */
bool test_behavior_idle(void);

/**
 * Test search behavior
 * @return True if test passes
 */
bool test_behavior_search(void);

// ========================================
// Performance Tests
// ========================================

/**
 * Performance test with large trees
 * @return True if test passes
 */
bool test_performance_large_trees(void);

/**
 * Performance test with deep trees
 * @return True if test passes
 */
bool test_performance_deep_trees(void);

/**
 * Performance test with many trees
 * @return True if test passes
 */
bool test_performance_many_trees(void);

/**
 * Performance test with complex blackboard
 * @return True if test passes
 */
bool test_performance_complex_blackboard(void);

// ========================================
// Stress Tests
// ========================================

/**
 * Stress test with rapid tree switching
 * @return True if test passes
 */
bool stress_test_rapid_tree_switching(void);

/**
 * Stress test with memory pressure
 * @return True if test passes
 */
bool stress_test_memory_pressure(void);

/**
 * Stress test with node creation/destruction
 * @return True if test passes
 */
bool stress_test_node_creation_destruction(void);

/**
 * Stress test with concurrent execution
 * @return True if test passes
 */
bool stress_test_concurrent_execution(void);

// ========================================
// Utility Functions
// ========================================

/**
 * Validate tree structure
 * @param tree Tree to validate
 * @return True if tree is valid
 */
bool behavior_tree_validate_structure(const BehaviorTree *tree);

/**
 * Calculate tree depth
 * @param tree Behavior tree
 * @return Tree depth
 */
uint32_t behavior_tree_calculate_depth(const BehaviorTree *tree);

/**
 * Count nodes of specific type
 * @param tree Behavior tree
 * @param type Node type to count
 * @return Number of nodes of type
 */
uint32_t behavior_tree_count_nodes(const BehaviorTree *tree, BehaviorNodeType type);

/**
 * Get execution statistics
 * @param tree Behavior tree
 * @param total_nodes Output total nodes
 * @param executed_nodes Output executed nodes
 * @param success_rate Output success rate
 */
void behavior_tree_get_statistics(const BehaviorTree *tree, 
                                uint32_t *total_nodes,
                                uint32_t *executed_nodes,
                                float *success_rate);

/**
 * Generate tree execution report
 * @param tree Behavior tree
 * @param report Output report string
 * @param max_size Maximum report size
 * @return True if report generated
 */
bool behavior_tree_generate_report(const BehaviorTree *tree, 
                                 char *report, size_t max_size);

/**
 * Export tree to JSON
 * @param tree Behavior tree
 * @param filename Output filename
 * @return True if export successful
 */
bool behavior_tree_export_json(const BehaviorTree *tree, const char *filename);

/**
 * Import tree from JSON
 * @param filename Input filename
 * @return Imported tree or NULL on failure
 */
BehaviorTree* behavior_tree_import_json(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* AI_BEHAVIOR_TREE_TESTS_H */
