#include "hair_common.h"

void hair_render(id<MTLRenderCommandEncoder> encoder, HairSystem* system, const simd_float4x4* view_proj) {
    if (!system || !system->render_pipeline) return;
    
    [encoder setRenderPipelineState:system->render_pipeline];
    
    // Bind Vertices (Control Points)
    [encoder setVertexBuffer:system->strands_buffer offset:0 atIndex:0];
    
    // Bind Uniforms (View/Proj)
    [encoder setVertexBytes:view_proj length:sizeof(simd_float4x4) atIndex:1];
    
    // Bind other properties if needed (e.g. thickness, lighting params, density map)
    if (system->density_map) {
        [encoder setFragmentTexture:system->density_map atIndex:0];
    }
    
    // Draw
    // Assuming simple line list from indices generated in hair_strand.c
    // System->points_per_strand is 16.
    // Total indices = strand_count * (16-1) * 2
    
    uint32_t segment_count = system->points_per_strand - 1;
    uint32_t index_count = system->strand_count * segment_count * 2;
    
    [encoder drawIndexedPrimitives:MTLPrimitiveTypeLine
                        indexCount:index_count 
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:system->render_index_buffer
                 indexBufferOffset:0];
}
