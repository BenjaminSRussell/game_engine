#ifndef FRAME_ALLOCATOR_H
#define FRAME_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

// Initialize frame allocator
bool frame_allocator_init(void);

// Shutdown frame allocator
void frame_allocator_shutdown(void);

// Allocate from frame stack
void *frame_alloc(size_t size);

// Allocate from frame stack (zero-initialized)
void *frame_alloc_zero(size_t size);

// Reset frame allocator (call at end of each frame)
void frame_allocator_reset(void);

// Get current usage
size_t frame_allocator_get_usage(void);

// Get peak usage
size_t frame_allocator_get_peak(void);

// Print statistics
void frame_allocator_print_stats(void);

// Convenience macros
#define FRAME_ALLOC(size) frame_alloc(size)
#define FRAME_ALLOC_TYPE(type) (type *)frame_alloc(sizeof(type))
#define FRAME_ALLOC_ARRAY(type, count)                                         \
  (type *)frame_alloc(sizeof(type) * (count))

#endif // FRAME_ALLOCATOR_H
