/*
 * texture_encoder.h
 * Texture compression and encoding management
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_ENCODER_H
#define TEXTURE_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS & ENUMS
 * ============================================================================ */

typedef enum texture_format {
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_BC1,
    TEXTURE_FORMAT_BC3,
    TEXTURE_FORMAT_BC7,
    TEXTURE_FORMAT_ASTC_4x4,
    TEXTURE_FORMAT_ASTC_8x8
} texture_format_t;

typedef enum compression_quality {
    COMPRESSION_QUALITY_FAST,
    COMPRESSION_QUALITY_BALANCED,
    COMPRESSION_QUALITY_BEST
} compression_quality_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_encoder_config {
    texture_format_t format;
    compression_quality_t quality;
    bool generate_mips;
} texture_encoder_config_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int texture_encoder_init(void);
void texture_encoder_shutdown(void);

/* Operations */
int texture_encoder_compress(const void* src_data, uint32_t width, uint32_t height, 
                            void** out_data, size_t* out_size, const texture_encoder_config_t* config);

/* Original stub compatibility */
int texture_texture_encoder_init(void);
void texture_texture_encoder_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_ENCODER_H */

