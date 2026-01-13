// Thread pool job system implementation.
#include <core/logger.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <core/threading/job.h>

#if PLATFORM_WEB
#if defined(__EMSCRIPTEN__) && __has_include(<emscripten.h>)
#include <emscripten.h>
#else
// Emscripten stubs when not available
#define emscripten_set_main_loop(func, fps, simulate) ((void)0)
#endif
#else
#include <pthread.h>
#include <unistd.h>
#endif

bool job_queue_init(JobQueue *queue, u32 capacity) {
  if (!queue || capacity == 0)
    return false;

  queue->jobs = (Job *)malloc(sizeof(Job) * capacity);
  if (!queue->jobs) {
    queue->count = 0;
    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;
    return false;
  }

  queue->count = 0;
  queue->capacity = capacity;
  queue->head = 0;
  queue->tail = 0;

#ifndef PLATFORM_WEB
  if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
    free(queue->jobs);
    queue->jobs = NULL;
    return false;
  }

  if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
    pthread_mutex_destroy(&queue->mutex);
    free(queue->jobs);
    queue->jobs = NULL;
    return false;
  }

  if (pthread_cond_init(&queue->not_full, NULL) != 0) {
    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->mutex);
    free(queue->jobs);
    queue->jobs = NULL;
    return false;
  }
#else
  queue->lock = 0;
#endif

  // limits.
  return true;
}

u32 thread_pool_queue_size(ThreadPool *pool) {
  if (!pool)
    return 0;
  return pool->queue.count;
}

void job_queue_free(JobQueue *queue) {
  if (!queue)
    return;

#ifndef PLATFORM_WEB
  pthread_cond_destroy(&queue->not_full);
  pthread_cond_destroy(&queue->not_empty);
  pthread_mutex_destroy(&queue->mutex);
#endif

  free(queue->jobs);
  queue->jobs = NULL;
  queue->count = 0;
  queue->capacity = 0;
}

static void spin_lock(volatile u32 *lock) {
  // Exponential backoff to reduce CPU waste
  u32 backoff = 1;
  while (__sync_lock_test_and_set(lock, 1)) {
#ifdef __x86_64__
    __asm__ __volatile__("pause");
#endif
    // Exponential backoff: sleep for increasing durations
    for (u32 i = 0; i < backoff; i++) {
#ifdef __x86_64__
      __asm__ __volatile__("pause");
#endif
    }
    if (backoff < 1024) {
      backoff *= 2; // Cap at ~1024 pause instructions
    }
  }
}

static void spin_unlock(volatile u32 *lock) { __sync_lock_release(lock); }

// Helper functions for heap operations
static void heap_swap(Job *a, Job *b) {
  Job temp = *a;
  *a = *b;
  *b = temp;
}

static void heapify_up(JobQueue *queue, u32 index) {
  while (index > 0) {
    u32 parent = (index - 1) / 2;
    if (queue->jobs[index].priority <= queue->jobs[parent].priority) {
      break;
    }
    heap_swap(&queue->jobs[index], &queue->jobs[parent]);
    index = parent;
  }
}

static void heapify_down(JobQueue *queue, u32 index) {
  while (1) {
    u32 largest = index;
    u32 left = 2 * index + 1;
    u32 right = 2 * index + 2;

    if (left < queue->count &&
        queue->jobs[left].priority > queue->jobs[largest].priority) {
      largest = left;
    }
    if (right < queue->count &&
        queue->jobs[right].priority > queue->jobs[largest].priority) {
      largest = right;
    }

    if (largest == index) {
      break;
    }
    heap_swap(&queue->jobs[index], &queue->jobs[largest]);
    index = largest;
  }
}

bool job_queue_push(JobQueue *queue, Job job) {
  if (!queue)
    return false;

#ifndef PLATFORM_WEB
  pthread_mutex_lock(&queue->mutex);

  // Wait while queue is full
  while (queue->count >= queue->capacity) {
    pthread_cond_wait(&queue->not_full, &queue->mutex);
  }

  // Insert at the end and heapify up for O(log n) insertion
  queue->jobs[queue->count] = job;
  heapify_up(queue, queue->count);
  queue->count++;

  // Signal waiting workers
  pthread_cond_signal(&queue->not_empty);
  pthread_mutex_unlock(&queue->mutex);

#else
  // Web spinlock fallback
  while (__sync_lock_test_and_set(&queue->lock, 1)) {
    // Spin
  }

  if (queue->count >= queue->capacity) {
    __sync_lock_release(&queue->lock);
    return false;
  }

  // Insert at the end and heapify up
  queue->jobs[queue->count] = job;
  heapify_up(queue, queue->count);
  queue->count++;
  __sync_lock_release(&queue->lock);
#endif

  return true;
}

bool job_queue_pop(JobQueue *queue, Job *out_job) {
  if (!queue || !out_job)
    return false;

#ifndef PLATFORM_WEB
  pthread_mutex_lock(&queue->mutex);

  // Wait while queue is empty
  while (queue->count == 0) {
    pthread_cond_wait(&queue->not_empty, &queue->mutex);
  }

  // Return the highest priority job (root of heap)
  *out_job = queue->jobs[0];

  // Move last element to root and heapify down for O(log n) removal
  if (queue->count > 1) {
    queue->jobs[0] = queue->jobs[queue->count - 1];
    heapify_down(queue, 0);
  }
  queue->count--;

  // Signal waiting submitters
  pthread_cond_signal(&queue->not_full);
  pthread_mutex_unlock(&queue->mutex);

#else
  // Web spinlock fallback
  while (__sync_lock_test_and_set(&queue->lock, 1)) {
    // Spin
  }

  if (queue->count == 0) {
    __sync_lock_release(&queue->lock);
    return false;
  }

  // Return the highest priority job
  *out_job = queue->jobs[0];

  // Move last element to root and heapify down
  if (queue->count > 1) {
    queue->jobs[0] = queue->jobs[queue->count - 1];
    heapify_down(queue, 0);
  }
  queue->count--;
  __sync_lock_release(&queue->lock);
#endif

  return true;
}

bool job_queue_empty(JobQueue *queue) { return queue->count == 0; }

#ifdef PLATFORM_WEB
// WebAssembly worker implementation (simplified)
static void worker_loop(void *arg) {
  ThreadPool *pool = (ThreadPool *)arg;
  Job job;

  while (pool->running) {
    if (job_queue_pop(&pool->queue, &job)) {
      job.function(job.data);
      job.completed = true;
    }
  }
}
#else
static void *worker_loop(void *arg) {
  ThreadPool *pool = (ThreadPool *)arg;
  Job job;

  while (pool->running) {
    if (job_queue_pop(&pool->queue, &job)) {
      job.function(job.data);
      job.completed = true;
    } else {
      // instead of spinning sleep.
    }
  }

  return NULL;
}
#endif

bool thread_pool_init(ThreadPool *pool, u32 thread_count) {
  if (!pool || thread_count == 0) {
    return false;
  }

  pool->thread_count = thread_count;
  pool->running = true;
  if (!job_queue_init(&pool->queue, 1024)) {
    return false;
  }

  // thread count from hardware concurrency.

#ifdef PLATFORM_WEB
  // WebAssembly: use main thread only for now
  pool->threads = NULL;
#else
  pool->threads = (void **)malloc(sizeof(pthread_t) * thread_count);
  if (!pool->threads) {
    // OOM: Failed to allocate thread array
    job_queue_free(&pool->queue);
    return false;
  }

  for (u32 i = 0; i < thread_count; i++) {
    pthread_t *thread = (pthread_t *)&pool->threads[i];
    int result = pthread_create(thread, NULL, worker_loop, pool);
    if (result != 0) {
      LOG_ERROR("pthread_create failed: %s (%d)", strerror(result), result);

      // If we couldn't create any threads at all, fail as before.
      if (i == 0) {
        pool->running = false;
        // Join none (nothing created), free resources and return failure.
        free(pool->threads);
        job_queue_free(&pool->queue);
        return false;
      }

      // For EAGAIN (resource temporarily unavailable) or other recoverable
      // errors, continue with the threads we managed to create and reduce the
      // active thread count to the number created so far.
#ifdef EAGAIN
      if (result == EAGAIN) {
        LOG_WARN(
            "pthread_create reached system limit; continuing with %u threads",
            i);
      } else {
        LOG_WARN(
            "pthread_create failed for one thread; continuing with %u threads",
            i);
      }
#endif

      pool->thread_count = i; // only the first i threads are valid
      break;
    }
    // debugging.
  }
#endif

  // Successfully initialized thread pool
  return true;
}

static void noop_job(void *data) { (void)data; }

void thread_pool_free(ThreadPool *pool) {
  if (!pool)
    return;

  // Stop accepting new work
  pool->running = false;

#ifndef PLATFORM_WEB
  // Push noop jobs to wake blocked workers so they can exit cleanly
  for (u32 i = 0; i < pool->thread_count; i++) {
    Job j = {
        .function = noop_job, .data = NULL, .priority = 0, .completed = false};
    job_queue_push(&pool->queue, j);
  }

  if (pool->threads) {
    for (u32 i = 0; i < pool->thread_count; i++) {
      pthread_t *thread = (pthread_t *)&pool->threads[i];
      pthread_join(*thread, NULL);
    }
    free(pool->threads);
    pool->threads = NULL;
  }
#else
  // Web: nothing to join
#endif

  job_queue_free(&pool->queue);
}

void thread_pool_submit(ThreadPool *pool, JobFunction function, void *data,
                        u32 priority) {
  Job job = {.function = function,
             .data = data,
             .priority = priority,
             .completed = false};

  // Insert job based on priority (higher priority = executed first)
  // O(log n) insertion using heap-based priority queue
  job_queue_push(&pool->queue, job);

#ifdef PLATFORM_WEB
  // Execute immediately on main thread for WebAssembly
  if (job_queue_pop(&pool->queue, &job)) {
    job.function(job.data);
  }
#endif
}

static bool job_queue_wait_empty(JobQueue *queue) {
#ifndef PLATFORM_WEB
  pthread_mutex_lock(&queue->mutex);
  while (queue->count != 0) {
    pthread_cond_wait(&queue->not_full, &queue->mutex);
  }
  pthread_mutex_unlock(&queue->mutex);
  return true;
#else
  // Fallback: spin with small sleep
  while (!job_queue_empty(queue)) {
    usleep(1000);
  }
  return true;
#endif
}

void thread_pool_wait(ThreadPool *pool) {
#ifdef PLATFORM_WEB
  // Process remaining jobs on main thread
  Job job;
  while (job_queue_pop(&pool->queue, &job)) {
    job.function(job.data);
  }
#else
  // Efficiently wait for queue to drain
  job_queue_wait_empty(&pool->queue);
#endif
}

// =================================================================================================
//                              JOB SCHEDULER COMPATIBILITY LAYER
// =================================================================================================

#include <stdatomic.h>

// Global thread pool for compatibility
static ThreadPool g_GlobalCompatPool;
static bool g_GlobalCompatPoolInit = false;

static void ensure_compat_pool(void) {
    if (!g_GlobalCompatPoolInit) {
        thread_pool_init(&g_GlobalCompatPool, 4); // Default 4 threads
        g_GlobalCompatPoolInit = true;
    }
}

typedef struct {
    void (*func)(void*);
    void* data;
    atomic_int* counter;
} CompatJobWrapper;

static void compat_job_entry(void* arg) {
    CompatJobWrapper* wrapper = (CompatJobWrapper*)arg;
    if (wrapper->func) {
        wrapper->func(wrapper->data);
    }
    if (wrapper->counter) {
        atomic_fetch_sub(wrapper->counter, 1);
    }
    free(wrapper);
}

void job_scheduler_submit_with_counter(void (*func)(void*), void* data, atomic_int* counter) {
    ensure_compat_pool();
    
    if (counter) {
        atomic_fetch_add(counter, 1);
    }
    
    CompatJobWrapper* wrapper = (CompatJobWrapper*)malloc(sizeof(CompatJobWrapper));
    if (!wrapper) return; // OOM check
    
    wrapper->func = func;
    wrapper->data = data;
    wrapper->counter = counter;
    
    thread_pool_submit(&g_GlobalCompatPool, compat_job_entry, wrapper, 1);
}

void job_scheduler_wait_for_counter(atomic_int* counter) {
    if (!counter) return;
    
    ensure_compat_pool();
    
    // Simple busy-wait because our thread_pool doesn't support helping yet
    // and we don't want to block main thread if jobs are there.
    // In a real system, we would steal jobs here.
    while (atomic_load(counter) > 0) {
        // Spin
        // Ideally: sched_yield();
    }
}