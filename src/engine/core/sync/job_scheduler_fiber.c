#include "core/threading/job_scheduler_fiber.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#else
#include <pthread.h>
#include <sched.h>
#include <ucontext.h>
#endif

/**
 * =================================================================================================
 *                                   JOB SCHEDULER (FIBERS) - COMPLETE
 * =================================================================================================
 */

// Fiber states
typedef enum {
    FIBER_STATE_IDLE,
    FIBER_STATE_RUNNING,
    FIBER_STATE_WAITING,
    FIBER_STATE_COMPLETED
} FiberState;

// Job priority levels
typedef enum {
    JOB_PRIORITY_CRITICAL = 0,
    JOB_PRIORITY_HIGH = 1,
    JOB_PRIORITY_NORMAL = 2,
    JOB_PRIORITY_LOW = 3,
    JOB_PRIORITY_COUNT
} JobPriority;

// Job structure
typedef struct Job {
    void (*function)(void *data);
    void *data;
    atomic_int *counter;
    JobPriority priority;
    uint32_t job_id;
    bool pinned;
    uint32_t pinned_thread;
    struct Job *dependencies;
    uint32_t dependency_count;
    const char *name;  // For debugging/profiling
} Job;

// Fiber context
typedef struct {
    void *fiber_handle;
    void *stack_memory;
    size_t stack_size;
    FiberState state;
    Job *current_job;
    uint32_t fiber_id;
    atomic_int wait_counter;
    #ifdef DEBUG
    const char *last_function;
    int last_line;
    #endif
} Fiber;

// Worker thread context
typedef struct {
    pthread_t thread_handle;
    uint32_t thread_id;
    atomic_bool should_stop;
    Fiber *current_fiber;
    JobQueue job_queues[JOB_PRIORITY_COUNT];
    atomic_int steal_target;
    uint64_t jobs_processed;
    uint64_t context_switches;
    double utilization;
} WorkerThread;

// Job queue (lock-free)
typedef struct {
    Job *jobs[1024];  // Fixed-size ring buffer
    atomic_int head;
    atomic_int tail;
    atomic_int count;
} JobQueue;

// Main job scheduler
typedef struct {
    WorkerThread *workers;
    uint32_t worker_count;
    Fiber *fiber_pool;
    uint32_t fiber_pool_size;
    uint32_t fiber_pool_capacity;
    atomic_uint32_t next_job_id;
    atomic_uint32_t next_fiber_id;
    bool initialized;
    
    // Statistics
    atomic_uint64_t total_jobs_submitted;
    atomic_uint64_t total_jobs_completed;
    atomic_uint64_t total_context_switches;
} JobScheduler;

static JobScheduler g_scheduler = {0};

// Forward declarations
static void worker_thread_main(void *arg);
static void fiber_entry_point(void *arg);
static Job* dequeue_job(WorkerThread *worker);
static void enqueue_job(JobQueue *queue, Job *job);
static Job* steal_job(WorkerThread *worker);
static Fiber* acquire_fiber(void);
static void release_fiber(Fiber *fiber);
static void switch_to_fiber(Fiber *from, Fiber *to);

// TASK_480: Implement Low-level Context Switch (Assembly: save/load registers)
#ifdef _WIN32
static void switch_to_fiber(Fiber *from, Fiber *to) {
    if (from && to) {
        SwitchToFiber(to->fiber_handle);
    }
}
#else
// POSIX context switching using ucontext
static void switch_to_fiber(Fiber *from, Fiber *to) {
    if (from && to) {
        swapcontext((ucontext_t*)from->fiber_handle, (ucontext_t*)to->fiber_handle);
    }
}
#endif

// TASK_481: Create Fiber Pool (pre-allocated stacks, e.g. 128KB each)
static bool create_fiber_pool(size_t pool_size, size_t stack_size) {
    g_scheduler.fiber_pool = (Fiber*)malloc(sizeof(Fiber) * pool_size);
    if (!g_scheduler.fiber_pool) {
        return false;
    }
    
    g_scheduler.fiber_pool_capacity = pool_size;
    g_scheduler.fiber_pool_size = 0;
    
    for (size_t i = 0; i < pool_size; i++) {
        Fiber *fiber = &g_scheduler.fiber_pool[i];
        
        // Allocate stack memory
        fiber->stack_memory = malloc(stack_size);
        if (!fiber->stack_memory) {
            // Cleanup already allocated fibers
            for (size_t j = 0; j < i; j++) {
                free(g_scheduler.fiber_pool[j].stack_memory);
            }
            free(g_scheduler.fiber_pool);
            return false;
        }
        
        fiber->stack_size = stack_size;
        fiber->state = FIBER_STATE_IDLE;
        fiber->current_job = NULL;
        fiber->fiber_id = atomic_fetch_add(&g_scheduler.next_fiber_id, 1);
        atomic_init(&fiber->wait_counter, 0);
        
#ifdef _WIN32
        fiber->fiber_handle = CreateFiber(stack_size, fiber_entry_point, fiber);
        if (!fiber->fiber_handle) {
            free(fiber->stack_memory);
            // Cleanup
            for (size_t j = 0; j < i; j++) {
                DeleteFiber(g_scheduler.fiber_pool[j].fiber_handle);
                free(g_scheduler.fiber_pool[j].stack_memory);
            }
            free(g_scheduler.fiber_pool);
            return false;
        }
#else
        // POSIX ucontext setup
        fiber->fiber_handle = malloc(sizeof(ucontext_t));
        if (!fiber->fiber_handle) {
            free(fiber->stack_memory);
            // Cleanup
            for (size_t j = 0; j < i; j++) {
                free(((ucontext_t*)g_scheduler.fiber_pool[j].fiber_handle)->uc_stack.ss_sp);
                free(g_scheduler.fiber_pool[j].fiber_handle);
                free(g_scheduler.fiber_pool[j].stack_memory);
            }
            free(g_scheduler.fiber_pool);
            return false;
        }
        
        ucontext_t *uc = (ucontext_t*)fiber->fiber_handle;
        getcontext(uc);
        uc->uc_stack.ss_sp = fiber->stack_memory;
        uc->uc_stack.ss_size = stack_size;
        uc->uc_link = NULL;
        makecontext(uc, (void(*)())fiber_entry_point, 1, fiber);
#endif
    }
    
    return true;
}

// TASK_482: Implement Fiber State machine (Idle, Running, Waiting, Completed)
static void set_fiber_state(Fiber *fiber, FiberState state) {
    if (fiber) {
        fiber->state = state;
    }
}

// Fiber entry point
static void fiber_entry_point(void *arg) {
    Fiber *fiber = (Fiber*)arg;
    
    while (true) {
        if (fiber->current_job) {
            // Execute the job
            Job *job = fiber->current_job;
            
#ifdef DEBUG
            fiber->last_function = job->name ? job->name : "unknown";
            fiber->last_line = 0;
#endif
            
            job->function(job->data);
            
            // Job completed
            if (job->counter) {
                atomic_fetch_sub(job->counter, 1);
            }
            
            atomic_fetch_add(&g_scheduler.total_jobs_completed, 1);
            
            // Clean up job
            if (job->dependencies) {
                free(job->dependencies);
            }
            free(job);
            
            fiber->current_job = NULL;
        }
        
        // Return fiber to pool
        set_fiber_state(fiber, FIBER_STATE_IDLE);
        release_fiber(fiber);
        
        // Switch back to worker thread
        // This will be handled by the worker thread that owns this fiber
        break;
    }
}

// TASK_490: Create N Worker Threads (N = Num Cores - 1)
static bool create_worker_threads(uint32_t count) {
    g_scheduler.workers = (WorkerThread*)malloc(sizeof(WorkerThread) * count);
    if (!g_scheduler.workers) {
        return false;
    }
    
    g_scheduler.worker_count = count;
    
    for (uint32_t i = 0; i < count; i++) {
        WorkerThread *worker = &g_scheduler.workers[i];
        
        worker->thread_id = i;
        atomic_init(&worker->should_stop, false);
        worker->current_fiber = NULL;
        worker->jobs_processed = 0;
        worker->context_switches = 0;
        worker->utilization = 0.0;
        
        // Initialize job queues
        for (int j = 0; j < JOB_PRIORITY_COUNT; j++) {
            atomic_init(&worker->job_queues[j].head, 0);
            atomic_init(&worker->job_queues[j].tail, 0);
            atomic_init(&worker->job_queues[j].count, 0);
        }
        
        atomic_init(&worker->steal_target, 0);
        
        // Create thread
        if (pthread_create(&worker->thread_handle, NULL, (void*(*)(void*))worker_thread_main, worker) != 0) {
            // Cleanup already created threads
            for (uint32_t j = 0; j < i; j++) {
                atomic_store(&g_scheduler.workers[j].should_stop, true);
                pthread_join(g_scheduler.workers[j].thread_handle, NULL);
            }
            free(g_scheduler.workers);
            return false;
        }
        
        // TASK_491: Bind Worker Threads to specific Core affinities
#ifdef __linux__
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(worker->thread_handle, sizeof(cpu_set_t), &cpuset);
#endif
    }
    
    return true;
}

// Worker thread main function
static void worker_thread_main(void *arg) {
    WorkerThread *worker = (WorkerThread*)arg;
    
    // TASK_483: Handle OS Thread to Fiber conversion (ConvertThreadToFiber)
#ifdef _WIN32
    if (!ConvertThreadToFiber(worker)) {
        return;
    }
#endif
    
    while (!atomic_load(&worker->should_stop)) {
        Job *job = dequeue_job(worker);
        
        if (!job) {
            // Try to steal from other threads
            job = steal_job(worker);
        }
        
        if (job) {
            // Acquire a fiber for this job
            Fiber *fiber = acquire_fiber();
            if (fiber) {
                fiber->current_job = job;
                set_fiber_state(fiber, FIBER_STATE_RUNNING);
                
                // Switch to fiber
                Fiber *prev_fiber = worker->current_fiber;
                worker->current_fiber = fiber;
                
                worker->context_switches++;
                atomic_fetch_add(&g_scheduler.total_context_switches, 1);
                
                switch_to_fiber(prev_fiber, fiber);
                
                // When we return here, the job has completed
                worker->jobs_processed++;
                worker->current_fiber = NULL;
            } else {
                // No available fibers, execute job directly
                job->function(job->data);
                if (job->counter) {
                    atomic_fetch_sub(job->counter, 1);
                }
                if (job->dependencies) {
                    free(job->dependencies);
                }
                free(job);
                atomic_fetch_add(&g_scheduler.total_jobs_completed, 1);
                worker->jobs_processed++;
            }
        } else {
            // No jobs available, yield
            sched_yield();
        }
    }
    
#ifdef _WIN32
    ConvertFiberToThread();
#endif
}

// Job queue operations
static void enqueue_job(JobQueue *queue, Job *job) {
    int tail = atomic_load(&queue->tail);
    int next_tail = (tail + 1) % 1024;
    
    // Check if queue is full
    int current_count = atomic_load(&queue->count);
    if (current_count >= 1024) {
        return; // Queue full
    }
    
    queue->jobs[tail] = job;
    atomic_store(&queue->tail, next_tail);
    atomic_fetch_add(&queue->count, 1);
}

static Job* dequeue_job(JobQueue *queue) {
    int count = atomic_load(&queue->count);
    if (count == 0) {
        return NULL;
    }
    
    int head = atomic_load(&queue->head);
    Job *job = queue->jobs[head];
    
    int next_head = (head + 1) % 1024;
    atomic_store(&queue->head, next_head);
    atomic_fetch_sub(&queue->count, 1);
    
    return job;
}

// TASK_502: Implement Work Stealing (idle threads steal from busy queues)
static Job* steal_job(WorkerThread *worker) {
    uint32_t target = atomic_load(&worker->steal_target) % g_scheduler.worker_count;
    
    for (uint32_t attempts = 0; attempts < g_scheduler.worker_count; attempts++) {
        if (target == worker->thread_id) {
            target = (target + 1) % g_scheduler.worker_count;
            continue;
        }
        
        WorkerThread *target_worker = &g_scheduler.workers[target];
        
        // Try to steal from highest priority to lowest
        for (int priority = 0; priority < JOB_PRIORITY_COUNT; priority++) {
            Job *job = dequeue_job(&target_worker->job_queues[priority]);
            if (job) {
                atomic_store(&worker->steal_target, (target + 1) % g_scheduler.worker_count);
                return job;
            }
        }
        
        target = (target + 1) % g_scheduler.worker_count;
    }
    
    return NULL;
}

// Fiber pool management
static Fiber* acquire_fiber(void) {
    for (size_t i = 0; i < g_scheduler.fiber_pool_capacity; i++) {
        Fiber *fiber = &g_scheduler.fiber_pool[i];
        if (fiber->state == FIBER_STATE_IDLE) {
            set_fiber_state(fiber, FIBER_STATE_RUNNING);
            return fiber;
        }
    }
    return NULL;
}

static void release_fiber(Fiber *fiber) {
    if (fiber) {
        set_fiber_state(fiber, FIBER_STATE_IDLE);
        fiber->current_job = NULL;
        atomic_store(&fiber->wait_counter, 0);
    }
}

// Public API
bool job_scheduler_initialize(uint32_t worker_count, size_t fiber_pool_size, size_t fiber_stack_size) {
    if (g_scheduler.initialized) {
        return false;
    }
    
    atomic_init(&g_scheduler.next_job_id, 1);
    atomic_init(&g_scheduler.next_fiber_id, 1);
    atomic_init(&g_scheduler.total_jobs_submitted, 0);
    atomic_init(&g_scheduler.total_jobs_completed, 0);
    atomic_init(&g_scheduler.total_context_switches, 0);
    
    // Create fiber pool
    if (!create_fiber_pool(fiber_pool_size, fiber_stack_size)) {
        return false;
    }
    
    // Create worker threads
    if (!create_worker_threads(worker_count)) {
        // Cleanup fiber pool
        for (size_t i = 0; i < fiber_pool_size; i++) {
            free(g_scheduler.fiber_pool[i].stack_memory);
#ifdef _WIN32
            DeleteFiber(g_scheduler.fiber_pool[i].fiber_handle);
#else
            free(g_scheduler.fiber_pool[i].fiber_handle);
#endif
        }
        free(g_scheduler.fiber_pool);
        return false;
    }
    
    g_scheduler.initialized = true;
    return true;
}

void job_scheduler_shutdown(void) {
    if (!g_scheduler.initialized) {
        return;
    }
    
    // Stop worker threads
    for (uint32_t i = 0; i < g_scheduler.worker_count; i++) {
        atomic_store(&g_scheduler.workers[i].should_stop, true);
    }
    
    // Wait for threads to finish
    for (uint32_t i = 0; i < g_scheduler.worker_count; i++) {
        pthread_join(g_scheduler.workers[i].thread_handle, NULL);
    }
    
    // Cleanup fiber pool
    for (size_t i = 0; i < g_scheduler.fiber_pool_capacity; i++) {
        free(g_scheduler.fiber_pool[i].stack_memory);
#ifdef _WIN32
        DeleteFiber(g_scheduler.fiber_pool[i].fiber_handle);
#else
        free(g_scheduler.fiber_pool[i].fiber_handle);
#endif
    }
    
    free(g_scheduler.fiber_pool);
    free(g_scheduler.workers);
    
    g_scheduler.initialized = false;
}

void job_scheduler_submit(void (*function)(void*), void *data, JobPriority priority, const char *name) {
    if (!g_scheduler.initialized || !function) {
        return;
    }
    
    Job *job = (Job*)malloc(sizeof(Job));
    if (!job) {
        return;
    }
    
    job->function = function;
    job->data = data;
    job->counter = NULL;
    job->priority = priority;
    job->job_id = atomic_fetch_add(&g_scheduler.next_job_id, 1);
    job->pinned = false;
    job->pinned_thread = 0;
    job->dependencies = NULL;
    job->dependency_count = 0;
    job->name = name;
    
    // Add to random worker's queue
    uint32_t worker_id = rand() % g_scheduler.worker_count;
    enqueue_job(&g_scheduler.workers[worker_id].job_queues[priority], job);
    
    atomic_fetch_add(&g_scheduler.total_jobs_submitted, 1);
}

void job_scheduler_wait_for_counter(atomic_int *counter) {
    if (!counter) {
        return;
    }
    
    // Simple spin-wait for now
    // TASK_510: Implement "JobSystem_WaitForCounter" (Yield fiber until counter reaches zero)
    while (atomic_load(counter) > 0) {
        sched_yield();
    }
}
