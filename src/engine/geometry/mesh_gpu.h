#ifndef GEOMETRY_MESH_GPU_H
#define GEOMETRY_MESH_GPU_H

#include "geometry/geometry_types.h"
#include <stdbool.h>

/**
 * GPU Integration for Mesh System
 * 
 * Handles upload/download of mesh data to GPU using the backend (Metal/Vulkan)
 */

// Forward declarations
struct metal_device;
struct metal_buffer;

// GPU Upload/Unload
bool mesh_gpu_upload(mesh_t* mesh, struct metal_device* device);
void mesh_gpu_unload(mesh_t* mesh);
bool mesh_gpu_update(mesh_t* mesh, struct metal_device* device);

// Asynchronous Upload
typedef void (*mesh_upload_callback_fn)(mesh_t* mesh, bool success, void* userdata);
bool mesh_gpu_upload_async(mesh_t* mesh, struct metal_device* device, 
                            mesh_upload_callback_fn callback, void* userdata);

// GPU Memory Tracking
u64 mesh_gpu_get_memory_usage(const mesh_t* mesh);
bool mesh_gpu_is_uploaded(const mesh_t* mesh);

// System Cleanup
void mesh_gpu_cleanup(void);

#endif // GEOMETRY_MESH_GPU_H
