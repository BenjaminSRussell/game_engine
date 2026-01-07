/*
 * indirect_instancing.c
 * Indirect draw instancing implementation
 */

#include "indirect_instancing.h"
#include <core/logger.h>
#include <core/memory.h>
#include <renderer/vulkan.h>
#include <string.h>

#define MAX_INDIRECT_BATCHES 64
#define DEFAULT_MAX_DRAWS 1024

typedef struct {
    uint32_t id;
    uint32_t max_draws;
    uint32_t current_draw_count;
    bool active;
    
    // CPU Mirror
    IndirectDrawCommand* commands;
    
    // GPU Resources
    VkBuffer cmd_buffer;
    VkDeviceMemory cmd_memory;
    
    VkBuffer count_buffer; // Optional, for count in MDI
    VkDeviceMemory count_memory;
} IndirectBatch;

static struct {
    IndirectBatch batches[MAX_INDIRECT_BATCHES];
    VulkanRenderer* renderer;
    bool initialized;
} g_indirect_system;

static VulkanRenderer* get_renderer() {
    extern VulkanRenderer* g_renderer;
    return g_renderer;
}

int geometry_indirect_instancing_init(void) {
    if (g_indirect_system.initialized) return 0;
    memset(&g_indirect_system, 0, sizeof(g_indirect_system));
    g_indirect_system.renderer = get_renderer();
    g_indirect_system.initialized = true;
    return 0;
}

void geometry_indirect_instancing_shutdown(void) {
    if (!g_indirect_system.initialized) return;
    for (int i = 0; i < MAX_INDIRECT_BATCHES; ++i) {
        if (g_indirect_system.batches[i].active) {
            geometry_indirect_instancing_handle_t h = { (uint32_t)i };
            geometry_indirect_instancing_destroy(h);
        }
    }
    g_indirect_system.initialized = false;
}

int geometry_indirect_instancing_create(geometry_indirect_instancing_handle_t* out_handle, const geometry_indirect_instancing_desc_t* desc) {
    if (!g_indirect_system.initialized) return -1;
    
    int slot = -1;
    for (int i = 0; i < MAX_INDIRECT_BATCHES; ++i) {
        if (!g_indirect_system.batches[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        LOG_ERROR("Max indirect batches reached");
        return -2;
    }
    
    IndirectBatch* batch = &g_indirect_system.batches[slot];
    batch->active = true;
    batch->id = slot;
    batch->max_draws = desc->max_draws > 0 ? desc->max_draws : DEFAULT_MAX_DRAWS;
    batch->current_draw_count = 0;
    
    // Allocate CPU mirror
    batch->commands = (IndirectDrawCommand*)malloc(batch->max_draws * sizeof(IndirectDrawCommand));
    if (!batch->commands) {
        LOG_ERROR("Failed to allocate indirect commands CPU buffer");
        batch->active = false;
        return -3;
    }
    
    // Create GPU Buffer (Indirect Buffer)
    if (g_indirect_system.renderer) {
        VkDeviceSize size = batch->max_draws * sizeof(IndirectDrawCommand);
        // Usage: INDIRECT_BUFFER | STORAGE_BUFFER (for compute culling write) | TRANSFER_DST
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | 
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                                   
        if (!vulkan_create_buffer(g_indirect_system.renderer, size, usage,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &batch->cmd_buffer, &batch->cmd_memory)) {
            LOG_ERROR("Failed to create GPU indirect buffer");
            free(batch->commands);
            batch->active = false;
            return -4;
        }
        
        // Create Count Buffer
        // Just a single uint32
        if (!vulkan_create_buffer(g_indirect_system.renderer, sizeof(uint32_t),
                                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &batch->count_buffer, &batch->count_memory)) {
             // Not fatal, but logged
             LOG_WARN("Failed to create indirect count buffer");
        }
    }
    
    out_handle->id = slot;
    return 0;
}

void geometry_indirect_instancing_destroy(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= MAX_INDIRECT_BATCHES) return;
    IndirectBatch* batch = &g_indirect_system.batches[handle.id];
    
    if (batch->active) {
        if (batch->commands) free(batch->commands);
        if (g_indirect_system.renderer) {
            if (batch->cmd_buffer) {
                vkDestroyBuffer(g_indirect_system.renderer->device, batch->cmd_buffer, NULL);
                vkFreeMemory(g_indirect_system.renderer->device, batch->cmd_memory, NULL);
            }
            if (batch->count_buffer) {
                vkDestroyBuffer(g_indirect_system.renderer->device, batch->count_buffer, NULL);
                vkFreeMemory(g_indirect_system.renderer->device, batch->count_memory, NULL);
            }
        }
        memset(batch, 0, sizeof(IndirectBatch));
    }
}

int geometry_indirect_instancing_add_command(geometry_indirect_instancing_handle_t handle, 
                                             const IndirectDrawCommand* command) {
    if (handle.id >= MAX_INDIRECT_BATCHES) return -1;
    IndirectBatch* batch = &g_indirect_system.batches[handle.id];
    if (!batch->active) return -1;
    
    if (batch->current_draw_count >= batch->max_draws) {
        LOG_WARN("Max draws reached for batch %d", handle.id);
        return -1;
    }
    
    batch->commands[batch->current_draw_count++] = *command;
    return 0;
}

void geometry_indirect_instancing_reset(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= MAX_INDIRECT_BATCHES) return;
    // Just reset the count, don't clear memory to save time
    g_indirect_system.batches[handle.id].current_draw_count = 0;
}

int geometry_indirect_instancing_upload(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= MAX_INDIRECT_BATCHES) return -1;
    IndirectBatch* batch = &g_indirect_system.batches[handle.id];
    if (!batch->active) return -1;
    
    if (g_indirect_system.renderer && batch->cmd_memory) {
        void* mapped;
        VkDeviceSize size = batch->current_draw_count * sizeof(IndirectDrawCommand);
        if (size > 0) {
            vkMapMemory(g_indirect_system.renderer->device, batch->cmd_memory, 0, size, 0, &mapped);
            memcpy(mapped, batch->commands, size);
            vkUnmapMemory(g_indirect_system.renderer->device, batch->cmd_memory);
        }
        
        // Also update count buffer
        if (batch->count_memory) {
            void* count_mapped;
            vkMapMemory(g_indirect_system.renderer->device, batch->count_memory, 0, sizeof(uint32_t), 0, &count_mapped);
            *(uint32_t*)count_mapped = batch->current_draw_count;
            vkUnmapMemory(g_indirect_system.renderer->device, batch->count_memory);
        }
    }
    return 0;
}

uint32_t geometry_indirect_instancing_get_buffer_id(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= MAX_INDIRECT_BATCHES) return 0;
    // Check if we need to return the buffer itself as a pointer cast to uint64/size_t or internal handle
    // For now assuming the caller knows how to retrieve the VkBuffer from the system or we'll add a getter for VkBuffer.
    return handle.id;
}

uint32_t geometry_indirect_instancing_get_draw_count(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= MAX_INDIRECT_BATCHES) return 0;
    return g_indirect_system.batches[handle.id].current_draw_count;
}

// Get actual VkBuffer handle for indirect commands
VkBuffer geometry_indirect_instancing_get_cmd_buffer(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= MAX_INDIRECT_BATCHES) return VK_NULL_HANDLE;
    return g_indirect_system.batches[handle.id].cmd_buffer;
}

// Get actual VkBuffer handle for count buffer
VkBuffer geometry_indirect_instancing_get_count_buffer(geometry_indirect_instancing_handle_t handle) {
    if (handle.id >= MAX_INDIRECT_BATCHES) return VK_NULL_HANDLE;
    return g_indirect_system.batches[handle.id].count_buffer;
}
