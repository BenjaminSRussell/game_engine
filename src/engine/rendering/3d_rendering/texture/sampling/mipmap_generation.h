/*
 * mipmap_generation.h
 * Mipmap generation and downsampling
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_MIPMAP_GENERATION_H
#define TEXTURE_MIPMAP_GENERATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/* Operations */
int mipmap_generate_chain(const void* src_data, uint32_t width, uint32_t height, 
                         void** out_mips, uint32_t* out_mip_count);

void mipmap_free_chain(void* mips, uint32_t mip_count);

/* Low-level downsampling */
int mipmap_downsample_box(const uint32_t* src, uint32_t src_w, uint32_t src_h, uint32_t* dst);

/* Original stub compatibility */
int texture_mipmap_generation_init(void);
void texture_mipmap_generation_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_MIPMAP_GENERATION_H */

