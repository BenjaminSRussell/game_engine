/*
 * mtl_sync_primitives.c
 * Metal synchronization primitives implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_sync_primitives.h"

#if defined(__OBJC__)
#import <Metal/Metal.h>

#define TO_FENCE(x) ((__bridge id<MTLFence>)(x))
#define TO_EVENT(x) ((__bridge id<MTLEvent>)(x))
#define TO_RENDER_ENC(x) ((__bridge id<MTLRenderCommandEncoder>)(x))
#define TO_COMPUTE_ENC(x) ((__bridge id<MTLComputeCommandEncoder>)(x))
#define TO_BLIT_ENC(x) ((__bridge id<MTLBlitCommandEncoder>)(x))

#endif

/* ============================================================================
 * FENCE IMPLEMENTATION
 * ============================================================================ */

mtl_fence_t metal_fence_create(metal_device_t* device) {
#if defined(__OBJC__)
    if (!device) return NULL;
    
    id<MTLDevice> mtl_device = metal_get_device(device);
    if (!mtl_device) return NULL;
    
    id<MTLFence> fence = [mtl_device newFence];
    return (__bridge_retained void*)fence;
#else
    return NULL;
#endif
}

void metal_fence_destroy(mtl_fence_t fence) {
#if defined(__OBJC__)
    if (fence) {
        CFRelease(fence);
    }
#endif
}

/* Render Encoder Fence Operations */

void metal_render_encoder_update_fence(mtl_render_command_encoder_t encoder, mtl_fence_t fence) {
#if defined(__OBJC__)
    if (encoder && fence) {
        [TO_RENDER_ENC(encoder) updateFence:TO_FENCE(fence) afterStages:MTLRenderStageFragment];
    }
#endif
}

void metal_render_encoder_wait_for_fence(mtl_render_command_encoder_t encoder, mtl_fence_t fence) {
#if defined(__OBJC__)
    if (encoder && fence) {
        [TO_RENDER_ENC(encoder) waitForFence:TO_FENCE(fence) beforeStages:MTLRenderStageVertex];
    }
#endif
}

/* Compute Encoder Fence Operations */

void metal_compute_encoder_update_fence(mtl_compute_command_encoder_t encoder, mtl_fence_t fence) {
#if defined(__OBJC__)
    if (encoder && fence) {
        [TO_COMPUTE_ENC(encoder) updateFence:TO_FENCE(fence)];
    }
#endif
}

void metal_compute_encoder_wait_for_fence(mtl_compute_command_encoder_t encoder, mtl_fence_t fence) {
#if defined(__OBJC__)
    if (encoder && fence) {
        [TO_COMPUTE_ENC(encoder) waitForFence:TO_FENCE(fence)];
    }
#endif
}

/* Blit Encoder Fence Operations */

void metal_blit_encoder_update_fence(mtl_blit_command_encoder_t encoder, mtl_fence_t fence) {
#if defined(__OBJC__)
    if (encoder && fence) {
        [TO_BLIT_ENC(encoder) updateFence:TO_FENCE(fence)];
    }
#endif
}

void metal_blit_encoder_wait_for_fence(mtl_blit_command_encoder_t encoder, mtl_fence_t fence) {
#if defined(__OBJC__)
    if (encoder && fence) {
        [TO_BLIT_ENC(encoder) waitForFence:TO_FENCE(fence)];
    }
#endif
}

/* ============================================================================
 * EVENT IMPLEMENTATION
 * ============================================================================ */

mtl_event_t metal_event_create(metal_device_t* device) {
#if defined(__OBJC__)
    if (!device) return NULL;
    
    id<MTLDevice> mtl_device = metal_get_device(device);
    if (!mtl_device) return NULL;
    
    id<MTLEvent> event = [mtl_device newEvent];
    return (__bridge_retained void*)event;
#else
    return NULL;
#endif
}

void metal_event_destroy(mtl_event_t event) {
#if defined(__OBJC__)
    if (event) {
        CFRelease(event);
    }
#endif
}

/* Render Encoder Event Operations */

void metal_render_encoder_signal_event(mtl_render_command_encoder_t encoder, mtl_event_t event, uint64_t value) {
#if defined(__OBJC__)
    if (encoder && event) {
        [TO_RENDER_ENC(encoder) signalEvent:TO_EVENT(event) value:value];
    }
#endif
}

void metal_render_encoder_wait_for_event(mtl_render_command_encoder_t encoder, mtl_event_t event, uint64_t value) {
#if defined(__OBJC__)
    if (encoder && event) {
        [TO_RENDER_ENC(encoder) waitForEvent:TO_EVENT(event) value:value];
    }
#endif
}

/* Compute Encoder Event Operations */

void metal_compute_encoder_signal_event(mtl_compute_command_encoder_t encoder, mtl_event_t event, uint64_t value) {
#if defined(__OBJC__)
    if (encoder && event) {
        [TO_COMPUTE_ENC(encoder) signalEvent:TO_EVENT(event) value:value];
    }
#endif
}

void metal_compute_encoder_wait_for_event(mtl_compute_command_encoder_t encoder, mtl_event_t event, uint64_t value) {
#if defined(__OBJC__)
    if (encoder && event) {
        [TO_COMPUTE_ENC(encoder) waitForEvent:TO_EVENT(event) value:value];
    }
#endif
}

/* Blit Encoder Event Operations */

void metal_blit_encoder_signal_event(mtl_blit_command_encoder_t encoder, mtl_event_t event, uint64_t value) {
#if defined(__OBJC__)
    if (encoder && event) {
        [TO_BLIT_ENC(encoder) signalEvent:TO_EVENT(event) value:value];
    }
#endif
}

void metal_blit_encoder_wait_for_event(mtl_blit_command_encoder_t encoder, mtl_event_t event, uint64_t value) {
#if defined(__OBJC__)
    if (encoder && event) {
        [TO_BLIT_ENC(encoder) waitForEvent:TO_EVENT(event) value:value];
    }
#endif
}

/* Event Value Query */

uint64_t metal_event_get_signaled_value(mtl_event_t event) {
#if defined(__OBJC__)
    if (event) {
        return [TO_EVENT(event) signaledValue];
    }
#endif
    return 0;
}
