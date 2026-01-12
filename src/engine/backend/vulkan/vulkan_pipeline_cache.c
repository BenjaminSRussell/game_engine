// Vulkan Pipeline Caching Implementation
// VULKAN-PL-001: Implement pipeline caching system for performance

#include <rendering/vulkan.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
#include <include/rendering/vulkan.h>
#else
// Stub constants when Vulkan is not available
#define VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO 3
#define VK_PIPELINE_CACHE_HEADER_VERSION_ONE 1
typedef void* VkPipelineCache;
typedef void* VkPipeline;
#endif

// VULKAN-PL-001: Pipeline Caching System Implementation
// ===================================================

// Pipeline cache entry structure
typedef struct PipelineCacheEntry {
    char pipeline_hash[64];  // SHA-256 hash of pipeline description
    VkPipeline pipeline;
    u32 size;
    u32 access_count;
    u64 last_access_time;
    struct PipelineCacheEntry* next;
} PipelineCacheEntry;

// Pipeline cache structure
typedef struct PipelineCache {
    PipelineCacheEntry* entries;
    VkPipelineCache vk_cache;
    u32 entry_count;
    u32 max_entries;
    u64 total_size;
    u64 cache_hits;
    u64 cache_misses;
    bool persistent_cache_enabled;
    char cache_file_path[256];
} PipelineCache;

// Global pipeline cache
static PipelineCache g_pipeline_cache = {0};

// Simple hash function for pipeline descriptions
static void hash_pipeline_description(const void* description, u32 size, char* out_hash) {
    // Simple hash implementation - in production, use SHA-256
    u32 hash = 5381;
    const u8* data = (const u8*)description;
    
    for (u32 i = 0; i < size; i++) {
        hash = ((hash << 5) + hash) + data[i];
    }
    
    // Convert to hex string
    for (int i = 0; i < 16; i++) {
    }
    out_hash[32] = '\0';
}

// Initialize pipeline cache
bool vulkan_pipeline_cache_init(VulkanRenderer* renderer, const char* cache_file_path, bool persistent) {
#ifdef VULKAN_BUILD
    if (!renderer || !renderer->device) {
        printf("Vulkan: Invalid renderer for pipeline cache initialization\n");
        return false;
    }
    
    printf("Vulkan: Initializing pipeline caching system\n");
    
    // Initialize cache structure
    memset(&g_pipeline_cache, 0, sizeof(PipelineCache));
    g_pipeline_cache.max_entries = 1024;  // Default max entries
    g_pipeline_cache.persistent_cache_enabled = persistent;
    
    if (cache_file_path) {
        strncpy(g_pipeline_cache.cache_file_path, cache_file_path, sizeof(g_pipeline_cache.cache_file_path) - 1);
    }
    
    // Create Vulkan pipeline cache
    VkPipelineCacheCreateInfo cache_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .initialDataSize = 0,
        .pInitialData = NULL
    };
    
    // Load persistent cache data if available
    void* cache_data = NULL;
    size_t cache_size = 0;
    
    if (persistent && cache_file_path) {
        FILE* file = fopen(cache_file_path, "rb");
        if (file) {
            // Get file size
            fseek(file, 0, SEEK_END);
            cache_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            if (cache_size > 0) {
                cache_data = malloc(cache_size);
                if (cache_data && fread(cache_data, 1, cache_size, file) == cache_size) {
                    printf("Vulkan: Loaded %zu bytes of pipeline cache data\n", cache_size);
                    cache_info.initialDataSize = cache_size;
                    cache_info.pInitialData = cache_data;
                } else {
                    printf("Vulkan: Failed to read pipeline cache data\n");
                    if (cache_data) {
                        free(cache_data);
                        cache_data = NULL;
                    }
                }
            }
            fclose(file);
        }
    }
    
    VkResult result = vkCreatePipelineCache(renderer->device, &cache_info, NULL, &g_pipeline_cache.vk_cache);
    
    if (cache_data) {
        free(cache_data);
    }
    
    if (result != VK_SUCCESS) {
        printf("Vulkan: Failed to create pipeline cache: %d\n", result);
        return false;
    }
    
    // Allocate hash table entries
    g_pipeline_cache.entries = calloc(g_pipeline_cache.max_entries, sizeof(PipelineCacheEntry));
    if (!g_pipeline_cache.entries) {
        printf("Vulkan: Failed to allocate pipeline cache entries\n");
        vkDestroyPipelineCache(renderer->device, g_pipeline_cache.vk_cache, NULL);
        return false;
    }
    
    printf("Vulkan: Pipeline cache initialized successfully\n");
    printf("  Max entries: %u\n", g_pipeline_cache.max_entries);
    printf("  Persistent cache: %s\n", persistent ? "enabled" : "disabled");
    if (persistent && cache_file_path) {
        printf("  Cache file: %s\n", cache_file_path);
    }
    
    return true;
#else
    (void)renderer;
    (void)cache_file_path;
    (void)persistent;
    return false;
#endif
}

// Find pipeline in cache
static PipelineCacheEntry* find_pipeline_in_cache(const char* pipeline_hash) {
    if (!pipeline_hash || !g_pipeline_cache.entries) return NULL;
    
    u32 hash_index = 0;
    for (int i = 0; i < 8; i++) {
        hash_index = (hash_index << 4) + (pipeline_hash[i] - '0');
    }
    hash_index %= g_pipeline_cache.max_entries;
    
    // Linear probe for collision resolution
    for (u32 i = 0; i < g_pipeline_cache.max_entries; i++) {
        u32 index = (hash_index + i) % g_pipeline_cache.max_entries;
        PipelineCacheEntry* entry = &g_pipeline_cache.entries[index];
        
        if (entry->pipeline == VK_NULL_HANDLE) {
            return NULL;  // Empty slot
        }
        
        if (strncmp(entry->pipeline_hash, pipeline_hash, 64) == 0) {
            return entry;  // Found match
        }
    }
    
    return NULL;
}

// Add pipeline to cache
static bool add_pipeline_to_cache(const char* pipeline_hash, VkPipeline pipeline, u32 size) {
    if (!pipeline_hash || pipeline == VK_NULL_HANDLE || !g_pipeline_cache.entries) return false;
    
    u32 hash_index = 0;
    for (int i = 0; i < 8; i++) {
        hash_index = (hash_index << 4) + (pipeline_hash[i] - '0');
    }
    hash_index %= g_pipeline_cache.max_entries;
    
    // Linear probe for collision resolution
    for (u32 i = 0; i < g_pipeline_cache.max_entries; i++) {
        u32 index = (hash_index + i) % g_pipeline_cache.max_entries;
        PipelineCacheEntry* entry = &g_pipeline_cache.entries[index];
        
        if (entry->pipeline == VK_NULL_HANDLE) {
            // Empty slot - add pipeline
            strncpy(entry->pipeline_hash, pipeline_hash, 63);
            entry->pipeline_hash[63] = '\0';
            entry->pipeline = pipeline;
            entry->size = size;
            entry->access_count = 1;
            entry->last_access_time = 0;  // Would use actual timestamp
            entry->next = NULL;
            
            g_pipeline_cache.entry_count++;
            g_pipeline_cache.total_size += size;
            
            printf("Vulkan: Added pipeline to cache (hash: %.8s...)\n", pipeline_hash);
            return true;
        }
    }
    
    printf("Vulkan: Pipeline cache full - cannot add pipeline\n");
    return false;
}

// Get or create pipeline with caching
VkPipeline vulkan_get_or_create_pipeline(VulkanRenderer* renderer, 
                                         const void* pipeline_description, 
                                         u32 description_size,
                                         VkPipeline (*create_pipeline_func)(VulkanRenderer*, const void*)) {
#ifdef VULKAN_BUILD
    if (!renderer || !pipeline_description || !create_pipeline_func) {
        return VK_NULL_HANDLE;
    }
    
    // Generate hash for pipeline description
    char pipeline_hash[64];
    hash_pipeline_description(pipeline_description, description_size, pipeline_hash);
    
    // Check cache
    PipelineCacheEntry* entry = find_pipeline_in_cache(pipeline_hash);
    if (entry) {
        // Cache hit
        entry->access_count++;
        entry->last_access_time = 0;  // Would use actual timestamp
        g_pipeline_cache.cache_hits++;
        
        printf("Vulkan: Pipeline cache hit (hash: %.8s...)\n", pipeline_hash);
        return entry->pipeline;
    }
    
    // Cache miss - create new pipeline
    g_pipeline_cache.cache_misses++;
    
    printf("Vulkan: Pipeline cache miss (hash: %.8s...) - creating new pipeline\n", pipeline_hash);
    
    VkPipeline pipeline = create_pipeline_func(renderer, pipeline_description);
    if (pipeline != VK_NULL_HANDLE) {
        // Add to cache
        add_pipeline_to_cache(pipeline_hash, pipeline, description_size);
    }
    
    return pipeline;
#else
    (void)renderer;
    (void)pipeline_description;
    (void)description_size;
    (void)create_pipeline_func;
    return VK_NULL_HANDLE;
#endif
}

// Save pipeline cache to file
bool vulkan_pipeline_cache_save(VulkanRenderer* renderer) {
#ifdef VULKAN_BUILD
    if (!renderer || !g_pipeline_cache.persistent_cache_enabled || !g_pipeline_cache.cache_file_path[0]) {
        printf("Vulkan: Persistent cache not enabled or no cache file specified\n");
        return false;
    }
    
    // Get cache data size
    size_t cache_size = 0;
    VkResult result = vkGetPipelineCacheData(renderer->device, g_pipeline_cache.vk_cache, &cache_size, NULL);
    if (result != VK_SUCCESS) {
        printf("Vulkan: Failed to get pipeline cache data size: %d\n", result);
        return false;
    }
    
    if (cache_size == 0) {
        printf("Vulkan: No pipeline cache data to save\n");
        return true;
    }
    
    // Allocate buffer for cache data
    void* cache_data = malloc(cache_size);
    if (!cache_data) {
        printf("Vulkan: Failed to allocate memory for pipeline cache data\n");
        return false;
    }
    
    // Get cache data
    result = vkGetPipelineCacheData(renderer->device, g_pipeline_cache.vk_cache, &cache_size, cache_data);
    if (result != VK_SUCCESS) {
        printf("Vulkan: Failed to get pipeline cache data: %d\n", result);
        free(cache_data);
        return false;
    }
    
    // Write to file
    FILE* file = fopen(g_pipeline_cache.cache_file_path, "wb");
    if (!file) {
        printf("Vulkan: Failed to open pipeline cache file for writing: %s\n", g_pipeline_cache.cache_file_path);
        free(cache_data);
        return false;
    }
    
    bool success = (fwrite(cache_data, 1, cache_size, file) == cache_size);
    fclose(file);
    
    if (success) {
        printf("Vulkan: Saved %zu bytes of pipeline cache data to %s\n", cache_size, g_pipeline_cache.cache_file_path);
    } else {
        printf("Vulkan: Failed to write pipeline cache data to file\n");
    }
    
    free(cache_data);
    return success;
#else
    (void)renderer;
    return false;
#endif
}

// Get pipeline cache statistics
void vulkan_pipeline_cache_get_stats(u32* entry_count, u64* cache_hits, u64* cache_misses, f32* hit_ratio) {
    if (entry_count) *entry_count = g_pipeline_cache.entry_count;
    if (cache_hits) *cache_hits = g_pipeline_cache.cache_hits;
    if (cache_misses) *cache_misses = g_pipeline_cache.cache_misses;
    
    if (hit_ratio) {
        u64 total_requests = g_pipeline_cache.cache_hits + g_pipeline_cache.cache_misses;
        *hit_ratio = total_requests > 0 ? (f32)g_pipeline_cache.cache_hits / total_requests : 0.0f;
    }
}

// Print pipeline cache statistics
void vulkan_pipeline_cache_print_stats() {
    printf("=== Vulkan Pipeline Cache Statistics ===\n");
    printf("Cache entries: %u / %u\n", g_pipeline_cache.entry_count, g_pipeline_cache.max_entries);
    printf("Total cache size: %.2f MB\n", g_pipeline_cache.total_size / (1024.0f * 1024.0f));
    printf("Cache hits: %llu\n", (unsigned long long)g_pipeline_cache.cache_hits);
    printf("Cache misses: %llu\n", (unsigned long long)g_pipeline_cache.cache_misses);
    
    u64 total_requests = g_pipeline_cache.cache_hits + g_pipeline_cache.cache_misses;
    if (total_requests > 0) {
        f32 hit_ratio = (f32)g_pipeline_cache.cache_hits / total_requests * 100.0f;
        printf("Hit ratio: %.2f%%\n", hit_ratio);
    }
    
    printf("Persistent cache: %s\n", g_pipeline_cache.persistent_cache_enabled ? "enabled" : "disabled");
    if (g_pipeline_cache.persistent_cache_enabled && g_pipeline_cache.cache_file_path[0]) {
        printf("Cache file: %s\n", g_pipeline_cache.cache_file_path);
    }
    
    printf("=====================================\n");
}

// Clear pipeline cache
void vulkan_pipeline_cache_clear() {
    if (g_pipeline_cache.entries) {
        memset(g_pipeline_cache.entries, 0, g_pipeline_cache.max_entries * sizeof(PipelineCacheEntry));
    }
    g_pipeline_cache.entry_count = 0;
    g_pipeline_cache.total_size = 0;
    g_pipeline_cache.cache_hits = 0;
    g_pipeline_cache.cache_misses = 0;
    
    printf("Vulkan: Pipeline cache cleared\n");
}

// Cleanup pipeline cache
void vulkan_pipeline_cache_cleanup(VulkanRenderer* renderer) {
#ifdef VULKAN_BUILD
    if (renderer && g_pipeline_cache.vk_cache != VK_NULL_HANDLE) {
        // Save cache before cleanup if persistent
        if (g_pipeline_cache.persistent_cache_enabled) {
            vulkan_pipeline_cache_save(renderer);
        }
        
        vkDestroyPipelineCache(renderer->device, g_pipeline_cache.vk_cache, NULL);
        g_pipeline_cache.vk_cache = VK_NULL_HANDLE;
    }
    
    if (g_pipeline_cache.entries) {
        free(g_pipeline_cache.entries);
        g_pipeline_cache.entries = NULL;
    }
    
    memset(&g_pipeline_cache, 0, sizeof(PipelineCache));
    
    printf("Vulkan: Pipeline cache cleaned up\n");
#else
    (void)renderer;
#endif
}
