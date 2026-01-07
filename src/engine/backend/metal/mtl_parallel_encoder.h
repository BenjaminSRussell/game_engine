/*
 * mtl_parallel_encoder.h
 * Metal parallel render command encoder interface
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_PARALLEL_ENCODER_H
#define PLATFORM_MTL_PARALLEL_ENCODER_H

#include "mtl_command_buffer.h"
#include "mtl_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef void* mtl_parallel_render_command_encoder_t;

/* ============================================================================
 * API
 * ============================================================================ */

mtl_parallel_render_command_encoder_t metal_parallel_render_command_encoder_create(mtl_command_buffer_t buffer, void* pass_descriptor);
mtl_render_command_encoder_t metal_parallel_render_encoder_create_command_encoder(mtl_parallel_render_command_encoder_t parallel_encoder);
void metal_parallel_render_encoder_end_encoding(mtl_parallel_render_command_encoder_t parallel_encoder);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_PARALLEL_ENCODER_H */
