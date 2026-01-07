/*
 * mesh_gpu_data.m
 * Metal buffer integration implementation
 */

#include "mesh_gpu_data.h"
#include <stdlib.h>

mesh_gpu_data_t *mesh_upload_to_gpu(id<MTLDevice> device,
                                    const mesh_data_t *mesh) {
  if (!device || !mesh)
    return NULL;

  mesh_gpu_data_t *gpu = calloc(1, sizeof(mesh_gpu_data_t));
  if (!gpu)
    return NULL;

  if (mesh->vertex_count > 0 && mesh->vertex_data) {
    // Calculate size: stride * count or use explicit data size
    // mesh_data_t has vertex_data_size
    NSUInteger vb_size = mesh->vertex_data_size;
    if (vb_size == 0)
      vb_size = mesh->vertex_count * mesh->vertex_stride;

    gpu->vertex_buffer =
        [device newBufferWithBytes:mesh->vertex_data
                            length:vb_size
                           options:MTLResourceStorageModeShared];
  }

  if (mesh->index_count > 0 && mesh->index_data) {
    NSUInteger ib_size = mesh->index_data_size;
    // fallback if size is 0
    if (ib_size == 0) {
      uint32_t index_stride = (mesh->index_type == INDEX_TYPE_UINT16) ? 2 : 4;
      ib_size = mesh->index_count * index_stride;
    }

    gpu->index_buffer =
        [device newBufferWithBytes:mesh->index_data
                            length:ib_size
                           options:MTLResourceStorageModeShared];
  }

  gpu->vertex_count = mesh->vertex_count;
  gpu->index_count = mesh->index_count;
  gpu->index_type = (mesh->index_type == INDEX_TYPE_UINT16)
                        ? MTLIndexTypeUInt16
                        : MTLIndexTypeUInt32;
  gpu->primitive_type = MTLPrimitiveTypeTriangle; // Default to triangles

  return gpu;
}

void mesh_gpu_data_free(mesh_gpu_data_t *gpu_data) {
  if (!gpu_data)
    return;

  // ARC handles Obj-C objects, but if we are in manual ref counting we might
  // need release. Assuming ARC is enabled for .m files usually. If this project
  // uses manual memory management, we'd need [gpu_data->vertex_buffer release].
  // But standard for modern Mac/iOS is ARC.
  // We do need to free the struct itself since we malloc'd it.

  // Note: If we are calling this from C, ARC rules might be tricky if the
  // struct holds obj-c pointers. Ideally this struct uses `__bridge` casts or
  // we keep it simple. For now assuming the caller knows what they are doing
  // with the struct pointer.

  // To be safe with ARC, we just explicitly set them to nil before free?
  // Actually if we compiled with ARC, the compiler handles struct members if
  // they are __strong id. But standard C struct with ARC is unsafe unless
  // __unsafe_unretained or handled carefully. Let's assume for now we just free
  // the container.

  // Actually, to update this for correctness:
  gpu_data->vertex_buffer = nil;
  gpu_data->index_buffer = nil;

  free(gpu_data);
}
