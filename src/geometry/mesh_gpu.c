#include "geometry/mesh_gpu.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"
#include "core/sync/thread_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// GPU data storage system
#define MAX_GPU_MESHES 1024
static mesh_gpu_data_t* gpu_mesh_storage[MAX_GPU_MESHES] = {0};
static u32 gpu_mesh_count = 0;

// Global thread pool for async operations
static ThreadPool* mesh_upload_thread_pool = NULL;

// Initialize thread pool for mesh uploads
static void init_mesh_upload_thread_pool(void) {
    if (!mesh_upload_thread_pool) {
        mesh_upload_thread_pool = thread_pool_create(4); // 4 worker threads
        if (mesh_upload_thread_pool) {
            LOG_INFO("Created mesh upload thread pool with 4 workers");
        } else {
            LOG_ERROR("Failed to create mesh upload thread pool");
        }
    }
}

// Cleanup thread pool
static void cleanup_mesh_upload_thread_pool(void) {
    if (mesh_upload_thread_pool) {
        thread_pool_wait(mesh_upload_thread_pool); // Wait for all pending uploads
        thread_pool_destroy(mesh_upload_thread_pool);
        mesh_upload_thread_pool = NULL;
        LOG_INFO("Destroyed mesh upload thread pool");
    }
}

// Helper functions for GPU data storage
static u32 store_gpu_data(mesh_gpu_data_t* data) {
    if (!data) return 0;
    
    // Find empty slot
    for (u32 i = 0; i < MAX_GPU_MESHES; i++) {
        if (gpu_mesh_storage[i] == NULL) {
            gpu_mesh_storage[i] = data;
            gpu_mesh_count++;
            LOG_INFO("Stored GPU data at slot %u (total: %u)", i, gpu_mesh_count);
            return i + 1; // Return non-zero handle
        }
    }
    
    LOG_ERROR("GPU mesh storage full - cannot store mesh data");
    return 0;
}

static mesh_gpu_data_t* retrieve_gpu_data(u32 handle) {
    if (handle == 0 || handle > MAX_GPU_MESHES) {
        return NULL;
    }
    
    u32 index = handle - 1;
    return gpu_mesh_storage[index];
}

static void remove_gpu_data(u32 handle) {
    if (handle == 0 || handle > MAX_GPU_MESHES) {
        return;
    }
    
    u32 index = handle - 1;
    if (gpu_mesh_storage[index]) {
        free(gpu_mesh_storage[index]);
        gpu_mesh_storage[index] = NULL;
        gpu_mesh_count--;
        LOG_INFO("Removed GPU data from slot %u (total: %u)", index, gpu_mesh_count);
    }
}

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
    
    // Store GPU data pointers in mesh handles and storage system
    u32 gpu_handle = store_gpu_data(gpu_data);
    if (gpu_handle == 0) {
        printf("Error: Failed to store GPU data for mesh '%s'\n", mesh->name);
        metal_destroy_buffer(gpu_data->vertex_buffer);
        metal_destroy_buffer(gpu_data->index_buffer);
        free(gpu_data);
        return false;
    }
    
    mesh->vertex_buffer_handle = gpu_handle;
    mesh->index_buffer_handle = gpu_handle; // Use same handle for both buffers
    
    gpu_data->is_uploaded = true;
    gpu_data->upload_frame = 0; // Would be current frame counter
    
    printf("Successfully uploaded mesh '%s' to GPU (V: %u, I: %u)\n", 
           mesh->name, mesh->vertex_count, mesh->index_count);
    
    return true;
}

void mesh_gpu_unload(mesh_t* mesh) {
    if (!mesh) {
        return;
    }
    
    if (!mesh_gpu_is_uploaded(mesh)) {
        return; // Already unloaded
    }
    
    // Retrieve GPU data from storage system
    mesh_gpu_data_t* gpu_data = retrieve_gpu_data(mesh->vertex_buffer_handle);
    if (!gpu_data) {
        printf("Warning: Could not retrieve GPU data for mesh '%s'\n", mesh->name);
        return;
    }
    
    // Destroy vertex buffer
    if (gpu_data->vertex_buffer) {
        metal_destroy_buffer(gpu_data->vertex_buffer);
        gpu_data->vertex_buffer = NULL;
    }
    
    // Destroy index buffer
    if (gpu_data->index_buffer) {
        metal_destroy_buffer(gpu_data->index_buffer);
        gpu_data->index_buffer = NULL;
    }
    
    // Remove from storage system
    remove_gpu_data(mesh->vertex_buffer_handle);
    
    // Clear handles
    mesh->vertex_buffer_handle = 0;
    mesh->index_buffer_handle = 0;
    
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
    
    // Retrieve GPU data from storage system
    mesh_gpu_data_t* gpu_data = retrieve_gpu_data(mesh->vertex_buffer_handle);
    if (!gpu_data) {
        printf("Warning: Could not retrieve GPU data for mesh '%s' during update\n", mesh->name);
        return false;
    }
    
    // Update vertex buffer if needed
    if (mesh->flags & MESH_FLAG_DYNAMIC) {
        if (gpu_data->vertex_buffer && mesh->vertices) {
            u32 vertex_size = mesh->vertex_count * sizeof(vertex_t);
            bool success = metal_update_buffer(gpu_data->vertex_buffer, mesh->vertices, vertex_size, 0);
            if (!success) {
                printf("Error: Failed to update vertex buffer for mesh '%s'\n", mesh->name);
                return false;
            }
        }
        
        if (gpu_data->index_buffer && mesh->indices) {
            u32 index_size = mesh->index_count * sizeof(u32);
            bool success = metal_update_buffer(gpu_data->index_buffer, mesh->indices, index_size, 0);
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
    
    // Initialize thread pool if needed
    init_mesh_upload_thread_pool();
    if (!mesh_upload_thread_pool) {
        printf("Error: Thread pool not available for async upload\n");
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
    
    // Submit to thread pool for async execution
    thread_pool_submit(mesh_upload_thread_pool, mesh_upload_background_task, upload_ctx);
    
    LOG_INFO("Submitted mesh '%s' for async upload to thread pool", mesh->name);
    
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

// Cleanup function to be called during engine shutdown
void mesh_gpu_cleanup(void) {
    // Clean up any remaining GPU data
    for (u32 i = 0; i < MAX_GPU_MESHES; i++) {
        if (gpu_mesh_storage[i]) {
            printf("Warning: Cleaning up leaked GPU data at slot %u\n", i);
            free(gpu_mesh_storage[i]);
            gpu_mesh_storage[i] = NULL;
        }
    }
    gpu_mesh_count = 0;
    
    // Cleanup thread pool
    cleanup_mesh_upload_thread_pool();
    
    LOG_INFO("Mesh GPU system cleanup completed");
}
