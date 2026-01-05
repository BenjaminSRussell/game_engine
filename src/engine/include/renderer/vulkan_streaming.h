#ifndef VULKAN_STREAMING_H
#define VULKAN_STREAMING_H

#include <renderer/vulkan.h>

#ifdef VULKAN_BUILD

// Configuration for streaming system
#define MESH_POOL_SIZE (128 * 1024 * 1024)   // 128MB per pool
#define STAGING_RING_SIZE (32 * 1024 * 1024) // 32MB ring buffer
#define MAX_STREAMING_ALLOCS 4096

// Allocation in a Mesh Pool
typedef struct {
  VkBuffer buffer;
  VkDeviceSize offset;
  VkDeviceSize size;
  u64 last_used_frame; // For LRU eviction
  u32 id;              // Unique allocation ID
  bool active;
} MeshAllocation;

// Free List Block
typedef struct MeshPoolBlock {
  VkDeviceSize offset;
  VkDeviceSize size;
  struct MeshPoolBlock *next;
  bool free;
  u32 alloc_id; // ID of the allocation using this block (if not free)
} MeshPoolBlock;

// Mesh Pool structure (Device Local)
typedef struct {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize capacity;
  VkDeviceSize used;

  // Free List Allocator
  MeshPoolBlock *head_block;

  // Usage tracking
  MeshAllocation *allocations; // Dynamic array of active allocations references
  u32 allocation_count;
  u32 allocation_capacity;
} MeshPool;

// Staging Ring Buffer (Host Visible)
typedef struct {
  VkBuffer buffer;
  VkDeviceMemory memory;
  void *mapped_ptr;
  VkDeviceSize size;
  VkDeviceSize head; // Current write position
  VkDeviceSize tail; // Last confirmed processed position
} StagingRing;

// Streaming System context
typedef struct {
  MeshPool vertex_pool;
  MeshPool index_pool;
  StagingRing staging_ring;
  u64 current_frame;
} VulkanStreamingSystem;

// Public API
bool vulkan_streaming_init(VulkanRenderer *renderer);
void vulkan_streaming_cleanup(VulkanRenderer *renderer);

// Allocate space in the mesh pool
bool vulkan_streaming_alloc(VulkanRenderer *renderer, VkDeviceSize vertex_size,
                            VkDeviceSize index_size,
                            MeshAllocation *out_vertex_alloc,
                            MeshAllocation *out_index_alloc);

// Request an async upload to the mesh pool
bool vulkan_streaming_upload(VulkanRenderer *renderer,
                             MeshAllocation vertex_alloc,
                             const void *vertex_data,
                             MeshAllocation index_alloc,
                             const void *index_data);

// Mark an allocation as used in the current frame (for LRU)
void vulkan_streaming_touch(MeshAllocation *alloc);

// Frame management
void vulkan_streaming_begin_frame(VulkanRenderer *renderer);
void vulkan_streaming_end_frame(VulkanRenderer *renderer);

#endif // VULKAN_BUILD

#endif // VULKAN_STREAMING_H
