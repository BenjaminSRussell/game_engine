#ifndef TAA_RESOLVE_H
#define TAA_RESOLVE_H

#include "taa_history.h"
#include <Metal/Metal.h>

// Initialize the TAA compute pipeline
id<MTLComputePipelineState> taa_init_pipeline(id<MTLDevice> device);

// Perform the TAA resolve pass
// taa: The TAA system state
// cmd: The command buffer to encode work into
// current_frame: The raw rendered frame (jittered)
// depth: The depth buffer (optional/used for velocity or other heuristics)
// output: The destination texture for the resolved image
void taa_render(taa_system_t* taa, id<MTLCommandBuffer> cmd,
                id<MTLTexture> current_frame, id<MTLTexture> depth,
                id<MTLTexture> output);

#endif // TAA_RESOLVE_H
