#ifndef WORK_STEALING_QUEUE_H
#define WORK_STEALING_QUEUE_H

#include "../../Memory/Public/Memory.h"
#include "../sync/atomic.h"

// Circular array size (power of 2)
#define WSQ_INITIAL_CAPACITY 1024

// Job type forward declaration
typedef struct Job Job;

// Lock-free work-stealing deque (Chase-Lev)
typedef struct WorkStealingQueue {
  atomic_ptr *buffer; // Array of job pointers
  u32 capacity;
  atomic_int64 bottom;
  atomic_int64 top;
} WorkStealingQueue;

// Create queue
WorkStealingQueue *wsq_create(void);

// Destroy queue
void wsq_destroy(WorkStealingQueue *q);

// Push job to bottom (local thread only)
void wsq_push(WorkStealingQueue *q, Job *job);

// Pop job from bottom (local thread only)
Job *wsq_pop(WorkStealingQueue *q);

// Steal job from top (other threads)
Job *wsq_steal(WorkStealingQueue *q);

#endif // WORK_STEALING_QUEUE_H
