/*
 * mtl_render_pass.c
 * Metal render pass optimization implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_render_pass.h"
#include <stdlib.h>


#if defined(__OBJC__)
#import <Metal/Metal.h>

#define TO_PASS_DESC(x) ((__bridge MTLRenderPassDescriptor*)(x))
#define TO_TEXTURE(x) ((__bridge id<MTLTexture>)(x))

#endif

/* ============================================================================
 * RENDER PASS DESCRIPTOR
 * ============================================================================ */

mtl_render_pass_descriptor_t metal_render_pass_descriptor_create(void) {
#if defined(__OBJC__)
    MTLRenderPassDescriptor* descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    return (__bridge_retained void*)descriptor;
#else
    return NULL;
#endif
}

void metal_render_pass_descriptor_destroy(mtl_render_pass_descriptor_t descriptor) {
#if defined(__OBJC__)
    if (descriptor) {
        CFRelease(descriptor);
    }
#endif
}

/* ============================================================================
 * COLOR ATTACHMENT CONFIGURATION
 * ============================================================================ */

void metal_render_pass_set_color_attachment_texture(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, void* texture) {
#if defined(__OBJC__)
    if (descriptor && attachment_index < 8) {
        TO_PASS_DESC(descriptor).colorAttachments[attachment_index].texture = TO_TEXTURE(texture);
    }
#endif
}

void metal_render_pass_set_color_attachment_load_action(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, mtl_load_action_t load_action) {
#if defined(__OBJC__)
    if (descriptor && attachment_index < 8) {
        TO_PASS_DESC(descriptor).colorAttachments[attachment_index].loadAction = (MTLLoadAction)load_action;
    }
#endif
}

void metal_render_pass_set_color_attachment_store_action(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, mtl_store_action_t store_action) {
#if defined(__OBJC__)
    if (descriptor && attachment_index < 8) {
        TO_PASS_DESC(descriptor).colorAttachments[attachment_index].storeAction = (MTLStoreAction)store_action;
    }
#endif
}

void metal_render_pass_set_color_attachment_clear_color(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, mtl_clear_color_t clear_color) {
#if defined(__OBJC__)
    if (descriptor && attachment_index < 8) {
        MTLClearColor mtl_color = MTLClearColorMake(clear_color.red, clear_color.green, clear_color.blue, clear_color.alpha);
        TO_PASS_DESC(descriptor).colorAttachments[attachment_index].clearColor = mtl_color;
    }
#endif
}

void metal_render_pass_set_color_attachment_store_action_options(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, mtl_store_action_options_t options) {
#if defined(__OBJC__)
    if (descriptor && attachment_index < 8) {
        TO_PASS_DESC(descriptor).colorAttachments[attachment_index].storeActionOptions = (MTLStoreActionOptions)options;
    }
#endif
}

/* ============================================================================
 * DEPTH/STENCIL ATTACHMENT CONFIGURATION
 * ============================================================================ */

void metal_render_pass_set_depth_attachment_texture(mtl_render_pass_descriptor_t descriptor, void* texture) {
#if defined(__OBJC__)
    if (descriptor) {
        TO_PASS_DESC(descriptor).depthAttachment.texture = TO_TEXTURE(texture);
    }
#endif
}

void metal_render_pass_set_depth_attachment_load_action(mtl_render_pass_descriptor_t descriptor, mtl_load_action_t load_action) {
#if defined(__OBJC__)
    if (descriptor) {
        TO_PASS_DESC(descriptor).depthAttachment.loadAction = (MTLLoadAction)load_action;
    }
#endif
}

void metal_render_pass_set_depth_attachment_store_action(mtl_render_pass_descriptor_t descriptor, mtl_store_action_t store_action) {
#if defined(__OBJC__)
    if (descriptor) {
        TO_PASS_DESC(descriptor).depthAttachment.storeAction = (MTLStoreAction)store_action;
    }
#endif
}

void metal_render_pass_set_depth_attachment_clear_depth(mtl_render_pass_descriptor_t descriptor, double clear_depth) {
#if defined(__OBJC__)
    if (descriptor) {
        TO_PASS_DESC(descriptor).depthAttachment.clearDepth = clear_depth;
    }
#endif
}

void metal_render_pass_set_stencil_attachment_texture(mtl_render_pass_descriptor_t descriptor, void* texture) {
#if defined(__OBJC__)
    if (descriptor) {
        TO_PASS_DESC(descriptor).stencilAttachment.texture = TO_TEXTURE(texture);
    }
#endif
}

void metal_render_pass_set_stencil_attachment_load_action(mtl_render_pass_descriptor_t descriptor, mtl_load_action_t load_action) {
#if defined(__OBJC__)
    if (descriptor) {
        TO_PASS_DESC(descriptor).stencilAttachment.loadAction = (MTLLoadAction)load_action;
    }
#endif
}

void metal_render_pass_set_stencil_attachment_store_action(mtl_render_pass_descriptor_t descriptor, mtl_store_action_t store_action) {
#if defined(__OBJC__)
    if (descriptor) {
        TO_PASS_DESC(descriptor).stencilAttachment.storeAction = (MTLStoreAction)store_action;
    }
#endif
}

void metal_render_pass_set_stencil_attachment_clear_stencil(mtl_render_pass_descriptor_t descriptor, uint32_t clear_stencil) {
#if defined(__OBJC__)
    if (descriptor) {
        TO_PASS_DESC(descriptor).stencilAttachment.clearStencil = clear_stencil;
    }
#endif
}

/* ============================================================================
 * MULTI-VIEW RENDERING
 * ============================================================================ */

void metal_render_pass_set_render_target_array_length(mtl_render_pass_descriptor_t descriptor, unsigned long length) {
#if defined(__OBJC__)
    if (descriptor) {
        if (@available(macOS 10.13, iOS 12.0, *)) {
            TO_PASS_DESC(descriptor).renderTargetArrayLength = length;
        }
    }
#endif
}
