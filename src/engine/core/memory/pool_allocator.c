#include "engine/include/core/memory.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * =================================================================================================
 *                          POOL ALLOCATOR - AGENT_CORE_1
 * =================================================================================================
 *
 * PURPOSE: Fixed-size block allocator for objects of same size (entities,
 * components, etc.) Eliminates fragmentation and provides O(1) alloc/free with
 * excellent cache locality.
 *
 * PERFORMANCE TARGET: <20ns per allocation (free-list traversal)
 *
 * =================================================================================================
 */

#define POOL_SIGNATURE 0x504F4F4C // "POOL"
#define GUARD_MAGIC 0xFEEDFACE
#define POISON_FREE 0xDDDDDDDD
#define POISON_ALLOC 0xCCCCCCCC

typedef struct PoolAllocator {
  uint32_t signature;
  void *buffer;             // Base memory buffer
  size_t block_size;        // Size of each block (with padding)
  size_t block_count;       // Total number of blocks
  void *free_list;          // Linked list of free blocks
  atomic_uint free_count;   // Number of free blocks
  uint32_t peak_usage;      // Peak number of allocated blocks
  uint8_t *allocation_map;  // Bitset tracking allocated blocks (for debugging)
  bool debug_mode;          // Debug features enabled
  bool thread_safe;         // Thread-safe operations
  void *mutex;              // Mutex for thread safety (if needed)
  size_t alignment;         // Block alignment
  size_t actual_block_size; // Block size including padding
} PoolAllocator;

//  COMPLETED: Internal helper functions
static inline size_t pool_align_up(size_t size, size_t alignment) {
  return (size + alignment - 1) & ~(alignment - 1);
}

static inline uint32_t pool_get_block_index(PoolAllocator *allocator,
                                            void *ptr) {
  return (uint32_t)((char *)ptr - (char *)allocator->buffer) /
         allocator->actual_block_size;
}

static inline bool pool_is_allocated(PoolAllocator *allocator, uint32_t index) {
  return (allocator->allocation_map[index / 8] & (1 << (index % 8))) != 0;
}

static inline void pool_set_allocated(PoolAllocator *allocator, uint32_t index,
                                      bool allocated) {
  if (allocated) {
    allocator->allocation_map[index / 8] |= (1 << (index % 8));
  } else {
    allocator->allocation_map[index / 8] &= ~(1 << (index % 8));
  }
}

static inline void *pool_get_block(PoolAllocator *allocator, uint32_t index) {
  return (char *)allocator->buffer + index * allocator->actual_block_size;
}

//  COMPLETED: Mutex operations for thread safety
#ifdef _WIN32
#include <windows.h>
static void *pool_create_mutex() {
  CRITICAL_SECTION *cs = malloc(sizeof(CRITICAL_SECTION));
  InitializeCriticalSection(cs);
  return cs;
}

static void pool_destroy_mutex(void *mutex) {
  DeleteCriticalSection((CRITICAL_SECTION *)mutex);
  free(mutex);
}

static void pool_lock_mutex(void *mutex) {
  EnterCriticalSection((CRITICAL_SECTION *)mutex);
}

static void pool_unlock_mutex(void *mutex) {
  LeaveCriticalSection((CRITICAL_SECTION *)mutex);
}
#else
#include <pthread.h>
static void *pool_create_mutex() {
  pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  return mutex;
}

static void pool_destroy_mutex(void *mutex) {
  pthread_mutex_destroy((pthread_mutex_t *)mutex);
  free(mutex);
}

static void pool_lock_mutex(void *mutex) {
  pthread_mutex_lock((pthread_mutex_t *)mutex);
}

static void pool_unlock_mutex(void *mutex) {
  pthread_mutex_unlock((pthread_mutex_t *)mutex);
}
#endif

static inline void pool_lock(PoolAllocator *allocator) {
  if (allocator->thread_safe && allocator->mutex) {
    pool_lock_mutex(allocator->mutex);
  }
}

static inline void pool_unlock(PoolAllocator *allocator) {
  if (allocator->thread_safe && allocator->mutex) {
    pool_unlock_mutex(allocator->mutex);
  }
}

//  COMPLETED: Create Pool Allocator
PoolAllocator *pool_allocator_create(size_t block_size, size_t block_count,
                                     size_t alignment) {
  PoolAllocator *allocator = malloc(sizeof(PoolAllocator));
  if (!allocator)
    return NULL;

  // Align block size to at least pointer size and requested alignment
  size_t min_size = sizeof(void *);
  size_t aligned_block_size = pool_align_up(block_size, alignment);
  aligned_block_size = pool_align_up(aligned_block_size, min_size);

  // Add debug padding if enabled
  size_t actual_block_size = aligned_block_size;
#ifdef DEBUG
  actual_block_size += 32; // 16 bytes before + 16 bytes after
#endif

  size_t total_size = actual_block_size * block_count;
  allocator->buffer = malloc(total_size);
  if (!allocator->buffer) {
    free(allocator);
    return NULL;
  }

  // Create allocation map (1 bit per block)
  size_t map_size = (block_count + 7) / 8;
  allocator->allocation_map = malloc(map_size);
  if (!allocator->allocation_map) {
    free(allocator->buffer);
    free(allocator);
    return NULL;
  }

  allocator->signature = POOL_SIGNATURE;
  allocator->block_size = block_size;
  allocator->block_count = block_count;
  allocator->actual_block_size = actual_block_size;
  allocator->alignment = alignment;
  allocator->peak_usage = 0;
  allocator->debug_mode = false;
  allocator->thread_safe = false;
  allocator->mutex = NULL;

  // Initialize free list by linking all blocks
  allocator->free_list = NULL;
  for (uint32_t i = 0; i < block_count; i++) {
    void *block = pool_get_block(allocator, i);
    void **next_ptr = (void **)block;
    *next_ptr = allocator->free_list;
    allocator->free_list = block;
  }

  atomic_store(&allocator->free_count, block_count);
  memset(allocator->allocation_map, 0, map_size);

  return allocator;
}

//  COMPLETED: Allocate from Pool
void *pool_allocator_alloc(PoolAllocator *allocator) {
  if (!allocator || allocator->signature != POOL_SIGNATURE) {
    return NULL;
  }

  pool_lock(allocator);

  if (allocator->free_list == NULL) {
    pool_unlock(allocator);
    if (allocator->debug_mode) {
      printf("Pool allocator exhausted: %zu blocks requested, 0 available\n",
             allocator->block_count);
    }
    return NULL;
  }

  // Pop first block from free list
  void *block = allocator->free_list;
  void **next_ptr = (void **)block;
  allocator->free_list = *next_ptr;

  uint32_t free_count = atomic_fetch_sub(&allocator->free_count, 1) - 1;
  uint32_t allocated_count = allocator->block_count - free_count;

  // Update peak usage
  if (allocated_count > allocator->peak_usage) {
    allocator->peak_usage = allocated_count;
  }

  // Mark as allocated
  uint32_t block_index = pool_get_block_index(allocator, block);
  pool_set_allocated(allocator, block_index, true);

  // Poison allocated memory in debug mode
  if (allocator->debug_mode) {
#ifdef DEBUG
    // Fill user data area with poison pattern
    void *user_data = (char *)block + 16;
    memset(user_data, POISON_ALLOC, allocator->block_size);

    // Set guard magic values
    uint32_t *front_guard = (uint32_t *)((char *)block + 12);
    uint32_t *back_guard =
        (uint32_t *)((char *)block + 16 + allocator->block_size);
    *front_guard = GUARD_MAGIC;
    *back_guard = GUARD_MAGIC;
#endif
  }

  pool_unlock(allocator);

// Return pointer to user data area
#ifdef DEBUG
  return (char *)block + 16;
#else
  return block;
#endif
}

//  COMPLETED: Free to Pool
void pool_allocator_free(PoolAllocator *allocator, void *ptr) {
  if (!ptr || !allocator || allocator->signature != POOL_SIGNATURE) {
    return;
  }

// Get actual block pointer (subtract debug offset)
#ifdef DEBUG
  void *block = (char *)ptr - 16;
#else
  void *block = ptr;
#endif

  pool_lock(allocator);

  // Validate pointer is within buffer range
  if ((char *)block < (char *)allocator->buffer ||
      (char *)block >=
          (char *)allocator->buffer +
              allocator->actual_block_size * allocator->block_count) {
    pool_unlock(allocator);
    if (allocator->debug_mode) {
      printf("Pool allocator: Invalid pointer %p (out of range)\n", ptr);
    }
    assert(false && "Pool allocator: pointer out of range");
    return;
  }

  // Validate pointer alignment
  if (((uintptr_t)block - (uintptr_t)allocator->buffer) %
          allocator->actual_block_size !=
      0) {
    pool_unlock(allocator);
    if (allocator->debug_mode) {
      printf("Pool allocator: Misaligned pointer %p\n", ptr);
    }
    assert(false && "Pool allocator: misaligned pointer");
    return;
  }

  uint32_t block_index = pool_get_block_index(allocator, block);

  // Check for double free
  if (!pool_is_allocated(allocator, block_index)) {
    pool_unlock(allocator);
    if (allocator->debug_mode) {
      printf("Pool allocator: Double free detected at %p (block %u)\n", ptr,
             block_index);
    }
    assert(false && "Pool allocator: double free detected");
    return;
  }

  // Check guard magic values in debug mode
  if (allocator->debug_mode) {
#ifdef DEBUG
    uint32_t *front_guard = (uint32_t *)((char *)block + 12);
    uint32_t *back_guard =
        (uint32_t *)((char *)block + 16 + allocator->block_size);

    if (*front_guard != GUARD_MAGIC) {
      printf("Pool allocator: Front guard corrupted at %p (found 0x%08X, "
             "expected 0x%08X)\n",
             ptr, *front_guard, GUARD_MAGIC);
      assert(false && "Pool allocator: front guard corrupted");
    }

    if (*back_guard != GUARD_MAGIC) {
      printf("Pool allocator: Back guard corrupted at %p (found 0x%08X, "
             "expected 0x%08X)\n",
             ptr, *back_guard, GUARD_MAGIC);
      assert(false && "Pool allocator: back guard corrupted");
    }
#endif
  }

  // Mark as free and add to free list
  pool_set_allocated(allocator, block_index, false);

  // Poison freed memory in debug mode
  if (allocator->debug_mode) {
#ifdef DEBUG
    memset(ptr, POISON_FREE, allocator->block_size);
#endif
  }

  // Push to front of free list (LIFO - better cache locality)
  void **next_ptr = (void **)block;
  *next_ptr = allocator->free_list;
  allocator->free_list = block;

  atomic_fetch_add(&allocator->free_count, 1);

  pool_unlock(allocator);
}

//  COMPLETED: Destroy Pool Allocator
void pool_allocator_destroy(PoolAllocator *allocator) {
  if (!allocator || allocator->signature != POOL_SIGNATURE) {
    return;
  }

  uint32_t free_count = atomic_load(&allocator->free_count);
  uint32_t leaked_blocks = allocator->block_count - free_count;

  if (leaked_blocks > 0) {
    if (allocator->debug_mode) {
      printf(
          "Pool allocator: Memory leak detected - %u blocks still allocated\n",
          leaked_blocks);
      printf("Peak usage was %u blocks out of %zu total\n",
             allocator->peak_usage, allocator->block_count);

      // Print allocated blocks
      for (uint32_t i = 0; i < allocator->block_count; i++) {
        if (pool_is_allocated(allocator, i)) {
          void *block = pool_get_block(allocator, i);
#ifdef DEBUG
          printf("Leaked block %u at %p\n", i, (char *)block + 16);
#else
          printf("Leaked block %u at %p\n", i, block);
#endif
        }
      }
    }
  }

  if (allocator->debug_mode) {
    printf("Pool allocator stats: peak usage %u/%zu blocks (%.1f%%)\n",
           allocator->peak_usage, allocator->block_count,
           (double)allocator->peak_usage / allocator->block_count * 100.0);
  }

  if (allocator->mutex) {
    pool_destroy_mutex(allocator->mutex);
  }

  free(allocator->allocation_map);
  free(allocator->buffer);
  free(allocator);
}

//  COMPLETED: Reset Pool Allocator
void pool_allocator_reset(PoolAllocator *allocator) {
  if (!allocator || allocator->signature != POOL_SIGNATURE) {
    return;
  }

  pool_lock(allocator);

  // Re-link all blocks into free list
  allocator->free_list = NULL;
  for (uint32_t i = 0; i < allocator->block_count; i++) {
    void *block = pool_get_block(allocator, i);
    void **next_ptr = (void **)block;
    *next_ptr = allocator->free_list;
    allocator->free_list = block;
  }

  atomic_store(&allocator->free_count, allocator->block_count);

  // Clear allocation map
  size_t map_size = (allocator->block_count + 7) / 8;
  memset(allocator->allocation_map, 0, map_size);

  pool_unlock(allocator);
}

//  COMPLETED: Enable Thread Safety
void pool_allocator_set_thread_safe(PoolAllocator *allocator,
                                    bool thread_safe) {
  if (!allocator || allocator->signature != POOL_SIGNATURE) {
    return;
  }

  if (thread_safe && !allocator->mutex) {
    allocator->mutex = pool_create_mutex();
    if (!allocator->mutex) {
      return; // Failed to create mutex
    }
  } else if (!thread_safe && allocator->mutex) {
    pool_destroy_mutex(allocator->mutex);
    allocator->mutex = NULL;
  }

  allocator->thread_safe = thread_safe;
}

//  COMPLETED: Get Statistics
void pool_allocator_get_stats(PoolAllocator *allocator, uint32_t *free_count,
                              uint32_t *peak_usage, uint32_t *allocated_count) {
  if (!allocator || allocator->signature != POOL_SIGNATURE) {
    return;
  }

  uint32_t free_cnt = atomic_load(&allocator->free_count);

  if (free_count)
    *free_count = free_cnt;
  if (peak_usage)
    *peak_usage = allocator->peak_usage;
  if (allocated_count)
    *allocated_count = allocator->block_count - free_cnt;
}

//  COMPLETED: Enable Debug Mode
void pool_allocator_set_debug_mode(PoolAllocator *allocator, bool enabled) {
  if (allocator && allocator->signature == POOL_SIGNATURE) {
    allocator->debug_mode = enabled;
  }
}

//  COMPLETED: Validate Pool Integrity
bool pool_allocator_validate(PoolAllocator *allocator) {
  if (!allocator || allocator->signature != POOL_SIGNATURE) {
    return false;
  }

  uint32_t free_count = 0;
  void *current = allocator->free_list;

  // Count free blocks by traversing free list
  while (current) {
    free_count++;

    // Validate block is within range
    if ((char *)current < (char *)allocator->buffer ||
        (char *)current >=
            (char *)allocator->buffer +
                allocator->actual_block_size * allocator->block_count) {
      return false;
    }

    // Validate alignment
    if (((uintptr_t)current - (uintptr_t)allocator->buffer) %
            allocator->actual_block_size !=
        0) {
      return false;
    }

    current = *(void **)current;
  }

  // Check free count matches
  uint32_t expected_free = atomic_load(&allocator->free_count);
  if (free_count != expected_free) {
    return false;
  }

  // Check allocation map consistency
  uint32_t allocated_count = 0;
  for (uint32_t i = 0; i < allocator->block_count; i++) {
    if (pool_is_allocated(allocator, i)) {
      allocated_count++;
    }
  }

  return (free_count + allocated_count == allocator->block_count);
}

//  COMPLETED: Debug Dump
void pool_allocator_debug_dump(PoolAllocator *allocator) {
  if (!allocator || allocator->signature != POOL_SIGNATURE) {
    printf("Pool Allocator: Invalid\n");
    return;
  }

  uint32_t free_count = atomic_load(&allocator->free_count);
  uint32_t allocated_count = allocator->block_count - free_count;

  printf("=== Pool Allocator Debug Info ===\n");
  printf("Block Size: %zu bytes\n", allocator->block_size);
  printf("Actual Block Size: %zu bytes\n", allocator->actual_block_size);
  printf("Block Count: %zu\n", allocator->block_count);
  printf("Total Memory: %zu bytes\n",
         allocator->actual_block_size * allocator->block_count);
  printf("Free Blocks: %u\n", free_count);
  printf("Allocated Blocks: %u\n", allocated_count);
  printf("Peak Usage: %u blocks (%.1f%%)\n", allocator->peak_usage,
         (double)allocator->peak_usage / allocator->block_count * 100.0);
  printf("Usage: %.1f%%\n",
         (double)allocated_count / allocator->block_count * 100.0);
  printf("Thread Safe: %s\n", allocator->thread_safe ? "YES" : "NO");
  printf("Debug Mode: %s\n", allocator->debug_mode ? "ON" : "OFF");
  printf("Valid: %s\n", pool_allocator_validate(allocator) ? "YES" : "NO");
  printf("================================\n");
}

//  COMPLETED: Convenience Macros
#define POOL_ALLOC(allocator) pool_allocator_alloc(allocator)
#define POOL_FREE(allocator, ptr) pool_allocator_free(allocator, ptr)
#define POOL_RESET(allocator) pool_allocator_reset(allocator)
#define POOL_GET_STATS(allocator, free, peak, allocated)                       \
  pool_allocator_get_stats(allocator, free, peak, allocated)
