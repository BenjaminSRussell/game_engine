/*
 * astc_compression.h
 * ASTC compression support
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_ASTC_COMPRESSION_H
#define TEXTURE_ASTC_COMPRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum astc_block_size {
    ASTC_BLOCK_4x4,
    ASTC_BLOCK_5x5,
    ASTC_BLOCK_6x6,
    ASTC_BLOCK_8x8
} astc_block_size_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Operations */
int astc_compress_block(const uint32_t* rgba, uint32_t block_w, uint32_t block_h, void* out_block);
int astc_compress_image(const uint32_t* rgba, uint32_t width, uint32_t height, void* out_data, astc_block_size_t block_size);

/* Original stub compatibility */
int texture_astc_compression_init(void);
void texture_astc_compression_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_ASTC_COMPRESSION_H */

