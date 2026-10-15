// Test-only stand-in for core/memory/unified_allocator.h. gpu_skinning.c calls
// FREE(ptr) with one argument; the real FREE takes (ptr, alloc_type).
#ifndef GPU_SKINNING_TEST_UNIFIED_ALLOCATOR_H
#define GPU_SKINNING_TEST_UNIFIED_ALLOCATOR_H

#include <stdlib.h>

#define MALLOC_PERSISTENT(size) malloc(size)
#define FREE(...) free(__VA_ARGS__)

#endif
