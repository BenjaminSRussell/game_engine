// work_stealing_queue.c - Chase-Lev Deque
#include "work_stealing_queue.h"
#include <stdlib.h>
#include <string.h>

WorkStealingQueue *wsq_create(void) {
  WorkStealingQueue *q = (WorkStealingQueue *)malloc(sizeof(WorkStealingQueue));
  if (!q)
    return NULL;

  q->capacity = WSQ_INITIAL_CAPACITY;
  q->buffer = (atomic_ptr *)malloc(sizeof(atomic_ptr) * q->capacity);
  if (!q->buffer) {
    free(q);
    return NULL;
  }

  // Initialize atomic counters
  // Using simple assignment for initialization
  q->bottom = 0;
  q->top = 0;

  return q;
}

void wsq_destroy(WorkStealingQueue *q) {
  if (!q)
    return;
  if (q->buffer)
    free((void *)q->buffer);
  free(q);
}

void wsq_push(WorkStealingQueue *q, Job *job) {
  long b = atomic_load_acquire(&q->bottom);
  long t = atomic_load_acquire(&q->top);

  // Simple resize check - real implementation would expand
  if (b - t >= q->capacity - 1) {
    // Queue full (stub behavior: drop or stall)
    return;
  }

  q->buffer[b % q->capacity] = job;

  // Ensure the write to buffer happens before incrementing bottom
  atomic_store_release(&q->bottom, b + 1);
}

Job *wsq_pop(WorkStealingQueue *q) {
  long b = atomic_load_acquire(&q->bottom);
  b--;
  atomic_store_release(&q->bottom, b);

  long t = atomic_load_acquire(&q->top);

  if (t > b) {
    // Empty
    atomic_store_release(&q->bottom, t);
    return NULL;
  }

  Job *job = (Job *)q->buffer[b % q->capacity];

  if (t != b) {
    // Normal case, no conflict
    return job;
  }

  // Contest for last item
  if (!atomic_compare_exchange(&q->top, &t, t + 1)) {
    // Failed race against steal
    job = NULL;
  }

  atomic_store_release(&q->bottom, t + 1);
  return job;
}

Job *wsq_steal(WorkStealingQueue *q) {
  long t = atomic_load_acquire(&q->top);
  long b = atomic_load_acquire(&q->bottom);

  if (t >= b)
    return NULL; // Empty

  Job *job = (Job *)q->buffer[t % q->capacity];

  if (!atomic_compare_exchange(&q->top, &t, t + 1)) {
    return NULL; // CAS failed, someone else stole it
  }

  return job;
}
