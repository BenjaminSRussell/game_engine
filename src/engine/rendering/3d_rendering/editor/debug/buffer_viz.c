#include "buffer_viz.h"

void debug_draw_texture_fullscreen(id<MTLRenderCommandEncoder> encoder, id<MTLTexture> texture, float scale) {
    if (!encoder || !texture) return;
    
    // Ideally use a cached pipeline state for full screen quad
    // and bind the texture.
    // Since we don't have the pipeline here, this is a placeholder
    // for where the full screen draw call would go.
    
    // [encoder setFragmentTexture:texture atIndex:0];
    // [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}
