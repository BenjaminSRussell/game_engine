/*
 * bc_compression.h
 * Block compression (BC1-BC7) support
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_BC_COMPRESSION_H
#define TEXTURE_BC_COMPRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum bc_format {
    BC_FORMAT_BC1, // DXT1
    BC_FORMAT_BC3, // DXT5
    BC_FORMAT_BC4, // LatM1
    BC_FORMAT_BC5, // LatM2
    BC_FORMAT_BC6H,// HDR
    BC_FORMAT_BC7  // High quality
} bc_format_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Operations */
int bc_compress_block(const uint32_t* rgba_4x4, void* out_block, bc_format_t format);
int bc_compress_image(const uint32_t* rgba, uint32_t width, uint32_t height, void* out_data, bc_format_t format);

/* Original stub compatibility */
int texture_bc_compression_init(void);
void texture_bc_compression_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_BC_COMPRESSION_H */

