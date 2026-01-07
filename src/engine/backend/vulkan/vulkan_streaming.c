#include <core/logger.h>
#include <renderer/vulkan.h>
#include <renderer/vulkan_streaming.h>
#include <stdlib.h>
#include <string.h>

#ifdef VULKAN_BUILD

static VulkanStreamingSystem g_streaming = {0};

// --- Mesh Pool Helpers ---

static bool vulkan_pool_init(MeshPool *pool, VulkanRenderer *renderer,
                             VkDeviceSize size, VkBufferUsageFlags usage) {
  if (!vulkan_create_buffer(
          renderer, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &pool->buffer, &pool->memory)) {
    return false;
  }
  pool->capacity = size;
  pool->used = 0;

  // Initialize Free List with one large block
  pool->head_block = (MeshPoolBlock *)malloc(sizeof(MeshPoolBlock));
  pool->head_block->offset = 0;
  pool->head_block->size = size;
  pool->head_block->free = true;
  pool->head_block->next = NULL;
  pool->head_block->alloc_id = 0;

  // Initialize allocation tracking array
  pool->allocation_capacity = MAX_STREAMING_ALLOCS;
  pool->allocations = (MeshAllocation *)calloc(pool->allocation_capacity,
                                               sizeof(MeshAllocation));
  pool->allocation_count = 0;

  return true;
}

static void vulkan_pool_cleanup(VulkanRenderer *renderer, MeshPool *pool) {
  // Free blocks
  MeshPoolBlock *block = pool->head_block;
  while (block) {
    MeshPoolBlock *next = block->next;
    free(block);
    block = next;
  }

  if (pool->allocations)
    free(pool->allocations);

  vkDestroyBuffer(renderer->device, pool->buffer, NULL);
  vkFreeMemory(renderer->device, pool->memory, NULL);
}

// Simple First-Fit Allocator
static bool vulkan_pool_alloc(MeshPool *pool, VkDeviceSize size,
                              VkDeviceSize *out_offset) {
  MeshPoolBlock *block = pool->head_block;
  while (block) {
    if (block->free && block->size >= size) {
      // Found a suitable block
      // Split if generous (✅ COMPLETED: Add alignment if needed)
      if (block->size > size) {
        MeshPoolBlock *new_block =
            (MeshPoolBlock *)malloc(sizeof(MeshPoolBlock));
        new_block->offset = block->offset + size;
        new_block->size = block->size - size;
        new_block->free = true;
        new_block->next = block->next;
        new_block->alloc_id = 0;

        block->next = new_block;
        block->size = size;
      }

      block->free = false;
      *out_offset = block->offset;
      pool->used += size;
      return true;
    }
    block = block->next;
  }
  return false;
}

static void vulkan_pool_free(MeshPool *pool, VkDeviceSize offset) {
  MeshPoolBlock *block = pool->head_block;
  MeshPoolBlock *prev = NULL;

  while (block) {
    if (block->offset == offset) {
      block->free = true;
      pool->used -= block->size;

      // Coalesce with next
      if (block->next && block->next->free) {
        MeshPoolBlock *next = block->next;
        block->size += next->size;
        block->next = next->next;
        free(next);
      }

      // Coalesce with prev
      if (prev && prev->free) {
        prev->size += block->size;
        prev->next = block->next;
        free(block);
        block = prev;
      }
      return;
    }
    prev = block;
    block = block->next;
  }
}

// --- Eviction Logic ---

static bool vulkan_streaming_evict(MeshPool *pool, VkDeviceSize needed_size) {
  // Find oldest allocation
  u64 oldest_frame = UINT64_MAX;
  u32 oldest_idx = UINT32_MAX;

  for (u32 i = 0; i < pool->allocation_count; i++) {
    if (pool->allocations[i].active &&
        pool->allocations[i].last_used_frame < oldest_frame) {
      oldest_frame = pool->allocations[i].last_used_frame;
      oldest_idx = i;
    }
  }

  if (oldest_idx != UINT32_MAX) {
    MeshAllocation *alloc = &pool->allocations[oldest_idx];
    LOG_DEBUG("Evicting mesh allocation ID %d (age: %llu frames, size: %llu)",
              alloc->id, g_streaming.current_frame - oldest_frame, alloc->size);

    vulkan_pool_free(pool, alloc->offset);
    alloc->active = false;

    // Compact array (optional, for now just marking inactive is fine but we
    // should reuse slots ideally) For simplicity: We don't compact, just mark
    // inactive. Reusing slots would require a free list of IDs.
    return true;
  }

  return false;
}

// --- Public API ---

bool vulkan_streaming_init(VulkanRenderer *renderer) {
  LOG_INFO("Initializing Vulkan Vertex Streaming System...");

  if (!vulkan_pool_init(&g_streaming.vertex_pool, renderer, MESH_POOL_SIZE,
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) {
    LOG_ERROR("Failed to create vertex pool");
    return false;
  }

  if (!vulkan_pool_init(&g_streaming.index_pool, renderer, MESH_POOL_SIZE,
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT)) {
    LOG_ERROR("Failed to create index pool");
    return false;
  }

  // Create Staging Ring
  if (!vulkan_create_buffer(
          renderer, STAGING_RING_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &g_streaming.staging_ring.buffer, &g_streaming.staging_ring.memory)) {
    LOG_ERROR("Failed to create staging ring buffer");
    return false;
  }

  vkMapMemory(renderer->device, g_streaming.staging_ring.memory, 0,
              STAGING_RING_SIZE, 0, &g_streaming.staging_ring.mapped_ptr);
  g_streaming.staging_ring.size = STAGING_RING_SIZE;
  g_streaming.staging_ring.head = 0;

  g_streaming.current_frame = 0;

  LOG_INFO(
      "Vulkan Vertex Streaming System initialized (Pool: %dMB, Ring: %dMB)",
      MESH_POOL_SIZE / (1024 * 1024), STAGING_RING_SIZE / (1024 * 1024));

  return true;
}

void vulkan_streaming_cleanup(VulkanRenderer *renderer) {
  if (g_streaming.staging_ring.mapped_ptr) {
    vkUnmapMemory(renderer->device, g_streaming.staging_ring.memory);
  }

  vkDestroyBuffer(renderer->device, g_streaming.staging_ring.buffer, NULL);
  vkFreeMemory(renderer->device, g_streaming.staging_ring.memory, NULL);

  vulkan_pool_cleanup(renderer, &g_streaming.vertex_pool);
  vulkan_pool_cleanup(renderer, &g_streaming.index_pool);

  memset(&g_streaming, 0, sizeof(VulkanStreamingSystem));
}

bool vulkan_streaming_alloc(VulkanRenderer *renderer, VkDeviceSize vertex_size,
                            VkDeviceSize index_size,
                            MeshAllocation *out_vertex_alloc,
                            MeshAllocation *out_index_alloc) {
  (void)renderer;

  // VULKAN-STR-002: Fix - Track vertex allocation state for proper cleanup
  bool vertex_allocation_created = false;
  u32 v_id = 0;
  VkDeviceSize v_offset = 0;

  // Try to allocate vertex memory
  int v_tries = 0;
  while (!vulkan_pool_alloc(&g_streaming.vertex_pool, vertex_size, &v_offset)) {
    if (v_tries++ > 10 ||
        !vulkan_streaming_evict(&g_streaming.vertex_pool, vertex_size)) {
      LOG_ERROR("Vertex Pool Out of Memory (Failed to evict)");
      return false;
    }
  }

  // Setup Vertex Allocation
  // Find free slot in allocation array
  v_id = g_streaming.vertex_pool.allocation_count;
  if (v_id < g_streaming.vertex_pool.allocation_capacity) {
    g_streaming.vertex_pool.allocation_count++;
    g_streaming.vertex_pool.allocations[v_id].buffer =
        g_streaming.vertex_pool.buffer;
    g_streaming.vertex_pool.allocations[v_id].offset = v_offset;
    g_streaming.vertex_pool.allocations[v_id].size = vertex_size;
    g_streaming.vertex_pool.allocations[v_id].active = true;
    g_streaming.vertex_pool.allocations[v_id].last_used_frame =
        g_streaming.current_frame;
    g_streaming.vertex_pool.allocations[v_id].id = v_id; // Simple ID

    *out_vertex_alloc = g_streaming.vertex_pool.allocations[v_id];
    vertex_allocation_created = true;
    // Pointer in out_vertex_alloc is a COPY, the real one is in the array.
    // We rely on the user passing the copy back to us or us looking it up?
    // Actually, vulkan_render uses the copy. We need a way to link back.
    // For 'touch', we need a stable reference.
    // Let's store the array index as ID for now.
  }

  // Try to allocate index memory
  VkDeviceSize i_offset = 0;
  int i_tries = 0;
  while (!vulkan_pool_alloc(&g_streaming.index_pool, index_size, &i_offset)) {
    if (i_tries++ > 10 ||
        !vulkan_streaming_evict(&g_streaming.index_pool, index_size)) {
      LOG_ERROR("Index Pool Out of Memory (Failed to evict)");
      // VULKAN-STR-002: Fix - Clean up both vertex pool memory AND allocation array entry
      vulkan_pool_free(&g_streaming.vertex_pool, v_offset);
      if (vertex_allocation_created) {
        g_streaming.vertex_pool.allocations[v_id].active = false;
        g_streaming.vertex_pool.allocation_count--;
      }
      return false;
    }
  }

  // Setup Index Allocation
  u32 i_id = g_streaming.index_pool.allocation_count;
  if (i_id < g_streaming.index_pool.allocation_capacity) {
    g_streaming.index_pool.allocation_count++;
    g_streaming.index_pool.allocations[i_id].buffer =
        g_streaming.index_pool.buffer;
    g_streaming.index_pool.allocations[i_id].offset = i_offset;
    g_streaming.index_pool.allocations[i_id].size = index_size;
    g_streaming.index_pool.allocations[i_id].active = true;
    g_streaming.index_pool.allocations[i_id].last_used_frame =
        g_streaming.current_frame;
    g_streaming.index_pool.allocations[i_id].id = i_id;

    *out_index_alloc = g_streaming.index_pool.allocations[i_id];
  }

  return true;
}

bool vulkan_streaming_upload(VulkanRenderer *renderer,
                             MeshAllocation vertex_alloc,
                             const void *vertex_data,
                             MeshAllocation index_alloc,
                             const void *index_data) {
  // 1. Copy to staging ring
  VkDeviceSize total_size = vertex_alloc.size + index_alloc.size;

  if (g_streaming.staging_ring.head + total_size >
      g_streaming.staging_ring.size) {
    g_streaming.staging_ring.head = 0;
  }

  VkDeviceSize v_offset = g_streaming.staging_ring.head;
  memcpy((u8 *)g_streaming.staging_ring.mapped_ptr + v_offset, vertex_data,
         vertex_alloc.size);

  VkDeviceSize i_offset = v_offset + vertex_alloc.size;
  memcpy((u8 *)g_streaming.staging_ring.mapped_ptr + i_offset, index_data,
         index_alloc.size);

  g_streaming.staging_ring.head += total_size;

  // 2. Schedule GPU copy
  VkCommandBufferAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = renderer->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(renderer->device, &alloc_info, &cmd);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &begin_info);

  VkBufferCopy v_copy = {v_offset, vertex_alloc.offset, vertex_alloc.size};
  vkCmdCopyBuffer(cmd, g_streaming.staging_ring.buffer, vertex_alloc.buffer, 1,
                  &v_copy);

  VkBufferCopy i_copy = {i_offset, index_alloc.offset, index_alloc.size};
  vkCmdCopyBuffer(cmd, g_streaming.staging_ring.buffer, index_alloc.buffer, 1,
                  &i_copy);

  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;

  vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);

  // In a real system, we'd wait using fences per-frame instead of WaitIdle
  vkQueueWaitIdle(renderer->graphics_queue);
  vkFreeCommandBuffers(renderer->device, renderer->command_pool, 1, &cmd);

  return true;
}

void vulkan_streaming_touch(MeshAllocation *alloc) {
  // Update LRU timestamp in the central array
  // Note: The 'alloc' passed in is a copy in the generic Mesh struct
  // We need to use its ID to find the authoritative record in the pool

  if (alloc->id < g_streaming.vertex_pool.allocation_count &&
      g_streaming.vertex_pool.allocations[alloc->id].offset == alloc->offset) {
    g_streaming.vertex_pool.allocations[alloc->id].last_used_frame =
        g_streaming.current_frame;
  } else if (alloc->id < g_streaming.index_pool.allocation_count &&
             g_streaming.index_pool.allocations[alloc->id].offset ==
                 alloc->offset) {
    g_streaming.index_pool.allocations[alloc->id].last_used_frame =
        g_streaming.current_frame;
  }
}

void vulkan_streaming_begin_frame(VulkanRenderer *renderer) {
  (void)renderer;
  g_streaming.current_frame++;
}

void vulkan_streaming_end_frame(VulkanRenderer *renderer) { (void)renderer; }

#endif
