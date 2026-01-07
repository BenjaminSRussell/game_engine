/*
 * mtl_command_internal.h
 * Internal Metal command buffer structure definition
 * DO NOT include this in public headers
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MTL_COMMAND_INTERNAL_H
#define MTL_COMMAND_INTERNAL_H

#ifdef __OBJC__
#import <Metal/Metal.h>

// Internal command buffer wrapper for sync system
struct metal_command_buffer {
    id<MTLCommandBuffer> buffer;
};

#endif // __OBJC__

#endif // MTL_COMMAND_INTERNAL_H
