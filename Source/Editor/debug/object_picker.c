#include <stdint.h>
#include <simd/simd.h>
#include <stdlib.h>
#include <string.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

// Forward declaration for renderable objects (assumed present in engine)
typedef struct renderable renderable_t; 

typedef struct object_picker {
    id id_texture;       // id<MTLTexture> R32Uint
    id readback_buffer;  // id<MTLBuffer>
    id depth_texture;    // id<MTLTexture>
    uint32_t width, height;
    
    id pipeline_state;
    id depth_state;
} object_picker_t;

object_picker_t* picker_create(id device_ptr, uint32_t width, uint32_t height) {
#ifdef __OBJC__
    id<MTLDevice> device = (id<MTLDevice>)device_ptr;
    object_picker_t* picker = (object_picker_t*)calloc(1, sizeof(object_picker_t));
    
    picker->width = width;
    picker->height = height;
    
    // Create ID Texture
    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Uint width:width height:height mipmapped:NO];
    desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    desc.storageMode = MTLStorageModePrivate;
    picker->id_texture = [device newTextureWithDescriptor:desc];
    
    // Create Depth Texture
    MTLTextureDescriptor* depth = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:width height:height mipmapped:NO];
    depth.usage = MTLTextureUsageRenderTarget;
    depth.storageMode = MTLStorageModePrivate;
    picker->depth_texture = [device newTextureWithDescriptor:depth];
    
    // Readback Buffer (small, for single pixel or region)
    picker->readback_buffer = [device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];
    
    return picker;
#else
    return NULL;
#endif
}

void picker_destroy(object_picker_t* picker) {
    if (picker) free(picker);
}

void picker_resize(object_picker_t* picker, id device_ptr, uint32_t width, uint32_t height) {
    // Recreate textures
}

void picker_render_ids(id encoder_ptr, object_picker_t* picker, const renderable_t* objects, uint32_t count) {
#ifdef __OBJC__
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)encoder_ptr;
    // Draw objects with a shader that outputs object ID to primitive/color attachment
#endif
}

uint32_t picker_query(object_picker_t* picker, uint32_t x, uint32_t y) {
#ifdef __OBJC__
    // Enqueue blit from texture to buffer
    // Wait for completion (sync) or assume deferred readback
    
    // Simplified sync readback
    uint32_t* ptr = (uint32_t*)[(id<MTLBuffer>)picker->readback_buffer contents];
    return *ptr; 
#else
    return 0;
#endif
}
