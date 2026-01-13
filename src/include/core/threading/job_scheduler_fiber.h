#ifndef JOB_SCHEDULER_FIBER_H
#define JOB_SCHEDULER_FIBER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

// Job priority levels
typedef enum {
    JOB_PRIORITY_CRITICAL = 0,
    JOB_PRIORITY_HIGH = 1,
    JOB_PRIORITY_NORMAL = 2,
    JOB_PRIORITY_LOW = 3,
    JOB_PRIORITY_COUNT
} JobPriority;

/**
 * Initialize the job scheduler with fiber support
 * @param worker_count Number of worker threads to create
 * @param fiber_pool_size Number of fibers to pre-allocate
 * @param fiber_stack_size Stack size for each fiber (typically 128KB)
 * @return True if initialization succeeded
 */
bool job_scheduler_initialize(uint32_t worker_count, size_t fiber_pool_size, size_t fiber_stack_size);

/**
 * Shutdown the job scheduler and cleanup resources
 */
void job_scheduler_shutdown(void);

/**
 * Submit a job to the scheduler
 * @param function Function to execute
 * @param data User data to pass to function
 * @param priority Job priority level
 * @param name Optional name for debugging/profiling
 */
void job_scheduler_submit(void (*function)(void*), void *data, JobPriority priority, const char *name);

/**
 * Submit a job with dependency counter
 * @param function Function to execute
 * @param data User data to pass to function
 * @param counter Atomic counter to decrement when job completes
 * @param priority Job priority level
 * @param name Optional name for debugging/profiling
 */
void job_scheduler_submit_with_counter(void (*function)(void*), void *data, atomic_int *counter, JobPriority priority, const char *name);

/**
 * Wait for a counter to reach zero
 * @param counter Counter to wait for
 */
void job_scheduler_wait_for_counter(atomic_int *counter);

/**
 * Yield current fiber to allow other jobs to run
 */
void job_scheduler_yield(void);

/**
 * Get scheduler statistics
 * @param total_jobs_submitted Output for total jobs submitted (can be NULL)
 * @param total_jobs_completed Output for total jobs completed (can be NULL)
 * @param total_context_switches Output for total context switches (can be NULL)
 */
void job_scheduler_get_stats(uint64_t *total_jobs_submitted, uint64_t *total_jobs_completed, uint64_t *total_context_switches);

/**
 * Get worker thread statistics
 * @param worker_id Worker thread ID
 * @param jobs_processed Output for jobs processed (can be NULL)
 * @param context_switches Output for context switches (can be NULL)
 * @param utilization Output for thread utilization (can be NULL)
 * @return True if worker exists
 */
bool job_scheduler_get_worker_stats(uint32_t worker_id, uint64_t *jobs_processed, uint64_t *context_switches, double *utilization);

/**
 * Parallel for loop implementation
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param chunk_size Size of each chunk per job
 * @param function Function to call for each chunk
 * @param data User data
 */
void job_scheduler_parallel_for(uint32_t start, uint32_t end, uint32_t chunk_size, void (*function)(uint32_t, uint32_t, void*), void *data);

#ifdef __cplusplus
}
#endif

#endif // JOB_SCHEDULER_FIBER_H
