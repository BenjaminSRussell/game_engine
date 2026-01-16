#ifndef MUTEX_H
#define MUTEX_H

#include <stdbool.h>

// Opaque mutex handle
typedef struct Mutex Mutex;

// Create a mutex
Mutex *mutex_create(void);

// Destroy a mutex
void mutex_destroy(Mutex *mutex);

// Lock the mutex (blocking)
void mutex_lock(Mutex *mutex);

// Unlock the mutex
void mutex_unlock(Mutex *mutex);

// Try to lock the mutex (non-blocking)
// Returns true if lock was acquired
bool mutex_try_lock(Mutex *mutex);

#endif // MUTEX_H
