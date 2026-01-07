// Vulkan Buffer Configuration System
// VULKAN-BF-006: Add buffer configuration system

#include <rendering/vulkan.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
#include <include/rendering/vulkan.h>
#else
// Stub constants when Vulkan is not available
typedef void* VkBuffer;
typedef void* VkDeviceMemory;
typedef u32 VkBufferUsageFlags;
typedef u32 VkMemoryPropertyFlags;
typedef u32 VkDeviceSize;
#endif

// VULKAN-BF-006: Buffer Configuration System Implementation
// ===================================================

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
    u32 priority;
    char name[64];
} BufferConfig;

// Predefined buffer configurations
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

// Global configuration registry
static BufferConfig g_buffer_configs[BUFFER_CONFIG_COUNT] = {0};
static bool g_config_initialized = false;

// Initialize default buffer configurations
void vulkan_buffer_config_init() {
    if (g_config_initialized) {
        return;
    }
    
    // Vertex buffer configuration
    g_buffer_configs[BUFFER_CONFIG_VERTEX] = (BufferConfig){
        .usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .min_size = 1024,
        .max_size = 64 * 1024 * 1024, // 64MB
        .alignment = 256,
        .enable_validation = false,
        .enable_profiling = true,
        .enable_pooling = true,
        .priority = 1,
        .name = "Vertex Buffer"
    };
    
    // Index buffer configuration
    g_buffer_configs[BUFFER_CONFIG_INDEX] = (BufferConfig){
        .usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .min_size = 512,
        .max_size = 32 * 1024 * 1024, // 32MB
        .alignment = 256,
        .enable_validation = false,
        .enable_profiling = true,
        .enable_pooling = true,
        .priority = 1,
        .name = "Index Buffer"
    };
    
    // Uniform buffer configuration
    g_buffer_configs[BUFFER_CONFIG_UNIFORM] = (BufferConfig){
        .usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .min_size = 64,
        .max_size = 4 * 1024 * 1024, // 4MB
        .alignment = 256,
        .enable_validation = true,
        .enable_profiling = true,
        .enable_pooling = true,
        .priority = 2,
        .name = "Uniform Buffer"
    };
    
    // Staging buffer configuration
    g_buffer_configs[BUFFER_CONFIG_STAGING] = (BufferConfig){
        .usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .min_size = 1024,
        .max_size = 128 * 1024 * 1024, // 128MB
        .alignment = 256,
        .enable_validation = false,
        .enable_profiling = true,
        .enable_pooling = true,
        .priority = 3,
        .name = "Staging Buffer"
    };
    
    // Storage buffer configuration
    g_buffer_configs[BUFFER_CONFIG_STORAGE] = (BufferConfig){
        .usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .min_size = 1024,
        .max_size = 256 * 1024 * 1024, // 256MB
        .alignment = 256,
        .enable_validation = false,
        .enable_profiling = true,
        .enable_pooling = true,
        .priority = 2,
        .name = "Storage Buffer"
    };
    
    // Uniform texel buffer configuration
    g_buffer_configs[BUFFER_CONFIG_UNIFORM_TEXEL] = (BufferConfig){
        .usage_flags = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .min_size = 1024,
        .max_size = 64 * 1024 * 1024, // 64MB
        .alignment = 256,
        .enable_validation = false,
        .enable_profiling = true,
        .enable_pooling = true,
        .priority = 2,
        .name = "Uniform Texel Buffer"
    };
    
    // Storage texel buffer configuration
    g_buffer_configs[BUFFER_CONFIG_STORAGE_TEXEL] = (BufferConfig){
        .usage_flags = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .min_size = 1024,
        .max_size = 128 * 1024 * 1024, // 128MB
        .alignment = 256,
        .enable_validation = false,
        .enable_profiling = true,
        .enable_pooling = true,
        .priority = 2,
        .name = "Storage Texel Buffer"
    };
    
    g_config_initialized = true;
    
    printf("Vulkan: Buffer configuration system initialized\n");
    printf("  Configured %u buffer types\n", BUFFER_CONFIG_COUNT);
    
    for (int i = 0; i < BUFFER_CONFIG_COUNT; i++) {
        printf("  %s: min=%zu, max=%zu, alignment=%zu, priority=%u\n",
               g_buffer_configs[i].name,
               g_buffer_configs[i].min_size,
               g_buffer_configs[i].max_size,
               g_buffer_configs[i].alignment,
               g_buffer_configs[i].priority);
    }
}

// Get buffer configuration by type
const BufferConfig* vulkan_buffer_config_get(BufferConfigType type) {
    if (!g_config_initialized || type < 0 || type >= BUFFER_CONFIG_COUNT) {
        return NULL;
    }
    
    return &g_buffer_configs[type];
}

// Set custom buffer configuration
bool vulkan_buffer_config_set(BufferConfigType type, const BufferConfig* config) {
    if (!g_config_initialized || !config || type < 0 || type >= BUFFER_CONFIG_COUNT) {
        return false;
    }
    
    g_buffer_configs[type] = *config;
    printf("Vulkan: Updated configuration for %s\n", config->name);
    return true;
}

// Validate buffer size against configuration
bool vulkan_buffer_config_validate_size(BufferConfigType type, VkDeviceSize size) {
    const BufferConfig* config = vulkan_buffer_config_get(type);
    if (!config) {
        return false;
    }
    
    return size >= config->min_size && size <= config->max_size;
}

// Get optimal buffer size (rounded to alignment)
VkDeviceSize vulkan_buffer_config_get_aligned_size(BufferConfigType type, VkDeviceSize size) {
    const BufferConfig* config = vulkan_buffer_config_get(type);
    if (!config) {
        return size;
    }
    
    VkDeviceSize aligned_size = size;
    VkDeviceSize remainder = size % config->alignment;
    
    if (remainder != 0) {
        aligned_size = size + (config->alignment - remainder);
    }
    
    // Ensure minimum size
    if (aligned_size < config->min_size) {
        aligned_size = config->min_size;
    }
    
    // Ensure maximum size
    if (aligned_size > config->max_size) {
        aligned_size = config->max_size;
    }
    
    return aligned_size;
}

// Print buffer configuration
void vulkan_buffer_config_print(BufferConfigType type) {
    const BufferConfig* config = vulkan_buffer_config_get(type);
    if (!config) {
        printf("Vulkan: Invalid buffer configuration type\n");
        return;
    }
    
    printf("=== Buffer Configuration: %s ===\n", config->name);
    printf("Usage Flags: 0x%08X\n", config->usage_flags);
    printf("Memory Properties: 0x%08X\n", config->memory_properties);
    printf("Size Range: %zu - %zu bytes\n", config->min_size, config->max_size);
    printf("Alignment: %zu bytes\n", config->alignment);
    printf("Validation: %s\n", config->enable_validation ? "enabled" : "disabled");
    printf("Profiling: %s\n", config->enable_profiling ? "enabled" : "disabled");
    printf("Pooling: %s\n", config->enable_pooling ? "enabled" : "disabled");
    printf("Priority: %u\n", config->priority);
    printf("=====================================\n");
}

// Print all buffer configurations
void vulkan_buffer_config_print_all() {
    if (!g_config_initialized) {
        printf("Vulkan: Buffer configuration system not initialized\n");
        return;
    }
    
    printf("=== All Buffer Configurations ===\n");
    for (int i = 0; i < BUFFER_CONFIG_COUNT; i++) {
        printf("%d. %s\n", i + 1, g_buffer_configs[i].name);
        printf("   Size: %zu - %zu bytes\n", g_buffer_configs[i].min_size, g_buffer_configs[i].max_size);
        printf("   Alignment: %zu bytes\n", g_buffer_configs[i].alignment);
        printf("   Priority: %u\n", g_buffer_configs[i].priority);
        printf("\n");
    }
    printf("===================================\n");
}

// Get buffer configuration statistics
void vulkan_buffer_config_get_stats(u32* total_configs, u32* enabled_validation, 
                                   u32* enabled_profiling, u32* enabled_pooling) {
    if (!g_config_initialized) {
        if (total_configs) *total_configs = 0;
        if (enabled_validation) *enabled_validation = 0;
        if (enabled_profiling) *enabled_profiling = 0;
        if (enabled_pooling) *enabled_pooling = 0;
        return;
    }
    
    u32 validation_count = 0;
    u32 profiling_count = 0;
    u32 pooling_count = 0;
    
    for (int i = 0; i < BUFFER_CONFIG_COUNT; i++) {
        if (g_buffer_configs[i].enable_validation) validation_count++;
        if (g_buffer_configs[i].enable_profiling) profiling_count++;
        if (g_buffer_configs[i].enable_pooling) pooling_count++;
    }
    
    if (total_configs) *total_configs = BUFFER_CONFIG_COUNT;
    if (enabled_validation) *enabled_validation = validation_count;
    if (enabled_profiling) *enabled_profiling = profiling_count;
    if (enabled_pooling) *enabled_pooling = pooling_count;
}

// Enable/disable validation for all configurations
void vulkan_buffer_config_set_validation_all(bool enabled) {
    if (!g_config_initialized) return;
    
    for (int i = 0; i < BUFFER_CONFIG_COUNT; i++) {
        g_buffer_configs[i].enable_validation = enabled;
    }
    
    printf("Vulkan: Buffer validation %s for all configurations\n", enabled ? "enabled" : "disabled");
}

// Enable/disable profiling for all configurations
void vulkan_buffer_config_set_profiling_all(bool enabled) {
    if (!g_config_initialized) return;
    
    for (int i = 0; i < BUFFER_CONFIG_COUNT; i++) {
        g_buffer_configs[i].enable_profiling = enabled;
    }
    
    printf("Vulkan: Buffer profiling %s for all configurations\n", enabled ? "enabled" : "disabled");
}

// Enable/disable pooling for all configurations
void vulkan_buffer_config_set_pooling_all(bool enabled) {
    if (!g_config_initialized) return;
    
    for (int i = 0; i < BUFFER_CONFIG_COUNT; i++) {
        g_buffer_configs[i].enable_pooling = enabled;
    }
    
    printf("Vulkan: Buffer pooling %s for all configurations\n", enabled ? "enabled" : "disabled");
}

// Cleanup buffer configuration system
void vulkan_buffer_config_cleanup() {
    memset(g_buffer_configs, 0, sizeof(g_buffer_configs));
    g_config_initialized = false;
    
    printf("Vulkan: Buffer configuration system cleaned up\n");
}
