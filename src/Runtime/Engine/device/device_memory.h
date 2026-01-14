#ifndef DEVICE_MEMORY_H
#define DEVICE_MEMORY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gpu_allocation_t {
    void *ptr;
    uint64_t size;
    uint32_t flags;
    uint64_t offset;
    void *internal_handle;
} gpu_allocation_t;

/**
 * Allocate memory on the GPU device
 * @param out_allocation Pointer to allocation structure to fill
 * @param size Size in bytes
 * @param flags Allocation flags (usage, visibility, etc)
 * @return 0 on success, non-zero on failure
 */
int core_device_memory_allocate(gpu_allocation_t *out_allocation, uint64_t size, uint32_t flags);

/**
 * Free memory on the GPU device
 * @param allocation Pointer to allocation structure to free
 */
void core_device_memory_free(gpu_allocation_t *allocation);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_MEMORY_H
