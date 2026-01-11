#include "include/gcd_integration.h"

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

/**
 * =================================================================================================
 *                             GRAND CENTRAL DISPATCH INTEGRATION
 * =================================================================================================
 */

// GCD task wrapper
typedef struct {
    void (*function)(void *data);
    void *data;
    atomic_int *counter;
    const char *name;
} GCDTask;

// GCD parallel for wrapper
typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t chunk_size;
    void (*function)(uint32_t, uint32_t, void*);
    void *user_data;
    atomic_int *counter;
} GCDParallelForData;

// Global dispatch queues
static dispatch_queue_t g_global_queue;
static dispatch_queue_t g_serial_queue;
static dispatch_queue_t g_concurrent_queue;
static bool g_gcd_initialized = false;

// Initialize GCD integration
bool gcd_initialize(void) {
    if (g_gcd_initialized) {
        return true;
    }
    
    // Create global concurrent queue with default priority
    g_concurrent_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    if (!g_concurrent_queue) {
        return false;
    }
    
    // Create serial queue for synchronization
    g_serial_queue = dispatch_queue_create("com.minecraft.engine.serial", DISPATCH_QUEUE_SERIAL);
    if (!g_serial_queue) {
        return false;
    }
    
    // Create custom concurrent queue
    g_global_queue = dispatch_queue_create("com.minecraft.engine.concurrent", DISPATCH_QUEUE_CONCURRENT);
    if (!g_global_queue) {
        dispatch_release(g_serial_queue);
        return false;
    }
    
    g_gcd_initialized = true;
    return true;
}

// Shutdown GCD integration
void gcd_shutdown(void) {
    if (!g_gcd_initialized) {
        return;
    }
    
    if (g_global_queue) {
        dispatch_release(g_global_queue);
        g_global_queue = NULL;
    }
    
    if (g_serial_queue) {
        dispatch_release(g_serial_queue);
        g_serial_queue = NULL;
    }
    
    // Note: We don't release the global queue as it's system-managed
    
    g_gcd_initialized = false;
}

// GCD task wrapper function
static void gcd_task_wrapper(void *context) {
    GCDTask *task = (GCDTask*)context;
    
    if (task && task->function) {
        task->function(task->data);
        
        // Decrement counter if provided
        if (task->counter) {
            atomic_fetch_sub(task->counter, 1);
        }
    }
    
    // Cleanup task
    if (task) {
        free(task);
    }
}

// Submit a task to GCD
void gcd_submit_task(void (*function)(void*), void *data, GCDPriority priority, const char *name) {
    if (!g_gcd_initialized || !function) {
        return;
    }
    
    GCDTask *task = (GCDTask*)malloc(sizeof(GCDTask));
    if (!task) {
        return;
    }
    
    task->function = function;
    task->data = data;
    task->counter = NULL;
    task->name = name;
    
    dispatch_queue_t queue = g_concurrent_queue;
    
    // Select queue based on priority
    switch (priority) {
        case GCD_PRIORITY_HIGH:
            queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
            break;
        case GCD_PRIORITY_DEFAULT:
            queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
            break;
        case GCD_PRIORITY_LOW:
            queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
            break;
        case GCD_PRIORITY_BACKGROUND:
            queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
            break;
    }
    
    dispatch_async_f(queue, task, gcd_task_wrapper);
}

// Submit a task with completion counter
void gcd_submit_task_with_counter(void (*function)(void*), void *data, atomic_int *counter, 
                                 GCDPriority priority, const char *name) {
    if (!g_gcd_initialized || !function) {
        return;
    }
    
    GCDTask *task = (GCDTask*)malloc(sizeof(GCDTask));
    if (!task) {
        return;
    }
    
    task->function = function;
    task->data = data;
    task->counter = counter;
    task->name = name;
    
    // Increment counter before submitting
    if (counter) {
        atomic_fetch_add(counter, 1);
    }
    
    dispatch_queue_t queue = g_concurrent_queue;
    
    // Select queue based on priority
    switch (priority) {
        case GCD_PRIORITY_HIGH:
            queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
            break;
        case GCD_PRIORITY_DEFAULT:
            queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
            break;
        case GCD_PRIORITY_LOW:
            queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
            break;
        case GCD_PRIORITY_BACKGROUND:
            queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
            break;
    }
    
    dispatch_async_f(queue, task, gcd_task_wrapper);
}

// Wait for counter to reach zero
void gcd_wait_for_counter(atomic_int *counter) {
    if (!counter) {
        return;
    }
    
    // Spin wait with yield
    while (atomic_load(counter) > 0) {
        sched_yield();
    }
}

// GCD parallel for wrapper function
static void gcd_parallel_for_wrapper(void *context, size_t index) {
    GCDParallelForData *data = (GCDParallelForData*)context;
    
    uint32_t start = data->start + (index * data->chunk_size);
    uint32_t end = start + data->chunk_size;
    
    // Clamp to the overall end
    if (end > data->end) {
        end = data->end;
    }
    
    if (start < end && data->function) {
        data->function(start, end, data->user_data);
    }
    
    // Decrement counter if provided
    if (data->counter) {
        atomic_fetch_sub(data->counter, 1);
    }
}

// Parallel for implementation
void gcd_parallel_for(uint32_t start, uint32_t end, uint32_t chunk_size, 
                     void (*function)(uint32_t, uint32_t, void*), void *data) {
    if (!g_gcd_initialized || !function || start >= end || chunk_size == 0) {
        return;
    }
    
    uint32_t total_range = end - start;
    uint32_t chunk_count = (total_range + chunk_size - 1) / chunk_size;
    
    GCDParallelForData parallel_data = {
        .start = start,
        .end = end,
        .chunk_size = chunk_size,
        .function = function,
        .user_data = data,
        .counter = NULL
    };
    
    // Use dispatch_apply for parallel execution
    dispatch_apply_f(chunk_count, g_concurrent_queue, &parallel_data, gcd_parallel_for_wrapper);
}

// Parallel for with counter
void gcd_parallel_for_with_counter(uint32_t start, uint32_t end, uint32_t chunk_size, 
                                  void (*function)(uint32_t, uint32_t, void*), void *data, 
                                  atomic_int *counter) {
    if (!g_gcd_initialized || !function || start >= end || chunk_size == 0) {
        return;
    }
    
    uint32_t total_range = end - start;
    uint32_t chunk_count = (total_range + chunk_size - 1) / chunk_size;
    
    // Set counter
    if (counter) {
        atomic_store(counter, chunk_count);
    }
    
    GCDParallelForData parallel_data = {
        .start = start,
        .end = end,
        .chunk_size = chunk_size,
        .function = function,
        .user_data = data,
        .counter = counter
    };
    
    // Use dispatch_apply for parallel execution
    dispatch_apply_f(chunk_count, g_concurrent_queue, &parallel_data, gcd_parallel_for_wrapper);
}

// Submit task to serial queue
void gcd_submit_serial_task(void (*function)(void*), void *data, const char *name) {
    if (!g_gcd_initialized || !function) {
        return;
    }
    
    GCDTask *task = (GCDTask*)malloc(sizeof(GCDTask));
    if (!task) {
        return;
    }
    
    task->function = function;
    task->data = data;
    task->counter = NULL;
    task->name = name;
    
    dispatch_async_f(g_serial_queue, task, gcd_task_wrapper);
}

// Submit task synchronously
void gcd_submit_sync_task(void (*function)(void*), void *data, const char *name) {
    if (!g_gcd_initialized || !function) {
        return;
    }
    
    GCDTask task = {
        .function = function,
        .data = data,
        .counter = NULL,
        .name = name
    };
    
    dispatch_sync_f(g_global_queue, &task, gcd_task_wrapper);
}

// Get GCD system information
void gcd_get_system_info(uint32_t *active_processors, uint32_t *logical_processors) {
    if (active_processors) {
        *active_processors = (uint32_t)[[NSProcessInfo processInfo] activeProcessorCount];
    }
    
    if (logical_processors) {
        *logical_processors = (uint32_t)[[NSProcessInfo processInfo] processorCount];
    }
}

// Check if GCD is available
bool gcd_is_available(void) {
    return g_gcd_initialized;
}

#else
// Non-Apple platform stubs
bool gcd_initialize(void) { return false; }
void gcd_shutdown(void) {}
void gcd_submit_task(void (*function)(void*), void *data, GCDPriority priority, const char *name) { (void)function; (void)data; (void)priority; (void)name; }
void gcd_submit_task_with_counter(void (*function)(void*), void *data, atomic_int *counter, GCDPriority priority, const char *name) { (void)function; (void)data; (void)counter; (void)priority; (void)name; }
void gcd_wait_for_counter(atomic_int *counter) { (void)counter; }
void gcd_parallel_for(uint32_t start, uint32_t end, uint32_t chunk_size, void (*function)(uint32_t, uint32_t, void*), void *data) { (void)start; (void)end; (void)chunk_size; (void)function; (void)data; }
void gcd_parallel_for_with_counter(uint32_t start, uint32_t end, uint32_t chunk_size, void (*function)(uint32_t, uint32_t, void*), void *data, atomic_int *counter) { (void)start; (void)end; (void)chunk_size; (void)function; (void)data; (void)counter; }
void gcd_submit_serial_task(void (*function)(void*), void *data, const char *name) { (void)function; (void)data; (void)name; }
void gcd_submit_sync_task(void (*function)(void*), void *data, const char *name) { (void)function; (void)data; (void)name; }
void gcd_get_system_info(uint32_t *active_processors, uint32_t *logical_processors) { (void)active_processors; (void)logical_processors; }
bool gcd_is_available(void) { return false; }
#endif
