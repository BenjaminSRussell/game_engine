/*
 * mtl_device.c
 * Metal device implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_device_caps.h"
#include "backend/metal/mtl_memory_heap.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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
static metal_device_t *g_device_instance = NULL;
static pthread_mutex_t g_device_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations of internal helpers from other units if not in headers
void metal_device_query_capabilities(id<MTLDevice> device,
                                     mtl_device_caps_t *out_caps);
extern void
metal_device_query_memory_info_internal(id<MTLDevice> device,
                                        mtl_memory_info_t *out_info);

#else
// Forward declaration for C compiler to avoid errors if compiled as C
// meaningful content is only visible to ObjC compiler
struct metal_device;
#endif

metal_device_t *metal_device_create_system_default(void) {
#if defined(__OBJC__)
  pthread_mutex_lock(&g_device_mutex);
  if (g_device_instance) {
    pthread_mutex_unlock(&g_device_mutex);
    return g_device_instance;
  }

  struct metal_device *dev = calloc(1, sizeof(struct metal_device));
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
  printf("  - GPU Family: %s\n",
         dev->caps.is_apple_silicon ? "Apple Silicon" : "Discrete/Other");

  g_device_instance = dev;
  pthread_mutex_unlock(&g_device_mutex);
  return dev;
#else
  return NULL;
#endif
}

metal_device_t *metal_device_get_default(void) {
#if defined(__OBJC__)
  return g_device_instance;
#else
  return NULL;
#endif
}

void *metal_get_device(metal_device_t *dev) {
#if defined(__OBJC__)
  if (!dev)
    return NULL;
  return (__bridge void *)dev->device;
#else
  return NULL;
#endif
}

void metal_device_destroy(metal_device_t *dev) {
#if defined(__OBJC__)
  if (!dev)
    return;

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

mtl_command_buffer_t metal_create_command_buffer(metal_device_t *dev) {
#if defined(__OBJC__)
  if (!dev || !dev->command_queue)
    return NULL;

  id<MTLCommandBuffer> buffer = [dev->command_queue commandBuffer];
  return (__bridge void *)
      buffer; // Return raw pointer, lifetime managed by autorelease initially
              // Caller might need to retain if keeping it across frames,
              // but typical command buffer usage is transient.
#else
  return NULL;
#endif
}

const mtl_device_caps_t *metal_device_get_caps(metal_device_t *dev) {
#if defined(__OBJC__)
  if (!dev)
    return NULL;
  return &dev->caps;
#else
  return NULL;
#endif
}

void metal_device_get_memory_info(metal_device_t *dev,
                                  mtl_memory_info_t *out_info) {
#if defined(__OBJC__)
  if (!dev || !out_info)
    return;
  // Refresh info
  metal_device_query_memory_info_internal(dev->device, &dev->memory_info);
  *out_info = dev->memory_info;
#endif
}

bool metal_device_supports_raytracing(metal_device_t *dev) {
#if defined(__OBJC__)
  if (!dev)
    return false;
  return dev->caps.supports_raytracing;
#else
  return false;
#endif
}
