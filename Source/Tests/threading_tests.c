#include "core/memory/frame_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "core/thread_pool.h"
#include "core/threading/parallel_utils.h"
#include "test_framework.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// THREAD POOL TESTS
// ============================================================================

static int g_test_counter = 0;

void test_job(void *data) {
  int *value = (int *)data;
  (*value)++;
  __sync_fetch_and_add(&g_test_counter, 1);
}

void test_basic_job_submission(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(4);

  int value = 0;
  g_test_counter = 0;

  u32 job_id =
      thread_pool_submit(thread_pool_get_global(), test_job, &value, "TestJob");
  TEST_ASSERT(job_id != 0, "Job submission failed");

  thread_pool_wait_for_job(thread_pool_get_global(), job_id, 1000);

  thread_pool_shutdown();
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

void test_multiple_jobs(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(4);

  g_test_counter = 0;
  int values[10] = {0};

  for (int i = 0; i < 10; i++) {
    thread_pool_submit(thread_pool_get_global(), test_job, &values[i],
                       "MultiJob");
  }

  // Give jobs time to complete
  for (int i = 0; i < 100 && g_test_counter < 10; i++) {
    usleep(10000); // 10ms
  }

  TEST_ASSERT(g_test_counter == 10, "Not all jobs completed");

  thread_pool_shutdown();
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

void test_job_pool_allocation(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(4);

  ThreadPool *pool = thread_pool_get_global();
  TEST_ASSERT(pool != NULL, "Thread pool not initialized");

  // Submit many jobs to test pool allocation
  for (int i = 0; i < 100; i++) {
    int *data = (int *)malloc(sizeof(int));
    *data = i;
    thread_pool_submit(pool, test_job, data, "PoolTest");
  }

  // Wait for completion
  usleep(500000); // 500ms

  thread_pool_shutdown();
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

// ============================================================================
// PARALLEL UTILITIES TESTS
// ============================================================================

static int g_parallel_array[1000];

void parallel_increment(u32 index, void *user_data) {
  int *array = (int *)user_data;
  array[index]++;
}

void test_parallel_for(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(4);

  // Initialize array
  for (int i = 0; i < 1000; i++) {
    g_parallel_array[i] = i;
  }

  // Parallel increment
  parallel_for(0, 1000, parallel_increment, g_parallel_array);

  // Verify all incremented
  for (int i = 0; i < 1000; i++) {
    TEST_ASSERT(g_parallel_array[i] == i + 1, "Parallel-for failed at index %d",
                i);
  }

  frame_allocator_reset();
  thread_pool_shutdown();
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

void test_parallel_for_batched(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(4);

  // Initialize array
  for (int i = 0; i < 1000; i++) {
    g_parallel_array[i] = 0;
  }

  // Parallel increment with explicit batch size
  parallel_for_batched(0, 1000, 50, parallel_increment, g_parallel_array);

  // Verify all incremented
  for (int i = 0; i < 1000; i++) {
    TEST_ASSERT(g_parallel_array[i] == 1, "Parallel-for batched failed");
  }

  frame_allocator_reset();
  thread_pool_shutdown();
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

void int_sum_reduce(const void *a, const void *b, void *result) {
  *(int *)result = *(const int *)a + *(const int *)b;
}

void test_parallel_reduce(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(4);

  // Initialize array
  int array[100];
  int expected_sum = 0;
  for (int i = 0; i < 100; i++) {
    array[i] = i + 1;
    expected_sum += i + 1;
  }

  int result = 0;
  parallel_reduce(array, 100, sizeof(int), int_sum_reduce, &result);

  TEST_ASSERT(result == expected_sum,
              "Parallel reduce failed: got %d, expected %d", result,
              expected_sum);

  frame_allocator_reset();
  thread_pool_shutdown();
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

void test_optimal_batch_size(void) {
  unified_memory_init(NULL);
  thread_pool_init(4);

  u32 batch_size = parallel_get_optimal_batch_size(1000);
  TEST_ASSERT(batch_size > 0 && batch_size <= 1000, "Invalid batch size");

  printf("  Optimal batch size for 1000 items: %u\n", batch_size);

  thread_pool_shutdown();
  unified_memory_shutdown();
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

void test_parallel_for_performance(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(8);

  const int SIZE = 10000;
  int *array = (int *)malloc(SIZE * sizeof(int));

  // Serial version
  clock_t start = clock();
  for (int i = 0; i < SIZE; i++) {
    array[i] = i * 2;
  }
  clock_t serial_time = clock() - start;

  // Parallel version
  start = clock();
  parallel_for(0, SIZE, parallel_increment, array);
  clock_t parallel_time = clock() - start;

  printf("  Serial: %ld, Parallel: %ld, Speedup: %.2fx\n", serial_time,
         parallel_time, (double)serial_time / (double)parallel_time);

  free(array);
  frame_allocator_reset();
  thread_pool_shutdown();
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

// ============================================================================
// TEST RUNNER
// ============================================================================

int main(void) {
  printf("\n=== Running Threading System Tests ===\n\n");

  // Thread pool tests
  RUN_TEST(test_basic_job_submission);
  RUN_TEST(test_multiple_jobs);
  RUN_TEST(test_job_pool_allocation);

  // Parallel utilities tests
  RUN_TEST(test_parallel_for);
  RUN_TEST(test_parallel_for_batched);
  RUN_TEST(test_parallel_reduce);
  RUN_TEST(test_optimal_batch_size);

  // Performance tests
  RUN_TEST(test_parallel_for_performance);

  printf("\n=== All Threading Tests Passed! ===\n");
  return 0;
}
