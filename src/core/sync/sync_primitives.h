#pragma once

#include "engine/include/common.h"
#include <core/types.h>

/**
 * =================================================================================================
 *                         SYNCHRONIZATION PRIMITIVES
 * =================================================================================================
 *
 * Purpose: Cross-platform synchronization primitives for thread coordination.
 * Includes semaphores, barriers, events, and condition variables.
 */

typedef struct Semaphore Semaphore;
typedef struct Barrier Barrier;
typedef struct Event Event;
typedef struct ConditionVariable ConditionVariable;

/* ===== SEMAPHORE ===== */

/**
 * Create a semaphore with initial count
 */
Semaphore* semaphore_create(u32 initial_count, u32 max_count);

/**
 * Destroy a semaphore
 */
void semaphore_destroy(Semaphore *sem);

/**
 * Wait (acquire) the semaphore
 */
void semaphore_wait(Semaphore *sem);

/**
 * Try to wait with timeout (milliseconds)
 * Returns true if acquired, false on timeout
 */
bool semaphore_try_wait(Semaphore *sem, u32 timeout_ms);

/**
 * Signal (release) the semaphore
 */
void semaphore_signal(Semaphore *sem);

/**
 * Signal multiple times
 */
void semaphore_signal_count(Semaphore *sem, u32 count);

/* ===== BARRIER ===== */

/**
 * Create a barrier for synchronizing N threads
 */
Barrier* barrier_create(u32 thread_count);

/**
 * Destroy a barrier
 */
void barrier_destroy(Barrier *barrier);

/**
 * Wait at the barrier (blocks until all threads arrive)
 */
bool barrier_wait(Barrier *barrier, uint32_t timeout_ms);

/* ===== EVENT (CONDITION VARIABLE) ===== */

/**
 * Create a manual-reset event (initially unsignaled)
 */
Event* event_create(bool manual_reset);

/**
 * Destroy an event
 */
void event_destroy(Event *event);

/**
 * Wait for event to be signaled
 */
void event_wait(Event *event);

/**
 * Try to wait with timeout (milliseconds)
 */
bool event_try_wait(Event *event, u32 timeout_ms);

/**
 * Signal the event, waking all waiting threads
 */
void event_signal(Event *event);

/**
 * Reset the event to unsignaled state
 */
void event_reset(Event *event);

/**
 * Check if event is signaled
 */
bool event_is_signaled(Event *event);

