/*
 * mtl_indirect_command.c
 * Metal indirect command buffer implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_indirect_command.h"
#include "backend/metal/mtl_device.h"

#if defined(__OBJC__)
#import <Metal/Metal.h>

#define TO_ICB(x) ((__bridge id<MTLIndirectCommandBuffer>)(x))

#endif

/* ============================================================================
 * INDIRECT COMMAND BUFFER IMPLEMENTATION
 * ============================================================================
 */

mtl_indirect_command_buffer_t metal_indirect_command_buffer_create(
    metal_device_t *device,
    mtl_indirect_command_buffer_descriptor_t *descriptor,
    unsigned long max_command_count) {
#if defined(__OBJC__)
  if (!device || !descriptor)
    return NULL;

  id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)metal_get_device(device);
  if (!mtl_device)
    return NULL;

  // Create MTLIndirectCommandBufferDescriptor
  MTLIndirectCommandBufferDescriptor *icb_desc =
      [[MTLIndirectCommandBufferDescriptor alloc] init];
  icb_desc.commandTypes = (MTLIndirectCommandType)descriptor->command_types;
  icb_desc.inheritBuffers = descriptor->inherit_buffers;
  icb_desc.inheritPipelineState = descriptor->inherit_pipeline_state;
  icb_desc.maxVertexBufferBindCount = descriptor->max_vertex_buffer_bind_count;
  icb_desc.maxFragmentBufferBindCount =
      descriptor->max_fragment_buffer_bind_count;

  if (@available(macOS 11.0, *)) {
    icb_desc.maxKernelBufferBindCount =
        descriptor->max_kernel_buffer_bind_count;
  }

  // Create the ICB
  id<MTLIndirectCommandBuffer> icb = [mtl_device
      newIndirectCommandBufferWithDescriptor:icb_desc
                             maxCommandCount:max_command_count
                                     options:MTLResourceStorageModePrivate];

  return (void *)icb;
#else
  return NULL;
#endif
}

void metal_indirect_command_buffer_destroy(mtl_indirect_command_buffer_t icb) {
#if defined(__OBJC__)
  if (icb) {
    CFRelease(icb);
  }
#endif
}

void metal_indirect_command_buffer_reset_range(
    mtl_indirect_command_buffer_t icb, unsigned long start_index,
    unsigned long count) {
#if defined(__OBJC__)
  if (icb) {
    NSRange range = NSMakeRange(start_index, count);
    [TO_ICB(icb) resetWithRange:range];
  }
#endif
}

unsigned long
metal_indirect_command_buffer_get_size(mtl_indirect_command_buffer_t icb) {
#if defined(__OBJC__)
  if (icb) {
    return [TO_ICB(icb) size];
  }
#endif
  return 0;
}

uint64_t metal_indirect_command_buffer_get_gpu_resource_id(
    mtl_indirect_command_buffer_t icb) {
#if defined(__OBJC__)
  if (icb) {
    if (@available(macOS 13.0, *)) {
      return [TO_ICB(icb) gpuResourceID]._impl;
    }
  }
#endif
  return 0;
}
