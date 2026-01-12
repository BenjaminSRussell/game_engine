/**
 * CUSTOM MEMORY ALLOCATOR
 * High-performance stack and pool allocators for game systems
 * Enhanced with memory tracking, leak detection, and consolidation
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// Memory tracking structure
typedef struct {
  size_t total_allocated;
  size_t peak_usage;
  size_t allocation_count;
  size_t free_count;
  bool tracking_enabled;
} MemoryTracker;

// Global memory tracker
static MemoryTracker g_memory_tracker = {0};

// Allocation header for tracking
typedef struct AllocationHeader {
  size_t size;
  const char* file;
  int line;
  struct AllocationHeader* next;
  struct AllocationHeader* prev;
} AllocationHeader;

// Global allocation list for leak detection
static AllocationHeader* g_allocation_list = NULL;

// Memory tracking functions
static void memory_track_allocation(size_t size, const char* file, int line);
static void memory_track_free(size_t size);
static void memory_add_allocation(AllocationHeader* header);
static void memory_remove_allocation(AllocationHeader* header);
static void memory_check_leaks(void);

// --- Stack Allocator ---
// Good for per-frame temporary allocations
typedef struct {
  uint8_t *memory;
  size_t total_size;
  size_t offset;
  size_t peak_offset;
  MemoryTracker tracker;
} StackAllocator;

StackAllocator *stack_allocator_create(size_t size) {
  StackAllocator *allocator = malloc(sizeof(StackAllocator));
  if (!allocator) return NULL;
  
  allocator->memory = malloc(size);
  if (!allocator->memory) {
    free(allocator);
    return NULL;
  }
  
  allocator->total_size = size;
  allocator->offset = 0;
  allocator->peak_offset = 0;
  allocator->tracker = (MemoryTracker){0};
  allocator->tracker.tracking_enabled = true;
  
  return allocator;
}

void *stack_alloc(StackAllocator *sa, size_t size) {
  if (!sa) return NULL;
  
  size_t current_addr = (size_t)sa->memory + sa->offset;
  size_t padding = 0;

  // Calculate alignment padding
  if (current_addr % ALIGNMENT != 0) {
    padding = ALIGNMENT - (current_addr % ALIGNMENT);
  }

  if (sa->offset + padding + size > sa->total_size) {
    return NULL; // Out of memory
  }

  sa->offset += padding;
  void *ptr = sa->memory + sa->offset;
  sa->offset += size;
  
  // Update tracking
  if (sa->offset > sa->peak_offset) {
    sa->peak_offset = sa->offset;
  }
  if (sa->tracker.tracking_enabled) {
    sa->tracker.total_allocated = sa->offset;
    sa->tracker.allocation_count++;
    if (sa->tracker.total_allocated > sa->tracker.peak_usage) {
      sa->tracker.peak_usage = sa->tracker.total_allocated;
    }
  }

  return ptr;
}

void stack_reset(StackAllocator *sa) { 
  if (sa) {
    sa->offset = 0;
    if (sa->tracker.tracking_enabled) {
      sa->tracker.total_allocated = 0;
    }
  }
}

void stack_allocator_destroy(StackAllocator *sa) {
  if (sa) {
    // Report stats before destroying
    if (sa->tracker.tracking_enabled) {
      printf("Stack Allocator Stats:\n");
      printf("  Peak Usage: %zu bytes\n", sa->peak_offset);
      printf("  Total Size: %zu bytes\n", sa->total_size);
      printf("  Efficiency: %.2f%%\n", 
             (double)sa->peak_offset / sa->total_size * 100.0);
    }
    free(sa->memory);
    free(sa);
  }
}

// Get stack allocator statistics
void stack_allocator_get_stats(const StackAllocator* sa, size_t* used, size_t* peak, size_t* total) {
  if (!sa) return;
  if (used) *used = sa->offset;
  if (peak) *peak = sa->peak_offset;
  if (total) *total = sa->total_size;
}

// --- Pool Allocator ---
// Good for fixed-size objects (particles, entities, nodes)
typedef struct {
  uint8_t *memory;
  size_t block_size;
  size_t block_count;
  void **free_list; // Stack of free block pointers
  size_t free_count;
  size_t allocated_count;
  size_t peak_allocated;
  MemoryTracker tracker;
} PoolAllocator;

PoolAllocator *pool_allocator_create(size_t block_size, size_t block_count) {
  PoolAllocator *pool = malloc(sizeof(PoolAllocator));
  if (!pool) return NULL;
  
  pool->block_size = ALIGN(block_size);
  pool->block_count = block_count;
  pool->memory = malloc(pool->block_size * block_count);
  if (!pool->memory) {
    free(pool);
    return NULL;
  }
  
  pool->free_list = malloc(block_count * sizeof(void *));
  if (!pool->free_list) {
    free(pool->memory);
    free(pool);
    return NULL;
  }
  
  pool->free_count = block_count;
  pool->allocated_count = 0;
  pool->peak_allocated = 0;
  pool->tracker = (MemoryTracker){0};
  pool->tracker.tracking_enabled = true;

  // Initialize free list
  for (size_t i = 0; i < block_count; i++) {
    pool->free_list[i] = pool->memory + (i * pool->block_size);
  }

  return pool;
}

void *pool_alloc(PoolAllocator *pool) {
  if (!pool || pool->free_count == 0)
    return NULL;
  
  void* ptr = pool->free_list[--pool->free_count];
  pool->allocated_count++;
  
  // Update tracking
  if (pool->allocated_count > pool->peak_allocated) {
    pool->peak_allocated = pool->allocated_count;
  }
  if (pool->tracker.tracking_enabled) {
    pool->tracker.total_allocated = pool->allocated_count * pool->block_size;
    pool->tracker.allocation_count++;
    if (pool->tracker.total_allocated > pool->tracker.peak_usage) {
      pool->tracker.peak_usage = pool->tracker.total_allocated;
    }
  }
  
  return ptr;
}

void pool_free(PoolAllocator *pool, void *ptr) {
  if (!pool || !ptr) return;
  if (pool->free_count >= pool->block_count)
    return; // Should not happen
    
  pool->free_list[pool->free_count++] = ptr;
  pool->allocated_count--;
  
  if (pool->tracker.tracking_enabled) {
    pool->tracker.total_allocated = pool->allocated_count * pool->block_size;
    pool->tracker.free_count++;
  }
}

void pool_allocator_destroy(PoolAllocator *pool) {
  if (pool) {
    // Report stats before destroying
    if (pool->tracker.tracking_enabled) {
      printf("Pool Allocator Stats:\n");
      printf("  Block Size: %zu bytes\n", pool->block_size);
      printf("  Block Count: %zu\n", pool->block_count);
      printf("  Peak Allocated: %zu blocks\n", pool->peak_allocated);
      printf("  Total Size: %zu bytes\n", pool->block_size * pool->block_count);
      printf("  Peak Usage: %.2f%%\n", 
             (double)pool->peak_allocated / pool->block_count * 100.0);
    }
    free(pool->memory);
    free(pool->free_list);
    free(pool);
  }
}

// Get pool allocator statistics
void pool_allocator_get_stats(const PoolAllocator* pool, size_t* allocated, size_t* peak, size_t* total) {
  if (!pool) return;
  if (allocated) *allocated = pool->allocated_count;
  if (peak) *peak = pool->peak_allocated;
  if (total) *total = pool->block_count;
}

// --- Linear Allocator (Arena) ---
// Simple "pointer bump", can only be freed all at once
typedef struct {
  uint8_t *memory;
  size_t total_size;
  size_t offset;
  size_t peak_offset;
  MemoryTracker tracker;
} LinearAllocator;

LinearAllocator *linear_allocator_create(size_t size) {
  LinearAllocator *allocator = malloc(sizeof(LinearAllocator));
  if (!allocator) return NULL;
  
  allocator->memory = malloc(size);
  if (!allocator->memory) {
    free(allocator);
    return NULL;
  }
  
  allocator->total_size = size;
  allocator->offset = 0;
  allocator->peak_offset = 0;
  allocator->tracker = (MemoryTracker){0};
  allocator->tracker.tracking_enabled = true;
  
  return allocator;
}

void *linear_alloc(LinearAllocator *la, size_t size) {
  if (!la) return NULL;
  
  size_t current_addr = (size_t)la->memory + la->offset;
  size_t padding = 0;

  if (current_addr % ALIGNMENT != 0) {
    padding = ALIGNMENT - (current_addr % ALIGNMENT);
  }

  if (la->offset + padding + size > la->total_size) {
    return NULL;
  }

  la->offset += padding;
  void *ptr = la->memory + la->offset;
  la->offset += size;
  
  // Update tracking
  if (la->offset > la->peak_offset) {
    la->peak_offset = la->offset;
  }
  if (la->tracker.tracking_enabled) {
    la->tracker.total_allocated = la->offset;
    la->tracker.allocation_count++;
    if (la->tracker.total_allocated > la->tracker.peak_usage) {
      la->tracker.peak_usage = la->tracker.total_allocated;
    }
  }

  return ptr;
}

void linear_reset(LinearAllocator *la) { 
  if (la) {
    la->offset = 0;
    if (la->tracker.tracking_enabled) {
      la->tracker.total_allocated = 0;
    }
  }
}

void linear_allocator_destroy(LinearAllocator *la) {
  if (la) {
    // Report stats before destroying
    if (la->tracker.tracking_enabled) {
      printf("Linear Allocator Stats:\n");
      printf("  Peak Usage: %zu bytes\n", la->peak_offset);
      printf("  Total Size: %zu bytes\n", la->total_size);
      printf("  Efficiency: %.2f%%\n", 
             (double)la->peak_offset / la->total_size * 100.0);
    }
    free(la->memory);
    free(la);
  }
}

// Get linear allocator statistics
void linear_allocator_get_stats(const LinearAllocator* la, size_t* used, size_t* peak, size_t* total) {
  if (!la) return;
  if (used) *used = la->offset;
  if (peak) *peak = la->peak_offset;
  if (total) *total = la->total_size;
}

// -----------------------------------------------------------------------------
// Global Memory Management Functions
// -----------------------------------------------------------------------------

// Initialize global memory tracking
void memory_tracking_init(void) {
  g_memory_tracker.tracking_enabled = true;
  g_memory_tracker.total_allocated = 0;
  g_memory_tracker.peak_usage = 0;
  g_memory_tracker.allocation_count = 0;
  g_memory_tracker.free_count = 0;
}

// Shutdown global memory tracking and report leaks
void memory_tracking_shutdown(void) {
  if (g_memory_tracker.tracking_enabled) {
    printf("\n=== Global Memory Statistics ===\n");
    printf("Total Allocations: %zu\n", g_memory_tracker.allocation_count);
    printf("Total Frees: %zu\n", g_memory_tracker.free_count);
    printf("Peak Usage: %zu bytes\n", g_memory_tracker.peak_usage);
    printf("Current Usage: %zu bytes\n", g_memory_tracker.total_allocated);
    
    if (g_memory_tracker.allocation_count != g_memory_tracker.free_count) {
      printf("WARNING: Memory leak detected!\n");
      printf("Leaked allocations: %zu\n", 
             g_memory_tracker.allocation_count - g_memory_tracker.free_count);
    }
    
    memory_check_leaks();
    printf("================================\n\n");
  }
  g_memory_tracker.tracking_enabled = false;
}

// Get global memory statistics
void memory_get_global_stats(size_t* total_allocated, size_t* peak_usage, 
                            size_t* allocation_count, size_t* free_count) {
  if (total_allocated) *total_allocated = g_memory_tracker.total_allocated;
  if (peak_usage) *peak_usage = g_memory_tracker.peak_usage;
  if (allocation_count) *allocation_count = g_memory_tracker.allocation_count;
  if (free_count) *free_count = g_memory_tracker.free_count;
}

// -----------------------------------------------------------------------------
// Memory Tracking Implementation
// -----------------------------------------------------------------------------

static void memory_track_allocation(size_t size, const char* file, int line) {
  if (!g_memory_tracker.tracking_enabled) return;
  
  g_memory_tracker.total_allocated += size;
  g_memory_tracker.allocation_count++;
  
  if (g_memory_tracker.total_allocated > g_memory_tracker.peak_usage) {
    g_memory_tracker.peak_usage = g_memory_tracker.total_allocated;
  }
}

static void memory_track_free(size_t size) {
  if (!g_memory_tracker.tracking_enabled) return;
  
  if (g_memory_tracker.total_allocated >= size) {
    g_memory_tracker.total_allocated -= size;
  }
  g_memory_tracker.free_count++;
}

static void memory_add_allocation(AllocationHeader* header) {
  if (!header) return;
  
  header->next = g_allocation_list;
  header->prev = NULL;
  
  if (g_allocation_list) {
    g_allocation_list->prev = header;
  }
  
  g_allocation_list = header;
}

static void memory_remove_allocation(AllocationHeader* header) {
  if (!header) return;
  
  if (header->prev) {
    header->prev->next = header->next;
  } else {
    g_allocation_list = header->next;
  }
  
  if (header->next) {
    header->next->prev = header->prev;
  }
}

static void memory_check_leaks(void) {
  AllocationHeader* current = g_allocation_list;
  size_t leak_count = 0;
  size_t total_leaked = 0;
  
  printf("\n=== Memory Leak Report ===\n");
  
  while (current) {
    printf("Leak: %zu bytes at %s:%d\n", current->size, 
           current->file ? current->file : "unknown", current->line);
    total_leaked += current->size;
    leak_count++;
    current = current->next;
  }
  
  if (leak_count == 0) {
    printf("No memory leaks detected!\n");
  } else {
    printf("Total leaked: %zu bytes in %zu allocations\n", total_leaked, leak_count);
  }
  
  printf("===========================\n");
}
