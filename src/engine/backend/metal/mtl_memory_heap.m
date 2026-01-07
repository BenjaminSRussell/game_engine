/*
 * mtl_memory_heap.c
 * Metal memory heap management implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_memory_heap.h"
#include <string.h>

#ifdef __OBJC__
#import <Metal/Metal.h>

void metal_device_query_memory_info_internal(id<MTLDevice> device, mtl_memory_info_t* out_info) {
    if (!device || !out_info) return;
    
    memset(out_info, 0, sizeof(mtl_memory_info_t));
    
    if (@available(macOS 10.12, *)) {
        out_info->current_allocated_size = [device currentAllocatedSize];
        out_info->recommended_working_set_size = [device recommendedMaxWorkingSetSize];
    }
    
    if (@available(macOS 10.15, *)) {
        out_info->has_unified_memory = [device hasUnifiedMemory];
    }
    
    // Heaps size tracking would typically require manual accounting if not exposed by API directly per heap type.
    // Metal doesn't expose "total heap size" per type easily globally without tracking allocations.
    // However, we can query memory limits.
    
    // Memoryless is supported on Apple Silicon (Unified Memory)
    if (out_info->has_unified_memory) {
        // Technically system memory is shared
        out_info->shared_heap_size = out_info->recommended_working_set_size; 
    } else {
        // Discrete GPU memory
        // API doesn't always expose VRAM size directly in a standard property across all OS versions easily
        // but recommendedMaxWorkingSetSize is a good proxy for usable VRAM on discrete.
        out_info->private_heap_size = out_info->recommended_working_set_size;
    }
    
    // Memory Pressure
    // This is usually an event-driven notification (MTLDeviceProximateFreeBytes), 
    // but for snapshotting we might just check if we are near the limit.
    if (out_info->recommended_working_set_size > 0) {
        double usage = (double)out_info->current_allocated_size / (double)out_info->recommended_working_set_size;
        if (usage > 0.9) {
            out_info->memory_pressure_level = 2; // Critical
        } else if (usage > 0.8) {
            out_info->memory_pressure_level = 1; // Warning
        } else {
            out_info->memory_pressure_level = 0; // Normal
        }
    }
}

#endif
