// include/core/thread_pool.h
//
// Purpose: Thread pool interface for parallel task execution

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <core/types.h>

// Forward declaration
typedef struct ThreadPool ThreadPool;

// Job function type
typedef bool (*JobFunction)(void* user_data);

// Thread pool lifecycle
bool thread_pool_init(u32 thread_count);
void thread_pool_shutdown(void);
ThreadPool* thread_pool_get_global(void);

// Thread pool management
ThreadPool* thread_pool_create(u32 thread_count);
void thread_pool_destroy(ThreadPool* pool);

// Job submission
u32 thread_pool_submit(ThreadPool* pool, JobFunction function, void* user_data, const char* name);
u32 thread_pool_submit_with_dependencies(ThreadPool* pool, JobFunction function, void* user_data, 
                                       u32* dependency_ids, u32 dependency_count, const char* name);

// Job management
bool thread_pool_wait_for_job(ThreadPool* pool, u32 job_id, u32 timeout_ms);

// Statistics
void thread_pool_get_stats(ThreadPool* pool, u64* submitted, u64* completed, u64* failed);

#endif // THREAD_POOL_H
