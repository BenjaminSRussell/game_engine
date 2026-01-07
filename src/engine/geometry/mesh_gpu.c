#include "geometry/mesh_gpu.h"
#include "geometry/mesh.h"
#include <core/memory.h>
#include <core/logger.h>
#include <backend/metal/mtl_buffer.h>
#include <backend/metal/mtl_device.h>
#include <string.h>

// ----------------------------------------------------------------------------
// GPU Upload
// ----------------------------------------------------------------------------

bool mesh_gpu_upload(mesh_t* mesh, metal_device_t* device) {
    if (!mesh || !device) {
        LOG_ERROR("mesh_gpu_upload: Invalid parameters");
        return false;
    }
    
    if (mesh->vertex_count == 0 || mesh->index_count == 0) {
        LOG_ERROR("mesh_gpu_upload: Mesh '%s' has no geometry", mesh->name);
        return false;
    }
    
    // Check if already uploaded
    if (mesh->flags & MESH_FLAG_GPU_UPLOADED) {
        LOG_WARN("Mesh '%s' is already uploaded to GPU", mesh->name);
        return true;
    }
    
    // Create vertex buffer
    metal_buffer_desc_t vb_desc = {0};
    vb_desc.size = mesh->vertex_count * sizeof(vertex_t);
    vb_desc.storage_mode = METAL_STORAGE_SHARED; // or PRIVATE for optimal perf
    vb_desc.usage = METAL_BUFFER_USAGE_VERTEX;
    vb_desc.initial_data = mesh->vertices;
    
    char vb_label[128];
    snprintf(vb_label, sizeof(vb_label), "VertexBuffer_%s", mesh->name);
    vb_desc.label = vb_label;
    
    metal_buffer_t* vertex_buffer = metal_buffer_create(device, &vb_desc);
    if (!vertex_buffer) {
        LOG_ERROR("Failed to create vertex buffer for mesh '%s'", mesh->name);
        return false;
    }
    
    // Create index buffer
    metal_buffer_desc_t ib_desc = {0};
    ib_desc.size = mesh->index_count * sizeof(u32);
    ib_desc.storage_mode = METAL_STORAGE_SHARED;
    ib_desc.usage = METAL_BUFFER_USAGE_INDEX;
    ib_desc.initial_data = mesh->indices;
    
    char ib_label[128];
    snprintf(ib_label, sizeof(ib_label), "IndexBuffer_%s", mesh->name);
    ib_desc.label = ib_label;
    
    metal_buffer_t* index_buffer = metal_buffer_create(device, &ib_desc);
    if (!index_buffer) {
        LOG_ERROR("Failed to create index buffer for mesh '%s'", mesh->name);
        metal_buffer_destroy(vertex_buffer);
        return false;
    }
    
    // Store buffer handles (simplified - in real system use handle manager)
    // For now, store the pointer as handle (NOT PRODUCTION QUALITY)
    mesh->vertex_buffer_handle = (u32)(uintptr_t)vertex_buffer;
    mesh->index_buffer_handle = (u32)(uintptr_t)index_buffer;
    
    mesh->flags |= MESH_FLAG_GPU_UPLOADED;
    
    // Free CPU memory if not flagged to keep
    if (!(mesh->flags & MESH_FLAG_KEEP_CPU)) {
        FREE(mesh->vertices);
        FREE(mesh->indices);
        mesh->vertices = NULL;
        mesh->indices = NULL;
        LOG_INFO("Mesh '%s' CPU data freed after GPU upload", mesh->name);
    }
    
    LOG_INFO("Mesh '%s' uploaded to GPU (VB: %zu bytes, IB: %zu bytes)",
        mesh->name, vb_desc.size, ib_desc.size);
    
    return true;
}

// ----------------------------------------------------------------------------
// GPU Unload
// ----------------------------------------------------------------------------

void mesh_gpu_unload(mesh_t* mesh) {
    if (!mesh) return;
    
    if (!(mesh->flags & MESH_FLAG_GPU_UPLOADED)) {
        return; // Not uploaded
    }
    
    // Destroy buffers
    if (mesh->vertex_buffer_handle != 0) {
        metal_buffer_t* vb = (metal_buffer_t*)(uintptr_t)mesh->vertex_buffer_handle;
        metal_buffer_destroy(vb);
        mesh->vertex_buffer_handle = 0;
    }
    
    if (mesh->index_buffer_handle != 0) {
        metal_buffer_t* ib = (metal_buffer_t*)(uintptr_t)mesh->index_buffer_handle;
        metal_buffer_destroy(ib);
        mesh->index_buffer_handle = 0;
    }
    
    mesh->flags &= ~MESH_FLAG_GPU_UPLOADED;
    
    LOG_INFO("Mesh '%s' unloaded from GPU", mesh->name);
}

// ----------------------------------------------------------------------------
// GPU Update (for dynamic meshes)
// ----------------------------------------------------------------------------

bool mesh_gpu_update(mesh_t* mesh, metal_device_t* device) {
    if (!mesh || !device) return false;
    
    if (!(mesh->flags & MESH_FLAG_GPU_UPLOADED)) {
        LOG_ERROR("Cannot update mesh '%s': not uploaded to GPU", mesh->name);
        return false;
    }
    
    if (!(mesh->flags & MESH_FLAG_DYNAMIC)) {
        LOG_WARN("Updating non-dynamic mesh '%s', consider setting MESH_FLAG_DYNAMIC", 
            mesh->name);
    }
    
    // Get buffers
    metal_buffer_t* vb = (metal_buffer_t*)(uintptr_t)mesh->vertex_buffer_handle;
    metal_buffer_t* ib = (metal_buffer_t*)(uintptr_t)mesh->index_buffer_handle;
    
    if (!vb || !ib) {
        LOG_ERROR("Invalid buffer handles for mesh '%s'", mesh->name);
        return false;
    }
    
    // Update vertex data
    if (mesh->vertices) {
        metal_buffer_update(vb, mesh->vertices, 
            mesh->vertex_count * sizeof(vertex_t), 0);
    }
    
    // Update index data
    if (mesh->indices) {
        metal_buffer_update(ib, mesh->indices,
            mesh->index_count * sizeof(u32), 0);
    }
    
    LOG_INFO("Mesh '%s' GPU data updated", mesh->name);
    return true;
}

// ----------------------------------------------------------------------------
// Async Upload (Placeholder)
// ----------------------------------------------------------------------------

bool mesh_gpu_upload_async(mesh_t* mesh, metal_device_t* device,
                            mesh_upload_callback_fn callback, void* userdata) {
    // TODO: Implement async upload with completion callback
    // For now, just do sync upload and call callback
    
    bool success = mesh_gpu_upload(mesh, device);
    
    if (callback) {
        callback(mesh, success, userdata);
    }
    
    return success;
}

// ----------------------------------------------------------------------------
// GPU Memory Tracking
// ----------------------------------------------------------------------------

u64 mesh_gpu_get_memory_usage(const mesh_t* mesh) {
    if (!mesh || !(mesh->flags & MESH_FLAG_GPU_UPLOADED)) {
        return 0;
    }
    
    return mesh->vertex_count * sizeof(vertex_t) +
           mesh->index_count * sizeof(u32);
}

bool mesh_gpu_is_uploaded(const mesh_t* mesh) {
    return mesh && (mesh->flags & MESH_FLAG_GPU_UPLOADED);
}
