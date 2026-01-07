/*
 * mtl_device.c
 * Metal device implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_device.h"
#include "mtl_device_caps.h"
#include "mtl_memory_heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#if defined(__OBJC__)
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>

#define MAX_COMMAND_QUEUES 8

/* Queue Entry */
typedef struct queue_entry {
    id<MTLCommandQueue> queue;
    mtl_queue_type_t type;
    mtl_queue_priority_t priority;
    char label[64];
    uint64_t commands_submitted;
    bool in_use;
} queue_entry_t;

struct metal_device {
    id<MTLDevice> device;              // __bridge retained
    
    // Command Queues
    queue_entry_t queues[MAX_COMMAND_QUEUES];
    uint32_t num_queues;
    pthread_mutex_t queue_mutex;
    
    // Capabilities and Memory
    mtl_device_caps_t caps;
    mtl_memory_info_t memory_info;
    
    // Statistics
    mtl_device_stats_t stats;
    
    // Memory Management
    uint64_t memory_budget;
    mtl_memory_warning_callback_t warning_callback;
    void* warning_callback_user_data;
    
    // Reference counting
    uint32_t ref_count;
    pthread_mutex_t ref_mutex;
    
    // Debugging
    bool gpu_capture_enabled;
};

// Singleton instance
static metal_device_t* g_device_instance = NULL;
static pthread_mutex_t g_device_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
void metal_device_query_capabilities(id<MTLDevice> device, mtl_device_caps_t* out_caps);
extern void metal_device_query_memory_info_internal(id<MTLDevice> device, mtl_memory_info_t* out_info);

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static mtl_error_code_t internal_device_init(metal_device_t* dev, id<MTLDevice> mtl_dev, const mtl_device_options_t* opts) {
    dev->device = mtl_dev;
    dev->ref_count = 1;
    pthread_mutex_init(&dev->ref_mutex, NULL);
    pthread_mutex_init(&dev->queue_mutex, NULL);
    
    // Initialize statistics
    memset(&dev->stats, 0, sizeof(mtl_device_stats_t));
    
    // Initialize Capabilities
    metal_device_query_capabilities(dev->device, &dev->caps);
    
    // Initial Memory Info
    metal_device_query_memory_info_internal(dev->device, &dev->memory_info);
    
    // Set default memory budget to recommended working set
    dev->memory_budget = dev->memory_info.recommended_working_set_size;
    
    // Create primary graphics queue
    dev->queues[0].queue = [dev->device newCommandQueue];
    if (!dev->queues[0].queue) {
        return MTL_ERROR_QUEUE_CREATION_FAILED;
    }
    dev->queues[0].queue.label = @"Primary Graphics Queue";
    dev->queues[0].type = MTL_QUEUE_GRAPHICS;
    dev->queues[0].priority = MTL_QUEUE_PRIORITY_DEFAULT;
    strncpy(dev->queues[0].label, "Primary Graphics Queue", sizeof(dev->queues[0].label) - 1);
    dev->queues[0].in_use = true;
    dev->num_queues = 1;
    dev->stats.active_command_queues = 1;
    
    // Enable GPU capture if requested
    if (opts && opts->enable_debug_layer) {
        dev->gpu_capture_enabled = metal_device_enable_gpu_capture(dev);
    }
    
    printf("Metal Device Initialized: %s\n", dev->caps.device_name);
    printf("  - macOS Version: %u.%u.%u\n", dev->caps.os_version.major, dev->caps.os_version.minor, dev->caps.os_version.patch);
    printf("  - Feature Level: %d\n", dev->caps.feature_level);
    printf("  - Raytracing: %s\n", dev->caps.supports_raytracing ? "YES" : "NO");
    printf("  - GPU Family: %s\n", dev->caps.is_apple_silicon ? "Apple Silicon" : "Discrete/Other");
    printf("  - Unified Memory: %s\n", dev->caps.is_unified_memory ? "YES" : "NO");
    printf("  - Memory Budget: %.2f MB\n", dev->memory_budget / (1024.0 * 1024.0));
    
    return MTL_SUCCESS;
}

static int internal_find_queue_slot(metal_device_t* dev, mtl_queue_type_t type) {
    pthread_mutex_lock(&dev->queue_mutex);
    for (uint32_t i = 0; i < dev->num_queues; i++) {
        if (dev->queues[i].in_use && dev->queues[i].type == type) {
            pthread_mutex_unlock(&dev->queue_mutex);
            return (int)i;
        }
    }
    pthread_mutex_unlock(&dev->queue_mutex);
    return -1;
}

/* ============================================================================
 * DEVICE LIFECYCLE
 * ============================================================================ */

metal_device_t* metal_device_create_system_default(void) {
    return metal_device_create_with_options(NULL, NULL);
}

metal_device_t* metal_device_create_with_options(const mtl_device_options_t* options, mtl_error_code_t* out_error) {
    pthread_mutex_lock(&g_device_mutex);
    
    // Return singleton if already created
    if (g_device_instance) {
        metal_device_retain(g_device_instance);
        pthread_mutex_unlock(&g_device_mutex);
        if (out_error) *out_error = MTL_SUCCESS;
        return g_device_instance;
    }

    struct metal_device* dev = calloc(1, sizeof(struct metal_device));
    if (!dev) {
        pthread_mutex_unlock(&g_device_mutex);
        if (out_error) *out_error = MTL_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    // Create Metal device (with low power preference if requested)
    id<MTLDevice> mtl_dev;
    if (options && options->prefer_low_power) {
        NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
        mtl_dev = nil;
        for (id<MTLDevice> device in devices) {
            if ([device isLowPower]) {
                mtl_dev = device;
                break;
            }
        }
        if (!mtl_dev && [devices count] > 0) {
            mtl_dev = devices[0];
        }
    } else {
        mtl_dev = MTLCreateSystemDefaultDevice();
    }
    
    if (!mtl_dev) {
        fprintf(stderr, "Error: Failed to create Metal device.\n");
        free(dev);
        pthread_mutex_unlock(&g_device_mutex);
        if (out_error) *out_error = MTL_ERROR_DEVICE_NOT_FOUND;
        return NULL;
    }

    mtl_error_code_t init_result = internal_device_init(dev, mtl_dev, options);
    if (init_result != MTL_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize Metal device.\n");
        free(dev);
        pthread_mutex_unlock(&g_device_mutex);
        if (out_error) *out_error = init_result;
        return NULL;
    }

    g_device_instance = dev;
    pthread_mutex_unlock(&g_device_mutex);
    if (out_error) *out_error = MTL_SUCCESS;
    return dev;
}

metal_device_t* metal_device_get_default(void) {
    return g_device_instance;
}

void metal_device_retain(metal_device_t* dev) {
    if (!dev) return;
    pthread_mutex_lock(&dev->ref_mutex);
    dev->ref_count++;
    pthread_mutex_unlock(&dev->ref_mutex);
}

void metal_device_release(metal_device_t* dev) {
    if (!dev) return;
    
    pthread_mutex_lock(&dev->ref_mutex);
    dev->ref_count--;
    uint32_t ref_count = dev->ref_count;
    pthread_mutex_unlock(&dev->ref_mutex);
    
    if (ref_count == 0) {
        metal_device_destroy(dev);
    }
}

void metal_device_destroy(metal_device_t* dev) {
    if (!dev) return;
    
    pthread_mutex_lock(&g_device_mutex);
    if (dev == g_device_instance) {
        g_device_instance = NULL;
    }
    pthread_mutex_unlock(&g_device_mutex);
    
    // Clean up queues
    pthread_mutex_lock(&dev->queue_mutex);
    for (uint32_t i = 0; i < dev->num_queues; i++) {
        if (dev->queues[i].in_use) {
            dev->queues[i].queue = nil;
            dev->queues[i].in_use = false;
        }
    }
    pthread_mutex_unlock(&dev->queue_mutex);
    
    pthread_mutex_destroy(&dev->queue_mutex);
    pthread_mutex_destroy(&dev->ref_mutex);
    
    dev->device = nil;
    free(dev);
}

bool metal_device_validate(metal_device_t* dev) {
    if (!dev || !dev->device) return false;
    
    // Check if device is still valid
    @try {
        [dev->device name];
        return true;
    } @catch (NSException *exception) {
        return false;
    }
}

const char* metal_device_get_name(metal_device_t* dev) {
    if (!dev) return NULL;
    return dev->caps.device_name;
}

uint64_t metal_device_get_registry_id(metal_device_t* dev) {
    if (!dev) return 0;
    return dev->caps.registry_id;
}

/* ============================================================================
 * COMMAND QUEUE MANAGEMENT
 * ============================================================================ */

mtl_command_queue_t metal_device_create_queue(metal_device_t* dev, 
                                               mtl_queue_type_t type,
                                               mtl_queue_priority_t priority,
                                               const char* label,
                                               mtl_error_code_t* out_error) {
    if (!dev || !dev->device) {
        if (out_error) *out_error = MTL_ERROR_INVALID_PARAMETER;
        return NULL;
    }
    
    pthread_mutex_lock(&dev->queue_mutex);
    
    if (dev->num_queues >= MAX_COMMAND_QUEUES) {
        pthread_mutex_unlock(&dev->queue_mutex);
        if (out_error) *out_error = MTL_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    // Find an empty slot
    uint32_t slot = dev->num_queues;
    for (uint32_t i = 0; i < MAX_COMMAND_QUEUES; i++) {
        if (!dev->queues[i].in_use) {
            slot = i;
            break;
        }
    }
    
    id<MTLCommandQueue> queue = [dev->device newCommandQueue];
    if (!queue) {
        pthread_mutex_unlock(&dev->queue_mutex);
        if (out_error) *out_error = MTL_ERROR_QUEUE_CREATION_FAILED;
        return NULL;
    }
    
    if (label) {
        queue.label = [NSString stringWithUTF8String:label];
        strncpy(dev->queues[slot].label, label, sizeof(dev->queues[slot].label) - 1);
    } else {
        const char* type_str = (type == MTL_QUEUE_COMPUTE) ? "Compute" : 
                               (type == MTL_QUEUE_TRANSFER) ? "Transfer" : "Graphics";
        char auto_label[64];
        snprintf(auto_label, sizeof(auto_label), "%s Queue %u", type_str, slot);
        queue.label = [NSString stringWithUTF8String:auto_label];
        strncpy(dev->queues[slot].label, auto_label, sizeof(dev->queues[slot].label) - 1);
    }
    
    dev->queues[slot].queue = queue;
    dev->queues[slot].type = type;
    dev->queues[slot].priority = priority;
    dev->queues[slot].commands_submitted = 0;
    dev->queues[slot].in_use = true;
    
    if (slot >= dev->num_queues) {
        dev->num_queues = slot + 1;
    }
    dev->stats.active_command_queues++;
    
    pthread_mutex_unlock(&dev->queue_mutex);
    if (out_error) *out_error = MTL_SUCCESS;
    
    return (__bridge void*)queue;
}

mtl_command_queue_t metal_device_get_graphics_queue(metal_device_t* dev) {
    if (!dev) return NULL;
    
    int slot = internal_find_queue_slot(dev, MTL_QUEUE_GRAPHICS);
    if (slot >= 0) {
        return (__bridge void*)dev->queues[slot].queue;
    }
    
    return NULL;
}

mtl_command_queue_t metal_device_get_compute_queue(metal_device_t* dev) {
    if (!dev) return NULL;
    
    int slot = internal_find_queue_slot(dev, MTL_QUEUE_COMPUTE);
    if (slot >= 0) {
        return (__bridge void*)dev->queues[slot].queue;
    }
    
    // Create if not exists
    return metal_device_create_queue(dev, MTL_QUEUE_COMPUTE, MTL_QUEUE_PRIORITY_DEFAULT, "Compute Queue", NULL);
}

mtl_command_queue_t metal_device_get_transfer_queue(metal_device_t* dev) {
    if (!dev) return NULL;
    
    int slot = internal_find_queue_slot(dev, MTL_QUEUE_TRANSFER);
    if (slot >= 0) {
        return (__bridge void*)dev->queues[slot].queue;
    }
    
    // Create if not exists
    return metal_device_create_queue(dev, MTL_QUEUE_TRANSFER, MTL_QUEUE_PRIORITY_DEFAULT, "Transfer Queue", NULL);
}

void metal_device_destroy_queue(metal_device_t* dev, mtl_command_queue_t queue) {
    if (!dev || !queue) return;
    
    pthread_mutex_lock(&dev->queue_mutex);
    for (uint32_t i = 0; i < MAX_COMMAND_QUEUES; i++) {
        if (dev->queues[i].in_use && (__bridge void*)dev->queues[i].queue == queue) {
            dev->queues[i].queue = nil;
            dev->queues[i].in_use = false;
            dev->stats.active_command_queues--;
            break;
        }
    }
    pthread_mutex_unlock(&dev->queue_mutex);
}

mtl_command_buffer_t metal_create_command_buffer(metal_device_t* dev) {
    if (!dev) return NULL;
    return metal_create_command_buffer_from_queue(metal_device_get_graphics_queue(dev));
}

mtl_command_buffer_t metal_create_command_buffer_from_queue(mtl_command_queue_t queue) {
    if (!queue) return NULL;
    
    id<MTLCommandQueue> mtl_queue = (__bridge id<MTLCommandQueue>)queue;
    id<MTLCommandBuffer> buffer = [mtl_queue commandBuffer];
    return (__bridge void*)buffer;
}

/* ============================================================================
 * CAPABILITIES & INFORMATION
 * ============================================================================ */

const mtl_device_caps_t* metal_device_get_caps(metal_device_t* dev) {
    if (!dev) return NULL;
    return &dev->caps;
}

void metal_device_get_memory_info(metal_device_t* dev, mtl_memory_info_t* out_info) {
    if (!dev || !out_info) return;
    // Refresh info
    metal_device_query_memory_info_internal(dev->device, &dev->memory_info);
    *out_info = dev->memory_info;
    dev->stats.total_memory_allocated = dev->memory_info.current_allocated_size;
    if (dev->memory_info.current_allocated_size > dev->stats.peak_memory_usage) {
        dev->stats.peak_memory_usage = dev->memory_info.current_allocated_size;
    }
}

void metal_device_get_stats(metal_device_t* dev, mtl_device_stats_t* out_stats) {
    if (!dev || !out_stats) return;
    *out_stats = dev->stats;
}

bool metal_device_supports_raytracing(metal_device_t* dev) {
    if (!dev) return false;
    return dev->caps.supports_raytracing;
}

size_t metal_device_export_capabilities(metal_device_t* dev, char* buffer, size_t buffer_size) {
    if (!dev || !buffer || buffer_size == 0) return 0;
    
    size_t written = 0;
    written += snprintf(buffer + written, buffer_size - written, 
        "=== Metal Device Capabilities ===\n");
    written += snprintf(buffer + written, buffer_size - written,
        "Device: %s\n", dev->caps.device_name);
    written += snprintf(buffer + written, buffer_size - written,
        "Registry ID: %llu\n", dev->caps.registry_id);
    written += snprintf(buffer + written, buffer_size - written,
        "macOS Version: %u.%u.%u\n", dev->caps.os_version.major, dev->caps.os_version.minor, dev->caps.os_version.patch);
    written += snprintf(buffer + written, buffer_size - written,
        "Feature Level: %d\n", dev->caps.feature_level);
    written += snprintf(buffer + written, buffer_size - written,
        "\nGPU Properties:\n");
    written += snprintf(buffer + written, buffer_size - written,
        "  Apple Silicon: %s\n", dev->caps.is_apple_silicon ? "Yes" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "  Unified Memory: %s\n", dev->caps.is_unified_memory ? "Yes" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "  Low Power: %s\n", dev->caps.is_low_power ? "Yes" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "\nFeatures:\n");
    written += snprintf(buffer + written, buffer_size - written,
        "  Ray Tracing: %s\n", dev->caps.supports_raytracing ? "Yes" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "  Mesh Shaders: %s\n", dev->caps.supports_mesh_shaders ? "Yes" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "  Argument Buffers: %s\n", dev->caps.supports_argument_buffers ? "Yes" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "  Barycentric Coords: %s\n", dev->caps.supports_barycentric_coords ? "Yes" : "No");
    written += snprintf(buffer + written, buffer_size - written,
        "\nLimits:\n");
    written += snprintf(buffer + written, buffer_size - written,
        "  Max Buffer Size: %llu MB\n", dev->caps.max_buffer_size / (1024 * 1024));
    written += snprintf(buffer + written, buffer_size - written,
        "  Max Texture 2D: %ux%u\n", dev->caps.max_texture_width_2d, dev->caps.max_texture_height_2d);
    written += snprintf(buffer + written, buffer_size - written,
        "  Max Threads Per Group: %ux%ux%u\n", 
        dev->caps.max_threads_per_group[0], dev->caps.max_threads_per_group[1], dev->caps.max_threads_per_group[2]);
    
    return written;
}

/* ============================================================================
 * DEBUGGING & PROFILING
 * ============================================================================ */

bool metal_device_enable_gpu_capture(metal_device_t* dev) {
    if (!dev) return false;
    
    if (@available(macOS 10.15, *)) {
        MTLCaptureManager* captureManager = [MTLCaptureManager sharedCaptureManager];
        MTLCaptureDescriptor* descriptor = [[MTLCaptureDescriptor alloc] init];
        descriptor.captureObject = dev->device;
        descriptor.destination = MTLCaptureDestinationDeveloperTools;
        
        NSError* error = nil;
        if ([captureManager supportsDestination:MTLCaptureDestinationDeveloperTools]) {
            BOOL started = [captureManager startCaptureWithDescriptor:descriptor error:&error];
            if (started) {
                [captureManager stopCapture];
                dev->gpu_capture_enabled = true;
                return true;
            }
        }
    }
    
    return false;
}

void metal_device_begin_capture(metal_device_t* dev) {
    if (!dev || !dev->gpu_capture_enabled) return;
    
    if (@available(macOS 10.15, *)) {
        MTLCaptureManager* captureManager = [MTLCaptureManager sharedCaptureManager];
        MTLCaptureDescriptor* descriptor = [[MTLCaptureDescriptor alloc] init];
        descriptor.captureObject = dev->device;
        descriptor.destination = MTLCaptureDestinationDeveloperTools;
        
        NSError* error = nil;
        [captureManager startCaptureWithDescriptor:descriptor error:&error];
    }
}

void metal_device_end_capture(metal_device_t* dev) {
    if (!dev || !dev->gpu_capture_enabled) return;
    
    if (@available(macOS 10.15, *)) {
        MTLCaptureManager* captureManager = [MTLCaptureManager sharedCaptureManager];
        [captureManager stopCapture];
    }
}

#else
// Stub implementations for non-Objective-C compilation
struct metal_device;

metal_device_t* metal_device_create_system_default(void) { return NULL; }
metal_device_t* metal_device_create_with_options(const mtl_device_options_t* options, mtl_error_code_t* out_error) { 
    if (out_error) *out_error = MTL_ERROR_NOT_SUPPORTED;
    return NULL; 
}
metal_device_t* metal_device_get_default(void) { return NULL; }
void metal_device_retain(metal_device_t* dev) {}
void metal_device_release(metal_device_t* dev) {}
void metal_device_destroy(metal_device_t* dev) {}
bool metal_device_validate(metal_device_t* dev) { return false; }
const char* metal_device_get_name(metal_device_t* dev) { return NULL; }
uint64_t metal_device_get_registry_id(metal_device_t* dev) { return 0; }
mtl_command_queue_t metal_device_create_queue(metal_device_t* dev, mtl_queue_type_t type, mtl_queue_priority_t priority, const char* label, mtl_error_code_t* out_error) { 
    if (out_error) *out_error = MTL_ERROR_NOT_SUPPORTED;
    return NULL; 
}
mtl_command_queue_t metal_device_get_graphics_queue(metal_device_t* dev) { return NULL; }
mtl_command_queue_t metal_device_get_compute_queue(metal_device_t* dev) { return NULL; }
mtl_command_queue_t metal_device_get_transfer_queue(metal_device_t* dev) { return NULL; }
void metal_device_destroy_queue(metal_device_t* dev, mtl_command_queue_t queue) {}
mtl_command_buffer_t metal_create_command_buffer(metal_device_t* dev) { return NULL; }
mtl_command_buffer_t metal_create_command_buffer_from_queue(mtl_command_queue_t queue) { return NULL; }
const mtl_device_caps_t* metal_device_get_caps(metal_device_t* dev) { return NULL; }
void metal_device_get_memory_info(metal_device_t* dev, mtl_memory_info_t* out_info) {}
void metal_device_get_stats(metal_device_t* dev, mtl_device_stats_t* out_stats) {}
bool metal_device_supports_raytracing(metal_device_t* dev) { return false; }
size_t metal_device_export_capabilities(metal_device_t* dev, char* buffer, size_t buffer_size) { return 0; }
bool metal_device_enable_gpu_capture(metal_device_t* dev) { return false; }
void metal_device_begin_capture(metal_device_t* dev) {}
void metal_device_end_capture(metal_device_t* dev) {}
#endif
