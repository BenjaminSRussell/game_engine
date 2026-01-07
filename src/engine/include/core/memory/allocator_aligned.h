#ifndef ALLOCATOR_ALIGNED_H
#define ALLOCATOR_ALIGNED_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct AlignedAllocator AlignedAllocator;

/**
 * Create a new aligned memory allocator
 * @param backing_allocator Backing allocator to use (NULL for malloc)
 * @param default_alignment Default alignment for allocations
 * @return Pointer to allocator or NULL on failure
 */
AlignedAllocator* aligned_allocator_create(void *backing_allocator, size_t default_alignment);

/**
 * Destroy an aligned allocator
 * @param allocator Pointer to allocator
 */
void aligned_allocator_destroy(AlignedAllocator *allocator);

/**
 * Allocate aligned memory
 * @param allocator Pointer to allocator
 * @param size Size in bytes to allocate
 * @param alignment Alignment requirement (0 for default)
 * @return Pointer to allocated memory or NULL on failure
 */
void* aligned_allocator_allocate(AlignedAllocator *allocator, size_t size, size_t alignment);

/**
 * Deallocate aligned memory
 * @param allocator Pointer to allocator
 * @param ptr Pointer to memory to deallocate
 */
void aligned_allocator_deallocate(AlignedAllocator *allocator, void *ptr);

/**
 * Reallocate aligned memory
 * @param allocator Pointer to allocator
 * @param ptr Pointer to existing memory
 * @param size New size in bytes
 * @param alignment Alignment requirement (0 for default)
 * @return Pointer to reallocated memory or NULL on failure
 */
void* aligned_allocator_reallocate(AlignedAllocator *allocator, void *ptr, size_t size, size_t alignment);

/**
 * Get allocator statistics
 * @param allocator Pointer to allocator
 * @param total_allocated Output for total allocated memory (can be NULL)
 * @param allocation_count Output for allocation count (can be NULL)
 * @param default_alignment Output for default alignment (can be NULL)
 */
void aligned_allocator_get_stats(AlignedAllocator *allocator, size_t *total_allocated, 
                                uint32_t *allocation_count, size_t *default_alignment);

/**
 * Check if alignment is power of 2
 * @param alignment Alignment value to check
 * @return True if alignment is power of 2
 */
bool aligned_allocator_is_valid_alignment(size_t alignment);

/**
 * Get next power of 2 alignment
 * @param size Size to align
 * @return Next power of 2 >= size
 */
size_t aligned_allocator_get_next_power_of_2(size_t size);

/**
 * Round up size to alignment
 * @param size Size to round
 * @param alignment Alignment boundary
 * @return Rounded up size
 */
size_t aligned_allocator_round_up(size_t size, size_t alignment);

#ifdef __cplusplus
}
#endif

#endif // ALLOCATOR_ALIGNED_H
