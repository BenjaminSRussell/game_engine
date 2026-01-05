// Vulkan Buffer Configuration Header
// VULKAN-BF-006: Add buffer configuration system

#ifndef VULKAN_BUFFER_CONFIG_H
#define VULKAN_BUFFER_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct VulkanRenderer VulkanRenderer;

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
#include <vulkan/vulkan.h>
#else
// Stub types when Vulkan is not available
typedef void* VkBuffer;
typedef void* VkDeviceMemory;
typedef uint32_t VkBufferUsageFlags;
typedef uint32_t VkMemoryPropertyFlags;
typedef uint64_t VkDeviceSize;
#endif

// Buffer configuration structure
typedef struct BufferConfig {
    VkBufferUsageFlags usage_flags;
    VkMemoryPropertyFlags memory_properties;
    VkDeviceSize min_size;
    VkDeviceSize max_size;
    VkDeviceSize alignment;
    bool enable_validation;
    bool enable_profiling;
    bool enable_pooling;
    uint32_t priority;
    char name[64];
} BufferConfig;

// Predefined buffer configuration types
typedef enum BufferConfigType {
    BUFFER_CONFIG_VERTEX,
    BUFFER_CONFIG_INDEX,
    BUFFER_CONFIG_UNIFORM,
    BUFFER_CONFIG_STAGING,
    BUFFER_CONFIG_STORAGE,
    BUFFER_CONFIG_UNIFORM_TEXEL,
    BUFFER_CONFIG_STORAGE_TEXEL,
    BUFFER_CONFIG_COUNT
} BufferConfigType;

// Buffer Configuration API Functions
void vulkan_buffer_config_init();

// Get buffer configuration by type
const BufferConfig* vulkan_buffer_config_get(BufferConfigType type);

// Set custom buffer configuration
bool vulkan_buffer_config_set(BufferConfigType type, const BufferConfig* config);

// Validate buffer size against configuration
bool vulkan_buffer_config_validate_size(BufferConfigType type, VkDeviceSize size);

// Get optimal buffer size (rounded to alignment)
VkDeviceSize vulkan_buffer_config_get_aligned_size(BufferConfigType type, VkDeviceSize size);

// Print buffer configuration
void vulkan_buffer_config_print(BufferConfigType type);

// Print all buffer configurations
void vulkan_buffer_config_print_all();

// Get buffer configuration statistics
void vulkan_buffer_config_get_stats(uint32_t* total_configs, uint32_t* enabled_validation, 
                                   uint32_t* enabled_profiling, uint32_t* enabled_pooling);

// Enable/disable validation for all configurations
void vulkan_buffer_config_set_validation_all(bool enabled);

// Enable/disable profiling for all configurations
void vulkan_buffer_config_set_profiling_all(bool enabled);

// Enable/disable pooling for all configurations
void vulkan_buffer_config_set_pooling_all(bool enabled);

// Cleanup buffer configuration system
void vulkan_buffer_config_cleanup();

#ifdef __cplusplus
}
#endif

#endif // VULKAN_BUFFER_CONFIG_H
