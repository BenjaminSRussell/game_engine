// include/thread/job.h
//
// Purpose: Defines the public API and data structures for a robust thread pool
// and job queue system. This header provides mechanisms for offloading tasks
// (jobs) to a pool of worker threads, enabling efficient parallel processing
// and preventing the main thread from blocking on computationally intensive
// operations. It supports prioritized job execution and thread-safe queue
// management.
//
// Public APIs:
// - `JobFunction`: Function pointer signature for the task that a job will
// execute.
// - `Job`: Structure representing a single work item, containing its `function`
// to execute,
//   associated `data`, a `priority` level, and a `completed` flag.
// - `JobQueue`: A circular buffer queue structure for holding `Job`s, managed
// with
//   mutexes and condition variables (or a spinlock for web platforms) for
//   thread-safe access.
// - `ThreadPool`: The main structure managing a collection of worker threads
// and a `JobQueue`.
//   It controls the lifecycle of the threads and the execution of jobs.
// - `thread_pool_init`: Initializes the thread pool with a specified number of
// worker threads.
// - `thread_pool_free`: Shuts down the thread pool, freeing all associated
// resources.
// - `thread_pool_submit`: Adds a new `Job` to the queue for processing by the
// thread pool.
// - `thread_pool_wait`: Blocks until all currently submitted jobs have been
// completed.
// - `thread_pool_queue_size`: Returns the current number of jobs waiting in the
// queue.
// - `job_queue_init`, `job_queue_free`: Lifecycle functions for a `JobQueue`.
// - `job_queue_push`, `job_queue_pop`: Thread-safe operations for adding and
// removing jobs from the queue.
// - `job_queue_empty`: Checks if the job queue is empty.
//
// Ownership: The `ThreadPool` owns its worker threads and the `JobQueue`. The
// `JobQueue` owns the array of `Job`s. `Job`s store a `void* data` pointer, but
// do not own the memory pointed to by `data`; that memory must be managed
// externally by the job submitter.
//
// Invariants:
// - A `ThreadPool` must be initialized before submitting jobs and freed at
// shutdown.
// - `thread_count` should be carefully chosen based on system capabilities.
// - `JobQueue` operations are thread-safe, ensuring integrity in concurrent
// environments.
// - Jobs submitted must not block indefinitely or cause deadlocks within the
// pool.
// - `pthread_mutex_t` and `pthread_cond_t` are used for synchronization on
// non-web platforms.
//
#ifndef JOB_H
#define JOB_H

#include <common.h>

#ifndef PLATFORM_WEB
#include <pthread.h>
#endif

// Roadmap: docs/JOB_SYSTEM_ROADMAP.md.

// Job function signature
typedef void (*JobFunction)(void *data);

// Job structure
typedef struct {
  JobFunction function;
  void *data;
  u32 priority;
  bool completed;
} Job;

// Job queue with mutex and condition variables
typedef struct {
  Job *jobs;
  u32 count;
  u32 capacity;
  u32 head; // Read position
  u32 tail; // Write position

#ifndef PLATFORM_WEB
  pthread_mutex_t mutex;    // Protects queue state
  pthread_cond_t not_empty; // Signals workers when jobs available
  pthread_cond_t not_full;  // Signals submitters when space available
#else
  volatile u32 lock; // Web: fallback to spinlock
#endif
} JobQueue;

// Thread pool
typedef struct ThreadPool {
  void **threads; // Platform-specific thread handles
  u32 thread_count;
  JobQueue queue;
  volatile bool running;
} ThreadPool;

// Initialize thread pool (returns false on failure)
bool thread_pool_init(ThreadPool *pool, u32 thread_count);
void thread_pool_free(ThreadPool *pool);

// Submit job
void thread_pool_submit(ThreadPool *pool, JobFunction function, void *data,
                        u32 priority);

// Wait for all jobs to complete
void thread_pool_wait(ThreadPool *pool);

// Query queue size
u32 thread_pool_queue_size(ThreadPool *pool);

// Job queue operations (init returns false on failure)
bool job_queue_init(JobQueue *queue, u32 capacity);
void job_queue_free(JobQueue *queue);
bool job_queue_push(JobQueue *queue, Job job);
bool job_queue_pop(JobQueue *queue, Job *job);
bool job_queue_empty(JobQueue *queue);

#endif // JOB_H
