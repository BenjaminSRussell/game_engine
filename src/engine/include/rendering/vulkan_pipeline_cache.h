// Vulkan Pipeline Caching Header
// VULKAN-PL-001: Implement pipeline caching system for performance

#ifndef VULKAN_PIPELINE_CACHE_H
#define VULKAN_PIPELINE_CACHE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct VulkanRenderer VulkanRenderer;

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
#include <include/rendering/vulkan.h>
#else
// Stub types when Vulkan is not available
typedef void* VkPipeline;
#endif

// Pipeline Cache API Functions
bool vulkan_pipeline_cache_init(VulkanRenderer* renderer, const char* cache_file_path, bool persistent);

// Get or create pipeline with caching
VkPipeline vulkan_get_or_create_pipeline(VulkanRenderer* renderer, 
                                         const void* pipeline_description, 
                                         uint32_t description_size,
                                         VkPipeline (*create_pipeline_func)(VulkanRenderer*, const void*));

// Save pipeline cache to file
bool vulkan_pipeline_cache_save(VulkanRenderer* renderer);

// Get pipeline cache statistics
void vulkan_pipeline_cache_get_stats(uint32_t* entry_count, uint64_t* cache_hits, 
                                     uint64_t* cache_misses, float* hit_ratio);

// Print pipeline cache statistics
void vulkan_pipeline_cache_print_stats();

// Clear pipeline cache
void vulkan_pipeline_cache_clear();

// Cleanup pipeline cache
void vulkan_pipeline_cache_cleanup(VulkanRenderer* renderer);

#ifdef __cplusplus
}
#endif

#endif // VULKAN_PIPELINE_CACHE_H
