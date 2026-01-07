#ifndef ALLOCATOR_STACK_H
#define ALLOCATOR_STACK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct StackAllocator StackAllocator;
typedef struct StackMarker StackMarker;

/**
 * Create a new stack allocator with the specified size
 * @param size Size in bytes for the allocator
 * @return Pointer to allocator or NULL on failure
 */
StackAllocator* stack_allocator_create(size_t size);

/**
 * Create a stack allocator from existing memory
 * @param memory Pointer to existing memory block
 * @param size Size of the memory block
 * @return Pointer to allocator or NULL on failure
 */
StackAllocator* stack_allocator_create_from_memory(void *memory, size_t size);

/**
 * Destroy a stack allocator and free its memory
 * @param allocator Pointer to allocator
 */
void stack_allocator_destroy(StackAllocator *allocator);

/**
 * Set debug mode for memory scrubbing
 * @param allocator Pointer to allocator
 * @param debug_mode Whether to scrub memory on deallocation
 */
void stack_allocator_set_debug_mode(StackAllocator *allocator, bool debug_mode);

/**
 * Allocate memory from the stack allocator
 * @param allocator Pointer to allocator
 * @param size Size in bytes to allocate
 * @param alignment Alignment requirement (0 for default 16-byte alignment)
 * @return Pointer to allocated memory or NULL on failure
 */
void* stack_allocator_allocate(StackAllocator *allocator, size_t size, size_t alignment);

/**
 * Try to allocate memory (non-asserting version)
 * @param allocator Pointer to allocator
 * @param size Size in bytes to allocate
 * @param alignment Alignment requirement (0 for default 16-byte alignment)
 * @return Pointer to allocated memory or NULL on failure
 */
void* stack_allocator_try_allocate(StackAllocator *allocator, size_t size, size_t alignment);

/**
 * Get remaining space in allocator
 * @param allocator Pointer to allocator
 * @return Remaining bytes in allocator
 */
size_t stack_allocator_get_remaining_space(StackAllocator *allocator);

/**
 * Get a marker for the current allocation position
 * @param allocator Pointer to allocator
 * @param file Source file (debug builds only)
 * @param line Source line (debug builds only)
 * @return Marker representing current position
 */
StackMarker* stack_allocator_get_marker(StackAllocator *allocator
#ifdef DEBUG
                                         , const char *file, int line
#endif
                                        );

/**
 * Reset allocator to a specific marker position
 * @param allocator Pointer to allocator
 * @param marker Marker position to reset to
 */
void stack_allocator_free_to_marker(StackAllocator *allocator, StackMarker *marker);

/**
 * Reset the allocator to its initial state
 * @param allocator Pointer to allocator
 */
void stack_allocator_reset(StackAllocator *allocator);

/**
 * Get allocator statistics
 * @param allocator Pointer to allocator
 * @param total_size Output for total size (can be NULL)
 * @param used_size Output for used size (can be NULL)
 * @param peak_usage Output for peak usage (can be NULL)
 * @param allocation_count Output for allocation count (can be NULL)
 * @param marker_count Output for marker count (can be NULL)
 */
void stack_allocator_get_stats(StackAllocator *allocator, size_t *total_size, size_t *used_size, 
                             size_t *peak_usage, uint32_t *allocation_count, uint32_t *marker_count);

/**
 * Check if allocator owns a pointer
 * @param allocator Pointer to allocator
 * @param ptr Pointer to check
 * @return True if allocator owns the pointer
 */
bool stack_allocator_owns_pointer(StackAllocator *allocator, void *ptr);

/**
 * Get current allocation position (for advanced use)
 * @param allocator Pointer to allocator
 * @return Current allocation position
 */
void* stack_allocator_get_current_position(StackAllocator *allocator);

// Debug convenience macros
#ifdef DEBUG
#define STACK_GET_MARKER(alloc) stack_allocator_get_marker(alloc, __FILE__, __LINE__)
#else
#define STACK_GET_MARKER(alloc) stack_allocator_get_marker(alloc)
#endif

#ifdef __cplusplus
}
#endif

#endif // ALLOCATOR_STACK_H
