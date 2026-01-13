/*
 * semaphore_manager.h
 * Timeline semaphore management
 */

#ifndef SEMAPHORE_MANAGER_H
#define SEMAPHORE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct semaphore semaphore_t;

// Creation
semaphore_t* semaphore_create(uint64_t initial_value);
void semaphore_destroy(semaphore_t* semaphore);

// Operations
uint64_t semaphore_get_value(semaphore_t* semaphore);
void semaphore_signal(semaphore_t* semaphore, uint64_t value);
void semaphore_wait(semaphore_t* semaphore, uint64_t value, uint64_t timeout_ns);

// Backend handle accessor
void* semaphore_get_handle(semaphore_t* semaphore);

#endif // SEMAPHORE_MANAGER_H
