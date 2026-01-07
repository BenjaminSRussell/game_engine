#include "taa_history.h"
#include <stdlib.h>
#include "taa_resolve.h" // For pipeline initialization

taa_system_t* taa_create(id<MTLDevice> device, uint32_t width, uint32_t height) {
    taa_system_t* taa = (taa_system_t*)calloc(1, sizeof(taa_system_t));
    if (!taa) return NULL;

    taa->screen_size = simd_make_uint2(width, height);
    taa->current_history = 0;
    taa->frame_index = 0;

    // Create history textures
    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float 
                                                                                   width:width 
                                                                                  height:height 
                                                                               mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    // Store mode Valid because we read it next frame
    desc.storageMode = MTLStorageModePrivate;

    taa->history_texture[0] = [device newTextureWithDescriptor:desc];
    taa->history_texture[0].label = @"TAA History 0";
    
    taa->history_texture[1] = [device newTextureWithDescriptor:desc];
    taa->history_texture[1].label = @"TAA History 1";
    
    // Clear history textures to black initially? 
    // Usually not strictly necessary if we handle the first frame, but good practice.
    // Since they are private, we'd need a clear kernel or blit. 
    // For now, we assume the first frame resolve will overwrite or handle it.
    
    // Initialize pipeline
    // Note: We need to load the library. Assuming a default library or specific path.
    // For now, we will call the init function from taa_resolve.c which we will implement next.
    // Using a forward declaration or header inclusion.
    taa->resolve_pipeline = taa_init_pipeline(device);

    return taa;
}

void taa_destroy(taa_system_t* taa) {
    if (!taa) return;

    if (taa->history_texture[0]) {
        [taa->history_texture[0] release];
    }
    if (taa->history_texture[1]) {
        [taa->history_texture[1] release];
    }
    if (taa->resolve_pipeline) {
        [taa->resolve_pipeline release];
    }
    
    free(taa);
}

void taa_resize(taa_system_t* taa, id<MTLDevice> device, uint32_t width, uint32_t height) {
    if (!taa) return;
    
    if (taa->screen_size.x == width && taa->screen_size.y == height) {
        return;
    }
    
    taa->screen_size = simd_make_uint2(width, height);
    
    // Release old textures
    [taa->history_texture[0] release];
    [taa->history_texture[1] release];
    
    // Create new ones
    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float 
                                                                                   width:width 
                                                                                  height:height 
                                                                               mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    desc.storageMode = MTLStorageModePrivate;
    
    taa->history_texture[0] = [device newTextureWithDescriptor:desc];
    taa->history_texture[0].label = @"TAA History 0";
    
    taa->history_texture[1] = [device newTextureWithDescriptor:desc];
    taa->history_texture[1].label = @"TAA History 1";
    
    // Reset history index or frame index if needed?
    // Maybe not frame index (for jitter), but history might be invalid.
    // The resolve shader validates history UVs, but if we resize, the old history content is gone anyway (new textures).
    // So the FIRST frame after resize will have "garbage" history if we don't clear, 
    // OR we just rely on valid reprojection bounds.
    // Since we created new textures, they are uninitialized. 
    // Ideally we should flag this frame as "first frame" to disable history blend.
}
