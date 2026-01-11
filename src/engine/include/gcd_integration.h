#ifndef GCD_INTEGRATION_H
#define GCD_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

// GCD priority levels
typedef enum {
    GCD_PRIORITY_HIGH = 0,
    GCD_PRIORITY_DEFAULT = 1,
    GCD_PRIORITY_LOW = 2,
    GCD_PRIORITY_BACKGROUND = 3
} GCDPriority;

/**
 * Initialize GCD integration (Apple platforms only)
 * @return True if initialization succeeded
 */
bool gcd_initialize(void);

/**
 * Shutdown GCD integration and cleanup resources
 */
void gcd_shutdown(void);

/**
 * Submit a task to GCD for asynchronous execution
 * @param function Function to execute
 * @param data User data to pass to function
 * @param priority Task priority level
 * @param name Optional name for debugging
 */
void gcd_submit_task(void (*function)(void*), void *data, GCDPriority priority, const char *name);

/**
 * Submit a task with completion counter
 * @param function Function to execute
 * @param data User data to pass to function
 * @param counter Atomic counter to decrement when task completes
 * @param priority Task priority level
 * @param name Optional name for debugging
 */
void gcd_submit_task_with_counter(void (*function)(void*), void *data, atomic_int *counter, 
                                 GCDPriority priority, const char *name);

/**
 * Wait for a counter to reach zero
 * @param counter Counter to wait for
 */
void gcd_wait_for_counter(atomic_int *counter);

/**
 * Execute a parallel for loop using GCD
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param chunk_size Size of each chunk per iteration
 * @param function Function to call for each chunk
 * @param data User data
 */
void gcd_parallel_for(uint32_t start, uint32_t end, uint32_t chunk_size, 
                     void (*function)(uint32_t, uint32_t, void*), void *data);

/**
 * Execute a parallel for loop with completion counter
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param chunk_size Size of each chunk per iteration
 * @param function Function to call for each chunk
 * @param data User data
 * @param counter Counter to track completion
 */
void gcd_parallel_for_with_counter(uint32_t start, uint32_t end, uint32_t chunk_size, 
                                  void (*function)(uint32_t, uint32_t, void*), void *data, 
                                  atomic_int *counter);

/**
 * Submit a task to serial queue (executes in order)
 * @param function Function to execute
 * @param data User data to pass to function
 * @param name Optional name for debugging
 */
void gcd_submit_serial_task(void (*function)(void*), void *data, const char *name);

/**
 * Submit a task for synchronous execution
 * @param function Function to execute
 * @param data User data to pass to function
 * @param name Optional name for debugging
 */
void gcd_submit_sync_task(void (*function)(void*), void *data, const char *name);

/**
 * Get system processor information
 * @param active_processors Output for active processor count (can be NULL)
 * @param logical_processors Output for logical processor count (can be NULL)
 */
void gcd_get_system_info(uint32_t *active_processors, uint32_t *logical_processors);

/**
 * Check if GCD is available on this platform
 * @return True if GCD is available
 */
bool gcd_is_available(void);

#ifdef __cplusplus
}
#endif

#endif // GCD_INTEGRATION_H
