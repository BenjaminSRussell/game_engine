#include "task_graph.h"
#include "core/memory/memory.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Task node structure
struct TaskNode {
    uint32_t id;
    void (*function)(void*);
    void *data;
    char *name;
    
    // Dependencies
    uint32_t *dependencies;
    uint32_t dependency_count;
    uint32_t dependency_capacity;
    
    // Dependents (tasks that depend on this task)
    uint32_t *dependents;
    uint32_t dependent_count;
    uint32_t dependent_capacity;
    
    // Execution state
    atomic_bool completed;
    atomic_bool executing;
    uint32_t remaining_dependencies;
    
    // Callback
    void (*completion_callback)(TaskNode*);
    
    // Timing
    uint64_t start_time_us;
    uint64_t end_time_us;
};

// Task graph structure
struct TaskGraph {
    TaskNode *tasks;
    uint32_t task_count;
    uint32_t task_capacity;
    uint32_t next_task_id;
    
    // Execution state
    atomic_bool executing;
    atomic_bool should_stop;
    pthread_mutex_t execution_mutex;
    pthread_cond_t execution_cond;
    
    // Worker threads
    pthread_t *workers;
    uint32_t worker_count;
    
    // Statistics
    atomic_uint64_t completed_tasks;
    uint64_t total_execution_time_us;
    
    // Thread pool for task execution
    pthread_mutex_t task_mutex;
    pthread_cond_t task_available;
    pthread_cond_t task_completed;
};

// Internal helper functions
static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static TaskNode* find_task(TaskGraph *graph, uint32_t task_id) {
    for (uint32_t i = 0; i < graph->task_count; i++) {
        if (graph->tasks[i].id == task_id) {
            return &graph->tasks[i];
        }
    }
    return NULL;
}

static bool can_execute_task(TaskNode *task) {
    return atomic_load(&task->completed) == false &&
           atomic_load(&task->executing) == false &&
           task->remaining_dependencies == 0;
}

static void execute_task(TaskNode *task) {
    atomic_store(&task->executing, true);
    task->start_time_us = get_time_us();
    
    if (task->function) {
        task->function(task->data);
    }
    
    task->end_time_us = get_time_us();
    atomic_store(&task->executing, false);
    atomic_store(&task->completed, true);
    
    if (task->completion_callback) {
        task->completion_callback(task);
    }
}

static void* worker_thread(void *arg) {
    TaskGraph *graph = (TaskGraph*)arg;
    
    while (atomic_load(&graph->should_stop) == false) {
        pthread_mutex_lock(&graph->task_mutex);
        
        // Find a ready task
        TaskNode *ready_task = NULL;
        for (uint32_t i = 0; i < graph->task_count; i++) {
            if (can_execute_task(&graph->tasks[i])) {
                ready_task = &graph->tasks[i];
                break;
            }
        }
        
        if (ready_task) {
            pthread_mutex_unlock(&graph->task_mutex);
            execute_task(ready_task);
            
            // Update dependent tasks
            pthread_mutex_lock(&graph->task_mutex);
            for (uint32_t i = 0; i < ready_task->dependent_count; i++) {
                uint32_t dependent_id = ready_task->dependents[i];
                TaskNode *dependent = find_task(graph, dependent_id);
                if (dependent && dependent->remaining_dependencies > 0) {
                    dependent->remaining_dependencies--;
                }
            }
            
            atomic_fetch_add(&graph->completed_tasks, 1);
            pthread_cond_broadcast(&graph->task_completed);
            pthread_mutex_unlock(&graph->task_mutex);
        } else {
            // No ready tasks, wait for completion signal
            pthread_cond_wait(&graph->task_available, &graph->task_mutex);
            pthread_mutex_unlock(&graph->task_mutex);
        }
    }
    
    return NULL;
}

// Public API implementation
TaskGraph* task_graph_create(uint32_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = 64;
    }
    
    TaskGraph *graph = (TaskGraph*)memory_alloc(sizeof(TaskGraph), __FILE__, __LINE__);
    if (!graph) {
        LOG_ERROR("Failed to allocate task graph");
        return NULL;
    }
    
    memset(graph, 0, sizeof(TaskGraph));
    
    graph->tasks = (TaskNode*)memory_alloc(sizeof(TaskNode) * initial_capacity, __FILE__, __LINE__);
    if (!graph->tasks) {
        LOG_ERROR("Failed to allocate task array");
        memory_free(graph, __FILE__, __LINE__);
        return NULL;
    }
    
    memset(graph->tasks, 0, sizeof(TaskNode) * initial_capacity);
    graph->task_capacity = initial_capacity;
    graph->next_task_id = 1;
    
    // Initialize synchronization objects
    if (pthread_mutex_init(&graph->execution_mutex, NULL) != 0 ||
        pthread_cond_init(&graph->execution_cond, NULL) != 0 ||
        pthread_mutex_init(&graph->task_mutex, NULL) != 0 ||
        pthread_cond_init(&graph->task_available, NULL) != 0 ||
        pthread_cond_init(&graph->task_completed, NULL) != 0) {
        
        LOG_ERROR("Failed to initialize synchronization objects");
        memory_free(graph->tasks, __FILE__, __LINE__);
        memory_free(graph, __FILE__, __LINE__);
        return NULL;
    }
    
    atomic_store(&graph->executing, false);
    atomic_store(&graph->should_stop, false);
    atomic_store(&graph->completed_tasks, 0);
    
    LOG_INFO("Task graph created with capacity %u", initial_capacity);
    return graph;
}

void task_graph_destroy(TaskGraph *graph) {
    if (!graph) return;
    
    task_graph_stop_execution(graph);
    
    // Cleanup tasks
    for (uint32_t i = 0; i < graph->task_count; i++) {
        TaskNode *task = &graph->tasks[i];
        if (task->name) {
            memory_free(task->name, __FILE__, __LINE__);
        }
        if (task->dependencies) {
            memory_free(task->dependencies, __FILE__, __LINE__);
        }
        if (task->dependents) {
            memory_free(task->dependents, __FILE__, __LINE__);
        }
    }
    
    // Cleanup synchronization objects
    pthread_mutex_destroy(&graph->execution_mutex);
    pthread_cond_destroy(&graph->execution_cond);
    pthread_mutex_destroy(&graph->task_mutex);
    pthread_cond_destroy(&graph->task_available);
    pthread_cond_destroy(&graph->task_completed);
    
    // Cleanup workers
    if (graph->workers) {
        memory_free(graph->workers, __FILE__, __LINE__);
    }
    
    memory_free(graph->tasks, __FILE__, __LINE__);
    memory_free(graph, __FILE__, __LINE__);
    
    LOG_INFO("Task graph destroyed");
}

uint32_t task_graph_add_task(TaskGraph *graph, void (*function)(void*), void *data, const char *name) {
    if (!graph || !function) return 0;
    
    pthread_mutex_lock(&graph->task_mutex);
    
    // Check if we need to expand the task array
    if (graph->task_count >= graph->task_capacity) {
        uint32_t new_capacity = graph->task_capacity * 2;
        TaskNode *new_tasks = (TaskNode*)memory_realloc(graph->tasks, sizeof(TaskNode) * new_capacity, __FILE__, __LINE__);
        if (!new_tasks) {
            pthread_mutex_unlock(&graph->task_mutex);
            LOG_ERROR("Failed to expand task array");
            return 0;
        }
        graph->tasks = new_tasks;
        graph->task_capacity = new_capacity;
    }
    
    // Initialize new task
    TaskNode *task = &graph->tasks[graph->task_count];
    memset(task, 0, sizeof(TaskNode));
    
    task->id = graph->next_task_id++;
    task->function = function;
    task->data = data;
    
    if (name) {
        size_t name_len = strlen(name);
        task->name = (char*)memory_alloc(name_len + 1, __FILE__, __LINE__);
        if (task->name) {
            strcpy(task->name, name);
        }
    }
    
    atomic_store(&task->completed, false);
    atomic_store(&task->executing, false);
    task->remaining_dependencies = 0;
    
    uint32_t task_id = task->id;
    graph->task_count++;
    
    pthread_mutex_unlock(&graph->task_mutex);
    
    LOG_DEBUG("Added task %u to graph", task_id);
    return task_id;
}

bool task_graph_add_dependency(TaskGraph *graph, uint32_t task_id, uint32_t dependency_id) {
    if (!graph) return false;
    
    pthread_mutex_lock(&graph->task_mutex);
    
    TaskNode *task = find_task(graph, task_id);
    TaskNode *dependency = find_task(graph, dependency_id);
    
    if (!task || !dependency) {
        pthread_mutex_unlock(&graph->task_mutex);
        LOG_ERROR("Task or dependency not found");
        return false;
    }
    
    // Check for circular dependency
    if (dependency_id == task_id) {
        pthread_mutex_unlock(&graph->task_mutex);
        LOG_ERROR("Circular dependency detected");
        return false;
    }
    
    // Add dependency to task
    if (task->dependency_count >= task->dependency_capacity) {
        uint32_t new_capacity = task->dependency_capacity == 0 ? 4 : task->dependency_capacity * 2;
        uint32_t *new_deps = (uint32_t*)memory_realloc(task->dependencies, sizeof(uint32_t) * new_capacity, __FILE__, __LINE__);
        if (!new_deps) {
            pthread_mutex_unlock(&graph->task_mutex);
            LOG_ERROR("Failed to expand dependency array");
            return false;
        }
        task->dependencies = new_deps;
        task->dependency_capacity = new_capacity;
    }
    
    task->dependencies[task->dependency_count++] = dependency_id;
    task->remaining_dependencies++;
    
    // Add task as dependent to dependency
    if (dependency->dependent_count >= dependency->dependent_capacity) {
        uint32_t new_capacity = dependency->dependent_capacity == 0 ? 4 : dependency->dependent_capacity * 2;
        uint32_t *new_deps = (uint32_t*)memory_realloc(dependency->dependents, sizeof(uint32_t) * new_capacity, __FILE__, __LINE__);
        if (!new_deps) {
            pthread_mutex_unlock(&graph->task_mutex);
            LOG_ERROR("Failed to expand dependent array");
            return false;
        }
        dependency->dependents = new_deps;
        dependency->dependent_capacity = new_capacity;
    }
    
    dependency->dependents[dependency->dependent_count++] = task_id;
    
    pthread_mutex_unlock(&graph->task_mutex);
    
    LOG_DEBUG("Added dependency: task %u depends on %u", task_id, dependency_id);
    return true;
}

bool task_graph_execute(TaskGraph *graph, uint32_t worker_count) {
    if (!graph || worker_count == 0) return false;
    
    pthread_mutex_lock(&graph->execution_mutex);
    
    if (atomic_load(&graph->executing)) {
        pthread_mutex_unlock(&graph->execution_mutex);
        LOG_ERROR("Task graph is already executing");
        return false;
    }
    
    atomic_store(&graph->executing, true);
    atomic_store(&graph->should_stop, false);
    atomic_store(&graph->completed_tasks, 0);
    graph->total_execution_time_us = get_time_us();
    
    // Create worker threads
    graph->worker_count = worker_count;
    graph->workers = (pthread_t*)memory_alloc(sizeof(pthread_t) * worker_count, __FILE__, __LINE__);
    if (!graph->workers) {
        atomic_store(&graph->executing, false);
        pthread_mutex_unlock(&graph->execution_mutex);
        LOG_ERROR("Failed to allocate worker threads");
        return false;
    }
    
    // Start worker threads
    for (uint32_t i = 0; i < worker_count; i++) {
        if (pthread_create(&graph->workers[i], NULL, worker_thread, graph) != 0) {
            LOG_ERROR("Failed to create worker thread %u", i);
            task_graph_stop_execution(graph);
            pthread_mutex_unlock(&graph->execution_mutex);
            return false;
        }
    }
    
    pthread_mutex_unlock(&graph->execution_mutex);
    
    LOG_INFO("Task graph execution started with %u workers", worker_count);
    return true;
}

void task_graph_wait_completion(TaskGraph *graph) {
    if (!graph) return;
    
    pthread_mutex_lock(&graph->task_mutex);
    
    while (atomic_load(&graph->executing) && 
           atomic_load(&graph->completed_tasks) < graph->task_count) {
        pthread_cond_wait(&graph->task_completed, &graph->task_mutex);
    }
    
    pthread_mutex_unlock(&graph->task_mutex);
    
    // Calculate total execution time
    graph->total_execution_time_us = get_time_us() - graph->total_execution_time_us;
    
    LOG_INFO("Task graph execution completed in %llu us", graph->total_execution_time_us);
}

void task_graph_stop_execution(TaskGraph *graph) {
    if (!graph) return;
    
    atomic_store(&graph->should_stop, true);
    atomic_store(&graph->executing, false);
    
    // Wake up all workers
    pthread_cond_broadcast(&graph->task_available);
    
    // Wait for workers to finish
    if (graph->workers) {
        for (uint32_t i = 0; i < graph->worker_count; i++) {
            pthread_join(graph->workers[i], NULL);
        }
        memory_free(graph->workers, __FILE__, __LINE__);
        graph->workers = NULL;
        graph->worker_count = 0;
    }
    
    LOG_INFO("Task graph execution stopped");
}

void task_graph_get_stats(TaskGraph *graph, uint32_t *total_tasks, uint64_t *completed_tasks, 
                        uint64_t *total_execution_time_us) {
    if (!graph) return;
    
    if (total_tasks) *total_tasks = graph->task_count;
    if (completed_tasks) *completed_tasks = atomic_load(&graph->completed_tasks);
    if (total_execution_time_us) *total_execution_time_us = graph->total_execution_time_us;
}

void task_graph_set_completion_callback(TaskGraph *graph, uint32_t task_id, void (*callback)(TaskNode*)) {
    if (!graph) return;
    
    pthread_mutex_lock(&graph->task_mutex);
    TaskNode *task = find_task(graph, task_id);
    if (task) {
        task->completion_callback = callback;
    }
    pthread_mutex_unlock(&graph->task_mutex);
}

bool task_graph_is_task_completed(TaskGraph *graph, uint32_t task_id) {
    if (!graph) return false;
    
    pthread_mutex_lock(&graph->task_mutex);
    TaskNode *task = find_task(graph, task_id);
    bool completed = task ? atomic_load(&task->completed) : false;
    pthread_mutex_unlock(&graph->task_mutex);
    
    return completed;
}

bool task_graph_is_executing(TaskGraph *graph) {
    return graph ? atomic_load(&graph->executing) : false;
}

#ifdef __cplusplus
}
#endif
