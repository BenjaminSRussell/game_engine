#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan pipeline cache implementation
#define VK_MAX_PIPELINE_CACHE_ENTRIES 1024
#define VK_MAX_PIPELINE_CACHE_SIZE (64 * 1024 * 1024) // 64MB

typedef struct vk_pipeline_cache_entry {
    u32 hash;
    VkPipeline pipeline;
    VkGraphicsPipelineCreateInfo create_info;
    u64 last_used;
    u32 use_count;
    bool is_valid;
} vk_pipeline_cache_entry_t;

typedef struct vk_pipeline_cache {
    VkPipelineCache handle;
    VkDevice device;
    
    vk_pipeline_cache_entry_t entries[VK_MAX_PIPELINE_CACHE_ENTRIES];
    u32 entry_count;
    u32 next_free_entry;
    
    u64 total_size;
    u32 total_pipelines_created;
    u32 total_pipelines_destroyed;
    u32 cache_hits;
    u32 cache_misses;
    
    bool is_initialized;
} vk_pipeline_cache_t;

static vk_pipeline_cache_t g_pipeline_cache = {0};

// Simple hash function for pipeline data
static u32 hash_pipeline_data(const VkGraphicsPipelineCreateInfo* create_info) {
    u32 hash = 5381;
    const u8* data = (const u8*)create_info;
    
    for (size_t i = 0; i < sizeof(VkGraphicsPipelineCreateInfo); i++) {
        hash = ((hash << 5) + hash) + data[i];
    }
    
    return hash;
}

// Initialize pipeline cache
bool vk_pipeline_cache_init(VkDevice device) {
    if (!device) {
        printf("Error: Invalid device for pipeline cache initialization\n");
        return false;
    }
    
    if (g_pipeline_cache.is_initialized) {
        printf("Warning: Pipeline cache already initialized\n");
        return true;
    }
    
    g_pipeline_cache.device = device;
    
    // Create pipeline cache
    VkPipelineCacheCreateInfo cache_info = {0};
    cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cache_info.initialDataSize = 0;
    cache_info.pInitialData = NULL;
    
    VkResult result = vkCreatePipelineCache(device, &cache_info, NULL, &g_pipeline_cache.handle);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create pipeline cache\n");
        return false;
    }
    
    g_pipeline_cache.entry_count = 0;
    g_pipeline_cache.next_free_entry = 0;
    g_pipeline_cache.total_size = 0;
    g_pipeline_cache.total_pipelines_created = 0;
    g_pipeline_cache.total_pipelines_destroyed = 0;
    g_pipeline_cache.cache_hits = 0;
    g_pipeline_cache.cache_misses = 0;
    g_pipeline_cache.is_initialized = true;
    
    printf("Vulkan pipeline cache initialized\n");
    return true;
}

// Cleanup pipeline cache
void vk_pipeline_cache_cleanup(void) {
    if (!g_pipeline_cache.is_initialized) {
        return;
    }
    
    // Destroy all cached pipelines
    for (u32 i = 0; i < g_pipeline_cache.entry_count; i++) {
        if (g_pipeline_cache.entries[i].is_valid && g_pipeline_cache.entries[i].pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(g_pipeline_cache.device, g_pipeline_cache.entries[i].pipeline, NULL);
        }
    }
    
    // Destroy pipeline cache
    if (g_pipeline_cache.handle != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(g_pipeline_cache.device, g_pipeline_cache.handle, NULL);
    }
    
    memset(&g_pipeline_cache, 0, sizeof(g_pipeline_cache));
    
    printf("Vulkan pipeline cache cleaned up\n");
}

// Create graphics pipeline with caching
VkPipeline vk_pipeline_cache_create_graphics(const VkGraphicsPipelineCreateInfo* create_info, bool* was_cached) {
    if (!g_pipeline_cache.is_initialized || !create_info) {
        return VK_NULL_HANDLE;
    }
    
    if (was_cached) *was_cached = false;
    
    // Calculate hash
    u32 hash = hash_pipeline_data(create_info);
    
    // Check cache
    for (u32 i = 0; i < g_pipeline_cache.entry_count; i++) {
        if (g_pipeline_cache.entries[i].is_valid && g_pipeline_cache.entries[i].hash == hash) {
            g_pipeline_cache.entries[i].last_used = 0;
            g_pipeline_cache.entries[i].use_count++;
            g_pipeline_cache.cache_hits++;
            
            if (was_cached) *was_cached = true;
            
            printf("Pipeline cache hit (hash: %u, uses: %u)\n", hash, g_pipeline_cache.entries[i].use_count);
            return g_pipeline_cache.entries[i].pipeline;
        }
    }
    
    // Cache miss - create new pipeline
    VkPipeline pipeline;
    VkResult result = vkCreateGraphicsPipelines(g_pipeline_cache.device, g_pipeline_cache.handle, 1, create_info, NULL, &pipeline);
    
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create graphics pipeline\n");
        return VK_NULL_HANDLE;
    }
    
    // Add to cache
    u32 entry_index = g_pipeline_cache.next_free_entry;
    if (entry_index >= VK_MAX_PIPELINE_CACHE_ENTRIES) {
        entry_index = 0; // Simple wrap-around
    }
    
    vk_pipeline_cache_entry_t* entry = &g_pipeline_cache.entries[entry_index];
    
    // Destroy old pipeline if valid
    if (entry->is_valid && entry->pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_pipeline_cache.device, entry->pipeline, NULL);
        g_pipeline_cache.total_pipelines_destroyed++;
    }
    
    entry->hash = hash;
    entry->pipeline = pipeline;
    entry->create_info = *create_info;
    entry->last_used = 0;
    entry->use_count = 1;
    entry->is_valid = true;
    
    g_pipeline_cache.next_free_entry = (entry_index + 1) % VK_MAX_PIPELINE_CACHE_ENTRIES;
    if (g_pipeline_cache.next_free_entry > g_pipeline_cache.entry_count) {
        g_pipeline_cache.entry_count = g_pipeline_cache.next_free_entry;
    }
    
    g_pipeline_cache.total_pipelines_created++;
    g_pipeline_cache.cache_misses++;
    
    printf("Created graphics pipeline (hash: %u, cache size: %u)\n", hash, g_pipeline_cache.entry_count);
    
    return pipeline;
}

// Get cache statistics
void vk_pipeline_cache_get_stats(u32* total_pipelines_created, u32* total_pipelines_destroyed, u32* cache_hits, u32* cache_misses) {
    if (total_pipelines_created) *total_pipelines_created = g_pipeline_cache.total_pipelines_created;
    if (total_pipelines_destroyed) *total_pipelines_destroyed = g_pipeline_cache.total_pipelines_destroyed;
    if (cache_hits) *cache_hits = g_pipeline_cache.cache_hits;
    if (cache_misses) *cache_misses = g_pipeline_cache.cache_misses;
}

// Get cache hit ratio
f32 vk_pipeline_cache_get_hit_ratio(void) {
    if (!g_pipeline_cache.is_initialized) {
        return 0.0f;
    }
    
    u32 total_requests = g_pipeline_cache.cache_hits + g_pipeline_cache.cache_misses;
    if (total_requests == 0) {
        return 0.0f;
    }
    
    return (f32)g_pipeline_cache.cache_hits / (f32)total_requests;
}

// Get pipeline cache handle
VkPipelineCache vk_pipeline_cache_get_handle(void) {
    if (!g_pipeline_cache.is_initialized) {
        return VK_NULL_HANDLE;
    }
    
    return g_pipeline_cache.handle;
}