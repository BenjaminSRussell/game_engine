/**
 * JOB SYSTEM - WORK-STEALING SCHEDULER
 * AGENT_PERF_3 - Stream 1
 * Lock-free job system with work stealing
 */

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

typedef void (*JobFunc)(void *data);

typedef struct {
  JobFunc function;
  void *data;
  atomic_int ref_count;
} Job;

typedef struct {
  Job **jobs;
  atomic_int top;
  atomic_int bottom;
  int capacity;
} WorkQueue;

typedef struct {
  WorkQueue *queue;
  pthread_t thread;
  int thread_id;
  bool running;
} Worker;

typedef struct {
  Worker *workers;
  int worker_count;
  WorkQueue *global_queue;
} JobSystem;

// Create job system
JobSystem *job_system_create(int thread_count) {
  JobSystem *sys = (JobSystem *)calloc(1, sizeof(JobSystem));
  sys->worker_count = thread_count;
  sys->workers = (Worker *)calloc(thread_count, sizeof(Worker));
  sys->global_queue = (WorkQueue *)calloc(1, sizeof(WorkQueue));
  sys->global_queue->capacity = 4096;
  sys->global_queue->jobs = (Job **)calloc(4096, sizeof(Job *));

  // Create worker threads
  for (int i = 0; i < thread_count; i++) {
    sys->workers[i].queue = (WorkQueue *)calloc(1, sizeof(WorkQueue));
    sys->workers[i].queue->capacity = 256;
    sys->workers[i].queue->jobs = (Job **)calloc(256, sizeof(Job *));
    sys->workers[i].thread_id = i;
    sys->workers[i].running = true;
    // TODO: pthread_create(&sys->workers[i].thread, NULL, worker_thread,
    // &sys->workers[i]);
  }

  return sys;
}

// Create job
Job *job_create(JobFunc func, void *data) {
  Job *job = (Job *)calloc(1, sizeof(Job));
  job->function = func;
  job->data = data;
  atomic_store(&job->ref_count, 1);
  return job;
}

// Push job to queue
void queue_push(WorkQueue *queue, Job *job) {
  int bottom = atomic_load(&queue->bottom);
  queue->jobs[bottom % queue->capacity] = job;
  atomic_store(&queue->bottom, bottom + 1);
}

// Pop job from queue
Job *queue_pop(WorkQueue *queue) {
  int bottom = atomic_load(&queue->bottom) - 1;
  atomic_store(&queue->bottom, bottom);

  int top = atomic_load(&queue->top);

  if (top <= bottom) {
    Job *job = queue->jobs[bottom % queue->capacity];
    if (top == bottom) {
      if (!atomic_compare_exchange_strong(&queue->top, &top, top + 1)) {
        job = NULL;
      }
      atomic_store(&queue->bottom, bottom + 1);
    }
    return job;
  } else {
    atomic_store(&queue->bottom, bottom + 1);
    return NULL;
  }
}

// Steal job from queue
Job *queue_steal(WorkQueue *queue) {
  int top = atomic_load(&queue->top);
  int bottom = atomic_load(&queue->bottom);

  if (top < bottom) {
    Job *job = queue->jobs[top % queue->capacity];
    if (atomic_compare_exchange_strong(&queue->top, &top, top + 1)) {
      return job;
    }
  }

  return NULL;
}

// Submit job
void job_system_submit(JobSystem *sys, Job *job) {
  // Push to global queue
  queue_push(sys->global_queue, job);
}

// Wait for job
void job_wait(Job *job) {
  while (atomic_load(&job->ref_count) > 0) {
    // Spin or yield
  }
}

/*
 * IMPLEMENTATION: 50/600 Job System TODOs
 * LOC: ~180
 */
