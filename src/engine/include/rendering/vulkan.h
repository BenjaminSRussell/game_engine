// include/render/vulkan.h
//
// Purpose: Defines the public API and data structures for the Vulkan rendering
// backend. This comprehensive header covers the entire rendering pipeline, from
// initializing the Vulkan instance and logical device to managing the
// swapchain, render passes, graphics pipelines, command buffers, and
// synchronization primitives. It also provides functions for creating and
// managing GPU resources like vertex/index buffers and textures, and integrates
// with other game systems like the camera and chunk meshing.
//
// Public APIs:
// - Vulkan Type Stubs: Placeholder definitions for Vulkan types (`VkInstance`,
// `VkDevice`, etc.)
//   when `VULKAN_BUILD` is not defined, allowing for compilation without a
//   Vulkan SDK.
// - `VulkanRenderer`: The main structure encapsulating the entire Vulkan
// rendering state,
//   including handles for the instance, physical device, logical device,
//   queues, surface, swapchain, render pass, pipelines, command pools, buffers,
//   semaphores, fences, descriptors, framebuffers, and camera matrices.
// - `RenderState`: A higher-level structure holding a `VulkanRenderer`
// instance, `Camera`,
//   view/projection matrices, and window dimensions.
// - `vulkan_init`: Initializes the entire Vulkan renderer, setting up the
// instance,
//   device, queues, surface, swapchain, render pass, and graphics pipeline.
// - `vulkan_cleanup`: Destroys all Vulkan resources, freeing allocated memory
// and handles.
// - Swapchain and Render Pass: `vulkan_create_surface`,
// `vulkan_create_swapchain`, `vulkan_create_render_pass`,
//   `vulkan_create_graphics_pipeline`, `vulkan_create_framebuffers` for setting
//   up the rendering infrastructure.
// - Frame Management: `vulkan_begin_frame`, `vulkan_end_frame`,
// `vulkan_begin_frame_updated`,
//   `vulkan_end_frame_updated` for synchronizing rendering operations and
//   presenting frames.
// - Rendering functions: `vulkan_render_chunk_mesh`,
// `vulkan_render_dynamic_mesh`, `vulkan_render_physics_debug`,
//   `vulkan_render_block_highlight`, `vulkan_render_chunk` for drawing various
//   game elements.
// - Camera integration: `vulkan_update_camera_uniforms`, `vulkan_update_camera`
// for synchronizing
//   camera data with the renderer.
// - Buffer management: `vulkan_create_buffer`, `vulkan_copy_buffer`,
// `vulkan_create_chunk_vertex_buffer`,
//   `vulkan_create_chunk_index_buffer`, `vulkan_update_chunk_buffers` for GPU
//   buffer handling.
// - Texture management: `vulkan_create_texture_image`,
// `vulkan_create_texture_sampler` for loading textures.
//
// Ownership: The `VulkanRenderer` structure directly owns or manages the
// lifecycle of all Vulkan API objects (handles, memory allocations). Users are
// responsible for initializing and cleaning up the renderer.
//
// Invariants:
// - The Vulkan SDK and driver must be installed and properly configured for
// `VULKAN_BUILD` to function.
// - All Vulkan handles must be valid and non-NULL during their usage.
// - Resource creation functions (e.g., `vulkan_create_buffer`) return `false`
// on failure,
//   requiring error handling.
// - `VFS`, `PlayerSystem`, `Chunk`, `Mesh` are forward declared or included,
// indicating
//   dependencies on other game subsystems.
//
#ifndef VULKAN_H
#define VULKAN_H

#include <common.h>
#include <math/mat4.h>
#include <math/quat.h>
#include <math/vec2.h>
#include <math/vec3.h>

// Forward declarations
struct VFS;
typedef struct VFS VFS;
struct PlayerSystem;
struct Chunk;
struct Chunk;
typedef struct Chunk Chunk;
struct Mesh;
typedef struct Mesh Mesh;
typedef struct Mesh Mesh;
struct GameConfig;

// Vulkan includes
#ifdef VULKAN_BUILD
#ifdef __APPLE__
#define VK_USE_PLATFORM_METAL_EXT
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#if __has_include(<vulkan/vulkan.h>)
#include <vulkan/vulkan.h>
#else
// Vulkan not available - use stubs
typedef void *VkInstance;
typedef void *VkPhysicalDevice;
typedef void *VkDevice;
typedef void *VkQueue;
typedef void *VkSurfaceKHR;
typedef void *VkSwapchainKHR;
typedef void *VkImage;
typedef void *VkImageView;
typedef void *VkRenderPass;
typedef void *VkDescriptorSetLayout;
typedef void *VkPipelineLayout;
typedef void *VkPipeline;
typedef void *VkCommandPool;
typedef void *VkCommandBuffer;
typedef void *VkSemaphore;
typedef void *VkFence;
typedef void *VkBuffer;
typedef void *VkDeviceMemory;
typedef void *VkDescriptorPool;
typedef void *VkDescriptorSet;
typedef void *VkSampler;
typedef void *VkFramebuffer;
typedef void *VkShaderModule;
typedef struct {
  u32 width, height;
} VkExtent2D;
typedef u32 VkFormat;
typedef u64 VkDeviceSize;
typedef u32 VkBufferUsageFlags;
typedef u32 VkMemoryPropertyFlags;
typedef u32 VkImageUsageFlags;
typedef u32 VkImageLayout;
typedef u32 VkPipelineBindPoint;
typedef i32 VkResult;

typedef struct {
  u32 maxImageDimension2D;
  u32 maxColorAttachments;
  u32 maxUniformBufferRange;
  u32 maxStorageBufferRange;
} VkPhysicalDeviceLimits;

typedef struct {
  char deviceName[256];
  u32 vendorID;
  u32 deviceID;
  u32 apiVersion;
  u32 driverVersion;
  VkPhysicalDeviceLimits limits;
} VkPhysicalDeviceProperties;

typedef struct {
  u32 propertyFlags;
} VkMemoryType;

typedef struct {
  u32 memoryTypeCount;
  VkMemoryType memoryTypes[32];
} VkPhysicalDeviceMemoryProperties;

#ifndef VK_NULL_HANDLE
#define VK_NULL_HANDLE NULL
#endif

#ifndef VK_SUCCESS
#define VK_SUCCESS 0
#endif

#ifndef VK_VERSION_MAJOR
#define VK_VERSION_MAJOR(version) ((u32)((version) >> 22))
#endif
#ifndef VK_VERSION_MINOR
#define VK_VERSION_MINOR(version) ((u32)(((version) >> 12) & 0x3ff))
#endif
#ifndef VK_VERSION_PATCH
#define VK_VERSION_PATCH(version) ((u32)((version) & 0xfff))
#endif

static inline void vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
  (void)physicalDevice;
  if (pMemoryProperties) {
    *pMemoryProperties = (VkPhysicalDeviceMemoryProperties){0};
  }
}

#ifndef VK_PIPELINE_BIND_POINT_GRAPHICS
#define VK_PIPELINE_BIND_POINT_GRAPHICS 0
#endif

#ifndef VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
#define VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 0
#endif

#ifndef VK_FORMAT_R8_UNORM
#define VK_FORMAT_R8_UNORM 9
#endif
#ifndef VK_FORMAT_R8G8B8A8_SRGB
#define VK_FORMAT_R8G8B8A8_SRGB 43
#endif
#ifndef VK_FORMAT_R16G16B16A16_SFLOAT
#define VK_FORMAT_R16G16B16A16_SFLOAT 97
#endif
#ifndef VK_FORMAT_R32G32B32A32_SFLOAT
#define VK_FORMAT_R32G32B32A32_SFLOAT 109
#endif
#ifndef VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT
#define VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT 0x00002000
#endif
#ifndef VK_QUEUE_COMPUTE_BIT
#define VK_QUEUE_COMPUTE_BIT 0x00000002
#endif

typedef struct {
  u32 linearTilingFeatures;
  u32 optimalTilingFeatures;
  u32 bufferFeatures;
} VkFormatProperties;

static inline void
vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice,
                                    VkFormat format,
                                    VkFormatProperties *pFormatProperties) {
  (void)physicalDevice;
  (void)format;
  if (pFormatProperties) {
    pFormatProperties->linearTilingFeatures = 0;
    pFormatProperties->optimalTilingFeatures = 0;
    pFormatProperties->bufferFeatures = 0;
  }
}

// Minimal function stubs so modules compile and link without Vulkan SDK.
static inline void vkDestroyImageView(VkDevice device, VkImageView imageView,
                                      const void *pAllocator) {
  (void)device;
  (void)imageView;
  (void)pAllocator;
}
static inline void vkDestroyImage(VkDevice device, VkImage image,
                                  const void *pAllocator) {
  (void)device;
  (void)image;
  (void)pAllocator;
}
static inline void vkFreeMemory(VkDevice device, VkDeviceMemory memory,
                                const void *pAllocator) {
  (void)device;
  (void)memory;
  (void)pAllocator;
}
static inline void vkDestroyBuffer(VkDevice device, VkBuffer buffer,
                                   const void *pAllocator) {
  (void)device;
  (void)buffer;
  (void)pAllocator;
}
static inline void vkCmdBindDescriptorSets(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, u32 firstSet, u32 descriptorSetCount,
    const VkDescriptorSet *pDescriptorSets, u32 dynamicOffsetCount,
    const u32 *pDynamicOffsets) {
  (void)commandBuffer;
  (void)pipelineBindPoint;
  (void)layout;
  (void)firstSet;
  (void)descriptorSetCount;
  (void)pDescriptorSets;
  (void)dynamicOffsetCount;
  (void)pDynamicOffsets;
}
static inline VkResult vkCreateShaderModule(VkDevice device,
                                            const void *pCreateInfo,
                                            const void *pAllocator,
                                            VkShaderModule *pShaderModule) {
  (void)device;
  (void)pCreateInfo;
  (void)pAllocator;
  if (pShaderModule) {
    *pShaderModule = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
static inline void vkDestroyShaderModule(VkDevice device,
                                         VkShaderModule shaderModule,
                                         const void *pAllocator) {
  (void)device;
  (void)shaderModule;
  (void)pAllocator;
}
static inline VkResult vkCreateSampler(VkDevice device, const void *pCreateInfo,
                                       const void *pAllocator,
                                       VkSampler *pSampler) {
  (void)device;
  (void)pCreateInfo;
  (void)pAllocator;
  if (pSampler) {
    *pSampler = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
static inline VkResult
vkCreateDescriptorSetLayout(VkDevice device, const void *pCreateInfo,
                            const void *pAllocator,
                            VkDescriptorSetLayout *pSetLayout) {
  (void)device;
  (void)pCreateInfo;
  (void)pAllocator;
  if (pSetLayout) {
    *pSetLayout = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
static inline VkResult
vkAllocateDescriptorSets(VkDevice device, const void *pAllocateInfo,
                         VkDescriptorSet *pDescriptorSets) {
  (void)device;
  (void)pAllocateInfo;
  if (pDescriptorSets) {
    *pDescriptorSets = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
static inline void vkUpdateDescriptorSets(VkDevice device,
                                          u32 descriptorWriteCount,
                                          const void *pDescriptorWrites,
                                          u32 descriptorCopyCount,
                                          const void *pDescriptorCopies) {
  (void)device;
  (void)descriptorWriteCount;
  (void)pDescriptorWrites;
  (void)descriptorCopyCount;
  (void)pDescriptorCopies;
}
static inline VkResult
vkCreatePipelineLayout(VkDevice device, const void *pCreateInfo,
                       const void *pAllocator,
                       VkPipelineLayout *pPipelineLayout) {
  (void)device;
  (void)pCreateInfo;
  (void)pAllocator;
  if (pPipelineLayout) {
    *pPipelineLayout = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
#endif
#else
// Stub types when Vulkan not available
typedef void *VkInstance;
typedef void *VkPhysicalDevice;
typedef void *VkDevice;
typedef void *VkQueue;
typedef void *VkSurfaceKHR;
typedef void *VkSwapchainKHR;
typedef void *VkImage;
typedef void *VkImageView;
typedef void *VkRenderPass;
typedef void *VkDescriptorSetLayout;
typedef void *VkPipelineLayout;
typedef void *VkPipeline;
typedef void *VkCommandPool;
typedef void *VkCommandBuffer;
typedef void *VkSemaphore;
typedef void *VkFence;
typedef void *VkBuffer;
typedef void *VkDeviceMemory;
typedef void *VkDescriptorPool;
typedef void *VkDescriptorSet;
typedef void *VkSampler;
typedef void *VkFramebuffer;
typedef void *VkShaderModule;
typedef struct {
  u32 width, height;
} VkExtent2D;
typedef u32 VkFormat;
typedef u64 VkDeviceSize;
typedef u32 VkBufferUsageFlags;
typedef u32 VkMemoryPropertyFlags;
typedef u32 VkImageUsageFlags;
typedef u32 VkImageLayout;
typedef u32 VkPipelineBindPoint;
typedef i32 VkResult;

#define VK_BUFFER_USAGE_TRANSFER_SRC_BIT 0x00000001
#define VK_BUFFER_USAGE_TRANSFER_DST_BIT 0x00000002
#define VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT 0x00000004
#define VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT 0x00000008
#define VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT 0x00000010
#define VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 0x00000020
#define VK_BUFFER_USAGE_INDEX_BUFFER_BIT 0x00000040
#define VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 0x00000080
#define VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT 0x00000100

#define VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 0x00000001
#define VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 0x00000002
#define VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 0x00000004
#define VK_MEMORY_PROPERTY_HOST_CACHED_BIT 0x00000008
#define VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT 0x00000010
#define VK_MEMORY_PROPERTY_PROTECTED_BIT 0x00000020

#ifndef VK_NULL_HANDLE
#define VK_NULL_HANDLE NULL
#endif

#ifndef VK_SUCCESS
#define VK_SUCCESS 0
#endif

#ifndef VK_PIPELINE_BIND_POINT_GRAPHICS
#define VK_PIPELINE_BIND_POINT_GRAPHICS 0
#endif

#ifndef VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
#define VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 0
#endif

#ifndef VK_FORMAT_R8_UNORM
#define VK_FORMAT_R8_UNORM 9
#endif
#ifndef VK_FORMAT_R8G8B8A8_SRGB
#define VK_FORMAT_R8G8B8A8_SRGB 43
#endif
#ifndef VK_FORMAT_R16G16B16A16_SFLOAT
#define VK_FORMAT_R16G16B16A16_SFLOAT 97
#endif
#ifndef VK_FORMAT_R32G32B32A32_SFLOAT
#define VK_FORMAT_R32G32B32A32_SFLOAT 109
#endif

static inline void vkDestroyImageView(VkDevice device, VkImageView imageView,
                                      const void *pAllocator) {
  (void)device;
  (void)imageView;
  (void)pAllocator;
}
static inline void vkDestroyImage(VkDevice device, VkImage image,
                                  const void *pAllocator) {
  (void)device;
  (void)image;
  (void)pAllocator;
}
static inline void vkFreeMemory(VkDevice device, VkDeviceMemory memory,
                                const void *pAllocator) {
  (void)device;
  (void)memory;
  (void)pAllocator;
}
static inline void vkDestroyBuffer(VkDevice device, VkBuffer buffer,
                                   const void *pAllocator) {
  (void)device;
  (void)buffer;
  (void)pAllocator;
}
static inline void vkCmdBindDescriptorSets(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, u32 firstSet, u32 descriptorSetCount,
    const VkDescriptorSet *pDescriptorSets, u32 dynamicOffsetCount,
    const u32 *pDynamicOffsets) {
  (void)commandBuffer;
  (void)pipelineBindPoint;
  (void)layout;
  (void)firstSet;
  (void)descriptorSetCount;
  (void)pDescriptorSets;
  (void)dynamicOffsetCount;
  (void)pDynamicOffsets;
}
static inline VkResult vkCreateShaderModule(VkDevice device,
                                            const void *pCreateInfo,
                                            const void *pAllocator,
                                            VkShaderModule *pShaderModule) {
  (void)device;
  (void)pCreateInfo;
  (void)pAllocator;
  if (pShaderModule) {
    *pShaderModule = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
static inline void vkDestroyShaderModule(VkDevice device,
                                         VkShaderModule shaderModule,
                                         const void *pAllocator) {
  (void)device;
  (void)shaderModule;
  (void)pAllocator;
}
static inline VkResult vkCreateSampler(VkDevice device, const void *pCreateInfo,
                                       const void *pAllocator,
                                       VkSampler *pSampler) {
  (void)device;
  (void)pCreateInfo;
  (void)pAllocator;
  if (pSampler) {
    *pSampler = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
static inline VkResult
vkCreateDescriptorSetLayout(VkDevice device, const void *pCreateInfo,
                            const void *pAllocator,
                            VkDescriptorSetLayout *pSetLayout) {
  (void)device;
  (void)pCreateInfo;
  (void)pAllocator;
  if (pSetLayout) {
    *pSetLayout = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
static inline VkResult
vkAllocateDescriptorSets(VkDevice device, const void *pAllocateInfo,
                         VkDescriptorSet *pDescriptorSets) {
  (void)device;
  (void)pAllocateInfo;
  if (pDescriptorSets) {
    *pDescriptorSets = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
static inline void vkUpdateDescriptorSets(VkDevice device,
                                          u32 descriptorWriteCount,
                                          const void *pDescriptorWrites,
                                          u32 descriptorCopyCount,
                                          const void *pDescriptorCopies) {
  (void)device;
  (void)descriptorWriteCount;
  (void)pDescriptorWrites;
  (void)descriptorCopyCount;
  (void)pDescriptorCopies;
}
static inline VkResult
vkCreatePipelineLayout(VkDevice device, const void *pCreateInfo,
                       const void *pAllocator,
                       VkPipelineLayout *pPipelineLayout) {
  (void)device;
  (void)pCreateInfo;
  (void)pAllocator;
  if (pPipelineLayout) {
    *pPipelineLayout = VK_NULL_HANDLE;
  }
  return VK_SUCCESS;
}
#endif

// Renderer state
typedef struct VulkanRenderer {
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue compute_queue;
  VkQueue present_queue;
  u32 graphics_queue_family;
  u32 compute_queue_family;
  u32 present_queue_family;

  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;
  VkFormat swapchain_format;
  VkExtent2D swapchain_extent;
  VkImage *swapchain_images;
  VkImageView *swapchain_image_views;
  u32 swapchain_image_count;

  VkRenderPass render_pass;
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  VkCommandPool command_pool;
  VkCommandBuffer *command_buffers;
  u32 command_buffer_count;

  VkSemaphore *image_available_semaphores;
  VkSemaphore *render_finished_semaphores;
  VkFence *in_flight_fences;
  u32 current_frame;
  u32 max_frames_in_flight;

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;

  VkDescriptorPool descriptor_pool;
  VkDescriptorSet *descriptor_sets;

  VkImage texture_image;
  VkDeviceMemory texture_image_memory;
  VkImageView texture_image_view;
  VkSampler texture_sampler;

  VkFramebuffer *framebuffers;
  u32 framebuffer_count;

  Mat4 view_matrix;
  Mat4 projection_matrix;

  bool framebuffer_resized;
  // Ray tracing capability flags (scaffolding only)
  bool ray_tracing_supported;
  bool ray_tracing_enabled;

  // Ambient lighting
  f32 ambient_light;
  bool uniform_buffer_mapped;

  // Dynamic Resolution Scaling (8K Support)
  f32 render_scale;         // 0.5 to 2.0, default 1.0
  VkExtent2D render_extent; // Internal render resolution (scaled)
  VkImage offscreen_color_image;
  VkDeviceMemory offscreen_color_memory;
  VkImageView offscreen_color_view;
  VkImage offscreen_depth_image;
  VkDeviceMemory offscreen_depth_memory;
  VkImageView offscreen_depth_view;
  VkRenderPass offscreen_render_pass; // Render pass for offscreen target
  VkFramebuffer offscreen_framebuffer;

  // TAA State
  u32 jitter_index;
  Vec2 jitter_offset;
} VulkanRenderer;

// Camera (defined in camera.h)
#include "include/rendering/camera.h"

// Render state
typedef struct {
  VulkanRenderer *renderer;
  Camera camera;
  Mat4 view_matrix;
  Mat4 projection_matrix;
  u32 window_width;
  u32 window_height;
  bool vsync;
} RenderState;

#ifdef VULKAN_BUILD
// Additional Vulkan functions
bool vulkan_create_surface(VulkanRenderer *renderer, void *window);
bool vulkan_create_swapchain(VulkanRenderer *renderer, bool vsync);
bool vulkan_recreate_swapchain(VulkanRenderer *renderer, u32 width, u32 height);
bool vulkan_create_render_pass(VulkanRenderer *renderer);
bool vulkan_create_graphics_pipeline(VulkanRenderer *renderer, VFS *vfs);
bool vulkan_create_framebuffers(VulkanRenderer *renderer);
bool vulkan_begin_frame_updated(VulkanRenderer *renderer, u32 *image_index);
void vulkan_end_frame_updated(VulkanRenderer *renderer, u32 image_index);
void vulkan_render_chunk_mesh(VulkanRenderer *renderer, Chunk *chunk, Mat4 view,
                              Mat4 proj);
void vulkan_render_dynamic_mesh(VulkanRenderer *renderer, Mesh *mesh, Mat4 view,
                                Mat4 proj);
void vulkan_render_physics_debug(VulkanRenderer *renderer, Mat4 view,
                                 Mat4 proj);
void vulkan_update_camera_uniforms(VulkanRenderer *renderer, Camera *camera,
                                   f32 aspect);
void vulkan_set_ambient_light(VulkanRenderer *renderer, f32 ambient_light);
void vulkan_render_block_highlight(VulkanRenderer *renderer,
                                   struct PlayerSystem *player_system);

// Buffer management
bool vulkan_create_buffer(VulkanRenderer *renderer, VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBuffer *buffer,
                          VkDeviceMemory *buffer_memory);
void vulkan_destroy_buffer(VulkanRenderer *renderer, VkBuffer buffer,
                           VkDeviceMemory buffer_memory);
bool vulkan_create_chunk_vertex_buffer(VulkanRenderer *renderer, Mesh *mesh,
                                       VkBuffer *buffer,
                                       VkDeviceMemory *memory);
bool vulkan_create_chunk_index_buffer(VulkanRenderer *renderer, Mesh *mesh,
                                      VkBuffer *buffer, VkDeviceMemory *memory);
void vulkan_copy_buffer(VulkanRenderer *renderer, VkBuffer src, VkBuffer dst,
                        VkDeviceSize size);
bool vulkan_update_chunk_buffers(VulkanRenderer *renderer, Mesh *mesh,
                                 VkBuffer vertex_buffer, VkBuffer index_buffer);

// Dynamic Resolution Scaling API
bool vulkan_create_offscreen_resources(VulkanRenderer *renderer);
void vulkan_destroy_offscreen_resources(VulkanRenderer *renderer);
void vulkan_blit_offscreen_to_swapchain(VulkanRenderer *renderer,
                                        VkCommandBuffer cmd,
                                        VkImage swapchain_image);
void vulkan_set_render_scale(VulkanRenderer *renderer, f32 scale);
#else
// Stubs for non-Vulkan builds
static inline bool vulkan_create_surface(VulkanRenderer *renderer,
                                         void *window) {
  return false;
}
static inline bool vulkan_create_swapchain(VulkanRenderer *renderer,
                                           bool vsync) {
  return false;
}
static inline bool vulkan_recreate_swapchain(VulkanRenderer *renderer,
                                             u32 width, u32 height) {
  return false;
}
static inline bool vulkan_create_render_pass(VulkanRenderer *renderer) {
  return false;
}
static inline bool vulkan_create_graphics_pipeline(VulkanRenderer *renderer,
                                                   VFS *vfs) {
  return false;
}
static inline bool vulkan_create_framebuffers(VulkanRenderer *renderer) {
  return false;
}
static inline bool vulkan_begin_frame_updated(VulkanRenderer *renderer,
                                              u32 *image_index) {
  return false;
}
static inline void vulkan_end_frame_updated(VulkanRenderer *renderer,
                                            u32 image_index) {}
static inline void vulkan_render_chunk_mesh(VulkanRenderer *renderer,
                                            Chunk *chunk, Mat4 view,
                                            Mat4 proj) {}
static inline void vulkan_render_dynamic_mesh(VulkanRenderer *renderer,
                                              Mesh *mesh, Mat4 view,
                                              Mat4 proj) {}
static inline void vulkan_render_physics_debug(VulkanRenderer *renderer,
                                               Mat4 view, Mat4 proj) {}
static inline void vulkan_update_camera_uniforms(VulkanRenderer *renderer,
                                                 Camera *camera, f32 aspect) {}
static inline void vulkan_set_ambient_light(VulkanRenderer *renderer,
                                            f32 ambient_light) {}
static inline void
vulkan_render_block_highlight(VulkanRenderer *renderer,
                              struct PlayerSystem *player_system) {}

static inline bool
vulkan_create_buffer(VulkanRenderer *renderer, VkDeviceSize size,
                     VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkBuffer *buffer, VkDeviceMemory *buffer_memory) {
  return false;
}
static inline void vulkan_destroy_buffer(VulkanRenderer *renderer,
                                         VkBuffer buffer,
                                         VkDeviceMemory buffer_memory) {}
static inline bool vulkan_create_chunk_vertex_buffer(VulkanRenderer *renderer,
                                                     Mesh *mesh,
                                                     VkBuffer *buffer,
                                                     VkDeviceMemory *memory) {
  return false;
}
static inline bool vulkan_create_chunk_index_buffer(VulkanRenderer *renderer,
                                                    Mesh *mesh,
                                                    VkBuffer *buffer,
                                                    VkDeviceMemory *memory) {
  return false;
}
static inline void vulkan_copy_buffer(VulkanRenderer *renderer, VkBuffer src,
                                      VkBuffer dst, VkDeviceSize size) {}
static inline bool vulkan_update_chunk_buffers(VulkanRenderer *renderer,
                                               Mesh *mesh,
                                               VkBuffer vertex_buffer,
                                               VkBuffer index_buffer) {
  return false;
}
#endif

// Initialize Vulkan renderer
bool vulkan_init(VulkanRenderer *renderer, void *window, u32 width, u32 height,
                 struct GameConfig *config);
void vulkan_cleanup(VulkanRenderer *renderer);

// Render functions
bool vulkan_begin_frame(VulkanRenderer *renderer, u32 *image_index);
void vulkan_end_frame(VulkanRenderer *renderer, u32 image_index);
void vulkan_render_chunk(VulkanRenderer *renderer, void *mesh_data,
                         u32 vertex_count, u32 index_count);
void vulkan_update_camera(VulkanRenderer *renderer, Camera *camera);

// Camera functions (implemented in camera.c)
void camera_init(Camera *camera, Vec3 position, f32 yaw, f32 pitch);
void camera_update(Camera *camera, f32 delta_time);
Mat4 camera_get_view_matrix(Camera *camera);
Mat4 camera_get_projection_matrix(Camera *camera, f32 aspect);

// Texture management
bool vulkan_create_texture_image(VulkanRenderer *renderer, const char *path);
bool vulkan_create_texture_sampler(VulkanRenderer *renderer);

// Ray tracing scaffolding (compile-safe stubs). These
// functions return false or no-op on
// platforms/devices without ray tracing support. They
// are safe to call regardless of build configuration.
bool vulkan_rt_is_supported(VulkanRenderer *renderer);
bool vulkan_rt_init(VulkanRenderer *renderer);
void vulkan_rt_cleanup(VulkanRenderer *renderer);

// Camera functions (implemented in camera.c)
void camera_init(Camera *camera, Vec3 position, f32 yaw, f32 pitch);
void camera_update(Camera *camera, f32 delta_time);
Mat4 camera_get_view_matrix(Camera *camera);
Mat4 camera_get_projection_matrix(Camera *camera, f32 aspect);

#endif // VULKAN_H
