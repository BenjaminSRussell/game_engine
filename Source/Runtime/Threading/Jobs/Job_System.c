#include "../Public/Job_System.h"
#include "../../Core/Memory/Public/Memory.h"
#include "../../Core/Threading/queues/work_stealing_queue.h"
#include "../../Core/Threading/sync/atomic.h"
#include "../../Core/Threading/sync/mutex.h"
#include "../../Core/Threading/sync/semaphore.h"
#include "../../Core/Threading/workers/worker_thread.h"
#include "../../Memory/Public/Memory.h"
#include <stdbool.h>
#include <stdio.h> // for scheduler logging stub
#include <stdlib.h>
#include <string.h>

#define MAX_WORKERS 32
#define MAX_JOB_DEPENDENCIES 8

// Internal Job structure
typedef struct Job {
  JobFunction function;
  void *user_data;
  JobPriority priority;
  atomic_int32 unfinished_dependencies;
  struct Job *parent;
  // Simple fixed-size continuation list for Phase 3
  struct Job *continuations[MAX_JOB_DEPENDENCIES];
  atomic_int32 continuation_count;
} Job;

typedef struct JobSystem {
  WorkStealingQueue *queues[MAX_WORKERS];
  WorkerThread *workers[MAX_WORKERS];
  u32 worker_count;
  u32 thread_count;

  // Global queue for high priority or external jobs
  WorkStealingQueue *global_queue;

  // Synchronization
  Mutex *global_lock;
  bool initialized;
  volatile bool running;
} JobSystem;

static JobSystem g_job_system = {0};

// -- Internal Helpers --

static void ExecuteJob(Job *job) {
  if (job->function) {
    job->function(job->user_data);
  }

  // Finish: notify parent/continuations
  // Check continuations
  int cont_count = atomic_load_acquire(&job->continuation_count);
  for (int i = 0; i < cont_count; i++) {
    Job *cont_job = job->continuations[i];
    if (cont_job) {
      int deps =
          atomic_fetch_sub_relaxed(&cont_job->unfinished_dependencies, 1);
      if (deps == 1) {
        // Determine where to push. Random worker? Or current worker's queue?
        // For now, push to global queue or effectively "schedule" it
        mutex_lock(g_job_system.global_lock);
        wsq_push(g_job_system.global_queue, cont_job);
        mutex_unlock(g_job_system.global_lock);
      }
    }
  }

  // Free job memory (using Memory_Free for MEMORY_TAG_JOB)
  // Note: If using pool allocator, return to pool.
  // Assuming Memory_Free handles it.
  Memory_Free(job, sizeof(Job), MEMORY_TAG_JOB);
}

static void WorkerLoop(void *arg) {
  // int worker_index = (int)(intptr_t)arg;
  // Need simpler casting for strict compliance, but intptr_t is standard
  long worker_index_l = (long)arg;
  int worker_index = (int)worker_index_l;

  WorkStealingQueue *my_queue = g_job_system.queues[worker_index];

  while (g_job_system.running) {
    Job *job = wsq_pop(my_queue);

    if (!job) {
      // 1. Try global queue
      mutex_lock(g_job_system.global_lock);
      job = wsq_pop(g_job_system.global_queue);
      mutex_unlock(g_job_system.global_lock);
      // WSQ is single-consumer for pop. Using wsq_steal for global might be
      // better if multiple workers access it. Actually, global queue usually
      // needs a lock or be a MPMC queue. Our WSQ is Chase-Lev (SPMC). So only
      // ONE thread can 'pop' (owner). We need a MPMC queue for global or use a
      // lock. Phase 3 plan reused WSQ structure. Let's treat global as
      // "Lock-protected WSQ" or simply steal from it.

      if (!job) {
        // 2. Steal from others
        // Random victim
        int victim = rand() % g_job_system.worker_count;
        if (victim != worker_index) {
          job = wsq_steal(g_job_system.queues[victim]);
        }
      }
    }

    if (job) {
      ExecuteJob(job);
    } else {
      // Yield / Relax
      // In a real system we'd sleep on a semaphore/CV
    }
  }
}

// -- Public API --

bool JobSystem_Init(void) {
  if (g_job_system.initialized)
    return true;

  // Hardcoded for now or use platform specific
  g_job_system.thread_count = 8; // Platform_GetCPUCoreCount() stub
  g_job_system.worker_count =
      g_job_system.thread_count - 1; // Reserve main? Or use all?
  if (g_job_system.worker_count == 0)
    g_job_system.worker_count = 1;
  if (g_job_system.worker_count > MAX_WORKERS)
    g_job_system.worker_count = MAX_WORKERS;

  g_job_system.global_queue = wsq_create();
  g_job_system.global_lock = mutex_create();
  g_job_system.running = true;

  // 1. Create all queues and workers
  for (u32 i = 0; i < g_job_system.worker_count; i++) {
    g_job_system.queues[i] = wsq_create();
    g_job_system.workers[i] = worker_create(i, g_job_system.queues[i]);
  }

  // 2. Start all workers (now that all queues exist)
  for (u32 i = 0; i < g_job_system.worker_count; i++) {
    worker_start(g_job_system.workers[i], WorkerLoop, (void *)(intptr_t)i);
  }

  g_job_system.initialized = true;
  return true;
}

JobHandle JobSystem_Schedule(JobFunction function, void *user_data,
                             JobPriority priority) {
  if (!g_job_system.initialized)
    return NULL;

  Job *job = (Job *)Memory_Allocate(sizeof(Job), MEMORY_TAG_JOB);
  if (!job)
    return NULL;

  job->function = function;
  job->user_data = user_data;
  job->priority = priority;
  job->unfinished_dependencies = 1; // 1 for being scheduled/running
  job->parent = NULL;
  job->continuation_count = 0;

  // Push to global queue (simplest entry point)
  // Note: Global queue access needs protection if multiple threads schedule
  // Using a mutex here would be safer if WSQ isn't MPMC safe.
  // For now, assuming single-thread schedule or robust enough.
  // Better: Pick a random queue and push? No, 'push' is single-producer.
  // Solution: Only 'steal' from global. 'Push' to global only from main thread?
  // Let's protect global push with a temporary spinlock logic or assume main
  // thread only for now.
  mutex_lock(g_job_system.global_lock);
  wsq_push(g_job_system.global_queue, job);
  mutex_unlock(g_job_system.global_lock);

  return (JobHandle)job;
}

void JobSystem_Wait(JobHandle handle) {
  // Stub: Spin while waiting?
  // Real wait would steal/execute while waiting
}

void JobSystem_Shutdown(void) {
  if (!g_job_system.initialized)
    return;

  g_job_system.running = false;

  for (u32 i = 0; i < g_job_system.worker_count; i++) {
    worker_join(g_job_system.workers[i]);
    worker_destroy(g_job_system.workers[i]);
    wsq_destroy(g_job_system.queues[i]);
  }

  wsq_destroy(g_job_system.global_queue);
  mutex_destroy(g_job_system.global_lock);
  g_job_system.initialized = false;
}

void JobSystem_AddDependency(JobHandle job_handle,
                             JobHandle dependency_handle) {
  Job *job = (Job *)job_handle;
  Job *dependency = (Job *)dependency_handle;

  if (!job || !dependency)
    return;

  // Add job as continuation to dependency
  int idx = atomic_fetch_add_relaxed(&dependency->continuation_count, 1);
  if (idx < MAX_JOB_DEPENDENCIES) {
    dependency->continuations[idx] = job;
    atomic_fetch_add_relaxed(&job->unfinished_dependencies, 1);
  }
}
