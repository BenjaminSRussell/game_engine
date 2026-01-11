#include <metal/metal.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for mesh_gpu.c
bool metal_create_vertex_buffer(struct metal_device* device, const void* data, u32 size, struct metal_buffer** out_buffer);
bool metal_create_index_buffer(struct metal_device* device, const void* data, u32 size, struct metal_buffer** out_buffer);
void metal_destroy_buffer(struct metal_buffer* buffer);
bool metal_update_buffer(struct metal_buffer* buffer, const void* data, u32 size, u32 offset);

// Simple Metal buffer wrapper
typedef struct metal_buffer {
    id<MTLBuffer> mtl_buffer;
    u32 size;
} metal_buffer_t;

// Simple Metal device wrapper  
typedef struct metal_device {
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
} metal_device_t;

bool metal_create_vertex_buffer(struct metal_device* device, const void* data, u32 size, struct metal_buffer** out_buffer) {
    if (!device || !data || size == 0 || !out_buffer) {
        return false;
    }
    
    // Create Metal buffer
    id<MTLBuffer> mtl_buffer = [device->device newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
    if (!mtl_buffer) {
        printf("Failed to create Metal vertex buffer\n");
        return false;
    }
    
    // Create wrapper
    metal_buffer_t* buffer = (metal_buffer_t*)malloc(sizeof(metal_buffer_t));
    if (!buffer) {
        [mtl_buffer release];
        return false;
    }
    
    buffer->mtl_buffer = mtl_buffer;
    buffer->size = size;
    
    *out_buffer = (struct metal_buffer*)buffer;
    printf("Created Metal vertex buffer: %u bytes\n", size);
    return true;
}

bool metal_create_index_buffer(struct metal_device* device, const void* data, u32 size, struct metal_buffer** out_buffer) {
    if (!device || !data || size == 0 || !out_buffer) {
        return false;
    }
    
    // Create Metal buffer
    id<MTLBuffer> mtl_buffer = [device->device newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
    if (!mtl_buffer) {
        printf("Failed to create Metal index buffer\n");
        return false;
    }
    
    // Create wrapper
    metal_buffer_t* buffer = (metal_buffer_t*)malloc(sizeof(metal_buffer_t));
    if (!buffer) {
        [mtl_buffer release];
        return false;
    }
    
    buffer->mtl_buffer = mtl_buffer;
    buffer->size = size;
    
    *out_buffer = (struct metal_buffer*)buffer;
    printf("Created Metal index buffer: %u bytes\n", size);
    return true;
}

void metal_destroy_buffer(struct metal_buffer* buffer) {
    if (!buffer) {
        return;
    }
    
    metal_buffer_t* mtl_buffer = (metal_buffer_t*)buffer;
    if (mtl_buffer->mtl_buffer) {
        [mtl_buffer->mtl_buffer release];
    }
    
    free(mtl_buffer);
    printf("Destroyed Metal buffer\n");
}

bool metal_update_buffer(struct metal_buffer* buffer, const void* data, u32 size, u32 offset) {
    if (!buffer || !data || size == 0) {
        return false;
    }
    
    metal_buffer_t* mtl_buffer = (metal_buffer_t*)buffer;
    if (!mtl_buffer->mtl_buffer) {
        return false;
    }
    
    // Check bounds
    if (offset + size > mtl_buffer->size) {
        printf("Buffer update out of bounds\n");
        return false;
    }
    
    // Update buffer contents
    void* ptr = (char*)[mtl_buffer->mtl_buffer contents] + offset;
    memcpy(ptr, data, size);
    
    printf("Updated Metal buffer: offset=%u, size=%u\n", offset, size);
    return true;
}

// Helper function to create a default Metal device
struct metal_device* metal_create_default_device() {
    metal_device_t* device = (metal_device_t*)malloc(sizeof(metal_device_t));
    if (!device) {
        return NULL;
    }
    
    device->device = MTLCreateSystemDefaultDevice();
    if (!device->device) {
        free(device);
        return NULL;
    }
    
    device->command_queue = [device->device newCommandQueue];
    if (!device->command_queue) {
        [device->device release];
        free(device);
        return NULL;
    }
    
    printf("Created Metal device and command queue\n");
    return (struct metal_device*)device;
}

void metal_destroy_device(struct metal_device* device) {
    if (!device) {
        return;
    }
    
    metal_device_t* mtl_device = (metal_device_t*)device;
    if (mtl_device->command_queue) {
        [mtl_device->command_queue release];
    }
    if (mtl_device->device) {
        [mtl_device->device release];
    }
    
    free(mtl_device);
    printf("Destroyed Metal device\n");
}
