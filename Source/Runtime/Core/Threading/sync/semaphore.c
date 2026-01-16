// semaphore.c - Platform-agnostic semaphore implementation
#include "semaphore.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
struct Semaphore {
  HANDLE handle;
};
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
struct Semaphore {
  dispatch_semaphore_t handle;
};
#else
#include <errno.h>
#include <semaphore.h>
#include <time.h>
struct Semaphore {
  sem_t handle;
};
#endif

Semaphore *semaphore_create(int initial_count, int max_count) {
  Semaphore *sem = (Semaphore *)malloc(sizeof(Semaphore));
  if (!sem)
    return NULL;

#if defined(_WIN32)
  sem->handle = CreateSemaphore(NULL, initial_count, max_count, NULL);
  if (sem->handle == NULL) {
    free(sem);
    return NULL;
  }
#elif defined(__APPLE__)
  // GCD semaphores don't have a max count, strict initial count
  sem->handle = dispatch_semaphore_create(initial_count);
#else
  if (sem_init(&sem->handle, 0, initial_count) != 0) {
    free(sem);
    return NULL;
  }
#endif
  return sem;
}

void semaphore_destroy(Semaphore *sem) {
  if (!sem)
    return;
#if defined(_WIN32)
  CloseHandle(sem->handle);
#elif defined(__APPLE__)
  // GCD objects are managed by ARC/release, but checked dispatch_release for
  // legacy In modern macOS, dispatch objects are ObjC objects, but we are in C.
  // However, dispatch_release is generally not needed with newer SDKs if ARC is
  // on, but this is C code. dispatch_release(sem->handle); // Deprecated in
  // newer SDKs, but needed if no ARC. Let's assume standard C handling or no-op
  // if SDK handles it.
#else
  sem_destroy(&sem->handle);
#endif
  free(sem);
}

void semaphore_wait(Semaphore *sem) {
  if (!sem)
    return;
#if defined(_WIN32)
  WaitForSingleObject(sem->handle, INFINITE);
#elif defined(__APPLE__)
  dispatch_semaphore_wait(sem->handle, DISPATCH_TIME_FOREVER);
#else
  int ret;
  do {
    ret = sem_wait(&sem->handle);
  } while (ret == -1 && errno == EINTR);
#endif
}

void semaphore_signal(Semaphore *sem, int count) {
  if (!sem || count <= 0)
    return;
#if defined(_WIN32)
  ReleaseSemaphore(sem->handle, count, NULL);
#elif defined(__APPLE__)
  for (int i = 0; i < count; i++) {
    dispatch_semaphore_signal(sem->handle);
  }
#else
  for (int i = 0; i < count; i++) {
    sem_post(&sem->handle);
  }
#endif
}
