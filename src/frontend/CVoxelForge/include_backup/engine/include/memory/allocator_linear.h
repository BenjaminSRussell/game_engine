#ifndef ALLOCATOR_LINEAR_H
#define ALLOCATOR_LINEAR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct LinearAllocator LinearAllocator;
typedef size_t LinearAllocatorMarker;

/**
 * Create a new linear allocator with the specified size
 * @param size Size in bytes for the allocator
 * @return Pointer to allocator or NULL on failure
 */
LinearAllocator* linear_allocator_create(size_t size);

/**
 * Create a linear allocator from existing memory
 * @param memory Pointer to existing memory block
 * @param size Size of the memory block
 * @return Pointer to allocator or NULL on failure
 */
LinearAllocator* linear_allocator_create_from_memory(void *memory, size_t size);

/**
 * Destroy a linear allocator and free its memory
 * @param allocator Pointer to allocator
 */
void linear_allocator_destroy(LinearAllocator *allocator);

/**
 * Allocate memory from the linear allocator
 * @param allocator Pointer to allocator
 * @param size Size in bytes to allocate
 * @param alignment Alignment requirement (0 for default 16-byte alignment)
 * @return Pointer to allocated memory or NULL on failure
 */
void* linear_allocator_allocate(LinearAllocator *allocator, size_t size, size_t alignment);

/**
 * Reset the allocator to its initial state
 * @param allocator Pointer to allocator
 */
void linear_allocator_reset(LinearAllocator *allocator);

/**
 * Get a marker for the current allocation position
 * @param allocator Pointer to allocator
 * @return Marker representing current position
 */
LinearAllocatorMarker linear_allocator_get_marker(LinearAllocator *allocator);

/**
 * Reset allocator to a specific marker position
 * @param allocator Pointer to allocator
 * @param marker Marker position to reset to
 */
void linear_allocator_reset_to_marker(LinearAllocator *allocator, LinearAllocatorMarker marker);

/**
 * Get allocator statistics
 * @param allocator Pointer to allocator
 * @param total_size Output for total size (can be NULL)
 * @param used_size Output for used size (can be NULL)
 * @param peak_usage Output for peak usage (can be NULL)
 * @param allocation_count Output for allocation count (can be NULL)
 */
void linear_allocator_get_stats(LinearAllocator *allocator, size_t *total_size, size_t *used_size, 
                               size_t *peak_usage, uint32_t *allocation_count);

/**
 * Get remaining space in allocator
 * @param allocator Pointer to allocator
 * @return Remaining bytes in allocator
 */
size_t linear_allocator_get_remaining_space(LinearAllocator *allocator);

/**
 * Check if allocator owns a pointer
 * @param allocator Pointer to allocator
 * @param ptr Pointer to check
 * @return True if allocator owns the pointer
 */
bool linear_allocator_owns_pointer(LinearAllocator *allocator, void *ptr);

#ifdef __cplusplus
}
#endif

#endif // ALLOCATOR_LINEAR_H
