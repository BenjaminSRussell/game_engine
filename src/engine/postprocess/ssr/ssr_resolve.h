/*
 * ssr_resolve.h
 * SSR Resolve and Spatial Filtering
 */

#ifndef SSR_RESOLVE_H
#define SSR_RESOLVE_H

#include <Metal/Metal.h>
#include <simd/simd.h>

/**
 * @brief Execute the SSR resolve pass
 * @param device Metal device
 * @param cmd Command buffer
 * @param pipeline Resolve compute pipeline
 * @param hit_texture Texture containing hit UVs
 * @param color_texture Scene color texture
 * @param output_texture Output reflection color texture
 * @param width Screen width
 * @param height Screen height
 */
void ssr_resolve_execute(id<MTLDevice> device, id<MTLCommandBuffer> cmd,
                         id<MTLComputePipelineState> pipeline,
                         id<MTLTexture> hit_texture,
                         id<MTLTexture> color_texture,
                         id<MTLTexture> output_texture,
                         uint32_t width, uint32_t height);

#endif /* SSR_RESOLVE_H */
