// src/engine/platform/vulkan/vulkan_removal.c
// Vulkan Backend Removal - Stub implementation for macOS-focused engine

#include <core/logger.h>
#include <stdbool.h>

// ============================================================================
// Vulkan Removal Notice
// ============================================================================

/*
 * VULKAN BACKEND REMOVAL NOTICE
 * ============================
 * 
 * This engine has been optimized for macOS and Metal rendering.
 * The Vulkan backend has been removed to simplify the codebase
 * and focus development efforts on Metal-specific optimizations.
 * 
 * If Vulkan support is needed in the future:
 * 1. This directory can be restored from version control
 * 2. A new Vulkan implementation can be built from scratch
 * 3. Consider cross-platform abstraction layers
 * 
 * For now, all Vulkan functionality returns appropriate errors
 * and logs informative messages about the removal.
 */

// ============================================================================
// Stub Implementation
// ============================================================================

bool vulkan_is_supported(void) {
    LOG_INFO("Vulkan backend has been removed from this macOS-focused engine");
    LOG_INFO("Use Metal backend for rendering functionality");
    return false;
}

bool vulkan_init(void) {
    LOG_ERROR("Vulkan backend is not available - removed for macOS optimization");
    LOG_ERROR("Please use Metal backend instead");
    return false;
}

void vulkan_shutdown(void) {
    LOG_DEBUG("Vulkan shutdown called - no action needed (backend removed)");
}

void* vulkan_get_instance(void) {
    LOG_WARNING("Attempted to get Vulkan instance - backend removed");
    return NULL;
}

void* vulkan_get_device(void) {
    LOG_WARNING("Attempted to get Vulkan device - backend removed");
    return NULL;
}

bool vulkan_create_surface(void* window_handle, void** out_surface) {
    LOG_ERROR("Vulkan surface creation failed - backend removed");
    LOG_ERROR("Use Metal surface creation instead");
    if (out_surface) *out_surface = NULL;
    return false;
}

void vulkan_destroy_surface(void* surface) {
    LOG_DEBUG("Vulkan surface destroy called - no action needed (backend removed)");
}

// ============================================================================
// Migration Helper
// ============================================================================

void vulkan_log_migration_info(void) {
    LOG_INFO("=== Vulkan Backend Migration Guide ===");
    LOG_INFO("The Vulkan backend has been removed from this engine.");
    LOG_INFO("Recommended migration path:");
    LOG_INFO("1. Use Metal backend for all rendering operations");
    LOG_INFO("2. Replace VkCommandBuffer with MTLCommandBuffer");
    LOG_INFO("3. Replace VkPipeline with MTLRenderPipelineState");
    LOG_INFO("4. Replace VkShaderModule with MTLFunction");
    LOG_INFO("5. Use Metal Memory Management instead of Vulkan Memory");
    LOG_INFO("6. See src/engine/backend/metal/ for Metal implementations");
    LOG_INFO("=========================================");
}
