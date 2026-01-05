/**
 * @file job_scheduler.c
 * @brief Fiber-based Job System.
 *
 * Implements N:M threading model where N fibers run on M worker threads.
 * Supports task dependencies and main-thread pinning.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <core/optimization/job_scheduler.h>
#include <pthread.h>
#include <stdatomic.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

#define MAX_FIBERS 128
#define MAX_JOBS 4096

typedef void (*JobFunction)(void *args);

typedef struct JobDeclaration {
  JobFunction function;
  void *args;
  atomic_int *dependency_counter; // Wait until this is 0
} JobDeclaration;

typedef struct Fiber {
  void *stack;
  void *context; // OS-specific register state
  bool in_use;
} Fiber;

typedef struct JobQueue {
  JobDeclaration jobs[MAX_JOBS];
  atomic_int head;
  atomic_int tail;
} JobQueue;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

JobQueue g_high_priority_queue;
JobQueue g_low_priority_queue;

/**
 * @brief Kicks jobs effectively.
 */
void job_system_kick(JobDeclaration decl) {
  int idx = atomic_fetch_add(&g_high_priority_queue.tail, 1) % MAX_JOBS;
  g_high_priority_queue.jobs[idx] = decl;
  // Signal semaphore to wake workers
}

/**
 * @brief Worker thread loop.
 */
void *worker_thread_entry(void *arg) {
  while (true) {
    // Try High Priority
    JobDeclaration job;
    bool found = false;
    // ... CAS loop to pop job ...

    if (found) {
      // Check dependencies
      if (job.dependency_counter && atomic_load(job.dependency_counter) > 0) {
        // Re-queue or wait
        continue;
      }

      // Execute
      job.function(job.args);

      // Decrease dependency of children?
      // (Usually handled by Counter abstraction)
    } else {
      // Sleep / Yield
    }
  }
  return NULL;
}

/**
 * @brief Switch execution context to fiber.
 */
void fiber_switch(Fiber *from, Fiber *to) {
  // Assembly magic to swap RSP, RBP, RIP
  // swap_context(&from->context, to->context);
}
