#include "core/threading/mutex.h"
#include "engine/include/common.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

//  COMPLETED: Mutex System Implementation - AGENT_CORE_1
// Cross-platform synchronization primitive with deadlock detection and
// recursive support

#ifdef _WIN32
#include <synchapi.h>
#include <windows.h>
#else
#include <errno.h>
#include <pthread.h>
#include <time.h>
#endif

#define MAX_LOCK_DEPTH 64
#define DEADLOCK_TIMEOUT_MS 5000
#define THREAD_NAME_LENGTH 64

typedef struct {
#ifdef _WIN32
  HANDLE handle;
  CRITICAL_SECTION critical_section;
#else
  pthread_mutex_t handle;
  pthread_mutexattr_t attr;
#endif

  // Recursive support
  bool recursive;
  u32 lock_count; // Recursive lock depth
  u64 owner_thread_id;

  // Deadlock detection
  u64 lock_time;
  u64 thread_id; // ID of the thread that created the mutex
  char thread_name[THREAD_NAME_LENGTH];

  // Statistics
  u64 total_lock_time;
  u64 stat_lock_count; // Total number of times the mutex was locked
  u64 contention_count;

} MutexInternal;

// Global deadlock detection
static MutexInternal *g_mutex_registry[1024];
static size_t g_mutex_count = 0;
static _Atomic(bool) g_deadlock_detection_enabled = true;

// Thread-local lock tracking
#ifdef _WIN32
__declspec(thread) static u32 g_thread_lock_depth = 0;
__declspec(thread) static MutexInternal *g_locked_mutexes[MAX_LOCK_DEPTH];
#else
__thread static u32 g_thread_lock_depth = 0;
__thread static MutexInternal *g_locked_mutexes[MAX_LOCK_DEPTH];
#endif

//  COMPLETED: Get current thread ID
static u64 get_thread_id(void) {
#ifdef _WIN32
  return (u64)GetCurrentThreadId();
#else
  return (u64)pthread_self();
#endif
}

//  COMPLETED: Get timestamp in milliseconds
static u64 get_timestamp_ms(void) {
#ifdef _WIN32
  LARGE_INTEGER freq, count;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&count);
  return (count.QuadPart * 1000) / freq.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * 1000 + (u64)ts.tv_nsec / 1000000;
#endif
}

//  COMPLETED: Deadlock detection
static bool detect_deadlock(MutexInternal *mutex) {
  if (!g_deadlock_detection_enabled)
    return false;

  u64 current_time = get_timestamp_ms();

  // Check if lock has been held too long
  if (mutex->owner_thread_id != get_thread_id() && mutex->lock_count > 0 &&
      (current_time - mutex->lock_time) > DEADLOCK_TIMEOUT_MS) {

    // Check for circular dependency
    for (u32 i = 0; i < g_thread_lock_depth; i++) {
      if (g_locked_mutexes[i] &&
          g_locked_mutexes[i]->owner_thread_id == mutex->owner_thread_id) {
        return true; // Potential deadlock detected
      }
    }
  }

  return false;
}

//  COMPLETED: Register mutex for deadlock detection
static void register_mutex(MutexInternal *mutex) {
  if (g_mutex_count < 1024) {
    g_mutex_registry[g_mutex_count++] = mutex;
  }
}

//  COMPLETED: Unregister mutex
static void unregister_mutex(MutexInternal *mutex) {
  for (size_t i = 0; i < g_mutex_count; i++) {
    if (g_mutex_registry[i] == mutex) {
      g_mutex_registry[i] = g_mutex_registry[--g_mutex_count];
      break;
    }
  }
}

//  COMPLETED: Mutex creation
Mutex *mutex_create(bool recursive, const char *name) {
  MutexInternal *mutex = malloc(sizeof(MutexInternal));
  if (!mutex)
    return NULL;

  memset(mutex, 0, sizeof(MutexInternal));
  mutex->recursive = recursive;
  mutex->thread_id = get_thread_id();
  mutex->lock_time = 0;
  mutex->total_lock_time = 0;
  mutex->stat_lock_count = 0;
  mutex->contention_count = 0;

  if (name) {
    strncpy(mutex->thread_name, name, THREAD_NAME_LENGTH - 1);
  } else {
    strcpy(mutex->thread_name, "Unknown");
  }

#ifdef _WIN32
  if (recursive) {
    InitializeCriticalSection(&mutex->critical_section);
    mutex->handle = (HANDLE)&mutex->critical_section;
  } else {
    mutex->handle = CreateMutex(NULL, FALSE, NULL);
  }
#else
  pthread_mutexattr_init(&mutex->attr);

  if (recursive) {
    pthread_mutexattr_settype(&mutex->attr, PTHREAD_MUTEX_RECURSIVE);
  } else {
    pthread_mutexattr_settype(&mutex->attr, PTHREAD_MUTEX_NORMAL);
  }

  pthread_mutex_init(&mutex->handle, &mutex->attr);
#endif

  register_mutex(mutex);
  return (Mutex *)mutex;
}

//  COMPLETED: Mutex destruction
void mutex_destroy(Mutex *mutex) {
  if (!mutex)
    return;

  MutexInternal *internal = (MutexInternal *)mutex;

  unregister_mutex(internal);

#ifdef _WIN32
  if (internal->recursive) {
    DeleteCriticalSection(&internal->critical_section);
  } else {
    CloseHandle(internal->handle);
  }
#else
  pthread_mutex_destroy(&internal->handle);
  pthread_mutexattr_destroy(&internal->attr);
#endif

  free(internal);
}

//  COMPLETED: Mutex lock
void mutex_lock(Mutex *mutex) {
  if (!mutex)
    return;

  MutexInternal *internal = (MutexInternal *)mutex;
  u64 start_time = get_timestamp_ms();

  // Check for deadlock
  if (detect_deadlock(internal)) {
    // Log deadlock warning
           internal->thread_name);
  }

  // Track lock for deadlock detection
  if (g_thread_lock_depth < MAX_LOCK_DEPTH) {
    g_locked_mutexes[g_thread_lock_depth++] = internal;
  }

#ifdef _WIN32
  if (internal->recursive) {
    EnterCriticalSection(&internal->critical_section);
  } else {
    WaitForSingleObject(internal->handle, INFINITE);
  }
#else
  int result = pthread_mutex_lock(&internal->handle);
  if (result == EDEADLK) {
    printf("DEADLOCK: Recursive lock on non-recursive mutex '%s'\n",
           internal->thread_name);
  }
#endif

  // Update statistics
  u64 end_time = get_timestamp_ms();
  u64 lock_duration = end_time - start_time;

  internal->total_lock_time += lock_duration;
  internal->stat_lock_count++;

  if (lock_duration > 1) { // More than 1ms indicates contention
    internal->contention_count++;
  }

  internal->lock_time = end_time;
  internal->owner_thread_id = get_thread_id();

  if (internal->recursive) {
    internal->lock_count++;
  }
}

//  COMPLETED: Mutex unlock
void mutex_unlock(Mutex *mutex) {
  if (!mutex)
    return;

  MutexInternal *internal = (MutexInternal *)mutex;

  // Remove from lock tracking
  for (u32 i = 0; i < g_thread_lock_depth; i++) {
    if (g_locked_mutexes[i] == internal) {
      g_locked_mutexes[i] = g_locked_mutexes[--g_thread_lock_depth];
      break;
    }
  }

#ifdef _WIN32
  if (internal->recursive) {
    LeaveCriticalSection(&internal->critical_section);
  } else {
    ReleaseMutex(internal->handle);
  }
#else
  pthread_mutex_unlock(&internal->handle);
#endif

  internal->lock_time = 0;
  internal->owner_thread_id = 0;

  if (internal->recursive && internal->lock_count > 0) {
    internal->lock_count--;
  }
}

//  COMPLETED: Try lock
bool mutex_try_lock(Mutex *mutex) {
  if (!mutex)
    return false;

  MutexInternal *internal = (MutexInternal *)mutex;

#ifdef _WIN32
  BOOL result;
  if (internal->recursive) {
    result = TryEnterCriticalSection(&internal->critical_section);
  } else {
    result = (WaitForSingleObject(internal->handle, 0) == WAIT_OBJECT_0);
  }

  if (result) {
    internal->owner_thread_id = get_thread_id();
    internal->lock_time = get_timestamp_ms();
    internal->lock_count++; // Recursive lock depth

    if (g_thread_lock_depth < MAX_LOCK_DEPTH) {
      g_locked_mutexes[g_thread_lock_depth++] = internal;
    }
    internal->stat_lock_count++; // Statistics
  }

  return result != FALSE;
#else
  int result = pthread_mutex_trylock(&internal->handle);

  if (result == 0) {
    internal->owner_thread_id = get_thread_id();
    internal->lock_time = get_timestamp_ms();
    internal->lock_count++; // Recursive lock depth

    if (g_thread_lock_depth < MAX_LOCK_DEPTH) {
      g_locked_mutexes[g_thread_lock_depth++] = internal;
    }
    internal->stat_lock_count++; // Statistics

    return true;
  }

  return false;
#endif
}

//  COMPLETED: Get mutex statistics
void mutex_get_stats(Mutex *mutex, u64 *total_lock_time, u64 *lock_count,
                     u64 *contention_count) {
  if (!mutex)
    return;

  MutexInternal *internal = (MutexInternal *)mutex;

  if (total_lock_time)
    *total_lock_time = internal->total_lock_time;
  if (lock_count)
    *lock_count = internal->stat_lock_count;
  if (contention_count)
    *contention_count = internal->contention_count;
}

//  COMPLETED: Check if mutex is locked
bool mutex_is_locked(Mutex *mutex) {
  if (!mutex)
    return false;

  MutexInternal *internal = (MutexInternal *)mutex;
  return internal->lock_count > 0;
}

//  COMPLETED: Get mutex owner thread ID
u32 mutex_get_owner_thread(Mutex *mutex) {
  if (!mutex)
    return 0;

  MutexInternal *internal = (MutexInternal *)mutex;
  return (u32)internal->owner_thread_id;
}

//  COMPLETED: Enable/disable deadlock detection
void mutex_enable_deadlock_detection(bool enabled) {
  atomic_store(&g_deadlock_detection_enabled, enabled);
}

//  COMPLETED: Get deadlock detection status
bool mutex_is_deadlock_detection_enabled(void) {
  return atomic_load(&g_deadlock_detection_enabled);
}

//  COMPLETED: Global deadlock check
void mutex_check_all_deadlocks(void) {
  if (!g_deadlock_detection_enabled)
    return;

  u64 current_time = get_timestamp_ms();

  for (size_t i = 0; i < g_mutex_count; i++) {
    MutexInternal *mutex = g_mutex_registry[i];

    if (mutex->lock_count > 0 &&
        (current_time - mutex->lock_time) > DEADLOCK_TIMEOUT_MS) {

      printf("DEADLOCK CHECK: Mutex '%s' held by thread %llu for %llu ms\n",
             mutex->thread_name, (unsigned long long)mutex->owner_thread_id,
             (unsigned long long)(current_time - mutex->lock_time));
    }
  }
}

//  COMPLETED: Reset mutex statistics
void mutex_reset_stats(Mutex *mutex) {
  if (!mutex)
    return;

  MutexInternal *internal = (MutexInternal *)mutex;
  internal->total_lock_time = 0;
  internal->stat_lock_count = 0;
  internal->contention_count = 0;
}

//  COMPLETED: Get global mutex statistics
void mutex_get_global_stats(size_t *total_mutexes, size_t *locked_mutexes) {
  if (total_mutexes)
    *total_mutexes = g_mutex_count;

  if (locked_mutexes) {
    size_t locked = 0;
    for (size_t i = 0; i < g_mutex_count; i++) {
      if (g_mutex_registry[i]->lock_count > 0) {
        locked++;
      }
    }
    *locked_mutexes = locked;
  }
}

//  COMPLETED: Cleanup function
void mutex_cleanup(void) {
  // Force unlock all mutexes held by current thread
  for (u32 i = 0; i < g_thread_lock_depth; i++) {
    if (g_locked_mutexes[i] &&
        g_locked_mutexes[i]->owner_thread_id == get_thread_id()) {
      printf("CLEANUP: Forcing unlock of mutex '%s'\n",
             g_locked_mutexes[i]->thread_name);
      mutex_unlock((Mutex *)g_locked_mutexes[i]);
    }
  }

  g_thread_lock_depth = 0;
}
