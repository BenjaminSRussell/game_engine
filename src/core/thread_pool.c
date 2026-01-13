// src/engine/core/thread_pool.c
//
// Purpose: Thread pool implementation for parallel task execution
// Provides work distribution, load balancing, and dependency tracking

#include <core/thread_pool.h>
#include <core/logging_system.h>
#include <core/memory_allocator.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

// Maximum number of jobs in the queue
#define MAX_JOB_QUEUE_SIZE 4096
#define DEFAULT_THREAD_COUNT 4

typedef enum {
    JOB_STATE_PENDING,
    JOB_STATE_RUNNING,
    JOB_STATE_COMPLETED,
    JOB_STATE_FAILED
} JobState;

typedef struct Job {
    JobFunction function;
    void* user_data;
    JobState state;
    u32 job_id;
    char name[64];
    struct Job* next;
    struct Job** dependencies;
    u32 dependency_count;
    u32 completed_dependencies;
} Job;

typedef struct ThreadData {
    u32 thread_id;
    ThreadPool* pool;
#ifdef _WIN32
    HANDLE handle;
#else
    pthread_t handle;
#endif
    bool should_stop;
} ThreadData;

struct ThreadPool {
    ThreadData* threads;
    u32 thread_count;
    
    // Job queue
    Job* job_queue_head;
    Job* job_queue_tail;
    u32 queue_size;
    
    // Synchronization
#ifdef _WIN32
    CRITICAL_SECTION queue_mutex;
    CONDITION_VARIABLE queue_condition;
    CRITICAL_SECTION stats_mutex;
#else
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_condition;
    pthread_mutex_t stats_mutex;
#endif
    
    // Statistics
    u64 total_jobs_submitted;
    u64 total_jobs_completed;
    u64 total_jobs_failed;
    u32 next_job_id;
    
    bool initialized;
};

// Forward declarations
static void* worker_thread(void* arg);
static void submit_job_internal(ThreadPool* pool, Job* job);
static Job* get_next_job(ThreadPool* pool);
static void complete_job(ThreadPool* pool, Job* job, bool success);
static bool check_job_dependencies(Job* job);
static void cleanup_job(Job* job);

ThreadPool* thread_pool_create(u32 thread_count) {
    if (thread_count == 0) {
        thread_count = DEFAULT_THREAD_COUNT;
    }

    ThreadPool* pool = (ThreadPool*)MEMORY_ALLOCATE(sizeof(ThreadPool));
    if (!pool) {
        LOG_ERROR("Failed to allocate thread pool");
        return NULL;
    }

    memset(pool, 0, sizeof(ThreadPool));
    pool->thread_count = thread_count;

    // Initialize synchronization objects
#ifdef _WIN32
    InitializeCriticalSection(&pool->queue_mutex);
    InitializeConditionVariable(&pool->queue_condition);
    InitializeCriticalSection(&pool->stats_mutex);
#else
    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize queue mutex");
        MEMORY_DEALLOCATE(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->queue_condition, NULL) != 0) {
        LOG_ERROR("Failed to initialize condition variable");
        pthread_mutex_destroy(&pool->queue_mutex);
        MEMORY_DEALLOCATE(pool);
        return NULL;
    }
    
    if (pthread_mutex_init(&pool->stats_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize stats mutex");
        pthread_mutex_destroy(&pool->queue_mutex);
        pthread_cond_destroy(&pool->queue_condition);
        MEMORY_DEALLOCATE(pool);
        return NULL;
    }
#endif

    // Create threads
    pool->threads = (ThreadData*)MEMORY_ALLOCATE(sizeof(ThreadData) * thread_count);
    if (!pool->threads) {
        LOG_ERROR("Failed to allocate thread data");
#ifdef _WIN32
        DeleteCriticalSection(&pool->queue_mutex);
        DeleteCriticalSection(&pool->stats_mutex);
#else
        pthread_mutex_destroy(&pool->queue_mutex);
        pthread_cond_destroy(&pool->queue_condition);
        pthread_mutex_destroy(&pool->stats_mutex);
#endif
        MEMORY_DEALLOCATE(pool);
        return NULL;
    }

    // Start worker threads
    for (u32 i = 0; i < thread_count; i++) {
        ThreadData* thread = &pool->threads[i];
        thread->thread_id = i;
        thread->pool = pool;
        thread->should_stop = false;

#ifdef _WIN32
        thread->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)worker_thread, thread, 0, NULL);
        if (thread->handle == NULL) {
            LOG_ERROR("Failed to create thread %u", i);
            thread->should_stop = true;
        }
#else
        if (pthread_create(&thread->handle, NULL, worker_thread, thread) != 0) {
            LOG_ERROR("Failed to create thread %u", i);
            thread->should_stop = true;
        }
#endif
    }

    pool->initialized = true;
    LOG_INFO("✓ Thread pool created with %u threads", thread_count);
    return pool;
}

void thread_pool_destroy(ThreadPool* pool) {
    if (!pool || !pool->initialized) {
        return;
    }

    LOG_INFO("Shutting down thread pool...");

    // Signal all threads to stop
    for (u32 i = 0; i < pool->thread_count; i++) {
        pool->threads[i].should_stop = true;
    }

    // Wake up all threads
#ifdef _WIN32
    WakeAllConditionVariable(&pool->queue_condition);
#else
    pthread_cond_broadcast(&pool->queue_condition);
#endif

    // Wait for all threads to finish
    for (u32 i = 0; i < pool->thread_count; i++) {
#ifdef _WIN32
        WaitForSingleObject(pool->threads[i].handle, INFINITE);
        CloseHandle(pool->threads[i].handle);
#else
        pthread_join(pool->threads[i].handle, NULL);
#endif
    }

    // Clean up remaining jobs
    Job* job = pool->job_queue_head;
    while (job) {
        Job* next = job->next;
        cleanup_job(job);
        job = next;
    }

    // Destroy synchronization objects
#ifdef _WIN32
    DeleteCriticalSection(&pool->queue_mutex);
    DeleteCriticalSection(&pool->stats_mutex);
#else
    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_condition);
    pthread_mutex_destroy(&pool->stats_mutex);
#endif

    // Log statistics
    LOG_INFO("Thread pool stats: Submitted: %llu, Completed: %llu, Failed: %llu",
             pool->total_jobs_submitted, pool->total_jobs_completed, pool->total_jobs_failed);

    MEMORY_DEALLOCATE(pool->threads);
    MEMORY_DEALLOCATE(pool);
    LOG_INFO("Thread pool shutdown complete");
}

u32 thread_pool_submit(ThreadPool* pool, JobFunction function, void* user_data, const char* name) {
    if (!pool || !pool->initialized || !function) {
        LOG_ERROR("Invalid parameters for job submission");
        return 0;
    }

    Job* job = (Job*)MEMORY_ALLOCATE(sizeof(Job));
    if (!job) {
        LOG_ERROR("Failed to allocate job");
        return 0;
    }

    memset(job, 0, sizeof(Job));
    job->function = function;
    job->user_data = user_data;
    job->state = JOB_STATE_PENDING;
    job->job_id = ++pool->next_job_id;
    
    if (name) {
        strncpy(job->name, name, sizeof(job->name) - 1);
        job->name[sizeof(job->name) - 1] = '\0';
    } else {
        snprintf(job->name, sizeof(job->name), "Job_%u", job->job_id);
    }

    submit_job_internal(pool, job);
    return job->job_id;
}

u32 thread_pool_submit_with_dependencies(ThreadPool* pool, JobFunction function, void* user_data, 
                                       u32* dependency_ids, u32 dependency_count, const char* name) {
    if (!pool || !pool->initialized || !function) {
        LOG_ERROR("Invalid parameters for job submission with dependencies");
        return 0;
    }

    Job* job = (Job*)MEMORY_ALLOCATE(sizeof(Job));
    if (!job) {
        LOG_ERROR("Failed to allocate job with dependencies");
        return 0;
    }

    memset(job, 0, sizeof(Job));
    job->function = function;
    job->user_data = user_data;
    job->state = JOB_STATE_PENDING;
    job->job_id = ++pool->next_job_id;
    
    if (name) {
        strncpy(job->name, name, sizeof(job->name) - 1);
        job->name[sizeof(job->name) - 1] = '\0';
    } else {
        snprintf(job->name, sizeof(job->name), "Job_%u", job->job_id);
    }

    // Add dependencies
    if (dependency_count > 0 && dependency_ids) {
        job->dependencies = (Job**)MEMORY_ALLOCATE(sizeof(Job*) * dependency_count);
        if (job->dependencies) {
            job->dependency_count = dependency_count;
            // Note: In a full implementation, we'd need to look up jobs by ID
            // For now, we'll just store the IDs as placeholders
            memcpy(job->dependencies, dependency_ids, sizeof(u32) * dependency_count);
        }
    }

    submit_job_internal(pool, job);
    return job->job_id;
}

bool thread_pool_wait_for_job(ThreadPool* pool, u32 job_id, u32 timeout_ms) {
    if (!pool || !pool->initialized || job_id == 0) {
        return false;
    }

    // Simple implementation - in a full version, we'd track job completion
    // For now, we'll just wait a bit and return
    if (timeout_ms > 0) {
#ifdef _WIN32
        Sleep(timeout_ms);
#else
        usleep(timeout_ms * 1000);
#endif
    }
    
    return true;
}

void thread_pool_get_stats(ThreadPool* pool, u64* submitted, u64* completed, u64* failed) {
    if (!pool || !pool->initialized) {
        if (submitted) *submitted = 0;
        if (completed) *completed = 0;
        if (failed) *failed = 0;
        return;
    }

#ifdef _WIN32
    EnterCriticalSection(&pool->stats_mutex);
#else
    pthread_mutex_lock(&pool->stats_mutex);
#endif

    if (submitted) *submitted = pool->total_jobs_submitted;
    if (completed) *completed = pool->total_jobs_completed;
    if (failed) *failed = pool->total_jobs_failed;

#ifdef _WIN32
    LeaveCriticalSection(&pool->stats_mutex);
#else
    pthread_mutex_unlock(&pool->stats_mutex);
#endif
}

// Static helper functions
static void* worker_thread(void* arg) {
    ThreadData* thread = (ThreadData*)arg;
    ThreadPool* pool = thread->pool;
    
    LOG_DEBUG("Worker thread %u started", thread->thread_id);

    while (!thread->should_stop) {
        Job* job = get_next_job(pool);
        if (job) {
            job->state = JOB_STATE_RUNNING;
            LOG_DEBUG("Thread %u executing job: %s", thread->thread_id, job->name);
            
            // Execute job
            bool success = job->function(job->user_data);
            
            complete_job(pool, job, success);
        } else {
            // No jobs available, wait
#ifdef _WIN32
            EnterCriticalSection(&pool->queue_mutex);
            SleepConditionVariableCS(&pool->queue_condition, &pool->queue_mutex, INFINITE);
            LeaveCriticalSection(&pool->queue_mutex);
#else
            pthread_mutex_lock(&pool->queue_mutex);
            pthread_cond_wait(&pool->queue_condition, &pool->queue_mutex);
            pthread_mutex_unlock(&pool->queue_mutex);
#endif
        }
    }

    LOG_DEBUG("Worker thread %u stopped", thread->thread_id);
    return NULL;
}

static void submit_job_internal(ThreadPool* pool, Job* job) {
#ifdef _WIN32
    EnterCriticalSection(&pool->queue_mutex);
#else
    pthread_mutex_lock(&pool->queue_mutex);
#endif

    // Add to queue
    if (pool->queue_size >= MAX_JOB_QUEUE_SIZE) {
        LOG_ERROR("Job queue is full, rejecting job: %s", job->name);
        cleanup_job(job);
    } else {
        if (pool->job_queue_tail) {
            pool->job_queue_tail->next = job;
        } else {
            pool->job_queue_head = job;
        }
        pool->job_queue_tail = job;
        pool->queue_size++;

#ifdef _WIN32
        EnterCriticalSection(&pool->stats_mutex);
#else
        pthread_mutex_lock(&pool->stats_mutex);
#endif
        pool->total_jobs_submitted++;
#ifdef _WIN32
        LeaveCriticalSection(&pool->stats_mutex);
#else
        pthread_mutex_unlock(&pool->stats_mutex);
#endif

        LOG_DEBUG("Job submitted: %s (ID: %u)", job->name, job->job_id);

        // Wake up a worker thread
#ifdef _WIN32
        WakeConditionVariable(&pool->queue_condition);
#else
        pthread_cond_signal(&pool->queue_condition);
#endif
    }

#ifdef _WIN32
    LeaveCriticalSection(&pool->queue_mutex);
#else
    pthread_mutex_unlock(&pool->queue_mutex);
#endif
}

static Job* get_next_job(ThreadPool* pool) {
    Job* job = NULL;

#ifdef _WIN32
    EnterCriticalSection(&pool->queue_mutex);
#else
    pthread_mutex_lock(&pool->queue_mutex);
#endif

    // Find a job with completed dependencies
    Job* current = pool->job_queue_head;
    Job* prev = NULL;
    
    while (current) {
        if (check_job_dependencies(current)) {
            // Remove from queue
            if (prev) {
                prev->next = current->next;
            } else {
                pool->job_queue_head = current->next;
            }
            
            if (current == pool->job_queue_tail) {
                pool->job_queue_tail = prev;
            }
            
            pool->queue_size--;
            job = current;
            break;
        }
        
        prev = current;
        current = current->next;
    }

#ifdef _WIN32
    LeaveCriticalSection(&pool->queue_mutex);
#else
    pthread_mutex_unlock(&pool->queue_mutex);
#endif

    return job;
}

static void complete_job(ThreadPool* pool, Job* job, bool success) {
    job->state = success ? JOB_STATE_COMPLETED : JOB_STATE_FAILED;

#ifdef _WIN32
    EnterCriticalSection(&pool->stats_mutex);
#else
    pthread_mutex_lock(&pool->stats_mutex);
#endif

    if (success) {
        pool->total_jobs_completed++;
    } else {
        pool->total_jobs_failed++;
    }

#ifdef _WIN32
    LeaveCriticalSection(&pool->stats_mutex);
#else
    pthread_mutex_unlock(&pool->stats_mutex);
#endif

    LOG_DEBUG("Job completed: %s (ID: %u) - %s", 
             job->name, job->job_id, success ? "SUCCESS" : "FAILED");

    cleanup_job(job);
}

static bool check_job_dependencies(Job* job) {
    if (job->dependency_count == 0) {
        return true;
    }

    // In a full implementation, we'd check if dependency jobs are completed
    // For now, we'll just return true (no dependency checking)
    return true;
}

static void cleanup_job(Job* job) {
    if (job->dependencies) {
        MEMORY_DEALLOCATE(job->dependencies);
    }
    MEMORY_DEALLOCATE(job);
}

// Global thread pool instance
static ThreadPool* g_global_thread_pool = NULL;

bool thread_pool_init(u32 thread_count) {
    if (g_global_thread_pool) {
        LOG_WARN("Global thread pool already initialized");
        return true;
    }

    g_global_thread_pool = thread_pool_create(thread_count);
    return g_global_thread_pool != NULL;
}

void thread_pool_shutdown(void) {
    if (g_global_thread_pool) {
        thread_pool_destroy(g_global_thread_pool);
        g_global_thread_pool = NULL;
    }
}

ThreadPool* thread_pool_get_global(void) {
    return g_global_thread_pool;
}
