#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "../../Public/core_types.h" // For u32 if needed, or just use standard types
#include <stdbool.h>
#include <stdint.h>

// Opaque semaphore handle
typedef struct Semaphore Semaphore;

// Create a semaphore with initial count
Semaphore *semaphore_create(int initial_count, int max_count);

// Destroy a semaphore
void semaphore_destroy(Semaphore *sem);

// Wait for the semaphore (decrement count)
// Blocks if count is 0
void semaphore_wait(Semaphore *sem);

// Signal the semaphore (increment count)
void semaphore_signal(Semaphore *sem, int count);

#endif // SEMAPHORE_H
