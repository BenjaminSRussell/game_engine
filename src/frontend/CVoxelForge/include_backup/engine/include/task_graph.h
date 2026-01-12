#ifndef TASK_GRAPH_H
#define TASK_GRAPH_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct TaskGraph TaskGraph;
typedef struct TaskNode TaskNode;

/**
 * Create a new task graph
 * @param initial_capacity Initial capacity for tasks
 * @return Pointer to task graph or NULL on failure
 */
TaskGraph* task_graph_create(uint32_t initial_capacity);

/**
 * Destroy a task graph and cleanup resources
 * @param graph Pointer to task graph
 */
void task_graph_destroy(TaskGraph *graph);

/**
 * Add a task to the graph
 * @param graph Pointer to task graph
 * @param function Function to execute for this task
 * @param data User data to pass to function
 * @param name Optional name for debugging
 * @return Task ID (0 on failure)
 */
uint32_t task_graph_add_task(TaskGraph *graph, void (*function)(void*), void *data, const char *name);

/**
 * Add dependency between tasks (task depends on dependency)
 * @param graph Pointer to task graph
 * @param task_id ID of task that depends on dependency
 * @param dependency_id ID of dependency task
 * @return True if dependency was added successfully
 */
bool task_graph_add_dependency(TaskGraph *graph, uint32_t task_id, uint32_t dependency_id);

/**
 * Execute the task graph with specified number of worker threads
 * @param graph Pointer to task graph
 * @param worker_count Number of worker threads
 * @return True if execution started successfully
 */
bool task_graph_execute(TaskGraph *graph, uint32_t worker_count);

/**
 * Wait for all tasks in the graph to complete
 * @param graph Pointer to task graph
 */
void task_graph_wait_completion(TaskGraph *graph);

/**
 * Stop task graph execution
 * @param graph Pointer to task graph
 */
void task_graph_stop_execution(TaskGraph *graph);

/**
 * Get task graph statistics
 * @param graph Pointer to task graph
 * @param total_tasks Output for total number of tasks (can be NULL)
 * @param completed_tasks Output for completed tasks (can be NULL)
 * @param total_execution_time_us Output for total execution time in microseconds (can be NULL)
 */
void task_graph_get_stats(TaskGraph *graph, uint32_t *total_tasks, uint64_t *completed_tasks, 
                        uint64_t *total_execution_time_us);

/**
 * Set completion callback for a specific task
 * @param graph Pointer to task graph
 * @param task_id ID of task
 * @param callback Function to call when task completes
 */
void task_graph_set_completion_callback(TaskGraph *graph, uint32_t task_id, void (*callback)(TaskNode*));

/**
 * Check if a task is completed
 * @param graph Pointer to task graph
 * @param task_id ID of task
 * @return True if task is completed
 */
bool task_graph_is_task_completed(TaskGraph *graph, uint32_t task_id);

/**
 * Check if the graph is currently executing
 * @param graph Pointer to task graph
 * @return True if graph is executing
 */
bool task_graph_is_executing(TaskGraph *graph);

/**
 * Get task by ID
 * @param graph Pointer to task graph
 * @param task_id ID of task
 * @return Pointer to task node or NULL if not found
 */
TaskNode* task_graph_get_task(TaskGraph *graph, uint32_t task_id);

/**
 * Get task name
 * @param task Pointer to task node
 * @return Task name or NULL
 */
const char* task_graph_get_task_name(TaskNode *task);

/**
 * Get task data
 * @param task Pointer to task node
 * @return Task data pointer
 */
void* task_graph_get_task_data(TaskNode *task);

#ifdef __cplusplus
}
#endif

#endif // TASK_GRAPH_H
