#pragma once

#include <common.h>
#include <core/types.h>
#include <stddef.h>

/**
 * =================================================================================================
 *                              JOB SCHEDULER SYSTEM
 * =================================================================================================
 *
 * Purpose: High-performance job scheduling system for parallel task execution.
 * Supports job priorities, dependencies, and work stealing for efficient load balancing.
 */

/* ===== JOB TYPES ===== */

typedef enum {
    JOB_PRIORITY_LOW,
    JOB_PRIORITY_NORMAL,
    JOB_PRIORITY_HIGH,
    JOB_PRIORITY_CRITICAL,
} JobPriority;

typedef enum {
    JOB_STATE_QUEUED,
    JOB_STATE_RUNNING,
    JOB_STATE_COMPLETED,
    JOB_STATE_FAILED,
} JobState;

/**
 * Job function signature
 */
typedef void (*JobFunction)(void *data);

/**
 * Job handle for tracking and synchronization
 */
typedef struct {
    u32 id;
    volatile u32 state;
    u32 dependency_count;
} JobHandle;

/* ===== JOB SCHEDULER API ===== */

/**
 * Initialize the job scheduler with specified worker thread count
 */
void job_scheduler_init(u32 worker_count);

/**
 * Shutdown the job scheduler
 */
void job_scheduler_shutdown(void);

/**
 * Submit a job to the scheduler
 */
JobHandle job_submit(JobFunction func, void *data, JobPriority priority);

/**
 * Submit a job with dependency on another job
 */
JobHandle job_submit_with_dependency(JobFunction func, void *data, JobPriority priority, JobHandle *dependencies, u32 dep_count);

/**
 * Wait for a job to complete
 */
void job_wait(JobHandle job);

/**
 * Wait for multiple jobs to complete
 */
void job_wait_all(JobHandle *jobs, u32 count);

/**
 * Check if a job has completed
 */
bool job_is_complete(JobHandle job);

/**
 * Get job current state
 */
JobState job_get_state(JobHandle job);

/**
 * Get number of active worker threads
 */
u32 job_get_worker_count(void);

/**
 * Process pending jobs (for single-threaded job execution)
 */
void job_process_pending(void);

