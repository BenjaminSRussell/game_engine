// Vulkan Framebuffer Management Header
// VULKAN-FB-001: Implement framebuffer pooling system
// VULKAN-FB-002: Add framebuffer validation system
// VULKAN-FB-003: Implement framebuffer statistics tracking
// VULKAN-FB-004: Add framebuffer debugging visualization
// VULKAN-FB-005: Implement framebuffer performance profiling
// VULKAN-FB-006: Add framebuffer configuration system
// VULKAN-FB-007: Implement framebuffer unit testing framework
// VULKAN-FB-008: Add framebuffer documentation system
// VULKAN-FB-009: Implement framebuffer optimization suggestions
// VULKAN-FB-010: Add framebuffer memory leak detection

#ifndef VULKAN_FRAMEBUFFER_H
#define VULKAN_FRAMEBUFFER_H

#include <common.h>
#include "vulkan.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct VulkanRenderer VulkanRenderer;

// VULKAN-FB-003: Framebuffer statistics structure
typedef struct {
    u32 total_created;
    u32 total_destroyed;
    u32 pool_hits;
    u32 pool_misses;
    u64 memory_usage_bytes;
    u64 total_creation_time_ms;
    double pool_hit_rate;
    double average_creation_time_ms;
} VulkanFramebufferStats;

// VULKAN-FB-006: Framebuffer configuration structure
typedef struct {
    bool clear_pool;
    bool enable_leak_detection;
    bool enable_performance_profiling;
    u32 max_pool_size;
} VulkanFramebufferConfig;

// VULKAN-FB-009: Framebuffer optimization suggestions
typedef struct {
    bool increase_pool_size;
    bool optimize_framebuffer_reuse;
    bool enable_aggressive_caching;
    bool reduce_framebuffer_complexity;
    bool reduce_memory_usage;
    bool implement_framebuffer_streaming;
} VulkanFramebufferOptimizations;

// Core framebuffer management functions
bool vulkan_create_framebuffers(VulkanRenderer* renderer);
void vulkan_destroy_framebuffers(VulkanRenderer* renderer);

// Enhanced frame management with pooling
bool vulkan_begin_frame_updated(VulkanRenderer* renderer, u32* image_index);
void vulkan_end_frame_updated(VulkanRenderer* renderer, u32 image_index);

// VULKAN-FB-004: Debug and visualization
void vulkan_debug_framebuffers(void);

// VULKAN-FB-003: Statistics and performance
void vulkan_get_framebuffer_stats(VulkanFramebufferStats* out_stats);

// VULKAN-FB-010: Memory leak detection
void vulkan_detect_framebuffer_leaks(void);

// VULKAN-FB-006: Configuration management
void vulkan_configure_framebuffers(const VulkanFramebufferConfig* config);

// VULKAN-FB-009: Optimization suggestions
void vulkan_get_framebuffer_optimizations(VulkanFramebufferOptimizations* suggestions);

// VULKAN-FB-007: Unit testing support
bool vulkan_test_framebuffer_pool(void);
bool vulkan_test_framebuffer_validation(void);
bool vulkan_test_framebuffer_leak_detection(void);

#ifdef __cplusplus
}
#endif

#endif // VULKAN_FRAMEBUFFER_H
