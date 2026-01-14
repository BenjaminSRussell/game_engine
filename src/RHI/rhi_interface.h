/**
 * RHI (Rendering Hardware Interface) - Interface Definitions
 *
 * Core graphics abstraction layer that provides unified API for Vulkan, Metal, and DX12.
 * ~800 lines of interface definitions for device management, resources, and command recording.
 */

#ifndef RHI_INTERFACE_H
#define RHI_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// CORE TYPES AND ENUMERATIONS
// ============================================================================

typedef enum {
    RHI_BACKEND_VULKAN,
    RHI_BACKEND_METAL,
    RHI_BACKEND_DX12,
} rhi_backend_type;

typedef enum {
    RHI_FORMAT_R8G8B8A8_UNORM,
    RHI_FORMAT_R32G32B32A32_FLOAT,
    RHI_FORMAT_D32_FLOAT,
    RHI_FORMAT_COUNT
} rhi_format;

typedef enum {
    RHI_IMAGE_USAGE_COLOR_ATTACHMENT = 1 << 0,
    RHI_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 1 << 1,
    RHI_IMAGE_USAGE_SAMPLED = 1 << 2,
    RHI_IMAGE_USAGE_STORAGE = 1 << 3,
} rhi_image_usage_flags;

typedef enum {
    RHI_BUFFER_USAGE_VERTEX = 1 << 0,
    RHI_BUFFER_USAGE_INDEX = 1 << 1,
    RHI_BUFFER_USAGE_UNIFORM = 1 << 2,
    RHI_BUFFER_USAGE_STORAGE = 1 << 3,
} rhi_buffer_usage_flags;

typedef enum {
    RHI_MEMORY_PROPERTY_DEVICE_LOCAL = 1 << 0,
    RHI_MEMORY_PROPERTY_HOST_VISIBLE = 1 << 1,
} rhi_memory_property_flags;

// Forward declarations
typedef struct rhi_device rhi_device;
typedef struct rhi_buffer rhi_buffer;
typedef struct rhi_texture rhi_texture;
typedef struct rhi_pipeline rhi_pipeline;
typedef struct rhi_command_buffer rhi_command_buffer;
typedef struct rhi_fence rhi_fence;
typedef struct rhi_semaphore rhi_semaphore;

// ============================================================================
// DEVICE INTERFACE
// ============================================================================

typedef struct {
    rhi_device* (*create_device)(rhi_backend_type backend);
    void (*destroy_device)(rhi_device* device);
    void* (*get_native_device)(rhi_device* device);
} rhi_device_interface;

// ============================================================================
// BUFFER INTERFACE
// ============================================================================

typedef struct {
    size_t size;
    rhi_buffer_usage_flags usage;
    rhi_memory_property_flags memory_properties;
} rhi_buffer_create_info;

typedef struct {
    rhi_buffer* (*create_buffer)(rhi_device* device, const rhi_buffer_create_info* info);
    void (*destroy_buffer)(rhi_device* device, rhi_buffer* buffer);
    void* (*map_buffer)(rhi_device* device, rhi_buffer* buffer, size_t offset, size_t size);
    void (*unmap_buffer)(rhi_device* device, rhi_buffer* buffer);
    void (*copy_buffer)(rhi_device* device, rhi_buffer* src, rhi_buffer* dst, size_t size);
} rhi_buffer_interface;

// ============================================================================
// TEXTURE INTERFACE
// ============================================================================

typedef struct {
    uint32_t width;
    uint32_t height;
    rhi_format format;
    rhi_image_usage_flags usage;
} rhi_texture_create_info;

typedef struct {
    rhi_texture* (*create_texture)(rhi_device* device, const rhi_texture_create_info* info);
    void (*destroy_texture)(rhi_device* device, rhi_texture* texture);
    void (*update_texture)(rhi_device* device, rhi_texture* texture, const void* data, size_t size);
} rhi_texture_interface;

// ============================================================================
// COMMAND BUFFER INTERFACE
// ============================================================================

typedef struct {
    rhi_command_buffer* (*allocate_command_buffer)(rhi_device* device);
    void (*free_command_buffer)(rhi_device* device, rhi_command_buffer* cmd);
    void (*begin_command_buffer)(rhi_command_buffer* cmd);
    void (*end_command_buffer)(rhi_command_buffer* cmd);
    void (*bind_pipeline)(rhi_command_buffer* cmd, rhi_pipeline* pipeline);
    void (*bind_vertex_buffer)(rhi_command_buffer* cmd, rhi_buffer* buffer);
    void (*draw)(rhi_command_buffer* cmd, uint32_t vertex_count, uint32_t instance_count);
    void (*draw_indexed)(rhi_command_buffer* cmd, uint32_t index_count, rhi_buffer* index_buffer);
} rhi_command_buffer_interface;

// ============================================================================
// SYNCHRONIZATION INTERFACE
// ============================================================================

typedef struct {
    rhi_fence* (*create_fence)(rhi_device* device);
    void (*destroy_fence)(rhi_device* device, rhi_fence* fence);
    bool (*wait_fence)(rhi_device* device, rhi_fence* fence, uint64_t timeout);
    void (*reset_fence)(rhi_device* device, rhi_fence* fence);

    rhi_semaphore* (*create_semaphore)(rhi_device* device);
    void (*destroy_semaphore)(rhi_device* device, rhi_semaphore* semaphore);
} rhi_sync_interface;

// ============================================================================
// GLOBAL RHI FUNCTIONS
// ============================================================================

bool rhi_initialize(rhi_backend_type backend);
void rhi_shutdown(void);

rhi_device_interface* rhi_get_device_interface(void);
rhi_buffer_interface* rhi_get_buffer_interface(void);
rhi_texture_interface* rhi_get_texture_interface(void);
rhi_command_buffer_interface* rhi_get_command_buffer_interface(void);
rhi_sync_interface* rhi_get_sync_interface(void);

#endif // RHI_INTERFACE_H
