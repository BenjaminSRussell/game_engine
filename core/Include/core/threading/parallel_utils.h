#ifndef PARALLEL_UTILS_H
#define PARALLEL_UTILS_H

#include <core/types.h>
#include <stddef.h>

/**
 * =================================================================================================
 *                              PARALLEL UTILITIES
 * =================================================================================================
 *
 * Purpose: High-level parallel programming utilities built on top of the job
 * system. Provides parallel-for, parallel-reduce, and other common parallel
 * patterns.
 */

/**
 * Parallel for function signature
 * @param index Current iteration index
 * @param user_data User-provided data pointer
 */
typedef void (*ParallelForFunc)(u32 index, void *user_data);

/**
 * Parallel reduce function signature
 * @param a First value to combine
 * @param b Second value to combine
 * @param result Output combined value
 */
typedef void (*ParallelReduceFunc)(const void *a, const void *b, void *result);

/**
 * Execute a function in parallel for a range of indices
 * Automatically batches work across available threads
 *
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param func Function to execute for each index
 * @param user_data User data passed to function
 */
void parallel_for(u32 start, u32 end, ParallelForFunc func, void *user_data);

/**
 * Execute a function in parallel with explicit batch size
 *
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param batch_size Number of iterations per job
 * @param func Function to execute for each index
 * @param user_data User data passed to function
 */
void parallel_for_batched(u32 start, u32 end, u32 batch_size,
                          ParallelForFunc func, void *user_data);

/**
 * Parallel reduce operation
 * Combines array elements using a binary operation
 *
 * @param data Array of elements
 * @param count Number of elements
 * @param element_size Size of each element in bytes
 * @param reduce_func Function to combine two elements
 * @param result Output result
 */
void parallel_reduce(const void *data, size_t count, size_t element_size,
                     ParallelReduceFunc reduce_func, void *result);

/**
 * Get optimal batch size for parallel-for based on work count
 * @param total_count Total number of iterations
 * @return Recommended batch size
 */
u32 parallel_get_optimal_batch_size(u32 total_count);

#endif // PARALLEL_UTILS_H
