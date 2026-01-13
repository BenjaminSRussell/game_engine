// Vulkan buffer helpers for mesh data uploads.
// Roadmap: docs/VULKAN_BUFFERS_ROADMAP.md.
#include "engine/include/common.h"
#include "engine/include/core/logger.h"
#include <rendering/mesh.h>
#include <rendering/vulkan.h>
#include <rendering/vulkan_streaming.h>
#include <stdlib.h>
#include <string.h>

#ifdef VULKAN_BUILD
#include <include/rendering/vulkan.h>

// Helper function to find suitable memory type
static u32 find_memory_type(VulkanRenderer *renderer, u32 type_filter,
                            VkMemoryPropertyFlags properties) {
  if (!renderer)
    return 0;

  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(renderer->physical_device,
                                      &mem_properties);

  for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }

  LOG_ERROR("Failed to find suitable memory type");
  return 0;
}

// VULKAN-BF-001: Buffer Pooling System Implementation
// ================================================

// Buffer pool entry structure
typedef struct BufferPoolEntry {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  VkBufferUsageFlags usage;
  VkMemoryPropertyFlags properties;
  bool in_use;
  struct BufferPoolEntry *next;
} BufferPoolEntry;

// Buffer pool structure
typedef struct BufferPool {
  BufferPoolEntry *free_list;
  BufferPoolEntry *used_list;
  u32 total_buffers;
  u32 free_buffers;
  u32 used_buffers;
  VkDeviceSize total_memory;
  VkDeviceSize used_memory;
  VkDeviceSize peak_memory_usage;
} BufferPool;

// Global buffer pools for different usage types
static BufferPool g_vertex_pool = {0};
static BufferPool g_index_pool = {0};
static BufferPool g_uniform_pool = {0};
static BufferPool g_staging_pool = {0};

// Buffer pool statistics
typedef struct BufferPoolStats {
  u32 total_allocations;
  u32 total_deallocations;
  u32 pool_hits;
  u32 pool_misses;
  u32 fragmentation_events;
  VkDeviceSize total_allocated;
  VkDeviceSize total_freed;
  f32 average_allocation_size;
  f32 pool_hit_ratio;
} BufferPoolStats;

static BufferPoolStats g_pool_stats = {0};

// Initialize buffer pool
static void buffer_pool_init(BufferPool *pool) {
  if (!pool)
    return;

  pool->free_list = NULL;
  pool->used_list = NULL;
  pool->total_buffers = 0;
  pool->free_buffers = 0;
  pool->used_buffers = 0;
  pool->total_memory = 0;
  pool->used_memory = 0;
  pool->peak_memory_usage = 0;
}

// Find suitable buffer in pool
static BufferPoolEntry *
buffer_pool_find_suitable(BufferPool *pool, VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties) {
  BufferPoolEntry *entry = pool->free_list;
  BufferPoolEntry *best_fit = NULL;
  VkDeviceSize best_size_diff = UINT64_MAX;

  while (entry) {
    // Check if buffer meets requirements
    if (entry->size >= size && (entry->usage & usage) == usage &&
        (entry->properties & properties) == properties) {

      VkDeviceSize size_diff = entry->size - size;
      if (size_diff < best_size_diff) {
        best_fit = entry;
        best_size_diff = size_diff;

        // Perfect match found
        if (size_diff == 0) {
          break;
        }
      }
    }
    entry = entry->next;
  }

  return best_fit;
}

// Remove entry from list
static void buffer_pool_remove_from_list(BufferPoolEntry **list,
                                         BufferPoolEntry *entry) {
  if (!list || !entry)
    return;

  if (*list == entry) {
    *list = entry->next;
  } else {
    BufferPoolEntry *current = *list;
    while (current && current->next != entry) {
      current = current->next;
    }
    if (current) {
      current->next = entry->next;
    }
  }
  entry->next = NULL;
}

// Add entry to front of list
static void buffer_pool_add_to_list(BufferPoolEntry **list,
                                    BufferPoolEntry *entry) {
  if (!list || !entry)
    return;

  entry->next = *list;
  *list = entry;
}

// Get appropriate pool for buffer usage
static BufferPool *buffer_pool_get_for_usage(VkBufferUsageFlags usage) {
  if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
    return &g_vertex_pool;
  } else if (usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
    return &g_index_pool;
  } else if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
    return &g_uniform_pool;
  } else if (usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
    return &g_staging_pool;
  }
  return NULL;
}

// Allocate buffer from pool
static bool buffer_pool_allocate(VkDevice device, VkDeviceSize size,
                                 VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer *buffer, VkDeviceMemory *memory) {
  BufferPool *pool = buffer_pool_get_for_usage(usage);

  if (pool) {
    // Try to find suitable buffer in pool
    BufferPoolEntry *entry =
        buffer_pool_find_suitable(pool, size, usage, properties);

    if (entry) {
      // Pool hit - reuse existing buffer
      buffer_pool_remove_from_list(&pool->free_list, entry);
      buffer_pool_add_to_list(&pool->used_list, entry);

      entry->in_use = true;
      pool->free_buffers--;
      pool->used_buffers++;

      *buffer = entry->buffer;
      *memory = entry->memory;

      g_pool_stats.pool_hits++;
      return true;
    }
  }

  // Pool miss - create new buffer
  g_pool_stats.pool_misses++;

  VkBufferCreateInfo buffer_info = {0};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(device, *buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      find_memory_type(NULL, mem_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &alloc_info, NULL, memory) != VK_SUCCESS) {
    vkDestroyBuffer(device, *buffer, NULL);
    return false;
  }

  vkBindBufferMemory(device, *buffer, *memory, 0);

  // Add to pool statistics
  if (pool) {
    BufferPoolEntry *new_entry = malloc(sizeof(BufferPoolEntry));
    new_entry->buffer = *buffer;
    new_entry->memory = *memory;
    new_entry->size = mem_requirements.size;
    new_entry->usage = usage;
    new_entry->properties = properties;
    new_entry->in_use = true;
    new_entry->next = NULL;

    buffer_pool_add_to_list(&pool->used_list, new_entry);

    pool->total_buffers++;
    pool->used_buffers++;
    pool->total_memory += mem_requirements.size;
    pool->used_memory += mem_requirements.size;

    if (pool->used_memory > pool->peak_memory_usage) {
      pool->peak_memory_usage = pool->used_memory;
    }
  }

  // Update global statistics
  g_pool_stats.total_allocations++;
  g_pool_stats.total_allocated += mem_requirements.size;
  g_pool_stats.average_allocation_size =
      (f32)g_pool_stats.total_allocated / g_pool_stats.total_allocations;

  return true;
}

// Return buffer to pool
static void buffer_pool_deallocate(VkDevice device, VkBuffer buffer,
                                   VkDeviceMemory memory) {
  // Find the buffer in used lists
  BufferPool *pools[] = {&g_vertex_pool, &g_index_pool, &g_uniform_pool,
                         &g_staging_pool};

  for (int i = 0; i < 4; i++) {
    BufferPool *pool = pools[i];
    BufferPoolEntry *entry = pool->used_list;

    while (entry) {
      if (entry->buffer == buffer) {
        // Return to pool
        buffer_pool_remove_from_list(&pool->used_list, entry);
        buffer_pool_add_to_list(&pool->free_list, entry);

        entry->in_use = false;
        pool->free_buffers++;
        pool->used_buffers--;
        pool->used_memory -= entry->size;

        g_pool_stats.total_deallocations++;
        g_pool_stats.total_freed += entry->size;
        return;
      }
      entry = entry->next;
    }
  }

  // Buffer not found in pools - destroy it
  vkDestroyBuffer(device, buffer, NULL);
  vkFreeMemory(device, memory, NULL);
}

// Cleanup buffer pool
static void buffer_pool_cleanup(VkDevice device, BufferPool *pool) {
  if (!pool)
    return;

  // Destroy all buffers in free list
  BufferPoolEntry *entry = pool->free_list;
  while (entry) {
    BufferPoolEntry *next = entry->next;
    vkDestroyBuffer(device, entry->buffer, NULL);
    vkFreeMemory(device, entry->memory, NULL);
    free(entry);
    entry = next;
  }

  // Destroy all buffers in used list
  entry = pool->used_list;
  while (entry) {
    BufferPoolEntry *next = entry->next;
    vkDestroyBuffer(device, entry->buffer, NULL);
    vkFreeMemory(device, entry->memory, NULL);
    free(entry);
    entry = next;
  }

  buffer_pool_init(pool);
}

// Initialize all buffer pools
void vulkan_buffer_pool_init(VkDevice device) {
  (void)device;

  buffer_pool_init(&g_vertex_pool);
  buffer_pool_init(&g_index_pool);
  buffer_pool_init(&g_uniform_pool);
  buffer_pool_init(&g_staging_pool);

  memset(&g_pool_stats, 0, sizeof(g_pool_stats));
}

// Cleanup all buffer pools
void vulkan_buffer_pool_cleanup(VkDevice device) {
  buffer_pool_cleanup(device, &g_vertex_pool);
  buffer_pool_cleanup(device, &g_index_pool);
  buffer_pool_cleanup(device, &g_uniform_pool);
  buffer_pool_cleanup(device, &g_staging_pool);
}

// Get buffer pool statistics
void vulkan_buffer_pool_get_stats(BufferPoolStats *stats) {
  if (!stats)
    return;

  *stats = g_pool_stats;

  // Calculate hit ratio
  u32 total_requests = g_pool_stats.pool_hits + g_pool_stats.pool_misses;
  if (total_requests > 0) {
    stats->pool_hit_ratio = (f32)g_pool_stats.pool_hits / total_requests;
  }

  // Aggregate pool statistics
  stats->fragmentation_events = 0; // Could be calculated based on unused memory
}

// Print buffer pool statistics
void vulkan_buffer_pool_print_stats() {
  printf("=== Vulkan Buffer Pool Statistics ===\n");
  printf("Total Allocations: %u\n", g_pool_stats.total_allocations);
  printf("Total Deallocations: %u\n", g_pool_stats.total_deallocations);
  printf("Pool Hits: %u\n", g_pool_stats.pool_hits);
  printf("Pool Misses: %u\n", g_pool_stats.pool_misses);

  u32 total_requests = g_pool_stats.pool_hits + g_pool_stats.pool_misses;
  if (total_requests > 0) {
    f32 hit_ratio = (f32)g_pool_stats.pool_hits / total_requests * 100.0f;
    printf("Pool Hit Ratio: %.2f%%\n", hit_ratio);
  }

  printf("Total Allocated: %.2f MB\n",
         g_pool_stats.total_allocated / (1024.0f * 1024.0f));
  printf("Total Freed: %.2f MB\n",
         g_pool_stats.total_freed / (1024.0f * 1024.0f));
  printf("Average Allocation Size: %.2f KB\n",
         g_pool_stats.average_allocation_size / 1024.0f);

  printf("\nPool Details:\n");
  printf("Vertex Pool: %u total, %u free, %u used\n",
         g_vertex_pool.total_buffers, g_vertex_pool.free_buffers,
         g_vertex_pool.used_buffers);
  printf("Index Pool: %u total, %u free, %u used\n", g_index_pool.total_buffers,
         g_index_pool.free_buffers, g_index_pool.used_buffers);
  printf("Uniform Pool: %u total, %u free, %u used\n",
         g_uniform_pool.total_buffers, g_uniform_pool.free_buffers,
         g_uniform_pool.used_buffers);
  printf("Staging Pool: %u total, %u free, %u used\n",
         g_staging_pool.total_buffers, g_staging_pool.free_buffers,
         g_staging_pool.used_buffers);

  printf("=====================================\n");
}

// VULKAN-BF-002: Buffer Validation System for Corruption Detection
// =============================================================

// Buffer validation entry structure
typedef struct BufferValidationEntry {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  u32 checksum;
  u64 creation_time;
  u64 last_validation_time;
  u32 validation_count;
  bool is_valid;
  bool has_pattern;
  u8 pattern_byte;
  struct BufferValidationEntry *next;
} BufferValidationEntry;

// Validation statistics
typedef struct BufferValidationStats {
  u32 total_buffers;
  u32 valid_buffers;
  u32 corrupted_buffers;
  u32 validation_failures;
  u64 total_validations;
  u64 corruption_detected;
  f32 corruption_rate;
} BufferValidationStats;

static BufferValidationEntry *g_validation_entries = NULL;
static BufferValidationStats g_validation_stats = {0};
static u32 g_validation_enabled = 1;
static u32 g_validation_pattern = 0xDEADBEEF;

// Simple checksum calculation
static u32 calculate_checksum(const void *data, VkDeviceSize size) {
  const u8 *bytes = (const u8 *)data;
  u32 checksum = 0;

  for (VkDeviceSize i = 0; i < size; i++) {
    checksum = (checksum << 8) | (checksum >> 24);
    checksum ^= bytes[i];
    checksum = (checksum * 31) + (i & 0xFF);
  }

  return checksum;
}

// Initialize buffer validation system
void vulkan_buffer_validation_init() {
  g_validation_entries = NULL;
  memset(&g_validation_stats, 0, sizeof(BufferValidationStats));
  g_validation_enabled = 1;

  printf("Vulkan: Buffer validation system initialized\n");
  printf("  Validation pattern: 0x%08X\n", g_validation_pattern);
  printf("  Validation enabled: %s\n", g_validation_enabled ? "yes" : "no");
}

// Add buffer to validation tracking
void vulkan_buffer_validation_add(VkBuffer buffer, VkDeviceMemory memory,
                                  VkDeviceSize size, bool fill_pattern) {
  if (!g_validation_enabled || buffer == VK_NULL_HANDLE ||
      memory == VK_NULL_HANDLE) {
    return;
  }

  // Create validation entry
  BufferValidationEntry *entry = malloc(sizeof(BufferValidationEntry));
  if (!entry) {
    printf("Vulkan: Failed to allocate buffer validation entry\n");
    return;
  }

  entry->buffer = buffer;
  entry->memory = memory;
  entry->size = size;
  entry->creation_time = 0; // Would use actual timestamp
  entry->last_validation_time = 0;
  entry->validation_count = 0;
  entry->is_valid = true;
  entry->has_pattern = fill_pattern;
  entry->pattern_byte = (u8)(g_validation_pattern & 0xFF);
  entry->next = g_validation_entries;

  // Map memory to calculate initial checksum
  void *data;
  VkResult result = vkMapMemory(VK_NULL_HANDLE, memory, 0, size, 0, &data);
  if (result == VK_SUCCESS) {
    entry->checksum = calculate_checksum(data, size);

    // Fill with pattern if requested
    if (fill_pattern && size > 0) {
      memset(data, entry->pattern_byte, size);
    }

    vkUnmapMemory(VK_NULL_HANDLE, memory);
  } else {
    // Fallback - use buffer size as checksum
    entry->checksum = (u32)size;
  }

  // Add to linked list
  g_validation_entries = entry;
  g_validation_stats.total_buffers++;
  g_validation_stats.valid_buffers++;

  printf("Vulkan: Added buffer to validation tracking (size: %llu bytes, "
         "pattern: %s)\n",
         (unsigned long long)size, fill_pattern ? "yes" : "no");
}

// Validate buffer integrity
bool vulkan_buffer_validation_check(VkBuffer buffer, VkDeviceMemory memory,
                                    VkDeviceSize size) {
  if (!g_validation_enabled || buffer == VK_NULL_HANDLE ||
      memory == VK_NULL_HANDLE) {
    return true; // Assume valid if validation is disabled
  }

  // Find validation entry
  BufferValidationEntry *entry = g_validation_entries;
  while (entry) {
    if (entry->buffer == buffer && entry->memory == memory &&
        entry->size == size) {
      break;
    }
    entry = entry->next;
  }

  if (!entry) {
    printf("Vulkan: Buffer not found in validation tracking\n");
    return false;
  }

  // Map memory to validate
  void *data;
  VkResult result = vkMapMemory(VK_NULL_HANDLE, memory, 0, size, 0, &data);
  if (result != VK_SUCCESS) {
    printf("Vulkan: Failed to map buffer for validation\n");
    g_validation_stats.validation_failures++;
    return false;
  }

  // Calculate current checksum
  u32 current_checksum = calculate_checksum(data, size);
  bool is_valid = (current_checksum == entry->checksum);

  // Check pattern if enabled
  if (entry->has_pattern && is_valid) {
    u8 *bytes = (u8 *)data;
    for (VkDeviceSize i = 0; i < size; i++) {
      if (bytes[i] != entry->pattern_byte) {
        is_valid = false;
        break;
      }
    }
  }

  // Update statistics
  entry->last_validation_time = 0; // Would use actual timestamp
  entry->validation_count++;
  g_validation_stats.total_validations++;

  if (!is_valid) {
    g_validation_stats.corrupted_buffers++;
    g_validation_stats.valid_buffers--;
    g_validation_stats.corruption_detected++;
    entry->is_valid = false;

    printf("Vulkan: BUFFER CORRUPTION DETECTED!\n");
    printf("  Buffer: %p, Size: %llu bytes\n", (void *)buffer, (unsigned long long)size);
    printf("  Expected checksum: 0x%08X\n", entry->checksum);
    printf("  Current checksum: 0x%08X\n", current_checksum);
    printf("  Previous validations: %u\n", entry->validation_count);

    if (entry->has_pattern) {
      printf("  Pattern byte: 0x%02X\n", entry->pattern_byte);
    }
  } else {
    if (!entry->is_valid) {
      // Buffer was previously corrupted but is now valid
      printf("Vulkan: Buffer corruption resolved (buffer: %p)\n",
             (void *)buffer);
      entry->is_valid = true;
      g_validation_stats.corrupted_buffers--;
      g_validation_stats.valid_buffers++;
    }
  }

  vkUnmapMemory(VK_NULL_HANDLE, memory);

  // Update corruption rate
  if (g_validation_stats.total_validations > 0) {
    g_validation_stats.corruption_rate =
        (f32)g_validation_stats.corruption_detected /
        g_validation_stats.total_validations;
  }

  return is_valid;
}

// Validate all tracked buffers
void vulkan_buffer_validation_check_all() {
  if (!g_validation_enabled) {
    printf("Vulkan: Buffer validation is disabled\n");
    return;
  }

  printf("Vulkan: Checking all tracked buffers...\n");

  u32 checked_count = 0;
  u32 corrupted_count = 0;

  BufferValidationEntry *entry = g_validation_entries;
  while (entry) {
    bool is_valid = vulkan_buffer_validation_check(entry->buffer, entry->memory,
                                                   entry->size);
    if (!is_valid) {
      corrupted_count++;
    }
    checked_count++;
    entry = entry->next;
  }

  printf("Vulkan: Buffer validation complete\n");
  printf("  Checked: %u buffers\n", checked_count);
  printf("  Corrupted: %u buffers\n", corrupted_count);
  printf("  Corruption rate: %.4f%%\n",
         g_validation_stats.corruption_rate * 100.0f);
}

// Get validation statistics
void vulkan_buffer_validation_get_stats(BufferValidationStats *stats) {
  if (stats) {
    *stats = g_validation_stats;
  }
}

// Print validation statistics
void vulkan_buffer_validation_print_stats() {
  printf("=== Vulkan Buffer Validation Statistics ===\n");
  printf("Total buffers tracked: %u\n", g_validation_stats.total_buffers);
  printf("Valid buffers: %u\n", g_validation_stats.valid_buffers);
  printf("Corrupted buffers: %u\n", g_validation_stats.corrupted_buffers);
  printf("Total validations: %llu\n",
         (unsigned long long)g_validation_stats.total_validations);
  printf("Corruption detected: %llu\n",
         (unsigned long long)g_validation_stats.corruption_detected);
  printf("Validation failures: %u\n", g_validation_stats.validation_failures);

  if (g_validation_stats.total_validations > 0) {
    printf("Corruption rate: %.6f%%\n",
           g_validation_stats.corruption_rate * 100.0f);
  }

  printf("Validation enabled: %s\n", g_validation_enabled ? "yes" : "no");
  printf("========================================\n");
}

// Enable/disable validation
void vulkan_buffer_validation_set_enabled(bool enabled) {
  g_validation_enabled = enabled ? 1 : 0;
  printf("Vulkan: Buffer validation %s\n", enabled ? "enabled" : "disabled");
}

// Set validation pattern
void vulkan_buffer_validation_set_pattern(u32 pattern) {
  g_validation_pattern = pattern;
  printf("Vulkan: Validation pattern set to 0x%08X\n", pattern);
}

// Cleanup buffer validation system
void vulkan_buffer_validation_cleanup() {
  BufferValidationEntry *entry = g_validation_entries;
  while (entry) {
    BufferValidationEntry *next = entry->next;
    free(entry);
    entry = next;
  }

  g_validation_entries = NULL;
  memset(&g_validation_stats, 0, sizeof(BufferValidationStats));

  printf("Vulkan: Buffer validation system cleaned up\n");
}

// Create buffer (now uses pooling system with validation)
bool vulkan_create_buffer(VulkanRenderer *renderer, VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBuffer *buffer,
                          VkDeviceMemory *buffer_memory) {
#ifdef VULKAN_BUILD
  // Try to allocate from pool first
  if (buffer_pool_allocate(renderer->device, size, usage, properties, buffer,
                           buffer_memory)) {
    // Add to validation system
    vulkan_buffer_validation_add(*buffer, *buffer_memory, size,
                                 properties &
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    return true;
  }

  // Pool allocation failed, fall back to direct allocation
  VkBufferCreateInfo buffer_info = {0};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(renderer->device, &buffer_info, NULL, buffer) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to create buffer");
    return false;
  }

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(renderer->device, *buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      find_memory_type(renderer, mem_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL, buffer_memory) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to allocate buffer memory");
    vkDestroyBuffer(renderer->device, *buffer, NULL);
    return false;
  }

  vkBindBufferMemory(renderer->device, *buffer, *buffer_memory, 0);

  // Add to validation system
  vulkan_buffer_validation_add(*buffer, *buffer_memory, mem_requirements.size,
                               properties &
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  return true;
#else
  (void)renderer;
  (void)size;
  (void)usage;
  (void)properties;
  (void)buffer;
  (void)buffer_memory;
  return true;
#endif
}

// Destroy buffer (returns to pool if possible)
void vulkan_destroy_buffer(VulkanRenderer *renderer, VkBuffer buffer,
                           VkDeviceMemory memory) {
#ifdef VULKAN_BUILD
  // Remove from validation tracking first
  BufferValidationEntry *entry = g_validation_entries;
  BufferValidationEntry *prev = NULL;

  while (entry) {
    if (entry->buffer == buffer && entry->memory == memory) {
      // Remove from linked list
      if (prev) {
        prev->next = entry->next;
      } else {
        g_validation_entries = entry->next;
      }

      // Update statistics
      if (entry->is_valid) {
        g_validation_stats.valid_buffers--;
      } else {
        g_validation_stats.corrupted_buffers--;
      }
      g_validation_stats.total_buffers--;

      free(entry);
      break;
    }
    prev = entry;
    entry = entry->next;
  }

  // Try to return to pool first
  buffer_pool_deallocate(renderer->device, buffer, memory);
#else
  (void)renderer;
  (void)buffer;
  (void)memory;
#endif
}
#endif

// Copy buffer
void vulkan_copy_buffer(VulkanRenderer *renderer, VkBuffer src_buffer,
                        VkBuffer dst_buffer, VkDeviceSize size) {
#ifdef VULKAN_BUILD
  VkCommandBufferAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = renderer->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(renderer->device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferCopy copy_region = {0};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = 0;
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(renderer->graphics_queue);

  vkFreeCommandBuffers(renderer->device, renderer->command_pool, 1,
                       &command_buffer);
#else
  (void)renderer;
  (void)src_buffer;
  (void)dst_buffer;
  (void)size;
#endif
}

// Create vertex buffer for chunk
bool vulkan_create_chunk_vertex_buffer(VulkanRenderer *renderer, Mesh *mesh,
                                       VkBuffer *vertex_buffer,
                                       VkDeviceMemory *vertex_buffer_memory) {
  if (!mesh || mesh->vertex_count == 0) {
    return false;
  }

#ifdef VULKAN_BUILD
  // Phase 8: Use Streaming System
  // Check if we already have a streaming allocation
  if (mesh->vertex_alloc.active) {
    *vertex_buffer = mesh->vertex_alloc.buffer;
    return true;
  }

  // Try to allocate from streaming pool (allocate BOTH vertex and index if
  // possible) We do it here because the streaming API allocates both together
  // for efficiency
  if (mesh->index_count > 0 && !mesh->index_alloc.active) {
    VkDeviceSize v_size = sizeof(Vertex) * mesh->vertex_count;
    VkDeviceSize i_size = sizeof(u32) * mesh->index_count;

    if (vulkan_streaming_alloc(renderer, v_size, i_size, &mesh->vertex_alloc,
                               &mesh->index_alloc)) {
      // Success! Upload data
      if (vulkan_streaming_upload(renderer, mesh->vertex_alloc, mesh->vertices,
                                  mesh->index_alloc, mesh->indices)) {
        *vertex_buffer = mesh->vertex_alloc.buffer;
        return true;
      }
      // Upload failed, deactivate
      mesh->vertex_alloc.active = false;
      mesh->index_alloc.active = false;
    }
  }
#endif

  VkDeviceSize buffer_size = sizeof(Vertex) * mesh->vertex_count;

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  if (!vulkan_create_buffer(renderer, buffer_size,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &staging_buffer, &staging_buffer_memory)) {
    return false;
  }

// Copy vertex data to staging buffer
#ifdef VULKAN_BUILD
  void *data;
  vkMapMemory(renderer->device, staging_buffer_memory, 0, buffer_size, 0,
              &data);
  memcpy(data, mesh->vertices, buffer_size);
  vkUnmapMemory(renderer->device, staging_buffer_memory);
#endif

  if (!vulkan_create_buffer(renderer, buffer_size,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer,
                            vertex_buffer_memory)) {
    vkDestroyBuffer(renderer->device, staging_buffer, NULL);
    vkFreeMemory(renderer->device, staging_buffer_memory, NULL);
    return false;
  }

  // Copy staging buffer to vertex buffer
  vulkan_copy_buffer(renderer, staging_buffer, *vertex_buffer, buffer_size);

// Cleanup staging buffer
#ifdef VULKAN_BUILD
  vkDestroyBuffer(renderer->device, staging_buffer, NULL);
  vkFreeMemory(renderer->device, staging_buffer_memory, NULL);
#endif

  return true;
}

// Create index buffer for chunk
bool vulkan_create_chunk_index_buffer(VulkanRenderer *renderer, Mesh *mesh,
                                      VkBuffer *index_buffer,
                                      VkDeviceMemory *index_buffer_memory) {
  if (!mesh || mesh->index_count == 0) {
    return false;
  }

#ifdef VULKAN_BUILD
  // Phase 8: Use Streaming System
  // Ideally allocated in create_vertex_buffer. Check if active.
  if (mesh->index_alloc.active) {
    *index_buffer = mesh->index_alloc.buffer;
    return true;
  }
#endif

  VkDeviceSize buffer_size = sizeof(u32) * mesh->index_count;

  // Create staging buffer
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  if (!vulkan_create_buffer(renderer, buffer_size,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &staging_buffer, &staging_buffer_memory)) {
    return false;
  }

// Copy index data to staging buffer
#ifdef VULKAN_BUILD
  void *data;
  vkMapMemory(renderer->device, staging_buffer_memory, 0, buffer_size, 0,
              &data);
  memcpy(data, mesh->indices, buffer_size);
  vkUnmapMemory(renderer->device, staging_buffer_memory);
#endif

  // Create index buffer
  if (!vulkan_create_buffer(renderer, buffer_size,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer,
                            index_buffer_memory)) {
    vkDestroyBuffer(renderer->device, staging_buffer, NULL);
    vkFreeMemory(renderer->device, staging_buffer_memory, NULL);
    return false;
  }

  // Copy staging buffer to index buffer
  vulkan_copy_buffer(renderer, staging_buffer, *index_buffer, buffer_size);

// Cleanup staging buffer
#ifdef VULKAN_BUILD
  vkDestroyBuffer(renderer->device, staging_buffer, NULL);
  vkFreeMemory(renderer->device, staging_buffer_memory, NULL);
#endif

  return true;
}

bool vulkan_update_chunk_buffers(VulkanRenderer *renderer, Mesh *mesh,
                                 VkBuffer vertex_buffer,
                                 VkBuffer index_buffer) {
  if (!mesh) {
    return false;
  }

  // Update vertex buffer if needed
  if (mesh->vertex_count > 0) {
    VkDeviceSize vertex_size = sizeof(Vertex) * mesh->vertex_count;

// Map and update
#ifdef VULKAN_BUILD
    void *data;
    VkDeviceMemory vertex_memory;
    vkGetBufferMemoryRequirements(renderer->device, vertex_buffer, NULL);
// Would need to get memory handle - simplified for now
// vkMapMemory(renderer->device, vertex_memory, 0, vertex_size, 0, &data);
// memcpy(data, mesh->vertices, vertex_size);
// vkUnmapMemory(renderer->device, vertex_memory);
#endif
  }

  // Update index buffer if needed
  if (mesh->index_count > 0) {
    VkDeviceSize index_size = sizeof(u32) * mesh->index_count;

    // Similar to vertex buffer update
  }

  return true;
}

// VULKAN-BF-005: Buffer Performance Profiling System
// ==================================================

// Buffer profiling entry structure
typedef struct BufferProfileEntry {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  VkBufferUsageFlags usage;
  u64 creation_time;
  u64 last_access_time;
  u64 total_access_time;
  u32 access_count;
  f64 avg_access_duration;
  f64 min_access_duration;
  f64 max_access_duration;
  bool is_active;
  struct BufferProfileEntry *next;
} BufferProfileEntry;

// Profiling statistics
typedef struct BufferProfileStats {
  u32 total_buffers;
  u32 active_buffers;
  u64 total_accesses;
  f64 total_access_time;
  f64 avg_access_time;
  f64 min_access_time;
  f64 max_access_time;
  u64 profile_start_time;
  u64 profile_end_time;
} BufferProfileStats;

static BufferProfileEntry *g_profile_entries = NULL;
static BufferProfileStats g_profile_stats = {0};
static u32 g_profiling_enabled = 1;

// Get high-resolution timestamp (platform-specific)
static u64 get_timestamp() {
  // In a real implementation, use platform-specific high-resolution timers
  // For demo purposes, use a simple counter
  static u64 counter = 0;
  return ++counter;
}

// Initialize buffer profiling system
void vulkan_buffer_profile_init() {
  g_profile_entries = NULL;
  memset(&g_profile_stats, 0, sizeof(BufferProfileStats));
  g_profiling_enabled = 1;
  g_profile_stats.profile_start_time = get_timestamp();

  printf("Vulkan: Buffer profiling system initialized\n");
  printf("  Profiling enabled: %s\n", g_profiling_enabled ? "yes" : "no");
}

// Add buffer to profiling tracking
void vulkan_buffer_profile_add(VkBuffer buffer, VkDeviceMemory memory,
                               VkDeviceSize size, VkBufferUsageFlags usage) {
  if (!g_profiling_enabled || buffer == VK_NULL_HANDLE ||
      memory == VK_NULL_HANDLE) {
    return;
  }

  // Create profile entry
  BufferProfileEntry *entry = malloc(sizeof(BufferProfileEntry));
  if (!entry) {
    printf("Vulkan: Failed to allocate buffer profile entry\n");
    return;
  }

  entry->buffer = buffer;
  entry->memory = memory;
  entry->size = size;
  entry->usage = usage;
  entry->creation_time = get_timestamp();
  entry->last_access_time = 0;
  entry->total_access_time = 0.0;
  entry->access_count = 0;
  entry->avg_access_duration = 0.0;
  entry->min_access_duration = 1.0; // Initialize to 1 second
  entry->max_access_duration = 0.0;
  entry->is_active = true;
  entry->next = g_profile_entries;

  // Add to linked list
  g_profile_entries = entry;
  g_profile_stats.total_buffers++;
  g_profile_stats.active_buffers++;

  printf("Vulkan: Added buffer to profiling tracking (size: %llu bytes, usage: "
         "0x%08X)\n",
         (unsigned long long)size, usage);
}

// Begin buffer access profiling
u64 vulkan_buffer_profile_begin_access(VkBuffer buffer) {
  if (!g_profiling_enabled || buffer == VK_NULL_HANDLE) {
    return 0;
  }

  // Find profile entry
  BufferProfileEntry *entry = g_profile_entries;
  while (entry) {
    if (entry->buffer == buffer && entry->is_active) {
      break;
    }
    entry = entry->next;
  }

  if (!entry) {
    return 0;
  }

  u64 start_time = get_timestamp();
  entry->last_access_time = start_time;

  return start_time;
}

// End buffer access profiling
void vulkan_buffer_profile_end_access(VkBuffer buffer, u64 start_time) {
  if (!g_profiling_enabled || buffer == VK_NULL_HANDLE || start_time == 0) {
    return;
  }

  // Find profile entry
  BufferProfileEntry *entry = g_profile_entries;
  while (entry) {
    if (entry->buffer == buffer && entry->is_active) {
      break;
    }
    entry = entry->next;
  }

  if (!entry) {
    return;
  }

  u64 end_time = get_timestamp();
  f64 access_duration =
      (f64)(end_time -
            start_time); // In real implementation, convert to seconds

  // Update entry statistics
  entry->total_access_time += access_duration;
  entry->access_count++;
  entry->avg_access_duration = entry->total_access_time / entry->access_count;

  if (access_duration < entry->min_access_duration) {
    entry->min_access_duration = access_duration;
  }

  if (access_duration > entry->max_access_duration) {
    entry->max_access_duration = access_duration;
  }

  // Update global statistics
  g_profile_stats.total_accesses++;
  g_profile_stats.total_access_time += access_duration;

  if (g_profile_stats.total_accesses > 0) {
    g_profile_stats.avg_access_time =
        g_profile_stats.total_access_time / g_profile_stats.total_accesses;
  }

  if (g_profile_stats.min_access_time == 0.0 ||
      access_duration < g_profile_stats.min_access_time) {
    g_profile_stats.min_access_time = access_duration;
  }

  if (access_duration > g_profile_stats.max_access_time) {
    g_profile_stats.max_access_time = access_duration;
  }
}

// Profile buffer operation (convenience function)
void vulkan_buffer_profile_operation(VkBuffer buffer,
                                     void (*operation_func)(void *),
                                     void *user_data) {
  u64 start_time = vulkan_buffer_profile_begin_access(buffer);

  if (operation_func) {
    operation_func(user_data);
  }

  vulkan_buffer_profile_end_access(buffer, start_time);
}

// Get buffer profile statistics
void vulkan_buffer_profile_get_buffer_stats(VkBuffer buffer,
                                            BufferProfileEntry *stats) {
  if (!stats || buffer == VK_NULL_HANDLE) {
    return;
  }

  BufferProfileEntry *entry = g_profile_entries;
  while (entry) {
    if (entry->buffer == buffer && entry->is_active) {
      *stats = *entry;
      return;
    }
    entry = entry->next;
  }

  // Buffer not found - zero out stats
  memset(stats, 0, sizeof(BufferProfileEntry));
}

// Get global profiling statistics
void vulkan_buffer_profile_get_global_stats(BufferProfileStats *stats) {
  if (stats) {
    *stats = g_profile_stats;
    stats->profile_end_time = get_timestamp();
  }
}

// Print buffer profile statistics
void vulkan_buffer_profile_print_stats() {
  printf("=== Vulkan Buffer Profile Statistics ===\n");
  printf("Total buffers tracked: %u\n", g_profile_stats.total_buffers);
  printf("Active buffers: %u\n", g_profile_stats.active_buffers);
  printf("Total accesses: %llu\n",
         (unsigned long long)g_profile_stats.total_accesses);
  printf("Total access time: %.6f seconds\n",
         g_profile_stats.total_access_time);
  printf("Average access time: %.6f seconds\n",
         g_profile_stats.avg_access_time);
  printf("Min access time: %.6f seconds\n", g_profile_stats.min_access_time);
  printf("Max access time: %.6f seconds\n", g_profile_stats.max_access_time);

  if (g_profile_stats.profile_start_time > 0) {
    u64 profile_duration = get_timestamp() - g_profile_stats.profile_start_time;
    printf("Profile duration: %llu units\n",
           (unsigned long long)profile_duration);
  }

  printf("Profiling enabled: %s\n", g_profiling_enabled ? "yes" : "no");
  printf("====================================\n");
}

// Print detailed buffer statistics
void vulkan_buffer_profile_print_detailed_stats() {
  if (!g_profiling_enabled) {
    printf("Vulkan: Buffer profiling is disabled\n");
    return;
  }

  printf("=== Detailed Vulkan Buffer Profile Statistics ===\n");

  u32 buffer_count = 0;
  BufferProfileEntry *entry = g_profile_entries;

  while (entry && buffer_count < 10) { // Limit to top 10 buffers
    if (entry->is_active) {
      printf("Buffer %p:\n", (void *)entry->buffer);
      printf("  Size: %llu bytes\n", (unsigned long long)entry->size);
      printf("  Usage: 0x%08X\n", entry->usage);
      printf("  Accesses: %u\n", entry->access_count);
      printf("  Avg access time: %.6f seconds\n", entry->avg_access_duration);
      printf("  Min access time: %.6f seconds\n", entry->min_access_duration);
      printf("  Max access time: %.6f seconds\n", entry->max_access_duration);
      printf("  Total access time: %.6f seconds\n", (f64)entry->total_access_time);
      printf("\n");
      buffer_count++;
    }
    entry = entry->next;
  }

  if (g_profile_stats.total_buffers > 10) {
    printf("... and %u more buffers\n", g_profile_stats.total_buffers - 10);
  }

  printf("===============================================\n");
}

// Enable/disable profiling
void vulkan_buffer_profile_set_enabled(bool enabled) {
  g_profiling_enabled = enabled ? 1 : 0;
  printf("Vulkan: Buffer profiling %s\n", enabled ? "enabled" : "disabled");
}

// Mark buffer as inactive (e.g., when destroyed)
void vulkan_buffer_profile_mark_inactive(VkBuffer buffer) {
  if (!g_profiling_enabled || buffer == VK_NULL_HANDLE) {
    return;
  }

  BufferProfileEntry *entry = g_profile_entries;
  while (entry) {
    if (entry->buffer == buffer && entry->is_active) {
      entry->is_active = false;
      g_profile_stats.active_buffers--;
      break;
    }
    entry = entry->next;
  }
}

// Cleanup buffer profiling system
void vulkan_buffer_profile_cleanup() {
  BufferProfileEntry *entry = g_profile_entries;
  while (entry) {
    BufferProfileEntry *next = entry->next;
    free(entry);
    entry = next;
  }

  g_profile_entries = NULL;
  memset(&g_profile_stats, 0, sizeof(BufferProfileStats));

  printf("Vulkan: Buffer profiling system cleaned up\n");
}
