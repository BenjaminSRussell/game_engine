#ifndef DEVICE_MEMORY_H
#define DEVICE_MEMORY_H

#include <stdint.h>
#include "core/memory/gpu_allocator.h" // For gpu_allocation_t

#ifdef __cplusplus
extern "C" {
#endif

// Stub functions for device memory management
int core_device_memory_allocate(gpu_allocation_t* allocation, uint64_t size, uint32_t flags);
void core_device_memory_free(gpu_allocation_t* allocation);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_MEMORY_H
