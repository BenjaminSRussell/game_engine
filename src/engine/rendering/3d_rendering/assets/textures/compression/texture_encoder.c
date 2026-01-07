/*
 * texture_encoder.c
 * Texture compression and encoding implementation
 */

#include "texture_encoder.h"
#include "bc_compression.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_encoder_init(void) {
    return 0;
}

void texture_encoder_shutdown(void) {
}

int texture_encoder_compress(const void* src_data, uint32_t width, uint32_t height, 
                            void** out_data, size_t* out_size, const texture_encoder_config_t* config) {
    if (!src_data || !out_data || !out_size || !config) return -1;
    
    switch (config->format) {
        case TEXTURE_FORMAT_BC1:
        case TEXTURE_FORMAT_BC3:
        case TEXTURE_FORMAT_BC7: {
            bc_format_t bc_fmt = BC_FORMAT_BC1;
            if (config->format == TEXTURE_FORMAT_BC3) bc_fmt = BC_FORMAT_BC3;
            if (config->format == TEXTURE_FORMAT_BC7) bc_fmt = BC_FORMAT_BC7;
            
            // Calculate output size
            size_t block_size = (config->format == TEXTURE_FORMAT_BC1) ? 8 : 16;
            size_t size = (width / 4) * (height / 4) * block_size;
            
            *out_data = malloc(size);
            if (!*out_data) return -2;
            *out_size = size;
            
            return bc_compress_image((const uint32_t*)src_data, width, height, *out_data, bc_fmt);
        }
        
        default:
            return -3; // Unsupported for now
    }
}

/* Original stub compatibility */
int texture_texture_encoder_init(void) {
    return 0;
}

void texture_texture_encoder_shutdown(void) {
}

