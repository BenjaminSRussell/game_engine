#include "core/threading/parallel_utils.h"
#include "core/memory/frame_allocator.h"
#include "core/memory/memory_macros.h"
#include "core/thread_pool.h"
#include <stdio.h>
#include <string.h>

// Default batch size for parallel-for
#define DEFAULT_BATCH_SIZE 64
#define MIN_BATCH_SIZE 8
#define MAX_BATCH_SIZE 1024

// Parallel-for job data
typedef struct {
  u32 start;
  u32 end;
  ParallelForFunc func;
  void *user_data;
} ParallelForJobData;

// Parallel-reduce job data
typedef struct {
  const void *data;
  size_t element_size;
  u32 start;
  u32 end;
  ParallelReduceFunc reduce_func;
  void *local_result;
} ParallelReduceJobData;

// Worker function for parallel-for
static bool parallel_for_worker(void *data) {
  ParallelForJobData *job_data = (ParallelForJobData *)data;

  for (u32 i = job_data->start; i < job_data->end; i++) {
    job_data->func(i, job_data->user_data);
  }

  return true;
}

// Worker function for parallel-reduce
static bool parallel_reduce_worker(void *data) {
  ParallelReduceJobData *job_data = (ParallelReduceJobData *)data;

  // Initialize local result with first element
  const u8 *array = (const u8 *)job_data->data;
  memcpy(job_data->local_result,
         array + (job_data->start * job_data->element_size),
         job_data->element_size);

  // Reduce remaining elements
  for (u32 i = job_data->start + 1; i < job_data->end; i++) {
    const void *element = array + (i * job_data->element_size);
    job_data->reduce_func(job_data->local_result, element,
                          job_data->local_result);
  }

  return true;
}

void parallel_for(u32 start, u32 end, ParallelForFunc func, void *user_data) {
  if (start >= end || !func) {
    return;
  }

  u32 count = end - start;
  u32 batch_size = parallel_get_optimal_batch_size(count);

  parallel_for_batched(start, end, batch_size, func, user_data);
}

void parallel_for_batched(u32 start, u32 end, u32 batch_size,
                          ParallelForFunc func, void *user_data) {
  if (start >= end || !func || batch_size == 0) {
    return;
  }

  ThreadPool *pool = thread_pool_get_global();
  if (!pool) {
    // No thread pool, execute serially
    for (u32 i = start; i < end; i++) {
      func(i, user_data);
    }
    return;
  }

  u32 count = end - start;
  u32 num_jobs = (count + batch_size - 1) / batch_size;

  // Allocate job data from frame allocator
  ParallelForJobData *job_data_array =
      FRAME_ALLOC_ARRAY(ParallelForJobData, num_jobs);
  u32 *job_ids = FRAME_ALLOC_ARRAY(u32, num_jobs);

  // Submit jobs
  u32 current = start;
  for (u32 i = 0; i < num_jobs; i++) {
    u32 batch_end = current + batch_size;
    if (batch_end > end) {
      batch_end = end;
    }

    job_data_array[i].start = current;
    job_data_array[i].end = batch_end;
    job_data_array[i].func = func;
    job_data_array[i].user_data = user_data;

    char job_name[64];
    snprintf(job_name, sizeof(job_name), "ParallelFor[%u-%u]", current,
             batch_end);

    job_ids[i] = thread_pool_submit(pool, parallel_for_worker,
                                    &job_data_array[i], job_name);

    current = batch_end;
  }

  // Wait for all jobs to complete
  for (u32 i = 0; i < num_jobs; i++) {
    thread_pool_wait_for_job(pool, job_ids[i], 10000); // 10 second timeout
  }
}

void parallel_reduce(const void *data, size_t count, size_t element_size,
                     ParallelReduceFunc reduce_func, void *result) {
  if (!data || count == 0 || element_size == 0 || !reduce_func || !result) {
    return;
  }

  ThreadPool *pool = thread_pool_get_global();
  if (!pool || count < 100) {
    // Small dataset or no thread pool, execute serially
    const u8 *array = (const u8 *)data;
    memcpy(result, array, element_size);

    for (size_t i = 1; i < count; i++) {
      const void *element = array + (i * element_size);
      reduce_func(result, element, result);
    }
    return;
  }

  // Parallel reduce
  u32 num_threads = thread_pool_get_size(pool);
  u32 batch_size = (count + num_threads - 1) / num_threads;
  u32 num_jobs = (count + batch_size - 1) / batch_size;

  // Allocate job data and local results
  ParallelReduceJobData *job_data_array =
      FRAME_ALLOC_ARRAY(ParallelReduceJobData, num_jobs);
  void **local_results = FRAME_ALLOC_ARRAY(void *, num_jobs);
  u32 *job_ids = FRAME_ALLOC_ARRAY(u32, num_jobs);

  // Allocate space for local results
  for (u32 i = 0; i < num_jobs; i++) {
    local_results[i] = FRAME_ALLOC(element_size);
  }

  // Submit reduce jobs
  u32 current = 0;
  for (u32 i = 0; i < num_jobs; i++) {
    u32 batch_end = current + batch_size;
    if (batch_end > count) {
      batch_end = count;
    }

    job_data_array[i].data = data;
    job_data_array[i].element_size = element_size;
    job_data_array[i].start = current;
    job_data_array[i].end = batch_end;
    job_data_array[i].reduce_func = reduce_func;
    job_data_array[i].local_result = local_results[i];

    char job_name[64];
    snprintf(job_name, sizeof(job_name), "ParallelReduce[%u-%u]", current,
             batch_end);

    job_ids[i] = thread_pool_submit(pool, parallel_reduce_worker,
                                    &job_data_array[i], job_name);

    current = batch_end;
  }

  // Wait for all jobs
  for (u32 i = 0; i < num_jobs; i++) {
    thread_pool_wait_for_job(pool, job_ids[i], 10000);
  }

  // Final reduce of local results
  memcpy(result, local_results[0], element_size);
  for (u32 i = 1; i < num_jobs; i++) {
    reduce_func(result, local_results[i], result);
  }
}

u32 parallel_get_optimal_batch_size(u32 total_count) {
  ThreadPool *pool = thread_pool_get_global();
  if (!pool) {
    return total_count; // No parallelism
  }

  u32 num_threads = thread_pool_get_size(pool);
  if (num_threads == 0) {
    num_threads = 1;
  }

  // Target: 4x jobs per thread for good load balancing
  u32 target_jobs = num_threads * 4;
  u32 batch_size = (total_count + target_jobs - 1) / target_jobs;

  // Clamp to reasonable range
  if (batch_size < MIN_BATCH_SIZE) {
    batch_size = MIN_BATCH_SIZE;
  }
  if (batch_size > MAX_BATCH_SIZE) {
    batch_size = MAX_BATCH_SIZE;
  }

  return batch_size;
}
