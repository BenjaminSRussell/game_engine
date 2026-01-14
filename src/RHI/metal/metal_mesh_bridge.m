#include <metal/metal.h>
#include <stdio.h>
#include <stdlib.h>
#include <include/core/types.h>

// Forward declarations for mesh_gpu.c
bool metal_create_vertex_buffer_wrapper(struct metal_device *device, const void *data,
                                u32 size, struct metal_buffer **out_buffer);
bool metal_create_index_buffer_wrapper(struct metal_device *device, const void *data,
                               u32 size, struct metal_buffer **out_buffer);
void metal_destroy_buffer_wrapper(struct metal_buffer *buffer);
bool metal_update_buffer_wrapper(struct metal_buffer *buffer, const void *data,
                         u32 size, u32 offset);

// Simple Metal buffer wrapper
// Simple Metal buffer wrapper
typedef struct metal_mesh_buffer {
  id<MTLBuffer> mtl_buffer;
  uint32_t size;
} metal_mesh_buffer_t;

// Simple Metal device wrapper
typedef struct metal_mesh_device {
  id<MTLDevice> device;
  id<MTLCommandQueue> command_queue;
} metal_mesh_device_t;

bool metal_create_vertex_buffer(struct metal_device *device, const void *data,
                                uint32_t size,
                                struct metal_buffer **out_buffer) {
  if (!device || !data || size == 0 || !out_buffer) {
    return false;
  }

  metal_mesh_device_t *mtl_dev = (metal_mesh_device_t *)device;

  // Create Metal buffer
  id<MTLBuffer> mtl_buffer =
      [mtl_dev->device newBufferWithBytes:data
                                   length:size
                                  options:MTLResourceStorageModeShared];
  if (!mtl_buffer) {
    printf("Failed to create Metal vertex buffer\n");
    return false;
  }

  // Create wrapper
  metal_mesh_buffer_t *buffer =
      (metal_mesh_buffer_t *)malloc(sizeof(metal_mesh_buffer_t));
  if (!buffer) {
    // [mtl_buffer release]; // ARC will handle if we don't store it
    mtl_buffer = nil;
    return false;
  }

  buffer->mtl_buffer = mtl_buffer;
  buffer->size = size;

  *out_buffer = (struct metal_buffer *)buffer;
  printf("Created Metal vertex buffer: %u bytes\n", size);
  return true;
}

bool metal_create_index_buffer(struct metal_device *device, const void *data,
                               uint32_t size,
                               struct metal_buffer **out_buffer) {
  if (!device || !data || size == 0 || !out_buffer) {
    return false;
  }

  metal_mesh_device_t *mtl_dev = (metal_mesh_device_t *)device;

  // Create Metal buffer
  id<MTLBuffer> mtl_buffer =
      [mtl_dev->device newBufferWithBytes:data
                                   length:size
                                  options:MTLResourceStorageModeShared];
  if (!mtl_buffer) {
    printf("Failed to create Metal index buffer\n");
    return false;
  }

  // Create wrapper
  metal_mesh_buffer_t *buffer =
      (metal_mesh_buffer_t *)malloc(sizeof(metal_mesh_buffer_t));
  if (!buffer) {
    // [mtl_buffer release]; // ARC handles
    mtl_buffer = nil;
    return false;
  }

  buffer->mtl_buffer = mtl_buffer;
  buffer->size = size;

  *out_buffer = (struct metal_buffer *)buffer;
  printf("Created Metal index buffer: %u bytes\n", size);
  return true;
}

void metal_destroy_buffer(struct metal_buffer *buffer) {
  if (!buffer) {
    return;
  }

  metal_mesh_buffer_t *mtl_buf = (metal_mesh_buffer_t *)buffer;
  if (mtl_buf->mtl_buffer) {
    // [mtl_buf->mtl_buffer release]; // ARC handles
    mtl_buf->mtl_buffer = nil;
  }

  free(mtl_buf);
  printf("Destroyed Metal buffer\n");
}

bool metal_update_buffer(struct metal_buffer *buffer, const void *data,
                         uint32_t size, uint32_t offset) {
  if (!buffer || !data || size == 0) {
    return false;
  }

  metal_mesh_buffer_t *mtl_buf = (metal_mesh_buffer_t *)buffer;
  if (!mtl_buf->mtl_buffer) {
    return false;
  }

  // Check bounds
  if (offset + size > mtl_buf->size) {
    printf("Buffer update out of bounds\n");
    return false;
  }

  // Update buffer contents
  void *ptr = (char *)[mtl_buf->mtl_buffer contents] + offset;
  memcpy(ptr, data, size);

  printf("Updated Metal buffer: offset=%u, size=%u\n", offset, size);
  return true;
}

// Helper function to create a default Metal device
struct metal_device *metal_create_default_device() {
  metal_mesh_device_t *device =
      (metal_mesh_device_t *)malloc(sizeof(metal_mesh_device_t));
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
    // [device->device release]; // ARC handles
    device->device = nil;
    free(device);
    return NULL;
  }

  printf("Created Metal device and command queue\n");
  return (struct metal_device *)device;
}

void metal_destroy_device(struct metal_device *device) {
  if (!device) {
    return;
  }

  metal_mesh_device_t *mtl_device = (metal_mesh_device_t *)device;
  if (mtl_device->command_queue) {
    // [mtl_device->command_queue release]; // ARC handles
    mtl_device->command_queue = nil;
  }
  if (mtl_device->device) {
    // [mtl_device->device release]; // ARC handles
    mtl_device->device = nil;
  }

  free(mtl_device);
  printf("Destroyed Metal device\n");
}
