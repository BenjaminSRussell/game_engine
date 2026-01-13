/*
 * mtl_memory_heap.h
 * Metal memory heap management interface
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_MEMORY_HEAP_H
#define PLATFORM_MTL_MEMORY_HEAP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct mtl_memory_info {
    uint64_t current_allocated_size;
    uint64_t recommended_working_set_size;
    bool has_unified_memory;
    
    // Heap Usage Statistics
    uint64_t shared_heap_size;
    uint64_t private_heap_size;
    uint64_t memoryless_heap_size; // Generally 0 size but tracking count/bandwidth might be relevant
    
    // Memory Pressure
    // 0 = Normal, 1 = Warning, 2 = Critical
    uint32_t memory_pressure_level; 
} mtl_memory_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Updates and retrieves the current memory statistics for the device.
 * @param dev The metal device (opaque pointer)
 * @param out_info Pointer to the structure to fill with memory info.
 */
// Function prototype to be used by mtl_device.c
// void metal_device_get_memory_info(metal_device_t* dev, mtl_memory_info_t* out_info);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_MEMORY_HEAP_H */
