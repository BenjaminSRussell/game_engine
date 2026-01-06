/*
 * mtl_swapchain.c
 * Metal swapchain (CAMetalLayer) implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_swapchain.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef __OBJC__

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

/* 
 * Local definition of metal_device struct to access its internal MTLDevice.
 * This MUST match the layout in mtl_device.c
 */
struct metal_device {
    id<MTLDevice> device;              // __bridge retained
    id<MTLCommandQueue> command_queue; // __bridge retained
    MTLSize max_threads_per_group;
    uint64_t recommended_working_set_size;
    bool supports_raytracing;
    bool supports_mesh_shaders;
};

/* ============================================================================
 * API IMPLEMENTATION
 * ============================================================================ */

metal_swapchain_t* metal_swapchain_create(metal_device_t* dev, void* ns_view, bool hdr) {
    if (!dev || !ns_view) return NULL;
    
    metal_swapchain_t* swap = (metal_swapchain_t*)calloc(1, sizeof(metal_swapchain_t));
    if (!swap) return NULL;

    struct metal_device* internal_dev = (struct metal_device*)dev;
    id<MTLDevice> mtl_device = internal_dev->device;

    if (!mtl_device) {
        free(swap);
        return NULL;
    }

    // Configure Layer
    swap->layer = [CAMetalLayer layer];
    swap->layer.device = mtl_device;
    swap->layer.pixelFormat = hdr ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
    swap->layer.framebufferOnly = YES; // Set to NO if you need to read back or use as compute target
    swap->layer.displaySyncEnabled = YES; // V-Sync
    
    // HDR Configuration
    if (hdr) {
        if (@available(macOS 10.11, *)) {
            swap->layer.wantsExtendedDynamicRangeContent = YES;
            swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearDisplayP3);
        } else {
             // Fallback for older macOS if necessary (though engine likely targets 10.15+)
            swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        }
    } else {
        swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    }

    // Attach to NSView
    NSView* view = (__bridge NSView*)ns_view;
    [view setLayer:swap->layer];
    [view setWantsLayer:YES];

    // Initial size
    swap->size = view.bounds.size;
    swap->layer.drawableSize = [view convertSizeToBacking:swap->size];
    
    swap->hdr_enabled = hdr;
    swap->pixel_format = swap->layer.pixelFormat;
    
    printf("Metal Swapchain Created:\n");
    printf("  - Size: %.0f x %.0f\n", swap->size.width, swap->size.height);
    printf("  - HDR: %s\n", hdr ? "YES" : "NO");
    printf("  - Pixel Format: %lu\n", (unsigned long)swap->pixel_format);

    return swap;
}

void metal_swapchain_destroy(metal_swapchain_t* swap) {
    if (!swap) return;

    if (swap->layer) {
        swap->layer.device = nil;
        swap->layer = nil;
    }
    
    swap->current_drawable = nil;
    
    free(swap);
}

void metal_swapchain_resize(metal_swapchain_t* swap, uint32_t width, uint32_t height) {
    if (!swap || !swap->layer) return;

    // width/height passed here might be usually points or pixels depending on caller.
    // Assuming the engine handles window events and passes points, but for exact pixel control we set drawableSize.
    // If width/height are pixels (framebuffer size), we set drawableSize directly.
    
    CGSize new_size = CGSizeMake((CGFloat)width, (CGFloat)height);
    swap->layer.drawableSize = new_size;
    swap->size = new_size; // Store pixel size or point size? struct says just CGSize.
}

id<MTLTexture> metal_swapchain_get_texture(metal_swapchain_t* swap) {
    if (!swap || !swap->layer) return nil;

    swap->current_drawable = [swap->layer nextDrawable];
    if (!swap->current_drawable) {
        return nil;
    }
    
    return swap->current_drawable.texture;
}

void metal_swapchain_present(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd) {
    if (!swap || !swap->current_drawable || !cmd) return;

    [cmd presentDrawable:swap->current_drawable];
    // Release drawable reference after command buffer consumes it? 
    // ARC keeps it alive until command buffer is committed and scheduled.
    // We clear our strong reference to it for the next frame.
    swap->current_drawable = nil; 
}

#endif /* __OBJC__ */
