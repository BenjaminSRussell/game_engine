/*
 * mtl_encoder.h
 * Metal command encoder interface
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_ENCODER_H
#define PLATFORM_MTL_ENCODER_H

#include "backend/metal/mtl_command_buffer.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================
 */

/* Opaque encoder types */
typedef void *mtl_render_command_encoder_t;
typedef void *mtl_compute_command_encoder_t;
typedef void *mtl_blit_command_encoder_t;

/* Primitives */
typedef enum mtl_primitive_type {
  MTL_PRIMITIVE_TYPE_POINT = 0,
  MTL_PRIMITIVE_TYPE_LINE = 1,
  MTL_PRIMITIVE_TYPE_LINE_STRIP = 2,
  MTL_PRIMITIVE_TYPE_TRIANGLE = 3,
  MTL_PRIMITIVE_TYPE_TRIANGLE_STRIP = 4
} mtl_primitive_type_t;

typedef enum mtl_index_type {
  MTL_INDEX_TYPE_UINT16 = 0,
  MTL_INDEX_TYPE_UINT32 = 1
} mtl_index_type_t;

typedef enum mtl_visibility_result_mode {
  MTL_VISIBILITY_RESULT_MODE_DISABLED = 0,
  MTL_VISIBILITY_RESULT_MODE_BOOLEAN = 1,
  MTL_VISIBILITY_RESULT_MODE_COUNTING = 2
} mtl_visibility_result_mode_t;

typedef struct mtl_viewport {
  double originX, originY, width, height, znear, zfar;
} mtl_viewport_t;

typedef struct mtl_scissor_rect {
  unsigned long x, y, width, height;
} mtl_scissor_rect_t;

typedef struct mtl_draw_primitives_args {
  mtl_primitive_type_t primitiveType;
  unsigned long vertexStart;
  unsigned long vertexCount;
  unsigned long instanceCount;
  unsigned long baseInstance;
} mtl_draw_primitives_args_t;

typedef struct mtl_draw_indexed_primitives_args {
  mtl_primitive_type_t primitiveType;
  unsigned long indexCount;
  mtl_index_type_t indexType;
  void *indexBuffer; /* id<MTLBuffer> */
  unsigned long indexBufferOffset;
  unsigned long instanceCount;
  unsigned long baseVertex;
  unsigned long baseInstance;
} mtl_draw_indexed_primitives_args_t;

typedef struct mtl_dispatch_threadgroups_args {
  unsigned long threadgroupsPerGrid[3];
  unsigned long threadsPerThreadgroup[3];
} mtl_dispatch_threadgroups_args_t;

/* ============================================================================
 * RENDER ENCODER API
 * ============================================================================
 */

mtl_render_command_encoder_t
metal_render_command_encoder_create(mtl_command_buffer_t buffer,
                                    void *pass_descriptor);
void metal_render_encoder_end_encoding(mtl_render_command_encoder_t encoder);

/* State Setup */
void metal_render_encoder_set_render_pipeline_state(
    mtl_render_command_encoder_t encoder, void *pipeline_state);
void metal_render_encoder_set_vertex_buffer(
    mtl_render_command_encoder_t encoder, void *buffer, unsigned long offset,
    unsigned long index);
void metal_render_encoder_set_vertex_bytes(mtl_render_command_encoder_t encoder,
                                           const void *bytes,
                                           unsigned long length,
                                           unsigned long index);
void metal_render_encoder_set_fragment_buffer(
    mtl_render_command_encoder_t encoder, void *buffer, unsigned long offset,
    unsigned long index);
void metal_render_encoder_set_fragment_bytes(
    mtl_render_command_encoder_t encoder, const void *bytes,
    unsigned long length, unsigned long index);
void metal_render_encoder_set_vertex_texture(
    mtl_render_command_encoder_t encoder, void *texture, unsigned long index);
void metal_render_encoder_set_fragment_texture(
    mtl_render_command_encoder_t encoder, void *texture, unsigned long index);
void metal_render_encoder_set_vertex_sampler_state(
    mtl_render_command_encoder_t encoder, void *sampler, unsigned long index);
void metal_render_encoder_set_fragment_sampler_state(
    mtl_render_command_encoder_t encoder, void *sampler, unsigned long index);
void metal_render_encoder_set_depth_stencil_state(
    mtl_render_command_encoder_t encoder, void *depth_stencil_state);
void metal_render_encoder_set_viewport(mtl_render_command_encoder_t encoder,
                                       mtl_viewport_t viewport);
void metal_render_encoder_set_scissor_rect(mtl_render_command_encoder_t encoder,
                                           mtl_scissor_rect_t rect);
void metal_render_encoder_set_cull_mode(mtl_render_command_encoder_t encoder,
                                        int cull_mode);
void metal_render_encoder_set_front_facing_winding(
    mtl_render_command_encoder_t encoder, int winding);

/* Drawing */
void metal_render_encoder_draw_primitives(mtl_render_command_encoder_t encoder,
                                          mtl_draw_primitives_args_t args);
void metal_render_encoder_draw_indexed_primitives(
    mtl_render_command_encoder_t encoder,
    mtl_draw_indexed_primitives_args_t args);
void metal_render_encoder_draw_primitives_indirect(
    mtl_render_command_encoder_t encoder, mtl_primitive_type_t primitiveType,
    void *indirectBuffer, unsigned long indirectBufferOffset);
void metal_render_encoder_draw_indexed_primitives_indirect(
    mtl_render_command_encoder_t encoder, mtl_primitive_type_t primitiveType,
    mtl_index_type_t indexType, void *indexBuffer,
    unsigned long indexBufferOffset, void *indirectBuffer,
    unsigned long indirectBufferOffset);
void metal_render_encoder_set_visibility_result_mode(
    mtl_render_command_encoder_t encoder, mtl_visibility_result_mode_t mode,
    unsigned long offset);
void metal_render_encoder_memory_barrier(mtl_render_command_encoder_t encoder,
                                         void *resources, unsigned long count);

/* ============================================================================
 * COMPUTE ENCODER API
 * ============================================================================
 */

mtl_compute_command_encoder_t
metal_compute_command_encoder_create(mtl_command_buffer_t buffer);
void metal_compute_encoder_end_encoding(mtl_compute_command_encoder_t encoder);

/* State Setup */
void metal_compute_encoder_set_compute_pipeline_state(
    mtl_compute_command_encoder_t encoder, void *pipeline_state);
void metal_compute_encoder_set_buffer(mtl_compute_command_encoder_t encoder,
                                      void *buffer, unsigned long offset,
                                      unsigned long index);
void metal_compute_encoder_set_texture(mtl_compute_command_encoder_t encoder,
                                       void *texture, unsigned long index);
void metal_compute_encoder_set_sampler_state(
    mtl_compute_command_encoder_t encoder, void *sampler, unsigned long index);
void metal_compute_encoder_set_threadgroup_memory_length(
    mtl_compute_command_encoder_t encoder, unsigned long length,
    unsigned long index);

/* Dispatch */
void metal_compute_encoder_dispatch_threadgroups(
    mtl_compute_command_encoder_t encoder,
    mtl_dispatch_threadgroups_args_t args);
void metal_compute_encoder_dispatch_threadgroups_indirect(
    mtl_compute_command_encoder_t encoder, void *indirectBuffer,
    unsigned long indirectBufferOffset,
    mtl_dispatch_threadgroups_args_t threadsPerThreadgroup);
void metal_compute_encoder_dispatch_threads(
    mtl_compute_command_encoder_t encoder,
    mtl_dispatch_threadgroups_args_t args);
void metal_compute_encoder_memory_barrier(mtl_compute_command_encoder_t encoder,
                                          void *resources, unsigned long count);

/* ============================================================================
 * BLIT ENCODER API
 * ============================================================================
 */

mtl_blit_command_encoder_t
metal_blit_command_encoder_create(mtl_command_buffer_t buffer);
void metal_blit_encoder_end_encoding(mtl_blit_command_encoder_t encoder);

/* Operations */
void metal_blit_encoder_copy_from_buffer_to_buffer(
    mtl_blit_command_encoder_t encoder, void *sourceBuffer,
    unsigned long sourceOffset, void *destinationBuffer,
    unsigned long destinationOffset, unsigned long size);

void metal_blit_encoder_copy_from_buffer_to_texture(
    mtl_blit_command_encoder_t encoder, void *sourceBuffer,
    unsigned long sourceOffset, unsigned long sourceBytesPerRow,
    unsigned long sourceBytesPerImage, unsigned long sourceSize[3],
    void *destinationTexture, unsigned long destinationSlice,
    unsigned long destinationLevel, unsigned long destinationOrigin[3]);

void metal_blit_encoder_copy_from_texture_to_buffer(
    mtl_blit_command_encoder_t encoder, void *sourceTexture,
    unsigned long sourceSlice, unsigned long sourceLevel,
    unsigned long sourceOrigin[3], unsigned long sourceSize[3],
    void *destinationBuffer, unsigned long destinationOffset,
    unsigned long destinationBytesPerRow,
    unsigned long destinationBytesPerImage);

void metal_blit_encoder_copy_from_texture_to_texture(
    mtl_blit_command_encoder_t encoder, void *sourceTexture,
    unsigned long sourceSlice, unsigned long sourceLevel,
    unsigned long sourceOrigin[3], unsigned long sourceSize[3],
    void *destinationTexture, unsigned long destinationSlice,
    unsigned long destinationLevel, unsigned long destinationOrigin[3]);

void metal_blit_encoder_generate_mipmaps(mtl_blit_command_encoder_t encoder,
                                         void *texture);
void metal_blit_encoder_fill_buffer(mtl_blit_command_encoder_t encoder,
                                    void *buffer, unsigned long range_start,
                                    unsigned long range_length, uint8_t value);
void metal_blit_encoder_synchronize_resource(mtl_blit_command_encoder_t encoder,
                                             void *resource);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_ENCODER_H */
