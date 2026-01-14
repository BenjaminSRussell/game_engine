// include/render/gpu_text.h
//
// Purpose: GPU-side text rendering system with font atlas binding and glyph
// rendering. Manages text mesh buffers, font atlas textures, and text rendering
// pipelines on GPU.
//
#ifndef GPU_TEXT_H
#define GPU_TEXT_H

#include "include/rendering/text_renderer.h"
#include <common.h>
#include <math/mat4.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

#include <include/rendering/vulkan.h>

// Forward declarations
typedef struct GPUTextManager GPUTextManager;
typedef struct GPUTextMesh GPUTextMesh;

// GPU text mesh (GPU buffers for rendered text)
struct GPUTextMesh {
  u32 mesh_id;
  char text[1024];

  // GPU buffers
  VkBuffer vertex_buffer;
  VkBuffer index_buffer;
  VkDeviceMemory vertex_memory;
  VkDeviceMemory index_memory;

  // Mesh data
  u32 vertex_count;
  u32 index_count;

  // Layout
  VkBufferView vertex_view;
  VkBufferView index_view;

  // Font and rendering properties
  u32 font_id;
  Vec4 color;
  Vec2 position;
  Mat4 transform;

  // Descriptor set for this text mesh
  VkDescriptorSet descriptor_set;

  // Layout state
  VkBuffer staging_buffer;
  VkDeviceMemory staging_memory;
  bool needs_update;
  bool initialized;
};

// GPU text manager
// Push constants
typedef struct {
  Vec4 color;
  Vec2 position;
  f32 opacity;
  u32 padding;
} GPUTextPushConstant;

struct GPUTextManager {
  // Vulkan objects
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkCommandPool transfer_pool;
  VkQueue transfer_queue;

  // Text mesh pool
  GPUTextMesh text_meshes[256];
  u32 mesh_count;

  // Font atlas texture management
  VkImage font_atlas_image;
  VkImageView font_atlas_view;
  VkDeviceMemory font_atlas_memory;
  VkSampler font_atlas_sampler;
  VkDescriptorSet font_atlas_descriptor;

  // Samplers for text rendering
  VkSampler linear_sampler;
  VkSampler nearest_sampler;

  // Pipeline and layout
  VkPipeline text_pipeline;
  VkPipelineLayout pipeline_layout;

  // Descriptor management
  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout descriptor_layout;

  // Push constants
  GPUTextPushConstant push_constants; // Use the struct type defined above

  // Logging
  bool verbose;
  bool initialized;
};

// ==============================================================================
// Lifecycle Management
// ==============================================================================

/**
 * Initialize the GPU text rendering system.
 *
 * @param manager Pointer to GPUTextManager
 * @param device Vulkan logical device
 * @param physical_device Vulkan physical device
 * @param transfer_pool Command pool for transfer operations
 * @param transfer_queue Graphics/transfer queue
 * @return true on success, false on failure
 */
bool gpu_text_init(GPUTextManager *manager, VkDevice device,
                   VkPhysicalDevice physical_device,
                   VkCommandPool transfer_pool, VkQueue transfer_queue);

/**
 * Shutdown the GPU text rendering system.
 *
 * @param manager Pointer to GPUTextManager
 */
void gpu_text_shutdown(GPUTextManager *manager);

// ==============================================================================
// Font Atlas Management
// ==============================================================================

/**
 * Upload font atlas texture to GPU.
 *
 * @param manager Pointer to GPUTextManager
 * @param font Pointer to Font object with atlas data
 * @return true on success, false on failure
 */
bool gpu_text_upload_font_atlas(GPUTextManager *manager, Font *font);

/**
 * Create font atlas descriptor set for shader binding.
 *
 * @param manager Pointer to GPUTextManager
 * @return true on success, false on failure
 */
bool gpu_text_create_font_atlas_descriptor(GPUTextManager *manager);

/**
 * Bind font atlas to rendering command buffer.
 *
 * @param manager Pointer to GPUTextManager
 * @param cmd_buffer Vulkan command buffer
 * @param layout Pipeline layout
 */
void gpu_text_bind_font_atlas(GPUTextManager *manager,
                              VkCommandBuffer cmd_buffer,
                              VkPipelineLayout layout);

// ==============================================================================
// Text Mesh Buffer Management
// ==============================================================================

/**
 * Create GPU text mesh from text data.
 * Allocates vertex and index buffers, uploads mesh data to GPU.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Unique identifier for this text mesh
 * @param text Pointer to text data to render
 * @param vertices Array of vertex positions (Vec3)
 * @param vertex_count Number of vertices
 * @param indices Array of indices
 * @param index_count Number of indices
 * @return true on success, false on failure
 */
bool gpu_text_create_mesh(GPUTextManager *manager, u32 mesh_id,
                          const char *text, Vec3 *vertices, u32 vertex_count,
                          u32 *indices, u32 index_count);

/**
 * Update existing text mesh with new geometry.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 * @param vertices New vertex data
 * @param vertex_count Number of vertices
 * @param indices New index data
 * @param index_count Number of indices
 * @return true on success, false on failure
 */
bool gpu_text_update_mesh(GPUTextManager *manager, u32 mesh_id, Vec3 *vertices,
                          u32 vertex_count, u32 *indices, u32 index_count);

/**
 * Delete text mesh and free GPU resources.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 */
void gpu_text_delete_mesh(GPUTextManager *manager, u32 mesh_id);

/**
 * Get text mesh by ID.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 * @return Pointer to GPUTextMesh or NULL if not found
 */
GPUTextMesh *gpu_text_get_mesh(GPUTextManager *manager, u32 mesh_id);

// ==============================================================================
// Text Rendering Properties
// ==============================================================================

/**
 * Set text color using push constants.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 * @param color Text color (RGBA)
 */
void gpu_text_set_color(GPUTextManager *manager, u32 mesh_id, Vec4 color);

/**
 * Set text position (world space or screen space).
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 * @param position Position (X, Y, Z or screen coords)
 */
void gpu_text_set_position(GPUTextManager *manager, u32 mesh_id, Vec2 position);

/**
 * Set text transformation matrix.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 * @param transform 4x4 transformation matrix
 */
void gpu_text_set_transform(GPUTextManager *manager, u32 mesh_id,
                            Mat4 transform);

/**
 * Set text opacity/alpha.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 * @param opacity Opacity value (0.0 - 1.0)
 */
void gpu_text_set_opacity(GPUTextManager *manager, u32 mesh_id, f32 opacity);

// ==============================================================================
// Rendering
// ==============================================================================

/**
 * Bind text mesh to command buffer for rendering.
 * Binds vertex/index buffers and descriptor sets.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 * @param cmd_buffer Vulkan command buffer
 */
void gpu_text_bind_mesh(GPUTextManager *manager, u32 mesh_id,
                        VkCommandBuffer cmd_buffer);

/**
 * Record text draw command.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_id Mesh identifier
 * @param cmd_buffer Vulkan command buffer
 */
void gpu_text_draw_mesh(GPUTextManager *manager, u32 mesh_id,
                        VkCommandBuffer cmd_buffer);

/**
 * Batch render multiple text meshes.
 *
 * @param manager Pointer to GPUTextManager
 * @param mesh_ids Array of mesh identifiers
 * @param mesh_count Number of meshes to render
 * @param cmd_buffer Vulkan command buffer
 */
void gpu_text_draw_batch(GPUTextManager *manager, u32 *mesh_ids, u32 mesh_count,
                         VkCommandBuffer cmd_buffer);

// ==============================================================================
// Pipeline Management
// ==============================================================================

/**
 * Create text rendering pipeline.
 *
 * @param manager Pointer to GPUTextManager
 * @param vertex_shader Path to vertex shader
 * @param fragment_shader Path to fragment shader
 * @return true on success, false on failure
 */
bool gpu_text_create_pipeline(GPUTextManager *manager,
                              const char *vertex_shader,
                              const char *fragment_shader);

/**
 * Bind text rendering pipeline to command buffer.
 *
 * @param manager Pointer to GPUTextManager
 * @param cmd_buffer Vulkan command buffer
 */
void gpu_text_bind_pipeline(GPUTextManager *manager,
                            VkCommandBuffer cmd_buffer);

// ==============================================================================
// Query Functions
// ==============================================================================

/**
 * Get number of allocated text meshes.
 *
 * @param manager Pointer to GPUTextManager
 * @return Number of text meshes
 */
u32 gpu_text_get_mesh_count(GPUTextManager *manager);

/**
 * Get font atlas texture handle.
 *
 * @param manager Pointer to GPUTextManager
 * @return VkImageView for font atlas
 */
VkImageView gpu_text_get_font_atlas(GPUTextManager *manager);

/**
 * Get text rendering pipeline.
 *
 * @param manager Pointer to GPUTextManager
 * @return VkPipeline for text rendering
 */
VkPipeline gpu_text_get_pipeline(GPUTextManager *manager);

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

/**
 * Log text system information.
 *
 * @param manager Pointer to GPUTextManager
 */
void gpu_text_log_info(GPUTextManager *manager);

/**
 * Log statistics about text meshes.
 *
 * @param manager Pointer to GPUTextManager
 */
void gpu_text_log_statistics(GPUTextManager *manager);

/**
 * Validate text system state.
 *
 * @param manager Pointer to GPUTextManager
 * @return true if valid, false otherwise
 */
bool gpu_text_validate(GPUTextManager *manager);

#endif // GPU_TEXT_H
