/*
 * mtl_device.c
 * Metal device implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_device.h"
#include <stdlib.h>
#include <stdio.h>

#if defined(__OBJC__)
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct metal_device {
    id<MTLDevice> device;              // __bridge retained
    id<MTLCommandQueue> command_queue; // __bridge retained
    MTLSize max_threads_per_group;
    uint64_t recommended_working_set_size;
    bool supports_raytracing;
    bool supports_mesh_shaders;
    // Additional features can be added here
};

#else
// Forward declaration for C compiler to avoid errors if compiled as C
// meaningful content is only visible to ObjC compiler
struct metal_device;
#endif

metal_device_t* metal_device_create(void) {
#if defined(__OBJC__)
    struct metal_device* dev = calloc(1, sizeof(struct metal_device));
    if (!dev) return NULL;

    dev->device = MTLCreateSystemDefaultDevice();
    if (!dev->device) {
        fprintf(stderr, "Error: Failed to create default Metal device.\n");
        free(dev);
        return NULL;
    }

    dev->command_queue = [dev->device newCommandQueue];
    if (!dev->command_queue) {
        fprintf(stderr, "Error: Failed to create Metal command queue.\n");
        // Release device if manual reference counting was used, but ARC handles it if compiled with ARC.
        // Assuming ARC is OFF for C interop struct members unless __unsafe_unretained or CFBridgingRetain used.
        // Standard idiomatic ObjC++ or ObjC in C projects often uses ARC. 
        // If ARC is ON, the struct members need __strong or similar. 
        // For pure C struct with ObjC objects, we usually use void* or __bridge casts.
        // However, since we defined the struct inside #if defined(__OBJC__), we can use id types.
        // If ARC is enabled, struct members holding objects are unsafe.
        // We will assume ARC is ENABLED for the project but for C-structs holding Obj-C objects 
        // we might need careful handling or generic pointers. 
        // Given the instructions: "id<MTLDevice> device; // __bridge retained", this implies Manual Retain Release logic or bridging.
        // Let's implement assuming ARC-compatible file, but using CFBridgingRetain to keep them in the C struct.
        // Or simply making the struct an ObjC class is better? No, instructions say "typedef struct metal_device".
        
        // Actually, let's stick to the prompt's suggested C-struct layout. 
        // Warning: storing id in a C struct is dangerous with ARC. 
        // We will standardly use void* in struct for C, but inside implementation use id with bridging.
        // Re-reading prompt: "typedef struct metal_device { id<MTLDevice> device; ... } metal_device_t;" 
        // This is valid in Obj-C++ or Obj-C.
        
        // However, to keep it simple and compile with C-linkage, we'll assume the compiler handles it or
        // we manually retain if ARC is off. For now, assuming standard retaining.
        
        // No explicit release needed here if we error out and free(dev) immediately? 
        // Actually, if we assigned `dev->device`, we own it.
        // Correct way with ARC: dev->device = ...; 
        // If we free(dev), the object is lost (leak).
        // So we should nil it out or release it.
        
        dev->device = nil;
        free(dev);
        return NULL;
    }

    // Capability queries
    if (@available(macOS 10.15, *)) {
        dev->supports_raytracing = [dev->device supportsRaytracing];
    } else {
        dev->supports_raytracing = false;
    }
    
    // Feature queries
    if (@available(macOS 11.0, *)) {
        // Example feature check
        dev->supports_mesh_shaders = [dev->device supportsFamily:MTLGPUFamilyApple7]; 
    } else {
        dev->supports_mesh_shaders = false;
    }

    dev->max_threads_per_group = [dev->device maxThreadsPerThreadgroup];
    
    // Recommended working set size
    if (@available(macOS 10.12, *)) {
        dev->recommended_working_set_size = [dev->device recommendedMaxWorkingSetSize];
    } else {
        dev->recommended_working_set_size = 0; // Unknown
    }

    printf("Metal Device Initialized: %s\n", [[dev->device name] UTF8String]);
    printf("  - Raytracing: %s\n", dev->supports_raytracing ? "YES" : "NO");
    printf("  - Max Threads/Group: %lu x %lu x %lu\n", 
           dev->max_threads_per_group.width,
           dev->max_threads_per_group.height,
           dev->max_threads_per_group.depth);

    return dev;
#else
    return NULL;
#endif
}

void metal_device_destroy(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev) return;
    
    // With generic C struct in ObjC file, ARC might not automatically release fields if not strictly __strong.
    // If we rely on default, id is __strong.
    // So setting to nil releases.
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

uint32_t metal_device_get_max_threads_per_group(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev) return 0;
    // Returning just the width or a simplified scalar for the C interface?
    // The C stub in `mtl_device_create` usage implies returning complex data might be needed,
    // but the getter signature returns uint32_t. We'll return width or total.
    // Let's assume width is the most relevant for 1D launch checks or similar.
    return (uint32_t)dev->max_threads_per_group.width;
#else
    return 0;
#endif
}

bool metal_device_supports_raytracing(metal_device_t* dev) {
#if defined(__OBJC__)
    if (!dev) return false;
    return dev->supports_raytracing;
#else
    return false;
#endif
}
