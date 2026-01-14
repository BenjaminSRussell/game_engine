#pragma once

#include "include/core/types.h"
#include <stdbool.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct LockFreeRingBuffer LockFreeRingBuffer;
typedef struct LockFreeRingBufferStats LockFreeRingBufferStats;

// Mode of operation
typedef enum {
    LFRING_SPSC = 0,        // Single Producer Single Consumer
    LFRING_MPMC = 1,        // Multi Producer Multi Consumer
    LFRING_SPMC = 2,        // Single Producer Multi Consumer
    LFRING_MPSC = 3         // Multi Producer Single Consumer
} LockFreeRingMode;

// Enqueue result
typedef enum {
    LFRING_OK = 0,
    LFRING_FULL = 1,
    LFRING_CONTENTION = 2,
    LFRING_INVALID = 3
} LockFreeRingResult;

// Statistics
typedef struct LockFreeRingBufferStats {
    u64 total_enqueued;
    u64 total_dequeued;
    u64 failed_enqueues;
    u64 failed_dequeues;
    u64 max_contention;
    f64 average_contention;
} LockFreeRingBufferStats;

/**
 * LockFreeRingBuffer: Wait-free/lock-free FIFO queue
 *
 * Properties:
 *   - No locks or mutexes
 *   - Cache-line padding for SPSC
 *   - Atomic operations for MPMC
 *   - Batch operations for throughput
 *   - Minimal false sharing
 *
 * Use cases:
 *   - Thread-safe message queues
 *   - Task distribution
 *   - Lock-free logging
 *   - Real-time audio processing
 *   - Multi-threaded rendering
 */

// ============================================================================
// Creation and Destruction
// ============================================================================

/**
 * Create a lock-free ring buffer
 *
 * Args:
 *   capacity: Number of elements (will be rounded to power of 2)
 *   element_size: Size of each element in bytes
 *   mode: Operating mode (SPSC, MPMC, etc.)
 *
 * Returns:
 *   New LockFreeRingBuffer (must be freed with lfring_destroy)
 */
LockFreeRingBuffer *lfring_create(u32 capacity, u32 element_size, LockFreeRingMode mode);

/**
 * Create SPSC buffer (optimized for single producer/consumer)
 *
 * Args:
 *   capacity: Number of elements
 *   element_size: Size of each element
 *
 * Returns:
 *   New LockFreeRingBuffer optimized for SPSC
 */
LockFreeRingBuffer *lfring_create_spsc(u32 capacity, u32 element_size);

/**
 * Create MPMC buffer (for multiple producers and consumers)
 *
 * Args:
 *   capacity: Number of elements
 *   element_size: Size of each element
 *
 * Returns:
 *   New LockFreeRingBuffer optimized for MPMC
 */
LockFreeRingBuffer *lfring_create_mpmc(u32 capacity, u32 element_size);

/**
 * Destroy lock-free ring buffer
 *
 * Args:
 *   buffer: LockFreeRingBuffer to destroy (NULL-safe)
 */
void lfring_destroy(LockFreeRingBuffer *buffer);

/**
 * Reset buffer to empty state
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 */
void lfring_reset(LockFreeRingBuffer *buffer);

// ============================================================================
// SPSC Operations (Single Producer Single Consumer)
// ============================================================================

/**
 * SPSC: Enqueue single element (wait-free)
 *
 * Args:
 *   buffer: SPSC LockFreeRingBuffer
 *   element: Pointer to element to enqueue
 *
 * Returns:
 *   LFRING_OK if successful, LFRING_FULL if no space
 *
 * Note: Safe without locks when single producer/consumer
 */
LockFreeRingResult lfring_spsc_enqueue(LockFreeRingBuffer *buffer, const void *element);

/**
 * SPSC: Dequeue single element (wait-free)
 *
 * Args:
 *   buffer: SPSC LockFreeRingBuffer
 *   out: Buffer to receive element
 *
 * Returns:
 *   LFRING_OK if element returned, LFRING_EMPTY if queue empty
 */
LockFreeRingResult lfring_spsc_dequeue(LockFreeRingBuffer *buffer, void *out);

/**
 * SPSC: Batch enqueue
 *
 * Args:
 *   buffer: SPSC LockFreeRingBuffer
 *   elements: Array of elements
 *   count: Number of elements
 *
 * Returns:
 *   Number of elements enqueued (may be less than count if full)
 */
u32 lfring_spsc_enqueue_batch(LockFreeRingBuffer *buffer, const void *elements, u32 count);

/**
 * SPSC: Batch dequeue
 *
 * Args:
 *   buffer: SPSC LockFreeRingBuffer
 *   out: Buffer for elements
 *   count: Maximum elements to dequeue
 *
 * Returns:
 *   Number of elements dequeued
 */
u32 lfring_spsc_dequeue_batch(LockFreeRingBuffer *buffer, void *out, u32 count);

/**
 * SPSC: Peek at head without removing
 *
 * Args:
 *   buffer: SPSC LockFreeRingBuffer
 *   out: Buffer to receive element
 *
 * Returns:
 *   LFRING_OK if element returned
 */
LockFreeRingResult lfring_spsc_peek(LockFreeRingBuffer *buffer, void *out);

// ============================================================================
// MPMC Operations (Multi Producer Multi Consumer)
// ============================================================================

/**
 * MPMC: Enqueue single element (lock-free with CAS)
 *
 * Args:
 *   buffer: MPMC LockFreeRingBuffer
 *   element: Pointer to element
 *
 * Returns:
 *   LFRING_OK if successful, LFRING_FULL if no space
 *
 * Note: Uses compare-and-swap, may have retries
 */
LockFreeRingResult lfring_mpmc_enqueue(LockFreeRingBuffer *buffer, const void *element);

/**
 * MPMC: Enqueue with maximum retries
 *
 * Args:
 *   buffer: MPMC LockFreeRingBuffer
 *   element: Element to enqueue
 *   max_retries: Maximum CAS retries before giving up
 *
 * Returns:
 *   LFRING_OK, LFRING_FULL, or LFRING_CONTENTION
 */
LockFreeRingResult lfring_mpmc_enqueue_with_retries(LockFreeRingBuffer *buffer, const void *element, u32 max_retries);

/**
 * MPMC: Dequeue single element
 *
 * Args:
 *   buffer: MPMC LockFreeRingBuffer
 *   out: Buffer for element
 *
 * Returns:
 *   LFRING_OK if element returned
 */
LockFreeRingResult lfring_mpmc_dequeue(LockFreeRingBuffer *buffer, void *out);

/**
 * MPMC: Dequeue with maximum retries
 *
 * Args:
 *   buffer: MPMC LockFreeRingBuffer
 *   out: Buffer for element
 *   max_retries: Maximum CAS retries
 *
 * Returns:
 *   LFRING_OK, LFRING_EMPTY, or LFRING_CONTENTION
 */
LockFreeRingResult lfring_mpmc_dequeue_with_retries(LockFreeRingBuffer *buffer, void *out, u32 max_retries);

/**
 * MPMC: Batch enqueue
 *
 * Args:
 *   buffer: MPMC LockFreeRingBuffer
 *   elements: Array of elements
 *   count: Number of elements
 *
 * Returns:
 *   Number of elements successfully enqueued
 */
u32 lfring_mpmc_enqueue_batch(LockFreeRingBuffer *buffer, const void *elements, u32 count);

/**
 * MPMC: Batch dequeue
 *
 * Args:
 *   buffer: MPMC LockFreeRingBuffer
 *   out: Buffer for elements
 *   count: Maximum to dequeue
 *
 * Returns:
 *   Number of elements dequeued
 */
u32 lfring_mpmc_dequeue_batch(LockFreeRingBuffer *buffer, void *out, u32 count);

// ============================================================================
// Capacity and Status
// ============================================================================

/**
 * Get buffer capacity
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   Maximum number of elements
 */
u32 lfring_capacity(LockFreeRingBuffer *buffer);

/**
 * Get current count of elements
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   Approximate number of elements (may be stale in MPMC)
 */
u32 lfring_count(LockFreeRingBuffer *buffer);

/**
 * Check if buffer is empty
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   true if no elements queued
 */
bool lfring_is_empty(LockFreeRingBuffer *buffer);

/**
 * Check if buffer is full
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   true if at capacity
 */
bool lfring_is_full(LockFreeRingBuffer *buffer);

/**
 * Get available space
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   Number of free slots
 */
u32 lfring_available(LockFreeRingBuffer *buffer);

// ============================================================================
// Cache-Line Padding and Performance
// ============================================================================

/**
 * Enable cache-line padding (for SPSC)
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *   enable: Whether to use padding
 *
 * Note: Increases memory usage but reduces false sharing
 */
void lfring_set_cache_padding(LockFreeRingBuffer *buffer, bool enable);

/**
 * Get cache-line size being used
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   Cache line size in bytes (typically 64)
 */
u32 lfring_cache_line_size(LockFreeRingBuffer *buffer);

/**
 * Get memory overhead due to padding
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   Extra bytes used for alignment
 */
u64 lfring_padding_overhead(LockFreeRingBuffer *buffer);

// ============================================================================
// Advanced: Batch Processing
// ============================================================================

/**
 * Reserve space for batch enqueue
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *   count: Number of elements to reserve
 *
 * Returns:
 *   Pointer to reserved space or NULL if can't reserve
 *
 * Note: Reserved space must be committed with lfring_commit_enqueue
 */
void *lfring_reserve_enqueue(LockFreeRingBuffer *buffer, u32 count);

/**
 * Commit reserved enqueue space
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *   count: Number of elements to commit
 *
 * Note: count must match or be less than reserved count
 */
void lfring_commit_enqueue(LockFreeRingBuffer *buffer, u32 count);

/**
 * Reserve space for batch dequeue
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *   count: Number of elements to dequeue
 *
 * Returns:
 *   Pointer to dequeueable elements or NULL if unavailable
 */
void *lfring_reserve_dequeue(LockFreeRingBuffer *buffer, u32 count);

/**
 * Commit reserved dequeue space
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *   count: Number of elements to dequeue
 */
void lfring_commit_dequeue(LockFreeRingBuffer *buffer, u32 count);

// ============================================================================
// Statistics and Benchmarking
// ============================================================================

/**
 * Get buffer statistics
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *   stats: Output structure
 */
void lfring_get_statistics(LockFreeRingBuffer *buffer, LockFreeRingBufferStats *stats);

/**
 * Reset statistics counters
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 */
void lfring_reset_statistics(LockFreeRingBuffer *buffer);

/**
 * Benchmark enqueue performance
 *
 * Args:
 *   capacity: Buffer capacity
 *   element_size: Element size
 *   iterations: Number of operations
 *   mode: Operating mode
 *
 * Returns:
 *   Average nanoseconds per operation
 */
u64 lfring_benchmark_enqueue(u32 capacity, u32 element_size, u32 iterations, LockFreeRingMode mode);

/**
 * Benchmark dequeue performance
 *
 * Args:
 *   capacity: Buffer capacity
 *   element_size: Element size
 *   iterations: Number of operations
 *   mode: Operating mode
 *
 * Returns:
 *   Average nanoseconds per operation
 */
u64 lfring_benchmark_dequeue(u32 capacity, u32 element_size, u32 iterations, LockFreeRingMode mode);

/**
 * Benchmark batch operations
 *
 * Args:
 *   capacity: Buffer capacity
 *   element_size: Element size
 *   batch_size: Size of batches
 *   iterations: Number of batch operations
 *   mode: Operating mode
 *   enqueue_ns: Output for enqueue time
 *   dequeue_ns: Output for dequeue time
 */
void lfring_benchmark_batch(u32 capacity, u32 element_size, u32 batch_size, u32 iterations, LockFreeRingMode mode, u64 *enqueue_ns, u64 *dequeue_ns);

// ============================================================================
// Memory and Debugging
// ============================================================================

/**
 * Get total memory usage
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   Total bytes allocated
 */
u64 lfring_memory_usage(LockFreeRingBuffer *buffer);

/**
 * Print diagnostic information
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 */
void lfring_print_diagnostics(LockFreeRingBuffer *buffer);

/**
 * Validate buffer integrity
 *
 * Args:
 *   buffer: LockFreeRingBuffer
 *
 * Returns:
 *   true if buffer is valid
 */
bool lfring_validate(LockFreeRingBuffer *buffer);

// ============================================================================
// Testing
// ============================================================================

/**
 * Run comprehensive tests
 *
 * Args:
 *   capacity: Buffer capacity to test with
 *   iterations: Number of test iterations
 *
 * Returns:
 *   0 if all tests passed, error count otherwise
 */
u32 lfring_run_tests(u32 capacity, u32 iterations);

/**
 * Stress test with multiple threads
 *
 * Args:
 *   capacity: Buffer capacity
 *   num_producers: Number of producer threads
 *   num_consumers: Number of consumer threads
 *   duration_ms: Test duration in milliseconds
 *   mode: Operating mode
 *
 * Returns:
 *   true if stress test passed
 */
bool lfring_stress_test(u32 capacity, u32 num_producers, u32 num_consumers, u32 duration_ms, LockFreeRingMode mode);

#ifdef __cplusplus
}
#endif

#endif // LOCK_FREE_RING_BUFFER_H
