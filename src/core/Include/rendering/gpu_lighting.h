// include/render/gpu_lighting.h
//
// Purpose: GPU-side lighting management.
// Handles uniform buffer objects (UBOs) for lighting data, light culling,
// and per-frame light updates.
//
#ifndef GPU_LIGHTING_H
#define GPU_LIGHTING_H

#include <common.h>
#include "include/rendering/lighting.h"
#include "include/rendering/vulkan.h"
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat4.h>

// GPU-side light structures (aligned for std140 layout)
typedef struct {
    Vec4 direction;
    Vec4 color;           // RGB + intensity in .w
    f32 ambient_intensity;
    f32 shadow_strength;
    f32 shadow_bias;
    u32 shadow_map_id;    // Texture ID for shadow map
} GPUDirectionalLight;

typedef struct {
    Vec4 position;        // XYZ + radius in .w
    Vec4 color;           // RGB + intensity in .w
    f32 falloff;
    f32 range_squared;
    u32 flags;
    u32 shadow_map_id;
} GPUPointLight;

typedef struct {
    Vec4 position;
    Vec4 direction;
    Vec4 color;
    f32 inner_angle;
    f32 outer_angle;
    f32 intensity;
    f32 range;
} GPUSpotLight;

// Lighting UBO (matches shader layout)
typedef struct {
    // Directional light
    GPUDirectionalLight directional;

    // Point light cluster
    u32 point_light_count;
    u32 spot_light_count;
    f32 padding1[2];

    // Ambient and environment
    Vec4 ambient_color;
    f32 ambient_intensity;
    f32 sky_brightness;
    f32 fog_density;
    f32 padding2;

    Vec4 fog_color;

    // Time of day
    f32 time_of_day;
    f32 day_cycle_phase;  // 0.0-1.0 normalized
    f32 day_cycle_speed;
    u32 day_phase;        // 0=dawn, 1=day, 2=dusk, 3=night

    // Point lights
    GPUPointLight point_lights[256];

    // Spot lights
    GPUSpotLight spot_lights[32];

    // Shadow parameters
    f32 shadow_bias;
    f32 shadow_softness;
    u32 shadow_map_count;
    u32 use_shadows;
} GPULightingUBO;

#define GPU_LIGHTING_UBO_SIZE sizeof(GPULightingUBO)

// GPU light culling structure
typedef struct {
    u32 point_light_indices[256];  // Indices of visible point lights
    u32 spot_light_indices[32];
    u32 point_light_count;
    u32 spot_light_count;
} GPULightCullingResult;

// GPU lighting manager
typedef struct {
    GPULightingUBO* ubo_data;
    VkBuffer ubo_buffer;
    VkDeviceMemory ubo_memory;
    VkDescriptorSet descriptor_set;

    // Light culling
    GPULightCullingResult culling_result;
    Vec3 last_cull_position;
    f32 cull_radius;

    // Device info
    VkDevice device;
    VkPhysicalDevice physical_device;

    bool initialized;
} GPULightingManager;

// ==============================================================================
// GPU Lighting Manager Lifecycle
// ==============================================================================

// Initialize GPU lighting manager
bool gpu_lighting_init(GPULightingManager* manager, VkDevice device,
                       VkPhysicalDevice physical_device);

// Shutdown GPU lighting manager
void gpu_lighting_shutdown(GPULightingManager* manager);

// ==============================================================================
// UBO Updates
// ==============================================================================

// Update lighting UBO from CPU lighting system
bool gpu_lighting_update_from_system(GPULightingManager* manager, LightingSystem* cpu_lighting);

// Update directional light
void gpu_lighting_set_directional(GPULightingManager* manager, DirectionalLight* light);

// Update point lights
void gpu_lighting_update_point_lights(GPULightingManager* manager,
                                     PointLight* lights, u32 count);

// Update spot lights
void gpu_lighting_update_spot_lights(GPULightingManager* manager,
                                    SpotLight* lights, u32 count);

// ==============================================================================
// Light Culling (for frustum/spatial culling)
// ==============================================================================

// Cull lights for a specific position and view frustum
void gpu_lighting_cull_lights(GPULightingManager* manager, Vec3 camera_pos,
                              f32 cull_radius, u32 max_point_lights, u32 max_spot_lights);

// Get culled light indices
GPULightCullingResult* gpu_lighting_get_culling_result(GPULightingManager* manager);

// ==============================================================================
// Buffer Management
// ==============================================================================

// Create UBO buffer
bool gpu_lighting_create_ubo_buffer(GPULightingManager* manager);

// Destroy UBO buffer
void gpu_lighting_destroy_ubo_buffer(GPULightingManager* manager);

// Update UBO data to GPU
bool gpu_lighting_upload_ubo(GPULightingManager* manager);

// Get UBO buffer handle
VkBuffer gpu_lighting_get_ubo_buffer(GPULightingManager* manager);

// ==============================================================================
// Descriptor Set Management
// ==============================================================================

// Create descriptor set for lighting UBO
bool gpu_lighting_create_descriptor_set(GPULightingManager* manager,
                                        VkDescriptorSetLayout layout);

// Bind lighting descriptor set
void gpu_lighting_bind_descriptor_set(GPULightingManager* manager,
                                     VkCommandBuffer cmd_buffer,
                                     VkPipelineLayout layout);

// ==============================================================================
// Shadow Mapping
// ==============================================================================

// Set shadow parameters
void gpu_lighting_set_shadow_params(GPULightingManager* manager, f32 bias, f32 softness);

// Enable/disable shadows
void gpu_lighting_set_shadows_enabled(GPULightingManager* manager, bool enabled);

// Bind shadow map texture
void gpu_lighting_bind_shadow_map(GPULightingManager* manager, u32 shadow_map_texture_id);

// ==============================================================================
// Fog and Atmosphere
// ==============================================================================

// Update atmospheric fog
void gpu_lighting_set_fog(GPULightingManager* manager, Vec4 color, f32 density);

// Set sky brightness
void gpu_lighting_set_sky_brightness(GPULightingManager* manager, f32 brightness);

// ==============================================================================
// Query Functions
// ==============================================================================

// Get current directional light from GPU data
GPUDirectionalLight* gpu_lighting_get_directional(GPULightingManager* manager);

// Get point light from GPU data
GPUPointLight* gpu_lighting_get_point_light(GPULightingManager* manager, u32 index);

// Get UBO data pointer (for manual updates)
GPULightingUBO* gpu_lighting_get_ubo_data(GPULightingManager* manager);

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

// Log lighting system information
void gpu_lighting_log_info(GPULightingManager* manager);

// Log lighting statistics
void gpu_lighting_log_statistics(GPULightingManager* manager);

// Validate lighting GPU resources
bool gpu_lighting_validate(GPULightingManager* manager);

#endif // GPU_LIGHTING_H
