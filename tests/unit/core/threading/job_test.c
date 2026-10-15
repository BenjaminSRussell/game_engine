// Unit tests for the thread pool and job queue in
// src/engine/core/threading/job.c (issue #323).
//
// job.c is compiled directly into this test. Engine is not linked, because
// Engine does not build on origin/main. The only external symbol job.c needs
// is the logger entry point, which is stubbed below.
//
// Waits are bounded: a worker that never runs a job fails the test instead of
// hanging it. thread_pool_wait is not used as a completion signal. It returns
// once the queue is empty, which can be before the last job has finished,
// because job_queue_pop decrements count before the job body runs. The pool
// tests wait on a counter that the jobs increment themselves.

#define _POSIX_C_SOURCE 200809L

#include "core/threading/job.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// ---- Logger stub ------------------------------------------------------------

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

// job.c defines these compat-layer functions, but job.h does not declare them.
void job_scheduler_submit_with_counter(void (*func)(void *), void *data,
                                       atomic_int *counter);
void job_scheduler_wait_for_counter(atomic_int *counter);

// ---- Helpers ----------------------------------------------------------------

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
      return false;                                                            \
    }                                                                          \
  } while (0)

static void noop_job(void *data) { (void)data; }

static bool push_priority(JobQueue *queue, u32 priority) {
  Job job = {.function = noop_job,
             .data = NULL,
             .priority = priority,
             .completed = false};
  return job_queue_push(queue, job);
}

// Polls every millisecond for up to about ten seconds. Returns false on timeout.
static bool wait_for_count(atomic_int *counter, int target) {
  struct timespec pause = {0, 1000000L};
  for (int i = 0; i < 10000; i++) {
    if (atomic_load(counter) >= target) {
      return true;
    }
    nanosleep(&pause, NULL);
  }
  return atomic_load(counter) >= target;
}

// Deterministic generator so the many-job test is reproducible.
static uint32_t lcg_next(uint32_t *state) {
  *state = *state * 1664525u + 1013904223u;
  return *state >> 8;
}

// ---- Job queue (single-threaded; never pops from an empty queue, which would
// block on the condition variable) -------------------------------------------

static bool test_queue_init_argument_checks(void) {
  JobQueue queue;
  CHECK(!job_queue_init(NULL, 8));
  CHECK(!job_queue_init(&queue, 0));
  return true;
}

static bool test_queue_init_and_free_state(void) {
  JobQueue queue;
  CHECK(job_queue_init(&queue, 8));
  CHECK(queue.jobs != NULL);
  CHECK(queue.capacity == 8);
  CHECK(queue.count == 0);
  CHECK(queue.head == 0 && queue.tail == 0);
  CHECK(job_queue_empty(&queue));

  job_queue_free(&queue);
  CHECK(queue.jobs == NULL);
  CHECK(queue.capacity == 0);
  CHECK(queue.count == 0);

  job_queue_free(NULL); // must be a no-op
  return true;
}

static bool test_queue_rejects_null_arguments(void) {
  JobQueue queue;
  Job out;
  CHECK(!push_priority(NULL, 1));
  CHECK(job_queue_init(&queue, 4));
  // Both calls return before taking the lock, so they cannot block.
  CHECK(!job_queue_pop(NULL, &out));
  CHECK(!job_queue_pop(&queue, NULL));
  CHECK(job_queue_empty(&queue));
  job_queue_free(&queue);
  return true;
}

static bool test_queue_pops_highest_priority_first(void) {
  static const u32 priorities[] = {3, 1, 4, 1, 5, 9, 2, 6};
  static const u32 expected[] = {9, 6, 5, 4, 3, 2, 1, 1};
  const u32 count = (u32)(sizeof(priorities) / sizeof(priorities[0]));

  JobQueue queue;
  CHECK(job_queue_init(&queue, count));
  for (u32 i = 0; i < count; i++) {
    // The payload is the submission index plus one, so each pop can be matched
    // back to the priority it was submitted with.
    Job job = {.function = noop_job,
               .data = (void *)(uintptr_t)(i + 1),
               .priority = priorities[i],
               .completed = false};
    CHECK(job_queue_push(&queue, job));
  }
  CHECK(queue.count == count);
  CHECK(!job_queue_empty(&queue));

  for (u32 i = 0; i < count; i++) {
    Job out;
    CHECK(job_queue_pop(&queue, &out));
    CHECK(out.priority == expected[i]);
    CHECK(out.function == noop_job);
    CHECK((uintptr_t)out.data >= 1 && (uintptr_t)out.data <= count);
    CHECK(priorities[(uintptr_t)out.data - 1] == out.priority);
  }
  CHECK(job_queue_empty(&queue));
  CHECK(queue.count == 0);
  job_queue_free(&queue);
  return true;
}

static bool test_queue_interleaved_push_pop_keeps_max_order(void) {
  JobQueue queue;
  Job out;
  CHECK(job_queue_init(&queue, 8));

  CHECK(push_priority(&queue, 5));
  CHECK(push_priority(&queue, 1));
  CHECK(push_priority(&queue, 7));
  CHECK(job_queue_pop(&queue, &out));
  CHECK(out.priority == 7);
  CHECK(push_priority(&queue, 6));
  CHECK(job_queue_pop(&queue, &out));
  CHECK(out.priority == 6);
  CHECK(job_queue_pop(&queue, &out));
  CHECK(out.priority == 5);
  CHECK(push_priority(&queue, 0));
  CHECK(job_queue_pop(&queue, &out));
  CHECK(out.priority == 1);
  CHECK(job_queue_pop(&queue, &out));
  CHECK(out.priority == 0);
  CHECK(job_queue_empty(&queue));

  job_queue_free(&queue);
  return true;
}

static bool test_queue_heap_order_holds_for_many_jobs(void) {
  enum { JOB_COUNT = 200 };
  JobQueue queue;
  CHECK(job_queue_init(&queue, JOB_COUNT));

  uint32_t seed = 2026u;
  uint64_t pushed_sum = 0;
  for (u32 i = 0; i < JOB_COUNT; i++) {
    u32 priority = lcg_next(&seed) % 50u; // narrow range: many duplicates
    pushed_sum += priority;
    CHECK(push_priority(&queue, priority));
  }
  CHECK(queue.count == JOB_COUNT);

  uint64_t popped_sum = 0;
  u32 previous = UINT32_MAX;
  for (u32 i = 0; i < JOB_COUNT; i++) {
    Job out;
    CHECK(job_queue_pop(&queue, &out));
    CHECK(out.priority <= previous);
    previous = out.priority;
    popped_sum += out.priority;
  }
  CHECK(popped_sum == pushed_sum);
  CHECK(job_queue_empty(&queue));
  job_queue_free(&queue);
  return true;
}

// ---- Thread pool ------------------------------------------------------------

static bool test_thread_pool_queue_size_counts_queued_jobs(void) {
  ThreadPool pool;
  Job out;
  memset(&pool, 0, sizeof(pool));
  CHECK(thread_pool_queue_size(NULL) == 0);

  // Only the queue is initialised. No workers exist, so nothing is consumed
  // behind the test's back.
  CHECK(job_queue_init(&pool.queue, 8));
  CHECK(thread_pool_queue_size(&pool) == 0);
  CHECK(push_priority(&pool.queue, 1));
  CHECK(push_priority(&pool.queue, 2));
  CHECK(push_priority(&pool.queue, 3));
  CHECK(thread_pool_queue_size(&pool) == 3);
  CHECK(job_queue_pop(&pool.queue, &out));
  CHECK(thread_pool_queue_size(&pool) == 2);
  job_queue_free(&pool.queue);
  return true;
}

static bool test_thread_pool_init_argument_checks(void) {
  ThreadPool pool;
  CHECK(!thread_pool_init(NULL, 2));
  CHECK(!thread_pool_init(&pool, 0));
  thread_pool_free(NULL); // must be a no-op
  return true;
}

static bool test_thread_pool_free_joins_idle_workers(void) {
  ThreadPool pool;
  memset(&pool, 0, sizeof(pool));
  CHECK(thread_pool_init(&pool, 3));
  CHECK(pool.running);
  CHECK(pool.thread_count >= 1);

  // No jobs queued. free must still wake and join every worker.
  thread_pool_free(&pool);
  CHECK(!pool.running);
  CHECK(pool.threads == NULL);
  return true;
}

static atomic_int g_pool_runs;
static atomic_int g_pool_sum;

static void count_job(void *data) {
  atomic_fetch_add(&g_pool_runs, 1);
  atomic_fetch_add(&g_pool_sum, (int)(intptr_t)data);
}

static bool test_thread_pool_runs_every_submitted_job(void) {
  enum { JOB_COUNT = 200 };
  ThreadPool pool;
  memset(&pool, 0, sizeof(pool));
  CHECK(thread_pool_init(&pool, 2));

  atomic_store(&g_pool_runs, 0);
  atomic_store(&g_pool_sum, 0);
  for (int i = 1; i <= JOB_COUNT; i++) {
    thread_pool_submit(&pool, count_job, (void *)(intptr_t)i,
                       (u32)(i % 7));
  }

  bool finished = wait_for_count(&g_pool_runs, JOB_COUNT);
  thread_pool_free(&pool);
  CHECK(finished);
  CHECK(atomic_load(&g_pool_runs) == JOB_COUNT);
  CHECK(atomic_load(&g_pool_sum) == JOB_COUNT * (JOB_COUNT + 1) / 2);
  CHECK(pool.threads == NULL);
  return true;
}

// ---- Compat counter layer (global 4-thread pool) ----------------------------

static atomic_int g_compat_runs;

static void compat_count_job(void *data) {
  (void)data;
  atomic_fetch_add(&g_compat_runs, 1);
}

static bool test_compat_counter_waits_for_all_jobs(void) {
  enum { JOB_COUNT = 64 };
  atomic_int pending;
  atomic_init(&pending, 0);
  atomic_store(&g_compat_runs, 0);

  for (int i = 0; i < JOB_COUNT; i++) {
    job_scheduler_submit_with_counter(compat_count_job, NULL, &pending);
  }
  job_scheduler_wait_for_counter(&pending);

  // A worker decrements the counter only after its job body has returned.
  CHECK(atomic_load(&pending) == 0);
  CHECK(atomic_load(&g_compat_runs) == JOB_COUNT);

  job_scheduler_wait_for_counter(NULL); // must return immediately
  return true;
}

// ---- Runner -----------------------------------------------------------------

typedef struct {
  const char *name;
  bool (*run)(void);
} TestCase;

int main(void) {
  static const TestCase tests[] = {
      {"queue init argument checks", test_queue_init_argument_checks},
      {"queue init and free state", test_queue_init_and_free_state},
      {"queue rejects null arguments", test_queue_rejects_null_arguments},
      {"queue pops highest priority first",
       test_queue_pops_highest_priority_first},
      {"queue interleaved push and pop keeps max order",
       test_queue_interleaved_push_pop_keeps_max_order},
      {"queue heap order holds for many jobs",
       test_queue_heap_order_holds_for_many_jobs},
      {"thread pool queue size counts queued jobs",
       test_thread_pool_queue_size_counts_queued_jobs},
      {"thread pool init argument checks",
       test_thread_pool_init_argument_checks},
      {"thread pool free joins idle workers",
       test_thread_pool_free_joins_idle_workers},
      {"thread pool runs every submitted job",
       test_thread_pool_runs_every_submitted_job},
      {"compat counter waits for all jobs",
       test_compat_counter_waits_for_all_jobs},
  };

  const size_t total = sizeof(tests) / sizeof(tests[0]);
  size_t failed = 0;
  for (size_t i = 0; i < total; i++) {
    bool ok = tests[i].run();
    printf("%s %s\n", ok ? "PASS" : "FAIL", tests[i].name);
    if (!ok) {
      failed++;
    }
  }
  printf("%zu/%zu passed\n", total - failed, total);
  return failed == 0 ? 0 : 1;
}
