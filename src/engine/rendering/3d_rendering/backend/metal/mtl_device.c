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
#include <pthread.h>

#if defined(__OBJC__)
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct metal_device {
    id<MTLDevice> device;              // __bridge retained
    id<MTLCommandQueue> command_queue; // __bridge retained
    
    mtl_device_caps_t caps;
    mtl_memory_info_t memory_info;
    
    // Internal synchronization if needed
};

// Singleton instance
static metal_device_t* g_device_instance = NULL;
static pthread_mutex_t g_device_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations of internal helpers from other units if not in headers
void metal_device_query_capabilities(id<MTLDevice> device, mtl_device_caps_t* out_caps);
extern void metal_device_query_memory_info_internal(id<MTLDevice> device, mtl_memory_info_t* out_info);

#else
// Forward declaration for C compiler to avoid errors if compiled as C
// meaningful content is only visible to ObjC compiler
struct metal_device;
#endif

metal_device_t* metal_device_create_system_default(void) {
#if defined(__OBJC__)
    pthread_mutex_lock(&g_device_mutex);
    if (g_device_instance) {
        pthread_mutex_unlock(&g_device_mutex);
        return g_device_instance;
    }

    struct metal_device* dev = calloc(1, sizeof(struct metal_device));
    if (!dev) {
        pthread_mutex_unlock(&g_device_mutex);
        return NULL;
    }

    dev->device = MTLCreateSystemDefaultDevice();
    if (!dev->device) {
        fprintf(stderr, "Error: Failed to create default Metal device.\n");
        free(dev);
        pthread_mutex_unlock(&g_device_mutex);
        return NULL;
    }

    dev->command_queue = [dev->device newCommandQueue];
    if (!dev->command_queue) {
        fprintf(stderr, "Error: Failed to create Metal command queue.\n");
        dev->device = nil;
        free(dev);
        pthread_mutex_unlock(&g_device_mutex);
        return NULL;
    }

    // Initialize Capabilities
    metal_device_query_capabilities(dev->device, &dev->caps);
    
    // Initial Memory Info
    metal_device_query_memory_info_internal(dev->device, &dev->memory_info);

    printf("Metal Device Initialized: %s\n", dev->caps.device_name);
    printf("  - Raytracing: %s\n", dev->caps.supports_raytracing ? "YES" : "NO");
    printf("  - GPU Family: %s\n", dev->caps.is_apple_silicon ? "Apple Silicon" : "Discrete/Other");

    g_device_instance = dev;
    pthread_mutex_unlock(&g_device_mutex);
    return dev;
#else
    return NULL;
#endif
}

metal_device_t* metal_device_get_default(void) {
#if defined(__OBJC__)
    return g_device_instance;
#else
    return NULL;
#endif
}

void* metal_device_get_mtl_device(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev) return NULL;
    return (__bridge void*)dev->device;
#else
    return NULL;
#endif
}

void metal_device_destroy(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev) return;
    
    pthread_mutex_lock(&g_device_mutex);
    if (dev == g_device_instance) {
        g_device_instance = NULL;
    }
    pthread_mutex_unlock(&g_device_mutex);
    
    dev->command_queue = nil;
    dev->device = nil;
    
    free(dev);
#endif
}

mtl_command_buffer_t metal_create_command_buffer(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev || !dev->command_queue) return NULL;
    
    id<MTLCommandBuffer> buffer = [dev->command_queue commandBuffer];
    return (__bridge void*)buffer; // Return raw pointer, lifetime managed by autorelease initially
                                   // Caller might need to retain if keeping it across frames, 
                                   // but typical command buffer usage is transient.
#else
    return NULL;
#endif
}

const mtl_device_caps_t* metal_device_get_caps(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev) return NULL;
    return &dev->caps;
#else
    return NULL;
#endif
}

void metal_device_get_memory_info(metal_device_t* dev, mtl_memory_info_t* out_info) {
#if defined(__OBJC__)
    if (!dev || !out_info) return;
    // Refresh info
    metal_device_query_memory_info_internal(dev->device, &dev->memory_info);
    *out_info = dev->memory_info;
#endif
}

bool metal_device_supports_raytracing(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev) return false;
    return dev->caps.supports_raytracing;
#else
    return false;
#endif
}

/* ============================================================================
 * MULTI-QUEUE SUPPORT
 * ============================================================================ */

mtl_command_queue_t metal_device_create_command_queue(metal_device_t* dev, int priority) {
#if defined(__OBJC__)
    if (!dev || !dev->device) return NULL;
    
    id<MTLCommandQueue> queue = [dev->device newCommandQueue];
    if (!queue) return NULL;
    
    // Set priority if requested (only some devices support this)
    if (priority > 0) {
        if (@available(macOS 10.13, *)) {
            // Priority is a hint, not guaranteed
            // MTLCommandQueue doesn't directly expose priority,
            // but you can create with descriptor for max buffer count
        }
    }
    
    return (__bridge_retained void*)queue;
#else
    return NULL;
#endif
}

void metal_command_queue_destroy(mtl_command_queue_t queue) {
#if defined(__OBJC__)
    if (queue) {
        CFRelease(queue);
    }
#endif
}

mtl_command_buffer_t metal_command_queue_create_command_buffer(mtl_command_queue_t queue) {
#if defined(__OBJC__)
    if (!queue) return NULL;
    
    id<MTLCommandQueue> mtl_queue = (__bridge id<MTLCommandQueue>)queue;
    id<MTLCommandBuffer> buffer = [mtl_queue commandBuffer];
    
    return (__bridge void*)buffer;
#else
    return NULL;
#endif
}

void metal_command_queue_set_label(mtl_command_queue_t queue, const char* label) {
#if defined(__OBJC__)
    if (queue && label) {
        id<MTLCommandQueue> mtl_queue = (__bridge id<MTLCommandQueue>)queue;
        [mtl_queue setLabel:[NSString stringWithUTF8String:label]];
    }
#endif
}

mtl_command_queue_t metal_device_get_default_queue(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev) return NULL;
    return (__bridge void*)dev->command_queue;
#else
    return NULL;
#endif
}
