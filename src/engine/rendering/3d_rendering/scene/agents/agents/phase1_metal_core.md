# Phase 1: Metal Core Infrastructure (macOS Native)

## Overview
This phase establishes the Metal-native rendering backend for macOS. All agents focus on Metal API only - no Vulkan/D3D12.

---

## Agent 1.1: Metal Device & Context

```
TASK: Implement Metal Device and GPU Context (Phase 1, Agent 1)

You are implementing the Metal device initialization for a macOS-native 3D rendering engine in C/Objective-C.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/platform/metal/

FILES TO MODIFY:
- mtl_device.c/h - REWRITE with actual Metal API calls
- mtl_command.c/h - REWRITE with actual Metal command encoding

WHAT EXISTS: Placeholder stubs with wrong TODOs (mentions Vulkan/D3D12 - remove those)

WHAT TO IMPLEMENT:
1. MTLDevice acquisition via MTLCreateSystemDefaultDevice()
2. MTLCommandQueue creation and management
3. Device capability queries (family, features, limits)
4. Memory heap enumeration (shared, private, memoryless)
5. Proper Objective-C bridging for C interface

METAL-SPECIFIC REQUIREMENTS:
```objc
// In implementation file (.m or use __bridge)
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

typedef struct metal_device {
    id<MTLDevice> device;              // __bridge retained
    id<MTLCommandQueue> command_queue;
    MTLSize max_threads_per_group;
    uint64_t recommended_working_set_size;
    bool supports_raytracing;
    bool supports_mesh_shaders;
} metal_device_t;

// C interface
metal_device_t* metal_device_create(void);
void metal_device_destroy(metal_device_t* dev);
id<MTLCommandBuffer> metal_create_command_buffer(metal_device_t* dev);
```

REMOVE ALL REFERENCES TO:
- Vulkan (VkDevice, vkCreateDevice, etc.)
- D3D12 (ID3D12Device, etc.)
- Cross-platform abstraction layers

OUTPUT: Working Metal device initialization that can be called from C code.
```

---

## Agent 1.2: Metal Command Buffers & Encoding

```
TASK: Implement Metal Command Buffer System (Phase 1, Agent 2)

You are implementing Metal command buffer recording for a macOS-native rendering engine.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/platform/metal/

FILES TO MODIFY:
- mtl_command.c/h - Metal command buffer and encoder management

WHAT TO IMPLEMENT:
1. Command buffer creation from command queue
2. Render command encoder (MTLRenderCommandEncoder)
3. Compute command encoder (MTLComputeCommandEncoder)
4. Blit command encoder (MTLBlitCommandEncoder)
5. Parallel render command encoder for multi-threaded recording
6. Command buffer completion handlers

METAL PATTERNS:
```objc
typedef struct metal_command_buffer {
    id<MTLCommandBuffer> buffer;
    id<MTLRenderCommandEncoder> render_encoder;
    id<MTLComputeCommandEncoder> compute_encoder;
    id<MTLBlitCommandEncoder> blit_encoder;
    bool encoding_render;
    bool encoding_compute;
} metal_command_buffer_t;

// Begin render pass
void metal_begin_render_pass(metal_command_buffer_t* cmd, MTLRenderPassDescriptor* desc) {
    if (cmd->render_encoder) {
        [cmd->render_encoder endEncoding];
    }
    cmd->render_encoder = [cmd->buffer renderCommandEncoderWithDescriptor:desc];
    cmd->encoding_render = true;
}

// Draw calls
void metal_draw(metal_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count) {
    [cmd->render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                            vertexStart:0
                            vertexCount:vertex_count
                          instanceCount:instance_count];
}

// Submit
void metal_submit(metal_command_buffer_t* cmd) {
    if (cmd->render_encoder) [cmd->render_encoder endEncoding];
    if (cmd->compute_encoder) [cmd->compute_encoder endEncoding];
    [cmd->buffer commit];
}
```

OUTPUT: Complete command buffer system ready for rendering.
```

---

## Agent 1.3: Metal Buffers & Textures

```
TASK: Implement Metal Buffer and Texture Management (Phase 1, Agent 3)

You are implementing GPU resource management using Metal API.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/platform/metal/

FILES TO MODIFY:
- mtl_buffer.c/h - Metal buffer creation and management
- mtl_texture.c/h - Metal texture creation and management

WHAT TO IMPLEMENT:
1. Buffer creation with storage modes (shared, private, memoryless)
2. Buffer sub-allocation from larger buffers
3. Ring buffer for per-frame data
4. Texture creation (2D, 3D, Cube, Array)
5. Texture views and pixel format conversion
6. Automatic mipmap generation
7. Texture compression format support (ASTC, BC)

METAL PATTERNS:
```objc
typedef struct metal_buffer {
    id<MTLBuffer> buffer;
    MTLResourceOptions options;
    size_t size;
    void* mapped_ptr;  // For shared storage mode
} metal_buffer_t;

metal_buffer_t* metal_buffer_create(metal_device_t* dev, size_t size, MTLResourceOptions options) {
    metal_buffer_t* buf = calloc(1, sizeof(metal_buffer_t));
    buf->buffer = [dev->device newBufferWithLength:size options:options];
    buf->size = size;
    buf->options = options;

    if (options & MTLResourceStorageModeShared) {
        buf->mapped_ptr = [buf->buffer contents];
    }
    return buf;
}

// Texture creation
typedef struct metal_texture {
    id<MTLTexture> texture;
    MTLPixelFormat format;
    uint32_t width, height, depth;
    uint32_t mip_levels;
    MTLTextureType type;
} metal_texture_t;

metal_texture_t* metal_texture_create_2d(metal_device_t* dev, uint32_t w, uint32_t h,
                                          MTLPixelFormat format, MTLTextureUsage usage) {
    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
                                                                                    width:w
                                                                                   height:h
                                                                                mipmapped:YES];
    desc.usage = usage;
    desc.storageMode = MTLStorageModePrivate;

    metal_texture_t* tex = calloc(1, sizeof(metal_texture_t));
    tex->texture = [dev->device newTextureWithDescriptor:desc];
    tex->format = format;
    tex->width = w;
    tex->height = h;
    return tex;
}
```

OUTPUT: Buffer and texture system ready for mesh/material data.
```

---

## Agent 1.4: Metal Pipeline State

```
TASK: Implement Metal Pipeline State Objects (Phase 1, Agent 4)

You are implementing render and compute pipeline state management.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/platform/metal/

FILES TO MODIFY:
- mtl_pipeline.c/h - Pipeline state creation and caching

WHAT TO IMPLEMENT:
1. Render pipeline state creation (MTLRenderPipelineState)
2. Compute pipeline state creation (MTLComputePipelineState)
3. Depth/stencil state (MTLDepthStencilState)
4. Shader library loading from .metallib files
5. Pipeline state caching by hash
6. Vertex descriptor configuration

METAL PATTERNS:
```objc
typedef struct metal_render_pipeline {
    id<MTLRenderPipelineState> state;
    id<MTLDepthStencilState> depth_stencil;
    MTLCullMode cull_mode;
    MTLTriangleFillMode fill_mode;
} metal_render_pipeline_t;

metal_render_pipeline_t* metal_create_render_pipeline(
    metal_device_t* dev,
    id<MTLFunction> vertex_fn,
    id<MTLFunction> fragment_fn,
    MTLPixelFormat color_format,
    MTLPixelFormat depth_format,
    MTLVertexDescriptor* vertex_desc
) {
    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vertex_fn;
    desc.fragmentFunction = fragment_fn;
    desc.colorAttachments[0].pixelFormat = color_format;
    desc.depthAttachmentPixelFormat = depth_format;
    desc.vertexDescriptor = vertex_desc;

    NSError* error = nil;
    id<MTLRenderPipelineState> state = [dev->device newRenderPipelineStateWithDescriptor:desc error:&error];

    if (error) {
        NSLog(@"Pipeline creation failed: %@", error);
        return NULL;
    }

    metal_render_pipeline_t* pipeline = calloc(1, sizeof(metal_render_pipeline_t));
    pipeline->state = state;
    return pipeline;
}
```

OUTPUT: Pipeline state system ready for shader binding.
```

---

## Agent 1.5: Metal Synchronization

```
TASK: Implement Metal Synchronization Primitives (Phase 1, Agent 5)

You are implementing GPU synchronization for Metal.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/platform/metal/

FILES TO CREATE:
- mtl_sync.c/h - Fences, events, semaphores

WHAT TO IMPLEMENT:
1. MTLFence for GPU-GPU synchronization
2. MTLEvent for timeline synchronization
3. MTLSharedEvent for CPU-GPU synchronization
4. Triple buffering frame management
5. Resource hazard tracking

METAL PATTERNS:
```objc
typedef struct metal_frame_sync {
    dispatch_semaphore_t frame_semaphore;
    id<MTLSharedEvent> shared_event;
    uint64_t frame_index;
    uint32_t max_frames_in_flight;
} metal_frame_sync_t;

metal_frame_sync_t* metal_frame_sync_create(metal_device_t* dev, uint32_t max_in_flight) {
    metal_frame_sync_t* sync = calloc(1, sizeof(metal_frame_sync_t));
    sync->frame_semaphore = dispatch_semaphore_create(max_in_flight);
    sync->shared_event = [dev->device newSharedEvent];
    sync->max_frames_in_flight = max_in_flight;
    return sync;
}

void metal_frame_begin(metal_frame_sync_t* sync) {
    dispatch_semaphore_wait(sync->frame_semaphore, DISPATCH_TIME_FOREVER);
}

void metal_frame_end(metal_frame_sync_t* sync, id<MTLCommandBuffer> cmd) {
    uint64_t signal_value = ++sync->frame_index;
    [cmd encodeSignalEvent:sync->shared_event value:signal_value];

    __block dispatch_semaphore_t sem = sync->frame_semaphore;
    [cmd addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(sem);
    }];
}
```

OUTPUT: Frame synchronization ready for rendering loop.
```

---

## Agent 1.6: Metal Swapchain & Presentation

```
TASK: Implement Metal Swapchain and Presentation (Phase 1, Agent 6)

You are implementing the display output path for Metal.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/platform/metal/

FILES TO CREATE:
- mtl_swapchain.c/h - CAMetalLayer integration

WHAT TO IMPLEMENT:
1. CAMetalLayer configuration
2. Drawable acquisition (nextDrawable)
3. Present scheduling
4. HDR output (EDR, P3 color space)
5. Variable refresh rate (ProMotion)
6. Resize handling

METAL PATTERNS:
```objc
#import <QuartzCore/CAMetalLayer.h>

typedef struct metal_swapchain {
    CAMetalLayer* layer;
    id<CAMetalDrawable> current_drawable;
    MTLPixelFormat pixel_format;
    CGSize size;
    bool hdr_enabled;
} metal_swapchain_t;

metal_swapchain_t* metal_swapchain_create(metal_device_t* dev, void* ns_view, bool hdr) {
    metal_swapchain_t* swap = calloc(1, sizeof(metal_swapchain_t));

    swap->layer = [CAMetalLayer layer];
    swap->layer.device = dev->device;
    swap->layer.pixelFormat = hdr ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
    swap->layer.framebufferOnly = YES;
    swap->layer.displaySyncEnabled = YES;

    if (hdr) {
        swap->layer.wantsExtendedDynamicRangeContent = YES;
        swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearDisplayP3);
    }

    // Attach to NSView
    NSView* view = (__bridge NSView*)ns_view;
    [view setLayer:swap->layer];
    [view setWantsLayer:YES];

    swap->size = view.bounds.size;
    swap->hdr_enabled = hdr;
    return swap;
}

id<MTLTexture> metal_swapchain_get_texture(metal_swapchain_t* swap) {
    swap->current_drawable = [swap->layer nextDrawable];
    return swap->current_drawable.texture;
}

void metal_swapchain_present(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd) {
    [cmd presentDrawable:swap->current_drawable];
}
```

OUTPUT: Display output ready for rendering to screen.
```
