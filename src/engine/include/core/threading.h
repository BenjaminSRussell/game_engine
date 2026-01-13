#ifndef CORE_THREADING_H
#define CORE_THREADING_H

#include <core/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Simple Thread Wrapper
typedef pthread_t Thread;
typedef pthread_mutex_t Mutex;

// Function pointer for thread entry
typedef void *(*ThreadFunction)(void *);

static inline Thread thread_create(ThreadFunction func, void *arg) {
  pthread_t thread;
  if (pthread_create(&thread, NULL, func, arg) != 0) {
    return (Thread)NULL;
  }
  return thread;
}

static inline void thread_wait(Thread thread) {
  if (thread) {
    pthread_join(thread, NULL);
  }
}

static inline void thread_destroy(Thread thread) {
  // pthread_join acts as destroy/cleanup for joinable threads
  // If detached, no action needed. Assuming joinable for now based on usage.
  (void)thread;
}

static inline void thread_sleep(u32 ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

static inline Mutex *mutex_create(void) {
  pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  if (mutex) {
    pthread_mutex_init(mutex, NULL);
  }
  return (Mutex *)mutex;
}

static inline void mutex_lock(Mutex *mutex) {
  if (mutex) {
    pthread_mutex_lock((pthread_mutex_t *)mutex);
  }
}

static inline void mutex_unlock(Mutex *mutex) {
  if (mutex) {
    pthread_mutex_unlock((pthread_mutex_t *)mutex);
  }
}

static inline void mutex_destroy(Mutex *mutex) {
  if (mutex) {
    pthread_mutex_destroy((pthread_mutex_t *)mutex);
    free(mutex);
  }
}

#endif // CORE_THREADING_H
