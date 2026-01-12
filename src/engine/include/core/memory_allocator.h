// include/core/memory_allocator.h
//
// Purpose: Unified memory allocator interface for the engine core

#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <core/types.h>
#include <stddef.h>

// Memory allocation functions with tracking
void* memory_allocate(size_t size, const char* file, int line);
void* memory_reallocate(void* ptr, size_t new_size, const char* file, int line);
void memory_deallocate(void* ptr);

// Convenience macros
#define MEMORY_ALLOCATE(size) memory_allocate(size, __FILE__, __LINE__)
#define MEMORY_REALLOCATE(ptr, size) memory_reallocate(ptr, size, __FILE__, __LINE__)
#define MEMORY_DEALLOCATE(ptr) memory_deallocate(ptr)

// Memory allocator lifecycle
bool memory_allocator_init(void);
void memory_allocator_shutdown(void);

// Memory statistics
void memory_allocator_get_stats(size_t* total_allocated, size_t* peak_allocated, u64* allocation_count);

#endif // MEMORY_ALLOCATOR_H
