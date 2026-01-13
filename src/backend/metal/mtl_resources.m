/*
 * mtl_resources.m
 * Metal resource helper implementations
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_resources.h"

#ifdef __OBJC__
#import <Metal/Metal.h>

size_t metal_pixel_format_bytes_per_pixel(MTLPixelFormat format) {
  switch (format) {
  case MTLPixelFormatRGBA8Unorm:
  case MTLPixelFormatRGBA8Unorm_sRGB:
  case MTLPixelFormatBGRA8Unorm:
  case MTLPixelFormatBGRA8Unorm_sRGB:
    return 4;
  case MTLPixelFormatR8Unorm:
    return 1;
  case MTLPixelFormatRG8Unorm:
    return 2;
  case MTLPixelFormatRGBA16Float:
    return 8;
  case MTLPixelFormatRGBA32Float:
    return 16;
  case MTLPixelFormatDepth32Float:
    return 4;
  case MTLPixelFormatStencil8:
    return 1;
  case MTLPixelFormatDepth32Float_Stencil8:
    return 5; // 4 + 1
  default:
    return 4; // Default assumption or 0
  }
}

#else

size_t metal_pixel_format_bytes_per_pixel(uint64_t format) {
  // Basic fallback for C-only compilation (unlikely used in practice for this
  // function)
  return 4;
}

#endif
