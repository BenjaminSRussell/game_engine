/*
 * mtl_argument_buffer.c
 * Metal argument buffer implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_argument_buffer.h"

#if defined(__OBJC__)
#import <Metal/Metal.h>

#define TO_ARG_ENC(x) ((__bridge id<MTLArgumentEncoder>)(x))
#define TO_FUNCTION(x) ((__bridge id<MTLFunction>)(x))
#define TO_BUFFER(x) ((__bridge id<MTLBuffer>)(x))
#define TO_TEXTURE(x) ((__bridge id<MTLTexture>)(x))
#define TO_SAMPLER(x) ((__bridge id<MTLSamplerState>)(x))

#endif

/* ============================================================================
 * ARGUMENT ENCODER IMPLEMENTATION
 * ============================================================================ */

mtl_argument_encoder_t metal_argument_encoder_create_from_function(void* function, unsigned long index) {
#if defined(__OBJC__)
    if (!function) return NULL;
    
    id<MTLFunction> mtl_function = TO_FUNCTION(function);
    id<MTLArgumentEncoder> encoder = [mtl_function newArgumentEncoderWithBufferIndex:index];
    
    return (__bridge_retained void*)encoder;
#else
    return NULL;
#endif
}

void metal_argument_encoder_destroy(mtl_argument_encoder_t encoder) {
#if defined(__OBJC__)
    if (encoder) {
        CFRelease(encoder);
    }
#endif
}

unsigned long metal_argument_encoder_get_encoded_length(mtl_argument_encoder_t encoder) {
#if defined(__OBJC__)
    if (encoder) {
        return [TO_ARG_ENC(encoder) encodedLength];
    }
#endif
    return 0;
}

unsigned long metal_argument_encoder_get_alignment(mtl_argument_encoder_t encoder) {
#if defined(__OBJC__)
    if (encoder) {
        return [TO_ARG_ENC(encoder) alignment];
    }
#endif
    return 0;
}

void metal_argument_encoder_set_argument_buffer(mtl_argument_encoder_t encoder, void* argument_buffer, unsigned long offset) {
#if defined(__OBJC__)
    if (encoder && argument_buffer) {
        [TO_ARG_ENC(encoder) setArgumentBuffer:TO_BUFFER(argument_buffer) offset:offset];
    }
#endif
}

void metal_argument_encoder_set_buffer(mtl_argument_encoder_t encoder, void* buffer, unsigned long offset, unsigned long index) {
#if defined(__OBJC__)
    if (encoder && buffer) {
        [TO_ARG_ENC(encoder) setBuffer:TO_BUFFER(buffer) offset:offset atIndex:index];
    }
#endif
}

void metal_argument_encoder_set_texture(mtl_argument_encoder_t encoder, void* texture, unsigned long index) {
#if defined(__OBJC__)
    if (encoder && texture) {
        [TO_ARG_ENC(encoder) setTexture:TO_TEXTURE(texture) atIndex:index];
    }
#endif
}

void metal_argument_encoder_set_sampler_state(mtl_argument_encoder_t encoder, void* sampler, unsigned long index) {
#if defined(__OBJC__)
    if (encoder && sampler) {
        [TO_ARG_ENC(encoder) setSamplerState:TO_SAMPLER(sampler) atIndex:index];
    }
#endif
}
