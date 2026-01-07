/*
 * mesh_gpu_data.h
 * Metal buffer integration for mesh data
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_GPU_DATA_H
#define GEOMETRY_MESH_GPU_DATA_H

#include "mesh_data.h"
#include <Metal/Metal.h>

#ifdef __cplusplus
extern "C" {
#endif

// GPU-resident mesh data
typedef struct mesh_gpu_data {
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
    uint32_t vertex_count;
    uint32_t index_count;
    MTLIndexType index_type;
    MTLPrimitiveType primitive_type;
} mesh_gpu_data_t;

// Forward declaration for Metal device wrapper if needed, 
// or Just use id<MTLDevice> directly if we are in Obj-C context.
// Since this header might be included from C, we need to be careful with Obj-C types.
// However, the task description showed Obj-C types in the struct.
// To use this in C files, we would need opaque pointers, but for now assuming this is used in Obj-C files.
// Or we wrap it.
// Given existing code is C, let's wrap the struct contents in #ifdef __OBJC__ blocks 
// and provide an opaque handle for C.

#ifdef __OBJC__
// Upload mesh to GPU
mesh_gpu_data_t* mesh_upload_to_gpu(id<MTLDevice> device, const mesh_data_t* mesh);
#else
typedef struct mesh_gpu_data mesh_gpu_data_t;
// C-compatible declaration if we had a wrapper, but for now let's assume usage in .m files
#endif

void mesh_gpu_data_free(mesh_gpu_data_t* gpu_data);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_GPU_DATA_H */
