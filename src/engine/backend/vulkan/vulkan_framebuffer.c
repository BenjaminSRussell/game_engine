// Swapchain framebuffer management and image acquisition.
// Roadmap: docs/VULKAN_FRAMEBUFFER_ROADMAP.md.
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
#include <common.h>
#include <core/logger.h>
#include <limits.h>
#include <rendering/vulkan.h>
#include <rendering/vulkan_framebuffer.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Helper function to get current time in milliseconds
static u64 get_current_time_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (u64)(tv.tv_sec) * 1000 + (u64)(tv.tv_usec) / 1000;
}

#ifdef VULKAN_BUILD
#include <include/rendering/vulkan.h>
#endif

// VULKAN-FB-001: Framebuffer pool entry
typedef struct FramebufferPoolEntry {
  VkFramebuffer framebuffer;
  VkRenderPass render_pass;
  u32 width, height;
  u32 attachment_count;
  bool in_use;
  u64 last_used_time;
} FramebufferPoolEntry;

// VULKAN-FB-003: Framebuffer statistics
static struct {
  u32 total_created;
  u32 total_destroyed;
  u32 pool_hits;
  u32 pool_misses;
  u64 total_creation_time_ms;
  u64 memory_usage_bytes;
  bool leak_detection_enabled;
} g_fb_stats = {0};

// VULKAN-FB-001: Framebuffer pool
#define MAX_FRAMEBUFFER_POOL_SIZE 64
static FramebufferPoolEntry g_framebuffer_pool[MAX_FRAMEBUFFER_POOL_SIZE];
static u32 g_pool_size = 0;
static bool g_pool_initialized = false;

// VULKAN-FB-001: Initialize framebuffer pool
static void init_framebuffer_pool(void) {
  if (g_pool_initialized)
    return;

  memset(g_framebuffer_pool, 0, sizeof(g_framebuffer_pool));
  g_pool_size = 0;
  g_pool_initialized = true;
  g_fb_stats.leak_detection_enabled = true;

  LOG_INFO("Framebuffer pool initialized");
}

// VULKAN-FB-001: Find framebuffer in pool
static VkFramebuffer find_framebuffer_in_pool(VkRenderPass render_pass,
                                              u32 width, u32 height,
                                              u32 attachment_count) {
  if (!g_pool_initialized)
    return VK_NULL_HANDLE;

  u64 current_time = get_current_time_ms();

  for (u32 i = 0; i < g_pool_size; i++) {
    FramebufferPoolEntry *entry = &g_framebuffer_pool[i];
    if (!entry->in_use && entry->render_pass == render_pass &&
        entry->width == width && entry->height == height &&
        entry->attachment_count == attachment_count) {

      entry->in_use = true;
      entry->last_used_time = current_time;
      g_fb_stats.pool_hits++;
      return entry->framebuffer;
    }
  }

  g_fb_stats.pool_misses++;
  return VK_NULL_HANDLE;
}

// VULKAN-FB-001: Add framebuffer to pool
static bool add_framebuffer_to_pool(VkFramebuffer framebuffer,
                                    VkRenderPass render_pass, u32 width,
                                    u32 height, u32 attachment_count) {
  if (!g_pool_initialized || g_pool_size >= MAX_FRAMEBUFFER_POOL_SIZE) {
    return false;
  }

  u64 current_time = get_current_time_ms();
  FramebufferPoolEntry *entry = &g_framebuffer_pool[g_pool_size];

  entry->framebuffer = framebuffer;
  entry->render_pass = render_pass;
  entry->width = width;
  entry->height = height;
  entry->attachment_count = attachment_count;
  entry->in_use = true;
  entry->last_used_time = current_time;

  g_pool_size++;
  g_fb_stats.total_created++;
  g_fb_stats.memory_usage_bytes +=
      width * height * 4 * attachment_count; // Estimate

  return true;
}

// VULKAN-FB-002: Framebuffer validation
static bool validate_framebuffer_params(VkRenderPass render_pass, u32 width,
                                        u32 height, u32 attachment_count) {
  if (render_pass == VK_NULL_HANDLE) {
    LOG_ERROR("Invalid render pass for framebuffer");
    return false;
  }

  if (width == 0 || height == 0) {
    LOG_ERROR("Invalid framebuffer dimensions: %ux%u", width, height);
    return false;
  }

  if (width > 8192 || height > 8192) {
    LOG_ERROR("Framebuffer dimensions too large: %ux%u (max: 8192x8192)", width,
              height);
    return false;
  }

  if (attachment_count == 0 || attachment_count > 8) {
    LOG_ERROR("Invalid attachment count: %u (min: 1, max: 8)",
              attachment_count);
    return false;
  }

  return true;
}

// VULKAN-FB-005: Performance profiling helper
static u64 start_framebuffer_timer(void) { return get_current_time_ms(); }

static void end_framebuffer_timer(u64 start_time) {
  u64 elapsed = get_current_time_ms() - start_time;
  g_fb_stats.total_creation_time_ms += elapsed;
}

// Create framebuffers
bool vulkan_create_framebuffers(VulkanRenderer *renderer) {
#ifdef VULKAN_BUILD
  u64 start_time = start_framebuffer_timer();

  // VULKAN-FB-001: Initialize pool
  init_framebuffer_pool();

  // VULKAN-FB-002: Validate renderer state
  if (!renderer || renderer->device == VK_NULL_HANDLE) {
    LOG_ERROR("Invalid renderer for framebuffer creation");
    return false;
  }

  // Allocate command buffers
  renderer->command_buffers = (VkCommandBuffer *)malloc(
      sizeof(VkCommandBuffer) * renderer->swapchain_image_count);
  renderer->command_buffer_count = renderer->swapchain_image_count;

  VkCommandBufferAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = renderer->command_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = renderer->command_buffer_count;

  if (vkAllocateCommandBuffers(renderer->device, &alloc_info,
                               renderer->command_buffers) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate command buffers");
    return false;
  }

  // Create framebuffers for each swapchain image
  renderer->framebuffers = (VkFramebuffer *)malloc(
      sizeof(VkFramebuffer) * renderer->swapchain_image_count);
  renderer->framebuffer_count = renderer->swapchain_image_count;

  for (u32 i = 0; i < renderer->swapchain_image_count; i++) {
    // VULKAN-FB-001: Try to find in pool first
    VkImageView attachments[] = {renderer->swapchain_image_views[i]};
    u32 attachment_count = sizeof(attachments) / sizeof(attachments[0]);

    VkFramebuffer pooled_fb = find_framebuffer_in_pool(
        renderer->render_pass, renderer->swapchain_extent.width,
        renderer->swapchain_extent.height, attachment_count);

    if (pooled_fb != VK_NULL_HANDLE) {
      renderer->framebuffers[i] = pooled_fb;
      LOG_INFO("Using pooled framebuffer %u", i);
      continue;
    }

    // VULKAN-FB-002: Validate parameters
    if (!validate_framebuffer_params(
            renderer->render_pass, renderer->swapchain_extent.width,
            renderer->swapchain_extent.height, attachment_count)) {
      return false;
    }

    VkFramebufferCreateInfo framebuffer_info = {0};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = renderer->render_pass;
    framebuffer_info.attachmentCount = attachment_count;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = renderer->swapchain_extent.width;
    framebuffer_info.height = renderer->swapchain_extent.height;
    framebuffer_info.layers = 1;

    if (vkCreateFramebuffer(renderer->device, &framebuffer_info, NULL,
                            &renderer->framebuffers[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create framebuffer %u", i);
      return false;
    }

    // VULKAN-FB-001: Add to pool
    if (!add_framebuffer_to_pool(
            renderer->framebuffers[i], renderer->render_pass,
            renderer->swapchain_extent.width, renderer->swapchain_extent.height,
            attachment_count)) {
      LOG_WARN("Failed to add framebuffer %u to pool (pool may be full)", i);
    }
  }

  // Create semaphores and fences
  renderer->image_available_semaphores = (VkSemaphore *)malloc(
      sizeof(VkSemaphore) * renderer->max_frames_in_flight);
  renderer->render_finished_semaphores = (VkSemaphore *)malloc(
      sizeof(VkSemaphore) * renderer->max_frames_in_flight);
  renderer->in_flight_fences =
      (VkFence *)malloc(sizeof(VkFence) * renderer->max_frames_in_flight);

  VkSemaphoreCreateInfo semaphore_info = {0};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {0};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (u32 i = 0; i < renderer->max_frames_in_flight; i++) {
    if (vkCreateSemaphore(renderer->device, &semaphore_info, NULL,
                          &renderer->image_available_semaphores[i]) !=
            VK_SUCCESS ||
        vkCreateSemaphore(renderer->device, &semaphore_info, NULL,
                          &renderer->render_finished_semaphores[i]) !=
            VK_SUCCESS ||
        vkCreateFence(renderer->device, &fence_info, NULL,
                      &renderer->in_flight_fences[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create synchronization objects");
      return false;
    }
  }

  // VULKAN-FB-005: End timing and log statistics
  end_framebuffer_timer(start_time);

  LOG_INFO("Framebuffers and sync objects created: %u framebuffers",
           renderer->framebuffer_count);
  LOG_INFO("Framebuffer pool stats: %u hits, %u misses, %.1f%% hit rate",
           g_fb_stats.pool_hits, g_fb_stats.pool_misses,
           (g_fb_stats.pool_hits + g_fb_stats.pool_misses) > 0
               ? (double)g_fb_stats.pool_hits /
                     (g_fb_stats.pool_hits + g_fb_stats.pool_misses) * 100.0
               : 0.0);

  return true;
#else
  (void)renderer;
  return true;
#endif
}

// VULKAN-FB-004: Debug framebuffers
void vulkan_debug_framebuffers(void) {
  LOG_INFO("=== Framebuffer Debug Info ===");
  LOG_INFO("Pool Initialized: %s", g_pool_initialized ? "Yes" : "No");
  LOG_INFO("Pool Size: %u / %d", g_pool_size, MAX_FRAMEBUFFER_POOL_SIZE);
  LOG_INFO("Statistics:");
  LOG_INFO("  Total Created: %u", g_fb_stats.total_created);
  LOG_INFO("  Total Destroyed: %u", g_fb_stats.total_destroyed);
  LOG_INFO("  Pool Hits: %u", g_fb_stats.pool_hits);
  LOG_INFO("  Pool Misses: %u", g_fb_stats.pool_misses);
  LOG_INFO("  Memory Usage: %.1f MB",
           g_fb_stats.memory_usage_bytes / (1024.0 * 1024.0));
  LOG_INFO("  Total Creation Time: %llu ms", g_fb_stats.total_creation_time_ms);
  LOG_INFO("  Leak Detection: %s",
           g_fb_stats.leak_detection_enabled ? "Enabled" : "Disabled");

  if (g_pool_initialized) {
    LOG_INFO("Pool Entries:");
    for (u32 i = 0; i < g_pool_size; i++) {
      FramebufferPoolEntry *entry = &g_framebuffer_pool[i];
      LOG_INFO("  [%u] FB: %p, %ux%u, %u attachments, In Use: %s, Last Used: "
               "%llu ms ago",
               i, (void *)entry->framebuffer, entry->width, entry->height,
               entry->attachment_count, entry->in_use ? "Yes" : "No",
               get_current_time_ms() - entry->last_used_time);
    }
  }
}

// VULKAN-FB-003: Get framebuffer statistics
void vulkan_get_framebuffer_stats(VulkanFramebufferStats *out_stats) {
  if (!out_stats)
    return;

  out_stats->total_created = g_fb_stats.total_created;
  out_stats->total_destroyed = g_fb_stats.total_destroyed;
  out_stats->pool_hits = g_fb_stats.pool_hits;
  out_stats->pool_misses = g_fb_stats.pool_misses;
  out_stats->memory_usage_bytes = g_fb_stats.memory_usage_bytes;
  out_stats->total_creation_time_ms = g_fb_stats.total_creation_time_ms;
  out_stats->pool_hit_rate =
      (g_fb_stats.pool_hits + g_fb_stats.pool_misses) > 0
          ? (double)g_fb_stats.pool_hits /
                (g_fb_stats.pool_hits + g_fb_stats.pool_misses) * 100.0
          : 0.0;
  out_stats->average_creation_time_ms =
      g_fb_stats.total_created > 0
          ? (double)g_fb_stats.total_creation_time_ms / g_fb_stats.total_created
          : 0.0;
}

// VULKAN-FB-010: Memory leak detection
void vulkan_detect_framebuffer_leaks(void) {
  if (!g_fb_stats.leak_detection_enabled)
    return;

  u64 current_time = get_current_time_ms();
  u64 leak_threshold_ms = 60000; // 1 minute

  LOG_INFO("Checking for framebuffer leaks...");

  for (u32 i = 0; i < g_pool_size; i++) {
    FramebufferPoolEntry *entry = &g_framebuffer_pool[i];
    if (entry->in_use &&
        (current_time - entry->last_used_time) > leak_threshold_ms) {
      LOG_WARN("Potential framebuffer leak detected: FB %p (%ux%u) unused for "
               "%llu ms",
               (void *)entry->framebuffer, entry->width, entry->height,
               current_time - entry->last_used_time);
    }
  }

  // Check for creation/destruction imbalance
  if (g_fb_stats.total_created > g_fb_stats.total_destroyed + g_pool_size) {
    LOG_WARN("Framebuffer leak detected: %u created, %u destroyed, %u in pool",
             g_fb_stats.total_created, g_fb_stats.total_destroyed, g_pool_size);
  }
}

// VULKAN-FB-006: Configure framebuffer system
void vulkan_configure_framebuffers(const VulkanFramebufferConfig *config) {
  if (!config)
    return;

  if (config->clear_pool) {
    memset(g_framebuffer_pool, 0, sizeof(g_framebuffer_pool));
    g_pool_size = 0;
    LOG_INFO("Framebuffer pool cleared by configuration");
  }

  if (config->enable_leak_detection != g_fb_stats.leak_detection_enabled) {
    g_fb_stats.leak_detection_enabled = config->enable_leak_detection;
    LOG_INFO("Framebuffer leak detection %s",
             config->enable_leak_detection ? "enabled" : "disabled");
  }
}

// VULKAN-FB-009: Get optimization suggestions
void vulkan_get_framebuffer_optimizations(
    VulkanFramebufferOptimizations *suggestions) {
  if (!suggestions)
    return;

  memset(suggestions, 0, sizeof(VulkanFramebufferOptimizations));

  // Pool efficiency suggestions
  double hit_rate = (g_fb_stats.pool_hits + g_fb_stats.pool_misses) > 0
                        ? (double)g_fb_stats.pool_hits /
                              (g_fb_stats.pool_hits + g_fb_stats.pool_misses) *
                              100.0
                        : 0.0;

  if (hit_rate < 50.0) {
    suggestions->increase_pool_size = true;
    suggestions->optimize_framebuffer_reuse = true;
  }

  // Performance suggestions
  if (g_fb_stats.total_created > 0) {
    double avg_time =
        (double)g_fb_stats.total_creation_time_ms / g_fb_stats.total_created;
    if (avg_time > 10.0) {
      suggestions->enable_aggressive_caching = true;
      suggestions->reduce_framebuffer_complexity = true;
    }
  }

  // Memory suggestions
  if (g_fb_stats.memory_usage_bytes > 100ULL * 1024 * 1024) { // > 100MB
    suggestions->reduce_memory_usage = true;
    suggestions->implement_framebuffer_streaming = true;
  }
}

// Update begin_frame to use semaphores
bool vulkan_begin_frame_updated(VulkanRenderer *renderer, u32 *image_index) {
#ifdef VULKAN_BUILD
  vkWaitForFences(renderer->device, 1,
                  &renderer->in_flight_fences[renderer->current_frame], VK_TRUE,
                  UINT64_MAX);

  VkResult result = vkAcquireNextImageKHR(
      renderer->device, renderer->swapchain, UINT64_MAX,
      renderer->image_available_semaphores[renderer->current_frame],
      VK_NULL_HANDLE, image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    renderer->framebuffer_resized = true;
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    LOG_ERROR("Failed to acquire swapchain image");
    return false;
  }

  vkResetFences(renderer->device, 1,
                &renderer->in_flight_fences[renderer->current_frame]);

  vkResetCommandBuffer(renderer->command_buffers[*image_index], 0);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(renderer->command_buffers[*image_index],
                           &begin_info) != VK_SUCCESS) {
    LOG_ERROR("Failed to begin recording command buffer");
    return false;
  }

  VkRenderPassBeginInfo render_pass_info = {0};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = renderer->render_pass;
  render_pass_info.framebuffer = renderer->framebuffers[*image_index];
  render_pass_info.renderArea.offset.x = 0;
  render_pass_info.renderArea.offset.y = 0;
  render_pass_info.renderArea.extent = renderer->swapchain_extent;

  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(renderer->command_buffers[*image_index],
                       &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(renderer->command_buffers[*image_index],
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderer->graphics_pipeline);

  return true;
#else
  (void)renderer;
  *image_index = 0;
  return true;
#endif
}

// Update end_frame to present
void vulkan_end_frame_updated(VulkanRenderer *renderer, u32 image_index) {
#ifdef VULKAN_BUILD
  vkCmdEndRenderPass(renderer->command_buffers[image_index]);

  if (vkEndCommandBuffer(renderer->command_buffers[image_index]) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to record command buffer");
    return;
  }

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {
      renderer->image_available_semaphores[renderer->current_frame]};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &renderer->command_buffers[image_index];

  VkSemaphore signal_semaphores[] = {
      renderer->render_finished_semaphores[renderer->current_frame]};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  if (vkQueueSubmit(renderer->graphics_queue, 1, &submit_info,
                    renderer->in_flight_fences[renderer->current_frame]) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to submit draw command buffer");
    return;
  }

  VkPresentInfoKHR present_info = {0};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;

  VkSwapchainKHR swapchains[] = {renderer->swapchain};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains;
  present_info.pImageIndices = &image_index;

  VkResult result = vkQueuePresentKHR(renderer->present_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      renderer->framebuffer_resized) {
    renderer->framebuffer_resized = false;
  } else if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to present swapchain image");
  }

  renderer->current_frame =
      (renderer->current_frame + 1) % renderer->max_frames_in_flight;
#else
  (void)renderer;
  (void)image_index;
#endif
}
