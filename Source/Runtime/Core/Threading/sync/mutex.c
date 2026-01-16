// mutex.c - Platform-agnostic mutex implementation
#include "mutex.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
struct Mutex {
  CRITICAL_SECTION cs;
};
#else
#include <pthread.h>
struct Mutex {
  pthread_mutex_t handle;
};
#endif

Mutex *mutex_create(void) {
  Mutex *mutex = (Mutex *)malloc(sizeof(Mutex));
  if (!mutex)
    return NULL;

#if defined(_WIN32)
  InitializeCriticalSection(&mutex->cs);
#else
  if (pthread_mutex_init(&mutex->handle, NULL) != 0) {
    free(mutex);
    return NULL;
  }
#endif
  return mutex;
}

void mutex_destroy(Mutex *mutex) {
  if (!mutex)
    return;
#if defined(_WIN32)
  DeleteCriticalSection(&mutex->cs);
#else
  pthread_mutex_destroy(&mutex->handle);
#endif
  free(mutex);
}

void mutex_lock(Mutex *mutex) {
  if (!mutex)
    return;
#if defined(_WIN32)
  EnterCriticalSection(&mutex->cs);
#else
  pthread_mutex_lock(&mutex->handle);
#endif
}

void mutex_unlock(Mutex *mutex) {
  if (!mutex)
    return;
#if defined(_WIN32)
  LeaveCriticalSection(&mutex->cs);
#else
  pthread_mutex_unlock(&mutex->handle);
#endif
}

bool mutex_try_lock(Mutex *mutex) {
  if (!mutex)
    return false;
#if defined(_WIN32)
  return TryEnterCriticalSection(&mutex->cs) != 0;
#else
  return pthread_mutex_trylock(&mutex->handle) == 0;
#endif
}
