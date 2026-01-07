/*
 * mtl_argument_buffer.h
 * Metal argument buffer interface for bindless resources
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_ARGUMENT_BUFFER_H
#define PLATFORM_MTL_ARGUMENT_BUFFER_H

#include "mtl_device.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef void* mtl_argument_encoder_t;

/* ============================================================================
 * ARGUMENT ENCODER API
 * ============================================================================ */

/**
 * Creates an argument encoder from a function's argument at a specific index.
 * @param function The MTLFunction (void*).
 * @param index The argument buffer index in the shader function.
 * @return The argument encoder.
 */
mtl_argument_encoder_t metal_argument_encoder_create_from_function(void* function, unsigned long index);

/**
 * Destroys an argument encoder.
 * @param encoder The encoder to destroy.
 */
void metal_argument_encoder_destroy(mtl_argument_encoder_t encoder);

/**
 * Gets the encoded length (size) of the argument buffer.
 * @param encoder The argument encoder.
 * @return The encoded length in bytes.
 */
unsigned long metal_argument_encoder_get_encoded_length(mtl_argument_encoder_t encoder);

/**
 * Gets the alignment requirement for the argument buffer.
 * @param encoder The argument encoder.
 * @return The alignment in bytes.
 */
unsigned long metal_argument_encoder_get_alignment(mtl_argument_encoder_t encoder);

/**
 * Sets the argument buffer to encode into.
 * @param encoder The argument encoder.
 * @param argument_buffer The buffer to encode arguments into (MTLBuffer).
 * @param offset The offset into the buffer.
 */
void metal_argument_encoder_set_argument_buffer(mtl_argument_encoder_t encoder, void* argument_buffer, unsigned long offset);

/**
 * Encodes a buffer into the argument buffer at the specified index.
 * @param encoder The argument encoder.
 * @param buffer The buffer to encode (MTLBuffer).
 * @param offset The offset into the buffer.
 * @param index The argument index.
 */
void metal_argument_encoder_set_buffer(mtl_argument_encoder_t encoder, void* buffer, unsigned long offset, unsigned long index);

/**
 * Encodes a texture into the argument buffer at the specified index.
 * @param encoder The argument encoder.
 * @param texture The texture to encode (MTLTexture).
 * @param index The argument index.
 */
void metal_argument_encoder_set_texture(mtl_argument_encoder_t encoder, void* texture, unsigned long index);

/**
 * Encodes a sampler state into the argument buffer at the specified index.
 * @param encoder The argument encoder.
 * @param sampler The sampler to encode (MTLSamplerState).
 * @param index The argument index.
 */
void metal_argument_encoder_set_sampler_state(mtl_argument_encoder_t encoder, void* sampler, unsigned long index);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_ARGUMENT_BUFFER_H */
