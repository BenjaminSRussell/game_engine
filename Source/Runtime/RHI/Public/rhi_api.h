/**
 * @file rhi_api.h
 * @brief Main RHI API
 */

#ifndef VOXELFORGE_RHI_API_H
#define VOXELFORGE_RHI_API_H

#include "RHI/Public/rhi_descriptors.h"
#include "RHI/Public/rhi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Initialization
// ============================================================================

VF_API VF_Result rhi_init(const RHI_DeviceDesc *desc);
VF_API void rhi_shutdown(void);
VF_API RHI_Backend rhi_get_backend(void);
VF_API RHI_Device *rhi_get_device(void);

// ============================================================================
// Swapchain
// ============================================================================

VF_API RHI_Swapchain *rhi_swapchain_create(const RHI_SwapchainDesc *desc);
VF_API void rhi_swapchain_destroy(RHI_Swapchain *swapchain);
VF_API VF_Result rhi_swapchain_resize(RHI_Swapchain *swapchain, u32 width,
                                      u32 height);
VF_API VF_Result rhi_swapchain_acquire_image(RHI_Swapchain *swapchain,
                                             RHI_Semaphore *signal_sem,
                                             u32 *image_index);
VF_API VF_Result rhi_swapchain_present(RHI_Swapchain *swapchain,
                                       RHI_Semaphore *wait_sem);
VF_API RHI_TextureView *rhi_swapchain_get_image(RHI_Swapchain *swapchain,
                                                u32 index);
VF_API u32 rhi_swapchain_get_image_count(RHI_Swapchain *swapchain);

// ============================================================================
// Buffer
// ============================================================================

VF_API RHI_Buffer *rhi_buffer_create(const RHI_BufferDesc *desc);
VF_API void rhi_buffer_destroy(RHI_Buffer *buffer);
VF_API void *rhi_buffer_map(RHI_Buffer *buffer);
VF_API void rhi_buffer_unmap(RHI_Buffer *buffer);
VF_API void rhi_buffer_upload(RHI_Buffer *buffer, usize offset,
                              const void *data, usize size);

// ============================================================================
// Texture
// ============================================================================

VF_API RHI_Texture *rhi_texture_create(const RHI_TextureDesc *desc);
VF_API void rhi_texture_destroy(RHI_Texture *texture);
VF_API RHI_TextureView *
rhi_texture_view_create(const RHI_TextureViewDesc *desc);
VF_API void rhi_texture_view_destroy(RHI_TextureView *view);

// ============================================================================
// Sampler
// ============================================================================

VF_API RHI_Sampler *rhi_sampler_create(const RHI_SamplerDesc *desc);
VF_API void rhi_sampler_destroy(RHI_Sampler *sampler);

// ============================================================================
// Shader
// ============================================================================

VF_API RHI_Shader *rhi_shader_create(const RHI_ShaderDesc *desc);
VF_API void rhi_shader_destroy(RHI_Shader *shader);

// ============================================================================
// Render Pass
// ============================================================================

VF_API RHI_RenderPass *rhi_render_pass_create(const RHI_RenderPassDesc *desc);
VF_API void rhi_render_pass_destroy(RHI_RenderPass *render_pass);

// ============================================================================
// Pipeline
// ============================================================================

VF_API RHI_Pipeline *
rhi_graphics_pipeline_create(const RHI_GraphicsPipelineDesc *desc);
VF_API RHI_Pipeline *
rhi_compute_pipeline_create(const RHI_ComputePipelineDesc *desc);
VF_API void rhi_pipeline_destroy(RHI_Pipeline *pipeline);

// ============================================================================
// Command Buffer
// ============================================================================

VF_API RHI_CommandPool *rhi_command_pool_create(RHI_QueueType queue_type);
VF_API void rhi_command_pool_destroy(RHI_CommandPool *pool);
VF_API void rhi_command_pool_reset(RHI_CommandPool *pool);

VF_API RHI_CommandBuffer *rhi_command_buffer_allocate(RHI_CommandPool *pool);
VF_API void rhi_command_buffer_free(RHI_CommandBuffer *cmd);
VF_API void rhi_command_buffer_begin(RHI_CommandBuffer *cmd);
VF_API void rhi_command_buffer_end(RHI_CommandBuffer *cmd);
VF_API void rhi_command_buffer_reset(RHI_CommandBuffer *cmd);

// ============================================================================
// Command Buffer - Render Commands
// ============================================================================

typedef struct RHI_RenderPassBeginInfo {
  RHI_RenderPass *render_pass;
  RHI_Framebuffer *framebuffer;
  i32 x, y;
  u32 width, height;
  f32 clear_color[4];
  f32 clear_depth;
  u8 clear_stencil;
} RHI_RenderPassBeginInfo;

VF_API void rhi_cmd_begin_render_pass(RHI_CommandBuffer *cmd,
                                      const RHI_RenderPassBeginInfo *info);
VF_API void rhi_cmd_end_render_pass(RHI_CommandBuffer *cmd);
VF_API void rhi_cmd_bind_pipeline(RHI_CommandBuffer *cmd,
                                  RHI_Pipeline *pipeline);
VF_API void rhi_cmd_bind_vertex_buffer(RHI_CommandBuffer *cmd, u32 binding,
                                       RHI_Buffer *buffer, usize offset);
VF_API void rhi_cmd_bind_index_buffer(RHI_CommandBuffer *cmd,
                                      RHI_Buffer *buffer, usize offset,
                                      b8 is_u32);
VF_API void rhi_cmd_set_viewport(RHI_CommandBuffer *cmd, f32 x, f32 y,
                                 f32 width, f32 height, f32 min_depth,
                                 f32 max_depth);
VF_API void rhi_cmd_set_scissor(RHI_CommandBuffer *cmd, i32 x, i32 y, u32 width,
                                u32 height);
VF_API void rhi_cmd_draw(RHI_CommandBuffer *cmd, u32 vertex_count,
                         u32 instance_count, u32 first_vertex,
                         u32 first_instance);
VF_API void rhi_cmd_draw_indexed(RHI_CommandBuffer *cmd, u32 index_count,
                                 u32 instance_count, u32 first_index,
                                 i32 vertex_offset, u32 first_instance);
VF_API void rhi_cmd_draw_indirect(RHI_CommandBuffer *cmd, RHI_Buffer *buffer,
                                  usize offset, u32 draw_count, u32 stride);

// ============================================================================
// Command Buffer - Compute Commands
// ============================================================================

VF_API void rhi_cmd_dispatch(RHI_CommandBuffer *cmd, u32 x, u32 y, u32 z);
VF_API void rhi_cmd_dispatch_indirect(RHI_CommandBuffer *cmd,
                                      RHI_Buffer *buffer, usize offset);

// ============================================================================
// Command Buffer - Transfer Commands
// ============================================================================

VF_API void rhi_cmd_copy_buffer(RHI_CommandBuffer *cmd, RHI_Buffer *src,
                                RHI_Buffer *dst, usize src_offset,
                                usize dst_offset, usize size);
VF_API void rhi_cmd_copy_buffer_to_texture(RHI_CommandBuffer *cmd,
                                           RHI_Buffer *src, RHI_Texture *dst,
                                           u32 mip, u32 layer);
VF_API void rhi_cmd_copy_texture_to_buffer(RHI_CommandBuffer *cmd,
                                           RHI_Texture *src, RHI_Buffer *dst,
                                           u32 mip, u32 layer);

// ============================================================================
// Synchronization
// ============================================================================

VF_API RHI_Fence *rhi_fence_create(b8 signaled);
VF_API void rhi_fence_destroy(RHI_Fence *fence);
VF_API void rhi_fence_wait(RHI_Fence *fence, u64 timeout_ns);
VF_API void rhi_fence_reset(RHI_Fence *fence);
VF_API b8 rhi_fence_is_signaled(RHI_Fence *fence);

VF_API RHI_Semaphore *rhi_semaphore_create(void);
VF_API void rhi_semaphore_destroy(RHI_Semaphore *semaphore);

// ============================================================================
// Queue Submission
// ============================================================================

typedef struct RHI_SubmitInfo {
  RHI_CommandBuffer **command_buffers;
  u32 command_buffer_count;
  RHI_Semaphore **wait_semaphores;
  u32 wait_semaphore_count;
  RHI_Semaphore **signal_semaphores;
  u32 signal_semaphore_count;
} RHI_SubmitInfo;

VF_API void rhi_queue_submit(RHI_QueueType queue, const RHI_SubmitInfo *info,
                             RHI_Fence *fence);
VF_API void rhi_queue_wait_idle(RHI_QueueType queue);
VF_API void rhi_device_wait_idle(void);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_RHI_API_H
