// Vulkan Backend Header - Cross-Platform 3D Rendering
// VULKAN-001: Device setup and management
// VULKAN-002: Command queue management
// VULKAN-003: Shader module and pipeline creation
// VULKAN-004: Buffer management
// VULKAN-005: Texture management
// VULKAN-006: Sampler management
// VULKAN-007: Render pass management
// VULKAN-008: Command encoding
// VULKAN-009: Swapchain management
// VULKAN-010: Synchronization and memory barriers

#ifndef VULKAN_BACKEND_H
#define VULKAN_BACKEND_H

#include <common.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for Vulkan types
struct VkInstance_T;
typedef struct VkInstance_T* VkInstance;

struct VkPhysicalDevice_T;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;

struct VkDevice_T;
typedef struct VkDevice_T* VkDevice;

struct VkQueue_T;
typedef struct VkQueue_T* VkQueue;

struct VkCommandBuffer_T;
typedef struct VkCommandBuffer_T* VkCommandBuffer;

struct VkCommandPool_T;
typedef struct VkCommandPool_T* VkCommandPool;

struct VkSwapchainKHR_T;
typedef struct VkSwapchainKHR_T* VkSwapchainKHR;

struct VkRenderPass_T;
typedef struct VkRenderPass_T* VkRenderPass;

struct VkFramebuffer_T;
typedef struct VkFramebuffer_T* VkFramebuffer;

struct VkSemaphore_T;
typedef struct VkSemaphore_T* VkSemaphore;

struct VkFence_T;
typedef struct VkFence_T* VkFence;

struct VkSurfaceKHR_T;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

// Vulkan initialization parameters
typedef struct {
    const char* app_name;
    void* window_handle;
    uint64_t device_local_budget;
    uint32_t max_frames_in_flight;
    bool enable_validation;
    bool enable_debug_markers;
} VulkanInitParams;

// Vulkan backend statistics
typedef struct {
    uint64_t frame_count;
    double average_frame_time;
    double last_frame_time;
    uint64_t memory_usage;
    uint32_t draw_calls;
    uint32_t compute_dispatches;
    uint32_t buffer_allocations;
    uint32_t texture_allocations;
    uint32_t pipeline_creations;
} VulkanStats;

// Opaque Vulkan backend structure
typedef struct VulkanBackend VulkanBackend;

// Core Vulkan backend functions
bool vulkan_init(void* window_handle, const VulkanInitParams* params);
void vulkan_shutdown(void);
bool vulkan_is_initialized(void);

// Frame rendering
bool vulkan_begin_frame(VulkanBackend* backend, void* command_buffer, uint32_t image_index);
void vulkan_end_frame(VulkanBackend* backend, void* command_buffer, uint32_t image_index);

// Backend validation and stability
bool vulkan_validate_backend(VulkanBackend* backend);

// Statistics and debugging
void vulkan_get_stats(VulkanStats* out_stats);
void vulkan_reset_stats(void);
void vulkan_debug_print_stats(const VulkanBackend* backend);

// Backend information
const char* vulkan_get_backend_name(void);

// Error handling
typedef enum {
    VULKAN_ERROR_NONE = 0,
    VULKAN_ERROR_INITIALIZATION_FAILED,
    VULKAN_ERROR_DEVICE_LOST,
    VULKAN_ERROR_OUT_OF_MEMORY,
    VULKAN_ERROR_SURFACE_LOST,
    VULKAN_ERROR_VALIDATION_FAILED
} VulkanError;

VulkanError vulkan_get_last_error(void);
const char* vulkan_get_error_string(VulkanError error);

#ifdef __cplusplus
}
#endif

#endif // VULKAN_BACKEND_H
