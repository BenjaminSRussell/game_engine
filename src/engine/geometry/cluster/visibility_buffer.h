#ifndef VISIBILITY_BUFFER_H
#define VISIBILITY_BUFFER_H

#include <Metal/Metal.h>
#include "../nanite/cluster_builder.h"

// Forward declaration
typedef struct gbuffer gbuffer_t;

typedef struct visibility_buffer {
    id<MTLTexture> visibility;    // R32UI - cluster_id | triangle_id
    id<MTLTexture> depth;         // Depth32Float
    uint32_t width, height;
} visibility_buffer_t;

// Initialize visibility buffer (helper, not strictly requested but useful)
void visibility_buffer_init(visibility_buffer_t* buffer, id<MTLDevice> device, uint32_t width, uint32_t height);
void visibility_buffer_free(visibility_buffer_t* buffer);

// Render clusters to visibility buffer
// Encoder should have visibility parameters bound (or be ready to bind pipeline)
// BUT typically the function sets up the pipeline.
// Given strict signature, we accept encoder.
void visibility_render(id<MTLRenderCommandEncoder> encoder,
                       visibility_buffer_t* vis_buffer,
                       const cluster_mesh_t* mesh,
                       id<MTLBuffer> visible_clusters,
                       uint32_t visible_count);

// Material pass - resolve visibility to final color (or G-Buffer)
void visibility_material_pass(id<MTLRenderCommandEncoder> encoder,
                              visibility_buffer_t* vis_buffer,
                              gbuffer_t* gbuffer);

#endif // VISIBILITY_BUFFER_H
