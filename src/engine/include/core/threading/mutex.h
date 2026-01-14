#ifndef CORE_MUTEX_H
#define CORE_MUTEX_H

#include <common.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Mutex Mutex;

Mutex *mutex_create(bool recursive, const char *name);
void mutex_destroy(Mutex *mutex);
void mutex_lock(Mutex *mutex);
void mutex_unlock(Mutex *mutex);
bool mutex_try_lock(Mutex *mutex);
void mutex_get_stats(Mutex *mutex, u64 *total_lock_time, u64 *lock_count,
                     u64 *contention_count);
bool mutex_is_locked(Mutex *mutex);
u32 mutex_get_owner_thread(Mutex *mutex);
void mutex_enable_deadlock_detection(bool enabled);
bool mutex_is_deadlock_detection_enabled(void);
void mutex_check_all_deadlocks(void);
void mutex_reset_stats(Mutex *mutex);
void mutex_get_global_stats(size_t *total_mutexes, size_t *locked_mutexes);
void mutex_cleanup(void);

#endif
