// worker_thread.c - Worker thread implementation
#include "worker_thread.h"
#include "../sync/semaphore.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

struct WorkerThread {
  int id;
  WorkStealingQueue *queue;
  bool should_stop;
#if defined(_WIN32)
  HANDLE thread_handle;
#else
  pthread_t thread_handle;
#endif
  ThreadFunction thread_func;
  void *thread_arg;
};

// Thread entry point wrapper
#if defined(_WIN32)
static DWORD WINAPI worker_entry(LPVOID arg) {
#else
static void *worker_entry(void *arg) {
#endif
  WorkerThread *worker = (WorkerThread *)arg;

  if (worker->thread_func) {
    worker->thread_func(worker->thread_arg);
  }

  return 0;
}

WorkerThread *worker_create(int id, WorkStealingQueue *queue) {
  WorkerThread *worker = (WorkerThread *)malloc(sizeof(WorkerThread));
  if (!worker)
    return NULL;

  worker->id = id;
  worker->queue = queue;
  worker->should_stop = false;
  worker->thread_func = NULL;
  worker->thread_arg = NULL;

  return worker;
}

void worker_start(WorkerThread *worker, ThreadFunction function, void *arg) {
  if (!worker)
    return;

  worker->thread_func = function;
  worker->thread_arg = arg;

#if defined(_WIN32)
  worker->thread_handle = CreateThread(NULL, 0, worker_entry, worker, 0, NULL);
#else
  pthread_create(&worker->thread_handle, NULL, worker_entry, worker);
#endif
}

void worker_stop(WorkerThread *worker) {
  if (worker)
    worker->should_stop = true;
}

void worker_join(WorkerThread *worker) {
  if (!worker)
    return;
#if defined(_WIN32)
  WaitForSingleObject(worker->thread_handle, INFINITE);
#else
  pthread_join(worker->thread_handle, NULL);
#endif
}

void worker_destroy(WorkerThread *worker) {
  if (!worker)
    return;
#if defined(_WIN32)
  CloseHandle(worker->thread_handle);
#endif
  free(worker);
}

int worker_get_id(WorkerThread *worker) { return worker ? worker->id : -1; }
