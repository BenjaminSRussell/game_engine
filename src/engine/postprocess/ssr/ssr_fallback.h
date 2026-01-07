/*
 * ssr_fallback.h
 * SSR Fallback (Cubemap/Probes)
 */

#ifndef SSR_FALLBACK_H
#define SSR_FALLBACK_H

#include <Metal/Metal.h>
#include <simd/simd.h>

/**
 * @brief Execute SSR fallback
 * @param cmd Command buffer
 * @param env_map Global environment cubemap
 * @param irradiance_probes Irradiance probe grid (optional)
 * @param output_texture Output reflection texture
 */
void ssr_fallback_execute(id<MTLCommandBuffer> cmd,
                          id<MTLTexture> env_map,
                          void* irradiance_probes,
                          id<MTLTexture> output_texture);

#endif /* SSR_FALLBACK_H */
