#include "visibility_buffer.h"
#include <simd/simd.h>

void visibility_buffer_init(visibility_buffer_t* buffer, id<MTLDevice> device, uint32_t width, uint32_t height) {
    MTLTextureDescriptor* visDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Uint width:width height:height mipmapped:NO];
    visDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    visDesc.storageMode = MTLStorageModePrivate;
    buffer->visibility = [device newTextureWithDescriptor:visDesc];
    
    MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:width height:height mipmapped:NO];
    depthDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    depthDesc.storageMode = MTLStorageModePrivate;
    buffer->depth = [device newTextureWithDescriptor:depthDesc];
    
    buffer->width = width;
    buffer->height = height;
}

void visibility_buffer_free(visibility_buffer_t* buffer) {
    [buffer->visibility release];
    [buffer->depth release];
    buffer->visibility = nil;
    buffer->depth = nil;
}

void visibility_render(id<MTLRenderCommandEncoder> encoder,
                       visibility_buffer_t* vis_buffer,
                       const cluster_mesh_t* mesh,
                       id<MTLBuffer> visible_clusters,
                       uint32_t visible_count) {
    if (visible_count == 0) return;
    
    // Bind mesh buffers
    [encoder setVertexBuffer:mesh->vertex_buffer offset:0 atIndex:0];
    [encoder setVertexBuffer:mesh->cluster_buffer offset:0 atIndex:1];
    [encoder setVertexBuffer:visible_clusters offset:0 atIndex:2];
    
    // In a real implementation we would need a PSO that outputs to R32Uint
    // for visibility buffer (ClusterID << 7 | TriangleID)
    
    // Draw using indirect command or instanced draw for visible clusters
    // Since we have a buffer of visible cluster IDs, we can use instancing to draw them.
    // Each instance is a cluster.
    // We need to know how many vertices/indices per cluster?
    // Our clusters are fixed size (CLUSTER_TRIANGLE_COUNT), so we can draw instanced.
    // Indices are in `mesh->index_buffer`.
    // But wait, `mesh->index_buffer` is valid for the whole mesh.
    // Each cluster stores `index_offset`.
    // We cannot draw all clusters with ONE draw call if they have different offsets, UNLESS we use indirect draw.
    // OR if we use a special vertex shader that fetches indices manually from a large buffer using (ClusterID + TriangleID).
    // Nanite uses a "programmable pull" or "mesh shader" paradigm.
    // Here we likely use a Vertex Shader that pulls vertices based on ClusterID.
    
    // Draw call:
    // [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:CLUSTER_TRIANGLE_COUNT * 3 instanceCount:visible_count];
    // In the vertex shader:
    // uint clusterId = visible_clusters[instance_id];
    // mesh_cluster cluster = clusters[clusterId];
    // uint index = index_buffer[cluster.index_offset + vertex_id];
    
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:CLUSTER_TRIANGLE_COUNT * 3 instanceCount:visible_count];
}

void visibility_material_pass(id<MTLRenderCommandEncoder> encoder,
                              visibility_buffer_t* vis_buffer,
                              gbuffer_t* gbuffer) {
    // This pass resolves the visibility buffer.
    // It typically draws a full screen quad.
    // Reads: vis_buffer->visibility
    // Writes: gbuffer targets (Albedo, Normal, etc.)
    
    // Bind visibility buffer
    [encoder setFragmentTexture:vis_buffer->visibility atIndex:0];
    
    // Draw full screen quad
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}
