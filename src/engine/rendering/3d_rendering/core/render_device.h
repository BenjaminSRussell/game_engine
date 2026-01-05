/**
 * @file render_device.h
 * @brief Graphics device abstraction layer
 * @details Provides abstraction over GPU-specific rendering APIs (Vulkan, Metal, DirectX)
 */

#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct RenderBuffer RenderBuffer;
typedef struct RenderTexture RenderTexture;
typedef struct RenderPipeline RenderPipeline;
typedef struct RenderPass RenderPass;
typedef struct RenderFramebuffer RenderFramebuffer;

/**
 * @brief Graphics API backend type
 */
typedef enum {
    RENDER_API_VULKAN = 0,
    RENDER_API_METAL = 1,
    RENDER_API_DIRECTX12 = 2,
} RenderAPIType;

/**
 * @brief Device capabilities structure
 */
typedef struct {
    uint32_t max_texture_width;
    uint32_t max_texture_height;
    uint32_t max_texture_array_layers;
    uint32_t max_uniform_buffer_range;
    uint32_t max_storage_buffer_range;
    uint32_t max_push_constants_size;
    uint32_t max_samplers;
    uint32_t max_color_attachments;
    uint32_t max_viewports;
    uint32_t max_scissor_rects;
    bool supports_ray_tracing;
    bool supports_mesh_shading;
    bool supports_variable_rate_shading;
    bool supports_dynamic_rendering;
    uint32_t subgroup_size;
} RenderDeviceCapabilities;

/**
 * @brief Abstract render device
 */
typedef struct RenderDevice {
    RenderAPIType api_type;
    RenderDeviceCapabilities capabilities;
    void* api_specific_data;
} RenderDevice;

/**
 * @brief Create a render device
 * @param[in] api_type Graphics API to use
 * @return Pointer to created device, NULL on failure
 */
RenderDevice* render_device_create(RenderAPIType api_type);

/**
 * @brief Destroy a render device
 * @param[in] device Device to destroy
 */
void render_device_destroy(RenderDevice* device);

/**
 * @brief Get device capabilities
 * @param[in] device The device
 * @return Pointer to capabilities structure
 */
const RenderDeviceCapabilities* render_device_get_capabilities(RenderDevice* device);

/**
 * @brief Wait for device to become idle (GPU sync point)
 * @param[in] device The device
 * @return true on success
 */
bool render_device_wait_idle(RenderDevice* device);

/**
 * @brief Reset device state
 * @param[in] device The device
 * @return true on success
 */
bool render_device_reset(RenderDevice* device);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_DEVICE_H */
