/*
 * mtl_swapchain.h
 * Metal swapchain (CAMetalLayer) integration
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_SWAPCHAIN_H
#define PLATFORM_MTL_SWAPCHAIN_H

#include "mtl_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

#ifdef __OBJC__
typedef struct metal_swapchain {
    CAMetalLayer* layer;
    id<CAMetalDrawable> current_drawable;
    MTLPixelFormat pixel_format;
    CGSize size;
    bool hdr_enabled;
} metal_swapchain_t;
#else
typedef struct metal_swapchain metal_swapchain_t;
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Creates a Metal swapchain backed by a CAMetalLayer for the given NSView.
 * @param dev The Metal device to use.
 * @param ns_view Pointer to the NSView (void* for C compatibility).
 * @param hdr If true, configures for HDR output (P3 color space, Float16 format).
 * @return A new swapchain instance or NULL on failure.
 */
metal_swapchain_t* metal_swapchain_create(metal_device_t* dev, void* ns_view, bool hdr);

/**
 * Destroys the swapchain.
 * @param swap The swapchain to destroy.
 */
void metal_swapchain_destroy(metal_swapchain_t* swap);

/**
 * Updates the swapchain size. Should be called when the window checks resize.
 * @param swap The swapchain to resize.
 * @param width New width in points (or pixels if layer scaling is set).
 * @param height New height.
 */
void metal_swapchain_resize(metal_swapchain_t* swap, uint32_t width, uint32_t height);

/**
 * Acquires the next drawable for rendering.
 * @param swap The swapchain.
 * @return The MTLTexture for the next frame (void* for C).
 */
#ifdef __OBJC__
id<MTLTexture> metal_swapchain_get_texture(metal_swapchain_t* swap);
#else
void* metal_swapchain_get_texture(metal_swapchain_t* swap);
#endif

/**
 * Presents the current drawable.
 * @param swap The swapchain.
 * @param cmd The command buffer that rendered to the drawable (to schedule presentation).
 */
#ifdef __OBJC__
void metal_swapchain_present(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd);
#else
void metal_swapchain_present(metal_swapchain_t* swap, void* cmd);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_SWAPCHAIN_H */
