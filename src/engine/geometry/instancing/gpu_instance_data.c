/*
 * gpu_instance_data.c
 * GPU-side instance storage implementation
 */

#include "gpu_instance_data.h"
#include <core/logger.h>
#include <core/memory.h>
#include <renderer/vulkan.h> // Assuming Vulkan backend usage for SSBOs
#include <string.h>

#define MAX_GPU_INSTANCE_BUFFERS 256
#define DEFAULT_INSTANCE_CAPACITY 1024

typedef struct {
    uint32_t id;
    uint32_t capacity;
    uint32_t count;
    bool active;
    
    // GPU Resources
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped_ptr;
    bool dirty;
} GPUInstanceBuffer;

static struct {
    GPUInstanceBuffer buffers[MAX_GPU_INSTANCE_BUFFERS];
    uint32_t active_count;
    bool initialized;
    VulkanRenderer* renderer; // Cached reference
} g_instance_data_system;

// Helper to get renderer
static VulkanRenderer* get_renderer() {
    // In a real system, this might be passed in init or retrieved from a global registry
    extern VulkanRenderer* g_renderer; 
    return g_renderer;
}

int geometry_gpu_instance_data_init(void) {
    if (g_instance_data_system.initialized) return 0;
    
    memset(&g_instance_data_system, 0, sizeof(g_instance_data_system));
    g_instance_data_system.renderer = get_renderer();
    g_instance_data_system.initialized = true;
    
    LOG_INFO("GPU Instance Data System initialized");
    return 0;
}

void geometry_gpu_instance_data_shutdown(void) {
    if (!g_instance_data_system.initialized) return;
    
    for (int i = 0; i < MAX_GPU_INSTANCE_BUFFERS; ++i) {
        if (g_instance_data_system.buffers[i].active) {
            geometry_gpu_instance_data_handle_t handle = { (uint32_t)i };
            geometry_gpu_instance_data_destroy(handle);
        }
    }
    
    g_instance_data_system.initialized = false;
}

int geometry_gpu_instance_data_create(geometry_gpu_instance_data_handle_t* out_handle, const geometry_gpu_instance_data_desc_t* desc) {
    if (!g_instance_data_system.initialized) return -1;
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_GPU_INSTANCE_BUFFERS; ++i) {
        if (!g_instance_data_system.buffers[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        LOG_ERROR("Max GPU instance buffers reached");
        return -2;
    }
    
    GPUInstanceBuffer* buf = &g_instance_data_system.buffers[slot];
    buf->active = true;
    buf->capacity = desc->initial_capacity > 0 ? desc->initial_capacity : DEFAULT_INSTANCE_CAPACITY;
    buf->count = 0;
    buf->id = slot;
    
    // Create GPU Buffer (SSBO)
    if (g_instance_data_system.renderer) {
        VkDeviceSize size = buf->capacity * sizeof(GPUInstance);
        if (!vulkan_create_buffer(g_instance_data_system.renderer, size, 
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buf->buffer, &buf->memory)) {
            LOG_ERROR("Failed to create GPU instance buffer");
            buf->active = false;
            return -3;
        }
    }
    
    out_handle->id = slot;
    g_instance_data_system.active_count++;
    
    return 0;
}

void geometry_gpu_instance_data_destroy(geometry_gpu_instance_data_handle_t handle) {
    if (handle.id >= MAX_GPU_INSTANCE_BUFFERS) return;
    GPUInstanceBuffer* buf = &g_instance_data_system.buffers[handle.id];
    
    if (buf->active) {
        if (g_instance_data_system.renderer && buf->buffer) {
            vkDestroyBuffer(g_instance_data_system.renderer->device, buf->buffer, NULL);
            vkFreeMemory(g_instance_data_system.renderer->device, buf->memory, NULL);
        }
        memset(buf, 0, sizeof(GPUInstanceBuffer));
        g_instance_data_system.active_count--;
    }
}

int geometry_gpu_instance_data_update(geometry_gpu_instance_data_handle_t handle, const void* data, size_t count) {
    if (handle.id >= MAX_GPU_INSTANCE_BUFFERS) return -1;
    GPUInstanceBuffer* buf = &g_instance_data_system.buffers[handle.id];
    if (!buf->active) return -1;
    
    if (count > buf->capacity) {
        // Resize needed logic here (omitted for brevity, assume sufficient capacity or user handles header)
        // Ideally should recreate buffer and copy
        LOG_WARN("Instance count %zu exceeds capacity %u, truncating", count, buf->capacity);
        count = buf->capacity;
    }
    
    if (g_instance_data_system.renderer && buf->memory) {
        void* mapped;
        vkMapMemory(g_instance_data_system.renderer->device, buf->memory, 0, count * sizeof(GPUInstance), 0, &mapped);
        memcpy(mapped, data, count * sizeof(GPUInstance));
        vkUnmapMemory(g_instance_data_system.renderer->device, buf->memory);
    }
    
    buf->count = (uint32_t)count;
    buf->dirty = false;
    
    return 0;
}

bool geometry_gpu_instance_data_is_valid(geometry_gpu_instance_data_handle_t handle) {
    if (handle.id >= MAX_GPU_INSTANCE_BUFFERS) return false;
    return g_instance_data_system.buffers[handle.id].active;
}

uint32_t geometry_gpu_instance_data_get_buffer_id(geometry_gpu_instance_data_handle_t handle) {
     if (handle.id >= MAX_GPU_INSTANCE_BUFFERS) return 0;
     // Return a handle/ID that the renderer understands, or cast pointer if unsafe
     return handle.id; // Placeholder
}
