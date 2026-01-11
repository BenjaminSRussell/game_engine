#include "geometry/mesh_gpu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations for backend integration
// These will be implemented when the specific backend (Metal/Vulkan) is integrated
extern bool metal_create_vertex_buffer(struct metal_device* device, const void* data, u32 size, struct metal_buffer** out_buffer);
extern bool metal_create_index_buffer(struct metal_device* device, const void* data, u32 size, struct metal_buffer** out_buffer);
extern void metal_destroy_buffer(struct metal_buffer* buffer);
extern bool metal_update_buffer(struct metal_buffer* buffer, const void* data, u32 size, u32 offset);

// Internal GPU mesh data structure
typedef struct mesh_gpu_data_t {
    struct metal_buffer* vertex_buffer;
    struct metal_buffer* index_buffer;
    u64 vertex_buffer_size;
    u64 index_buffer_size;
    bool is_uploaded;
    u32 upload_frame;
} mesh_gpu_data_t;

bool mesh_gpu_upload(mesh_t* mesh, struct metal_device* device) {
    if (!mesh || !device) {
        printf("Error: Invalid parameters for mesh_gpu_upload\n");
        return false;
    }
    
    if (mesh->vertex_count == 0 || mesh->index_count == 0) {
        printf("Warning: Attempting to upload empty mesh '%s'\n", mesh->name);
        return false;
    }
    
    // Check if already uploaded
    if (mesh_gpu_is_uploaded(mesh)) {
        printf("Warning: Mesh '%s' already uploaded\n", mesh->name);
        return true;
    }
    
    // Allocate GPU data structure
    mesh_gpu_data_t* gpu_data = (mesh_gpu_data_t*)calloc(1, sizeof(mesh_gpu_data_t));
    if (!gpu_data) {
        printf("Error: Failed to allocate GPU data for mesh '%s'\n", mesh->name);
        return false;
    }
    
    // Calculate buffer sizes
    gpu_data->vertex_buffer_size = mesh->vertex_count * sizeof(vertex_t);
    gpu_data->index_buffer_size = mesh->index_count * sizeof(u32);
    
    // Create vertex buffer
    bool success = metal_create_vertex_buffer(device, mesh->vertices, 
                                            (u32)gpu_data->vertex_buffer_size, 
                                            &gpu_data->vertex_buffer);
    if (!success) {
        printf("Error: Failed to create vertex buffer for mesh '%s'\n", mesh->name);
        free(gpu_data);
        return false;
    }
    
    // Create index buffer
    success = metal_create_index_buffer(device, mesh->indices, 
                                       (u32)gpu_data->index_buffer_size, 
                                       &gpu_data->index_buffer);
    if (!success) {
        printf("Error: Failed to create index buffer for mesh '%s'\n", mesh->name);
        metal_destroy_buffer(gpu_data->vertex_buffer);
        free(gpu_data);
        return false;
    }
    
    // Store GPU data pointers in mesh handles (using a simple approach for now)
    // In a real implementation, these would be proper handles managed by the backend
    mesh->vertex_buffer_handle = (u32)gpu_data->vertex_buffer;
    mesh->index_buffer_handle = (u32)gpu_data->index_buffer;
    
    gpu_data->is_uploaded = true;
    gpu_data->upload_frame = 0; // Would be current frame counter
    
    printf("Successfully uploaded mesh '%s' to GPU (V: %u, I: %u)\n", 
           mesh->name, mesh->vertex_count, mesh->index_count);
    
    // TODO: Store gpu_data somewhere for later cleanup
    // For now, we leak it slightly but this will be fixed with proper handle management
    (void)gpu_data;
    
    return true;
}

void mesh_gpu_unload(mesh_t* mesh) {
    if (!mesh) {
        return;
    }
    
    if (!mesh_gpu_is_uploaded(mesh)) {
        return; // Already unloaded
    }
    
    // TODO: Retrieve gpu_data from storage system
    // For now, we'll directly use the handles
    
    if (mesh->vertex_buffer_handle != 0) {
        struct metal_buffer* vertex_buffer = (struct metal_buffer*)mesh->vertex_buffer_handle;
        metal_destroy_buffer(vertex_buffer);
        mesh->vertex_buffer_handle = 0;
    }
    
    if (mesh->index_buffer_handle != 0) {
        struct metal_buffer* index_buffer = (struct metal_buffer*)mesh->index_buffer_handle;
        metal_destroy_buffer(index_buffer);
        mesh->index_buffer_handle = 0;
    }
    
    printf("Unloaded mesh '%s' from GPU\n", mesh->name);
}

bool mesh_gpu_update(mesh_t* mesh, struct metal_device* device) {
    if (!mesh || !device) {
        printf("Error: Invalid parameters for mesh_gpu_update\n");
        return false;
    }
    
    if (!mesh_gpu_is_uploaded(mesh)) {
        printf("Warning: Attempting to update mesh '%s' that is not uploaded\n", mesh->name);
        return mesh_gpu_upload(mesh, device);
    }
    
    // TODO: Retrieve gpu_data from storage system
    mesh_gpu_data_t* gpu_data = NULL; // Would be retrieved
    
    // Update vertex buffer if needed
    if (mesh->flags & MESH_FLAG_DYNAMIC) {
        struct metal_buffer* vertex_buffer = (struct metal_buffer*)mesh->vertex_buffer_handle;
        if (vertex_buffer && mesh->vertices) {
            u32 vertex_size = mesh->vertex_count * sizeof(vertex_t);
            bool success = metal_update_buffer(vertex_buffer, mesh->vertices, vertex_size, 0);
            if (!success) {
                printf("Error: Failed to update vertex buffer for mesh '%s'\n", mesh->name);
                return false;
            }
        }
        
        struct metal_buffer* index_buffer = (struct metal_buffer*)mesh->index_buffer_handle;
        if (index_buffer && mesh->indices) {
            u32 index_size = mesh->index_count * sizeof(u32);
            bool success = metal_update_buffer(index_buffer, mesh->indices, index_size, 0);
            if (!success) {
                printf("Error: Failed to update index buffer for mesh '%s'\n", mesh->name);
                return false;
            }
        }
    }
    
    return true;
}

// Async upload context
typedef struct mesh_upload_context_t {
    mesh_t* mesh;
    struct metal_device* device;
    mesh_upload_callback_fn callback;
    void* userdata;
} mesh_upload_context_t;

// Background upload function (would run on a thread pool)
static void mesh_upload_background_task(void* context) {
    mesh_upload_context_t* upload_ctx = (mesh_upload_context_t*)context;
    bool success = false;
    
    if (upload_ctx && upload_ctx->mesh && upload_ctx->device) {
        success = mesh_gpu_upload(upload_ctx->mesh, upload_ctx->device);
    }
    
    if (upload_ctx->callback) {
        upload_ctx->callback(upload_ctx->mesh, success, upload_ctx->userdata);
    }
    
    free(upload_ctx);
}

bool mesh_gpu_upload_async(mesh_t* mesh, struct metal_device* device, 
                          mesh_upload_callback_fn callback, void* userdata) {
    if (!mesh || !device) {
        printf("Error: Invalid parameters for mesh_gpu_upload_async\n");
        return false;
    }
    
    // Create upload context
    mesh_upload_context_t* upload_ctx = (mesh_upload_context_t*)malloc(sizeof(mesh_upload_context_t));
    if (!upload_ctx) {
        printf("Error: Failed to allocate upload context\n");
        return false;
    }
    
    upload_ctx->mesh = mesh;
    upload_ctx->device = device;
    upload_ctx->callback = callback;
    upload_ctx->userdata = userdata;
    
    // TODO: Submit to thread pool instead of direct call
    // For now, we'll simulate async by calling it directly
    mesh_upload_background_task(upload_ctx);
    
    return true;
}

u64 mesh_gpu_get_memory_usage(const mesh_t* mesh) {
    if (!mesh || !mesh_gpu_is_uploaded(mesh)) {
        return 0;
    }
    
    u64 vertex_memory = mesh->vertex_count * sizeof(vertex_t);
    u64 index_memory = mesh->index_count * sizeof(u32);
    
    return vertex_memory + index_memory;
}

bool mesh_gpu_is_uploaded(const mesh_t* mesh) {
    if (!mesh) {
        return false;
    }
    
    return (mesh->vertex_buffer_handle != 0 && mesh->index_buffer_handle != 0);
}
