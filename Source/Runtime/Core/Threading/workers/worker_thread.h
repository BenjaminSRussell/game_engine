#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <stdbool.h>

typedef struct WorkerThread WorkerThread;
typedef struct WorkStealingQueue WorkStealingQueue;

// Create a worker thread
typedef void (*ThreadFunction)(void *);

// Create a worker thread
WorkerThread *worker_create(int id, WorkStealingQueue *queue);

// Start worker loop
void worker_start(WorkerThread *worker, ThreadFunction function, void *arg);

// Stop worker loop (requests stop)
void worker_stop(WorkerThread *worker);

// Join/Wait for worker to finish
void worker_join(WorkerThread *worker);

// Destroy worker
void worker_destroy(WorkerThread *worker);

// Get worker ID
int worker_get_id(WorkerThread *worker);

#endif // WORKER_THREAD_H
