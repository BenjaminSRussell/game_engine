/*
 * mtl_memory_heap.c
 * Metal memory heap management implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_memory_heap.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef __OBJC__
#import <Metal/Metal.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct mtl_heap_internal {
    id<MTLHeap> heap;
    uint64_t total_size;
    uint64_t used_size;
    uint64_t peak_usage;
    uint32_t num_allocations;
    pthread_mutex_t mutex;
    bool valid;
} mtl_heap_internal_t;

typedef struct mtl_device_budget_info {
    uint64_t budget;
    uint64_t current_usage;
    mtl_memory_warning_callback_t callback;
    void* callback_user_data;
    pthread_mutex_t mutex;
} mtl_device_budget_info_t;

// Global budget tracking (simplified - in production you'd track per device)
static mtl_device_budget_info_t g_budget_info = {
    .budget = 0,
    .current_usage = 0,
    .callback = NULL,
    .callback_user_data = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

/* ============================================================================
 * MEMORY INFO QUERIES
 * ============================================================================ */

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
    
    // Heap size tracking
    if (out_info->has_unified_memory) {
        out_info->shared_heap_size = out_info->recommended_working_set_size; 
    } else {
        out_info->private_heap_size = out_info->recommended_working_set_size;
    }
    
    // Memory Pressure calculation
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

/* ============================================================================
 * HEAP MANAGEMENT
 * ============================================================================ */

mtl_heap_t mtl_heap_create(void* device, const mtl_heap_desc_t* desc) {
    if (!device || !desc || desc->size == 0) return NULL;
    
    id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)device;
    
    MTLHeapDescriptor* heapDesc = [[MTLHeapDescriptor alloc] init];
    heapDesc.size = desc->size;
    
    // Map storage mode
    switch (desc->storage_mode) {
        case MTL_STORAGE_SHARED:
            heapDesc.storageMode = MTLStorageModeShared;
            break;
        case MTL_STORAGE_PRIVATE:
            heapDesc.storageMode = MTLStorageModePrivate;
            break;
        case MTL_STORAGE_MEMORYLESS:
            if (@available(macOS 11.0, *)) {
                heapDesc.storageMode = MTLStorageModeMemoryless;
            } else {
                heapDesc.storageMode = MTLStorageModePrivate;
            }
            break;
    }
    
    // Map CPU cache mode
    switch (desc->cpu_cache_mode) {
        case MTL_CPU_CACHE_DEFAULT_CACHE:
            heapDesc.cpuCacheMode = MTLCPUCacheModeDefaultCache;
            break;
        case MTL_CPU_CACHE_WRITE_COMBINED:
            heapDesc.cpuCacheMode = MTLCPUCacheModeWriteCombined;
            break;
    }
    
    id<MTLHeap> heap = [mtl_device newHeapWithDescriptor:heapDesc];
    if (!heap) {
        return NULL;
    }
    
    if (desc->label) {
        heap.label = [NSString stringWithUTF8String:desc->label];
    }
    
    // Create internal tracking structure
    mtl_heap_internal_t* internal = malloc(sizeof(mtl_heap_internal_t));
    if (!internal) {
        return NULL;
    }
    
    internal->heap = heap;
    internal->total_size = desc->size;
    internal->used_size = 0;
    internal->peak_usage = 0;
    internal->num_allocations = 0;
    internal->valid = true;
    pthread_mutex_init(&internal->mutex, NULL);
    
    return (mtl_heap_t)internal;
}

void mtl_heap_destroy(mtl_heap_t heap) {
    if (!heap) return;
    
    mtl_heap_internal_t* internal = (mtl_heap_internal_t*)heap;
    
    pthread_mutex_lock(&internal->mutex);
    internal->valid = false;
    internal->heap = nil;
    pthread_mutex_unlock(&internal->mutex);
    
    pthread_mutex_destroy(&internal->mutex);
    free(internal);
}

uint64_t mtl_heap_get_available_size(mtl_heap_t heap) {
    if (!heap) return 0;
    
    mtl_heap_internal_t* internal = (mtl_heap_internal_t*)heap;
    if (!internal->valid) return 0;
    
    if (@available(macOS 10.13, *)) {
        return [internal->heap maxAvailableSizeWithAlignment:256];
    }
    
    return internal->total_size - internal->used_size;
}

uint64_t mtl_heap_get_size(mtl_heap_t heap) {
    if (!heap) return 0;
    
    mtl_heap_internal_t* internal = (mtl_heap_internal_t*)heap;
    return internal->valid ? [internal->heap size] : 0;
}

uint64_t mtl_heap_get_used_size(mtl_heap_t heap) {
    if (!heap) return 0;
    
    mtl_heap_internal_t* internal = (mtl_heap_internal_t*)heap;
    if (!internal->valid) return 0;
    
    return [internal->heap usedSize];
}

void mtl_heap_get_usage(mtl_heap_t heap, mtl_heap_usage_t* out_usage) {
    if (!heap || !out_usage) return;
    
    mtl_heap_internal_t* internal = (mtl_heap_internal_t*)heap;
    
    pthread_mutex_lock(&internal->mutex);
    
    if (internal->valid) {
        out_usage->total_size = [internal->heap size];
        out_usage->used_size = [internal->heap usedSize];
        out_usage->available_size = mtl_heap_get_available_size(heap);
        out_usage->num_allocations = internal->num_allocations;
        out_usage->peak_usage = internal->peak_usage;
    } else {
        memset(out_usage, 0, sizeof(mtl_heap_usage_t));
    }
    
    pthread_mutex_unlock(&internal->mutex);
}

bool mtl_heap_validate(mtl_heap_t heap) {
    if (!heap) return false;
    
    mtl_heap_internal_t* internal = (mtl_heap_internal_t*)heap;
    
    pthread_mutex_lock(&internal->mutex);
    bool valid = internal->valid && (internal->heap != nil);
    pthread_mutex_unlock(&internal->mutex);
    
    return valid;
}

/* ============================================================================
 * HEAP-BASED ALLOCATION
 * ============================================================================ */

mtl_buffer_t mtl_heap_allocate_buffer(mtl_heap_t heap, uint64_t size, uint64_t alignment) {
    if (!heap || size == 0) return NULL;
    
    mtl_heap_internal_t* internal = (mtl_heap_internal_t*)heap;
    if (!internal->valid) return NULL;
    
    if (alignment == 0) alignment = 256; // Default Metal alignment
    
    pthread_mutex_lock(&internal->mutex);
    
    MTLResourceOptions options = MTLResourceStorageModePrivate;
    
    if (@available(macOS 10.15, *)) {
        id<MTLBuffer> buffer = [internal->heap newBufferWithLength:size options:options];
        if (buffer) {
            internal->num_allocations++;
            uint64_t current_used = [internal->heap usedSize];
            if (current_used > internal->peak_usage) {
                internal->peak_usage = current_used;
            }
        }
        pthread_mutex_unlock(&internal->mutex);
        return (__bridge void*)buffer;
    }
    
    pthread_mutex_unlock(&internal->mutex);
    return NULL;
}

mtl_texture_t mtl_heap_allocate_texture(mtl_heap_t heap, void* descriptor) {
    if (!heap || !descriptor) return NULL;
    
    mtl_heap_internal_t* internal = (mtl_heap_internal_t*)heap;
    if (!internal->valid) return NULL;
    
    MTLTextureDescriptor* texDesc = (__bridge MTLTextureDescriptor*)descriptor;
    
    pthread_mutex_lock(&internal->mutex);
    
    if (@available(macOS 10.15, *)) {
        id<MTLTexture> texture = [internal->heap newTextureWithDescriptor:texDesc];
        if (texture) {
            internal->num_allocations++;
            uint64_t current_used = [internal->heap usedSize];
            if (current_used > internal->peak_usage) {
                internal->peak_usage = current_used;
            }
        }
        pthread_mutex_unlock(&internal->mutex);
        return (__bridge void*)texture;
    }
    
    pthread_mutex_unlock(&internal->mutex);
    return NULL;
}

/* ============================================================================
 * MEMORY BUDGET MANAGEMENT
 * ============================================================================ */

void mtl_memory_set_budget(void* device, uint64_t budget_bytes) {
    pthread_mutex_lock(&g_budget_info.mutex);
    g_budget_info.budget = budget_bytes;
    pthread_mutex_unlock(&g_budget_info.mutex);
}

void mtl_memory_get_budget(void* device, mtl_memory_budget_t* out_budget) {
    if (!out_budget) return;
    
    if (!device) {
        memset(out_budget, 0, sizeof(mtl_memory_budget_t));
        return;
    }
    
    id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)device;
    
    pthread_mutex_lock(&g_budget_info.mutex);
    
    uint64_t budget = g_budget_info.budget;
    if (budget == 0 && @available(macOS 10.12, *)) {
        budget = [mtl_device recommendedMaxWorkingSetSize];
    }
    
    uint64_t current = 0;
    if (@available(macOS 10.12, *)) {
        current = [mtl_device currentAllocatedSize];
    }
    
    out_budget->total_budget = budget;
    out_budget->current_usage = current;
    out_budget->available = (budget > current) ? (budget - current) : 0;
    out_budget->is_over_budget = (current > budget);
    
    pthread_mutex_unlock(&g_budget_info.mutex);
}

bool mtl_memory_would_exceed_budget(void* device, uint64_t size) {
    if (!device) return false;
    
    mtl_memory_budget_t budget;
    mtl_memory_get_budget(device, &budget);
    
    return (budget.current_usage + size) > budget.total_budget;
}

void mtl_memory_register_warning_callback(void* device, mtl_memory_warning_callback_t callback, void* user_data) {
    pthread_mutex_lock(&g_budget_info.mutex);
    g_budget_info.callback = callback;
    g_budget_info.callback_user_data = user_data;
    pthread_mutex_unlock(&g_budget_info.mutex);
}

/* ============================================================================
 * MEMORY STATISTICS
 * ============================================================================ */

size_t mtl_memory_export_stats(void* device, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return 0;
    
    mtl_memory_info_t info;
    if (device) {
        id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)device;
        metal_device_query_memory_info_internal(mtl_device, &info);
    } else {
        memset(&info, 0, sizeof(info));
    }
    
    mtl_memory_budget_t budget;
    mtl_memory_get_budget(device, &budget);
    
    size_t written = 0;
    written += snprintf(buffer + written, buffer_size - written,
        "=== Metal Memory Statistics ===\n");
    written += snprintf(buffer + written, buffer_size - written,
        "Current Allocated: %.2f MB\n", info.current_allocated_size / (1024.0 * 1024.0));
    written += snprintf(buffer + written, buffer_size - written,
        "Recommended Working Set: %.2f MB\n", info.recommended_working_set_size / (1024.0 * 1024.0));
    written += snprintf(buffer + written, buffer_size - written,
        "Unified Memory: %s\n", info.has_unified_memory ? "Yes" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "\nBudget:\n");
    written += snprintf(buffer + written, buffer_size - written,
        "  Total Budget: %.2f MB\n", budget.total_budget / (1024.0 * 1024.0));
    written += snprintf(buffer + written, buffer_size - written,
        "  Current Usage: %.2f MB\n", budget.current_usage / (1024.0 * 1024.0));
    written += snprintf(buffer + written, buffer_size - written,
        "  Available: %.2f MB\n", budget.available / (1024.0 * 1024.0));
    written += snprintf(buffer + written, buffer_size - written,
        "  Over Budget: %s\n", budget.is_over_budget ? "YES" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "\nMemory Pressure: ");
    
    switch (info.memory_pressure_level) {
        case 0: written += snprintf(buffer + written, buffer_size - written, "Normal\n"); break;
        case 1: written += snprintf(buffer + written, buffer_size - written, "Warning\n"); break;
        case 2: written += snprintf(buffer + written, buffer_size - written, "Critical\n"); break;
        default: written += snprintf(buffer + written, buffer_size - written, "Unknown\n"); break;
    }
    
    return written;
}

#else
// Stub implementations for non-Objective-C compilation
void metal_device_query_memory_info_internal(void* device, mtl_memory_info_t* out_info) {
    if (out_info) memset(out_info, 0, sizeof(mtl_memory_info_t));
}

mtl_heap_t mtl_heap_create(void* device, const mtl_heap_desc_t* desc) { return NULL; }
void mtl_heap_destroy(mtl_heap_t heap) {}
uint64_t mtl_heap_get_available_size(mtl_heap_t heap) { return 0; }
uint64_t mtl_heap_get_size(mtl_heap_t heap) { return 0; }
uint64_t mtl_heap_get_used_size(mtl_heap_t heap) { return 0; }
void mtl_heap_get_usage(mtl_heap_t heap, mtl_heap_usage_t* out_usage) {
    if (out_usage) memset(out_usage, 0, sizeof(mtl_heap_usage_t));
}
bool mtl_heap_validate(mtl_heap_t heap) { return false; }
mtl_buffer_t mtl_heap_allocate_buffer(mtl_heap_t heap, uint64_t size, uint64_t alignment) { return NULL; }
mtl_texture_t mtl_heap_allocate_texture(mtl_heap_t heap, void* descriptor) { return NULL; }
void mtl_memory_set_budget(void* device, uint64_t budget_bytes) {}
void mtl_memory_get_budget(void* device, mtl_memory_budget_t* out_budget) {
    if (out_budget) memset(out_budget, 0, sizeof(mtl_memory_budget_t));
}
bool mtl_memory_would_exceed_budget(void* device, uint64_t size) { return false; }
void mtl_memory_register_warning_callback(void* device, mtl_memory_warning_callback_t callback, void* user_data) {}
size_t mtl_memory_export_stats(void* device, char* buffer, size_t buffer_size) { return 0; }
#endif
