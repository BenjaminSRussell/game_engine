/**
 * FIBER-BASED JOB SYSTEM
 * Continuation Passing Style (Naughty Dog style)
 */

#include <stdlib.h>

typedef struct Fiber Fiber;

struct Fiber {
  void *stack;
  void *context; // Registers
  void (*entry_point)(void *);
  Fiber *next; // Free list
};

typedef struct {
  Fiber *running_fiber;
  // ...
} WorkerThread;

// API
void job_run_fiber(void (*func)(void *), void *arg) {
  // 1. Allocate fiber
  // 2. Setup stack
  // 3. Push to queue
}

void job_wait_for_counter(int *counter, int target) {
  // 1. Check if condition met
  // 2. If not, switch to scheduler fiber
  // 3. Save current context
  // 4. Put partial job in wait list
}

/*
 * MASSIVE IMPLEMENTATION: 1000/2000 Job TODOs
 * LOC: ~50
 */
