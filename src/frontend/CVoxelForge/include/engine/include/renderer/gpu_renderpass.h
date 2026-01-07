// include/render/gpu_renderpass.h
//
// Purpose: Complete GPU render pass integration system.
// Orchestrates all GPU subsystems (materials, lighting, textures, text) into
// cohesive rendering pipeline.
//
#ifndef GPU_RENDERPASS_H
#define GPU_RENDERPASS_H

#include "gpu_lighting.h"
#include "gpu_material.h"
#include "gpu_memory.h"
#include "gpu_postprocess.h"
#include "gpu_shader_reload.h"
#include "gpu_text.h"
#include "gpu_texture.h"
#include <common.h>
#include <math/mat4.h>

#include <vulkan/vulkan.h>

// Forward declarations
typedef struct GPURenderPassManager GPURenderPassManager;

// Render pass types
typedef enum {
  RENDERPASS_SCENE,        // Main 3D scene rendering
  RENDERPASS_TRANSPARENCY, // Transparent object rendering
  RENDERPASS_UI,           // UI/HUD rendering
  RENDERPASS_COMPUTE,      // Compute shader dispatch
  RENDERPASS_COUNT,
} RenderPassType;

// Render pass info
typedef struct {
  RenderPassType type;
  VkRenderPass render_pass;
  VkFramebuffer framebuffer;
  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;

  // Descriptor management
  VkDescriptorSetLayout descriptor_layout;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_set;

  // Viewport and scissor
  VkViewport viewport;
  VkRect2D scissor;

  bool initialized;
} RenderPassInfo;

// GPU render pass manager (top-level orchestrator)
struct GPURenderPassManager {
  // Vulkan core
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkQueue graphics_queue;
  VkQueue transfer_queue;
  VkCommandPool graphics_pool;
  VkCommandPool transfer_pool;

  // Primary framebuffer
  VkImage color_image;
  VkImageView color_view;
  VkImage depth_image;
  VkImageView depth_view;
  VkFramebuffer main_framebuffer;
  VkRenderPass main_render_pass;

  // Render passes
  RenderPassInfo render_passes[RENDERPASS_COUNT];

  // GPU subsystems
  GPUMaterialManager *material_manager;
  GPULightingManager *lighting_manager;
  GPUTextureManager *texture_manager;
  GPUTextManager *text_manager;
  GPUPostProcessManager *postprocess_manager;
  GPUShaderReloadManager *shader_reload_manager;
  GPUMemoryAllocator *memory_allocator;

  // Primary command buffers
  VkCommandBuffer primary_cmd_buffer;
  VkCommandBuffer transfer_cmd_buffer;

  // Synchronization
  VkSemaphore image_available;
  VkSemaphore render_complete;
  VkFence render_fence;

  // Camera and view matrices
  Mat4 view_matrix;
  Mat4 projection_matrix;
  Mat4 view_projection;

  // Rendering statistics
  u32 draw_call_count;
  u32 vertex_count;
  u32 triangle_count;
  u32 texture_bindings;

  // Configuration
  u32 width, height;
  bool vsync_enabled;
  bool verbose;

  bool initialized;
};

// ==============================================================================
// Lifecycle Management
// ==============================================================================

/**
 * Initialize GPU render pass manager.
 * Sets up all GPU subsystems and primary render pass.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param device Vulkan logical device
 * @param physical_device Vulkan physical device
 * @param graphics_queue Graphics queue
 * @param transfer_queue Transfer queue
 * @param graphics_pool Command pool for graphics
 * @param transfer_pool Command pool for transfers
 * @param width Framebuffer width
 * @param height Framebuffer height
 * @return true on success, false on failure
 */
bool gpu_renderpass_init(GPURenderPassManager *manager, VkDevice device,
                         VkPhysicalDevice physical_device,
                         VkQueue graphics_queue, VkQueue transfer_queue,
                         VkCommandPool graphics_pool,
                         VkCommandPool transfer_pool, u32 width, u32 height);

/**
 * Shutdown GPU render pass manager.
 * Cleans up all subsystems and GPU resources.
 *
 * @param manager Pointer to GPURenderPassManager
 */
void gpu_renderpass_shutdown(GPURenderPassManager *manager);

/**
 * Resize render pass framebuffer.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param new_width New width
 * @param new_height New height
 * @return true on success, false on failure
 */
bool gpu_renderpass_resize(GPURenderPassManager *manager, u32 new_width,
                           u32 new_height);

// ==============================================================================
// Render Pass Management
// ==============================================================================

/**
 * Begin rendering frame.
 * Acquires image, begins command buffer, starts render pass.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param render_type RenderPassType to execute
 * @return true on success, false on failure
 */
bool gpu_renderpass_begin_frame(GPURenderPassManager *manager,
                                RenderPassType render_type);

/**
 * End rendering frame.
 * Ends render pass, submits command buffer, presents to swapchain.
 *
 * @param manager Pointer to GPURenderPassManager
 * @return true on success, false on failure
 */
bool gpu_renderpass_end_frame(GPURenderPassManager *manager);

/**
 * Begin render pass on command buffer.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param pass_type RenderPassType
 * @param cmd_buffer Command buffer to record into
 * @return true on success, false on failure
 */
bool gpu_renderpass_begin(GPURenderPassManager *manager,
                          RenderPassType pass_type, VkCommandBuffer cmd_buffer);

/**
 * End render pass on command buffer.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param cmd_buffer Command buffer
 */
void gpu_renderpass_end(GPURenderPassManager *manager,
                        VkCommandBuffer cmd_buffer);

// ==============================================================================
// Scene Rendering
// ==============================================================================

/**
 * Record scene render commands.
 * Binds materials, lighting, textures and issues draw calls.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param cmd_buffer Command buffer to record into
 * @return true on success, false on failure
 */
bool gpu_renderpass_render_scene(GPURenderPassManager *manager,
                                 VkCommandBuffer cmd_buffer);

/**
 * Render single geometry with material.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param material_id Material to use
 * @param vertex_count Number of vertices to draw
 * @param cmd_buffer Command buffer
 */
void gpu_renderpass_draw_geometry(GPURenderPassManager *manager,
                                  u32 material_id, u32 vertex_count,
                                  VkCommandBuffer cmd_buffer);

/**
 * Render text mesh.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param text_mesh_id Text mesh ID
 * @param cmd_buffer Command buffer
 */
void gpu_renderpass_draw_text(GPURenderPassManager *manager, u32 text_mesh_id,
                              VkCommandBuffer cmd_buffer);

// ==============================================================================
// Camera and View Setup
// ==============================================================================

/**
 * Set view matrix (camera position and orientation).
 *
 * @param manager Pointer to GPURenderPassManager
 * @param view_matrix 4x4 view matrix
 */
void gpu_renderpass_set_view_matrix(GPURenderPassManager *manager,
                                    Mat4 view_matrix);

/**
 * Set projection matrix (perspective/orthographic).
 *
 * @param manager Pointer to GPURenderPassManager
 * @param projection_matrix 4x4 projection matrix
 */
void gpu_renderpass_set_projection_matrix(GPURenderPassManager *manager,
                                          Mat4 projection_matrix);

/**
 * Get combined view-projection matrix.
 *
 * @param manager Pointer to GPURenderPassManager
 * @return Combined view-projection matrix
 */
Mat4 gpu_renderpass_get_view_projection(GPURenderPassManager *manager);

// ==============================================================================
// Subsystem Access
// ==============================================================================

/**
 * Get material manager (for material operations).
 *
 * @param manager Pointer to GPURenderPassManager
 * @return Pointer to GPUMaterialManager
 */
GPUMaterialManager *
gpu_renderpass_get_material_manager(GPURenderPassManager *manager);

/**
 * Get lighting manager (for light operations).
 *
 * @param manager Pointer to GPURenderPassManager
 * @return Pointer to GPULightingManager
 */
GPULightingManager *
gpu_renderpass_get_lighting_manager(GPURenderPassManager *manager);

/**
 * Get texture manager (for texture operations).
 *
 * @param manager Pointer to GPURenderPassManager
 * @return Pointer to GPUTextureManager
 */
GPUTextureManager *
gpu_renderpass_get_texture_manager(GPURenderPassManager *manager);

/**
 * Get text manager (for text operations).
 *
 * @param manager Pointer to GPURenderPassManager
 * @return Pointer to GPUTextManager
 */
GPUTextManager *gpu_renderpass_get_text_manager(GPURenderPassManager *manager);

/**
 * Get post-processing manager.
 *
 * @param manager Pointer to GPURenderPassManager
 * @return Pointer to GPUPostProcessManager
 */
GPUPostProcessManager *
gpu_renderpass_get_postprocess_manager(GPURenderPassManager *manager);

/**
 * Get memory allocator.
 *
 * @param manager Pointer to GPURenderPassManager
 * @return Pointer to GPUMemoryAllocator
 */
GPUMemoryAllocator *
gpu_renderpass_get_memory_allocator(GPURenderPassManager *manager);

// ==============================================================================
// Synchronization
// ==============================================================================

/**
 * Wait for GPU to finish rendering.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param timeout_ns Timeout in nanoseconds
 * @return true if GPU finished, false on timeout
 */
bool gpu_renderpass_wait_gpu(GPURenderPassManager *manager, u64 timeout_ns);

/**
 * Reset GPU command buffers.
 *
 * @param manager Pointer to GPURenderPassManager
 */
void gpu_renderpass_reset_buffers(GPURenderPassManager *manager);

// ==============================================================================
// Statistics and Diagnostics
// ==============================================================================

/**
 * Get render statistics for this frame.
 *
 * @param manager Pointer to GPURenderPassManager
 * @param out_draw_calls Pointer to receive draw call count
 * @param out_vertices Pointer to receive vertex count
 * @param out_triangles Pointer to receive triangle count
 */
void gpu_renderpass_get_statistics(GPURenderPassManager *manager,
                                   u32 *out_draw_calls, u32 *out_vertices,
                                   u32 *out_triangles);

/**
 * Reset frame statistics.
 *
 * @param manager Pointer to GPURenderPassManager
 */
void gpu_renderpass_reset_statistics(GPURenderPassManager *manager);

/**
 * Log render pass information.
 *
 * @param manager Pointer to GPURenderPassManager
 */
void gpu_renderpass_log_info(GPURenderPassManager *manager);

/**
 * Log frame statistics.
 *
 * @param manager Pointer to GPURenderPassManager
 */
void gpu_renderpass_log_frame_stats(GPURenderPassManager *manager);

/**
 * Log all GPU subsystem information.
 *
 * @param manager Pointer to GPURenderPassManager
 */
void gpu_renderpass_log_subsystems(GPURenderPassManager *manager);

/**
 * Validate render pass state.
 *
 * @param manager Pointer to GPURenderPassManager
 * @return true if valid, false if errors detected
 */
bool gpu_renderpass_validate(GPURenderPassManager *manager);

#endif // GPU_RENDERPASS_H
