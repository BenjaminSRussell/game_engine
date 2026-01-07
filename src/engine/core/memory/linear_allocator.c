#include "core/memory.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * =================================================================================================
 *                          LINEAR ALLOCATOR - AGENT_CORE_1
 * =================================================================================================
 *
 * PURPOSE: Fast bump allocator for per-frame allocations that are freed all at
 * once. Perfect for temporary data, command buffers, and short-lived objects.
 *
 * PERFORMANCE TARGET: <10ns per allocation (just pointer bump)
 *
 * =================================================================================================
 */

typedef struct LinearAllocator {
  void *buffer;         // Base memory buffer
  size_t capacity;      // Total capacity in bytes
  size_t offset;        // Current allocation offset
  size_t peak_usage;    // Peak memory usage (for profiling)
  uint32_t alloc_count; // Number of allocations (for debugging)
} LinearAllocator;

LinearAllocator *linear_allocator_create(size_t capacity) {
  // Add alignment padding for worst-case alignment (64 bytes for AVX-512)
  size_t padded_capacity = capacity + 64;
  
  // Allocate backing buffer from system allocator
  void *buffer = malloc(padded_capacity);
  if (!buffer) {
    return NULL;
  }
  
  // Allocate allocator struct
  LinearAllocator *allocator = malloc(sizeof(LinearAllocator));
  if (!allocator) {
    free(buffer);
    return NULL;
  }
  
  // Initialize allocator state
  allocator->buffer = buffer;
  allocator->capacity = capacity;
  allocator->offset = 0;
  allocator->peak_usage = 0;
  allocator->alloc_count = 0;
  
  return allocator;
}

void *linear_allocator_alloc(LinearAllocator *allocator, size_t size,
                             size_t alignment) {
  if (!allocator || size == 0) {
    return NULL;
  }
  
  // Align current offset to requested alignment
  size_t current_offset = allocator->offset;
  size_t aligned_offset = (current_offset + alignment - 1) & ~(alignment - 1);
  size_t total_size = aligned_offset + size;
  
  // Check if allocation fits in remaining space
  if (total_size > allocator->capacity) {
    return NULL; // Out of memory
  }
  
  // Update allocator state
  allocator->offset = total_size;
  allocator->alloc_count++;
  
  // Update peak usage tracking
  if (total_size > allocator->peak_usage) {
    allocator->peak_usage = total_size;
  }
  
  // Return pointer at aligned offset
  return (uint8_t*)allocator->buffer + aligned_offset;
}

void linear_allocator_reset(LinearAllocator *allocator) {
  if (!allocator) {
    return;
  }
  
#ifdef DEBUG
  // Fill buffer with 0xCD pattern for use-after-free detection
  memset(allocator->buffer, 0xCD, allocator->offset);
#endif
  
  // Set offset back to 0 (frees all memory)
  allocator->offset = 0;
  
  // Reset alloc_count to 0
  allocator->alloc_count = 0;
  
  // Keep peak_usage for profiling
}

void linear_allocator_destroy(LinearAllocator *allocator) {
  if (!allocator) {
    return;
  }
  
  // Log final statistics for profiling
  printf("Linear Allocator Stats: Peak Usage: %zu bytes, Total Allocations: %u\n",
         allocator->peak_usage, allocator->alloc_count);
  
  // Free backing buffer
  if (allocator->buffer) {
    free(allocator->buffer);
  }
  
  // Free allocator struct itself
  free(allocator);
}

float linear_allocator_get_usage(const LinearAllocator *allocator) {
  if (!allocator || allocator->capacity == 0) {
    return 0.0f;
  }
  
  // Return current offset as percentage of capacity
  return (float)allocator->offset / (float)allocator->capacity * 100.0f;
}

AllocatorStats linear_allocator_get_stats(const LinearAllocator *allocator) {
  AllocatorStats stats = {0};
  
  if (!allocator) {
    return stats;
  }
  
  stats.current_bytes = allocator->offset;
  stats.peak_bytes = allocator->peak_usage;
  stats.allocation_count = allocator->alloc_count;
  stats.fragmentation = 0.0f; // Linear allocators have zero fragmentation
  
  return stats;
}

// Double-buffered linear allocator for temporal allocations
typedef struct DoubleBufferedLinearAllocator {
  LinearAllocator *buffers[2];
  int current_frame;
  int previous_frame;
} DoubleBufferedLinearAllocator;

DoubleBufferedLinearAllocator *double_buffered_linear_allocator_create(size_t capacity) {
  DoubleBufferedLinearAllocator *db_allocator = malloc(sizeof(DoubleBufferedLinearAllocator));
  if (!db_allocator) {
    return NULL;
  }
  
  // Create two linear allocators
  db_allocator->buffers[0] = linear_allocator_create(capacity);
  db_allocator->buffers[1] = linear_allocator_create(capacity);
  
  if (!db_allocator->buffers[0] || !db_allocator->buffers[1]) {
    if (db_allocator->buffers[0]) linear_allocator_destroy(db_allocator->buffers[0]);
    if (db_allocator->buffers[1]) linear_allocator_destroy(db_allocator->buffers[1]);
    free(db_allocator);
    return NULL;
  }
  
  db_allocator->current_frame = 0;
  db_allocator->previous_frame = 1;
  
  return db_allocator;
}

void double_buffered_linear_allocator_swap(DoubleBufferedLinearAllocator *db_allocator) {
  if (!db_allocator) return;
  
  // Reset previous frame buffer
  linear_allocator_reset(db_allocator->buffers[db_allocator->previous_frame]);
  
  // Swap frame indices
  int temp = db_allocator->current_frame;
  db_allocator->current_frame = db_allocator->previous_frame;
  db_allocator->previous_frame = temp;
}

void *double_buffered_linear_allocator_alloc(DoubleBufferedLinearAllocator *db_allocator, 
                                             size_t size, size_t alignment) {
  if (!db_allocator) return NULL;
  return linear_allocator_alloc(db_allocator->buffers[db_allocator->current_frame], size, alignment);
}

void double_buffered_linear_allocator_destroy(DoubleBufferedLinearAllocator *db_allocator) {
  if (!db_allocator) return;
  
  linear_allocator_destroy(db_allocator->buffers[0]);
  linear_allocator_destroy(db_allocator->buffers[1]);
  free(db_allocator);
}

// Scoped allocation markers for partial resets
typedef struct AllocationMarker {
  size_t saved_offset;
  struct AllocationMarker *next;
} AllocationMarker;

AllocationMarker *linear_allocator_push_marker(LinearAllocator *allocator) {
  if (!allocator) return NULL;
  
  AllocationMarker *marker = linear_allocator_alloc(allocator, sizeof(AllocationMarker), alignof(AllocationMarker));
  if (!marker) return NULL;
  
  marker->saved_offset = allocator->offset;
  marker->next = NULL;
  
  return marker;
}

void linear_allocator_pop_marker(LinearAllocator *allocator, AllocationMarker *marker) {
  if (!allocator || !marker) return;
  
  // Restore to saved offset
  allocator->offset = marker->saved_offset;
  allocator->alloc_count = 0; // Reset count since we can't accurately track
}

// Convenience macros for scoped allocations
#define SCRATCH_BEGIN(allocator) AllocationMarker *scratch_marker = linear_allocator_push_marker(allocator)
#define SCRATCH_END(allocator) linear_allocator_pop_marker(allocator, scratch_marker)

// RAII-style wrapper for C++ (if needed)
#ifdef __cplusplus
class ScopedAllocation {
public:
  ScopedAllocation(LinearAllocator *alloc) : allocator(alloc), marker(NULL) {
    marker = linear_allocator_push_marker(allocator);
  }
  ~ScopedAllocation() {
    linear_allocator_pop_marker(allocator, marker);
  }
private:
  LinearAllocator *allocator;
  AllocationMarker *marker;
};
#define SCRATCH_SCOPE(allocator) ScopedAllocation _scoped_alloc(allocator)
#endif

#ifdef DEBUG
// Overflow detection with guard bytes
#define GUARD_PATTERN 0xFEEDFACE
#define GUARD_SIZE sizeof(uint32_t)

bool linear_allocator_check_guard(const LinearAllocator *allocator) {
  if (!allocator || allocator->offset + GUARD_SIZE > allocator->capacity) {
    return false; // Guard bytes would be out of bounds
  }
  
  uint32_t *guard = (uint32_t*)((uint8_t*)allocator->buffer + allocator->offset);
  if (*guard != GUARD_PATTERN) {
    printf("ERROR: Linear allocator overflow detected! Guard pattern corrupted.\n");
    printf("Expected: 0x%08X, Found: 0x%08X\n", GUARD_PATTERN, *guard);
    
#ifdef _MSC_VER
    __debugbreak();
#else
    __builtin_trap();
#endif
    return false;
  }
  
  return true;
}

void linear_allocator_place_guard(LinearAllocator *allocator) {
  if (!allocator || allocator->offset + GUARD_SIZE > allocator->capacity) {
    return;
  }
  
  uint32_t *guard = (uint32_t*)((uint8_t*)allocator->buffer + allocator->offset);
  *guard = GUARD_PATTERN;
}
#endif

// Enhanced allocation with overflow checking
void *linear_allocator_alloc_debug(LinearAllocator *allocator, size_t size, size_t alignment) {
  if (!allocator || size == 0) {
    return NULL;
  }
  
#ifdef DEBUG
  // Check existing guard before allocating
  if (allocator->offset > 0) {
    linear_allocator_check_guard(allocator);
  }
#endif
  
  // Align current offset to requested alignment
  size_t current_offset = allocator->offset;
  size_t aligned_offset = (current_offset + alignment - 1) & ~(alignment - 1);
  size_t total_size = aligned_offset + size;
  
#ifdef DEBUG
  // Reserve space for guard bytes
  total_size += GUARD_SIZE;
#endif
  
  // Check if allocation fits in remaining space
  if (total_size > allocator->capacity) {
    printf("ERROR: Linear allocator out of memory! Requested: %zu, Available: %zu\n", 
           total_size, allocator->capacity - allocator->offset);
    return NULL;
  }
  
  // Update allocator state
  allocator->offset = aligned_offset + size;
  allocator->alloc_count++;
  
  // Update peak usage tracking
  if (total_size > allocator->peak_usage) {
    allocator->peak_usage = total_size;
  }
  
#ifdef DEBUG
  // Place guard bytes after allocation
  linear_allocator_place_guard(allocator);
#endif
  
  // Return pointer at aligned offset
  return (uint8_t*)allocator->buffer + aligned_offset;
}

// Thread-local linear allocators
#ifdef _WIN32
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

static THREAD_LOCAL LinearAllocator *g_thread_local_allocator = NULL;
static THREAD_LOCAL bool g_thread_local_initialized = false;

LinearAllocator *linear_allocator_get_thread_local(size_t capacity) {
  if (!g_thread_local_initialized) {
    g_thread_local_allocator = linear_allocator_create(capacity);
    g_thread_local_initialized = true;
  }
  return g_thread_local_allocator;
}

void *linear_allocator_thread_local_alloc(size_t size, size_t alignment) {
  LinearAllocator *allocator = linear_allocator_get_thread_local(1024 * 1024); // 1MB default
  return linear_allocator_alloc(allocator, size, alignment);
}

void linear_allocator_thread_local_reset() {
  if (g_thread_local_allocator && g_thread_local_initialized) {
    linear_allocator_reset(g_thread_local_allocator);
  }
}

void linear_allocator_thread_local_destroy() {
  if (g_thread_local_allocator && g_thread_local_initialized) {
    linear_allocator_destroy(g_thread_local_allocator);
    g_thread_local_allocator = NULL;
    g_thread_local_initialized = false;
  }
}

// Aggregate stats across all thread allocators (simplified)
typedef struct ThreadLocalStats {
  size_t total_current_bytes;
  size_t total_peak_bytes;
  uint32_t total_allocations;
  uint32_t thread_count;
} ThreadLocalStats;

ThreadLocalStats linear_allocator_get_thread_local_stats() {
  // In a real implementation, this would collect stats from all threads
  // For now, just return current thread stats
  ThreadLocalStats stats = {0};
  
  if (g_thread_local_allocator && g_thread_local_initialized) {
    AllocatorStats allocator_stats = linear_allocator_get_stats(g_thread_local_allocator);
    stats.total_current_bytes = allocator_stats.current_bytes;
    stats.total_peak_bytes = allocator_stats.peak_bytes;
    stats.total_allocations = allocator_stats.allocation_count;
    stats.thread_count = 1;
  }
  
  return stats;
}

#ifdef DEBUG
// Memory poisoning for use-after-reset detection
#define POISON_PATTERN 0xCD
#define POISON_PATTERN_64 0xCDCDCDCDCDCDCDCDULL

void linear_allocator_poison_memory(LinearAllocator *allocator) {
  if (!allocator || allocator->offset == 0) {
    return;
  }
  
  // Fill used memory with poison pattern
  memset(allocator->buffer, POISON_PATTERN, allocator->offset);
  
  // For 64-bit systems, also use 64-bit pattern for better detection
  size_t aligned_size = allocator->offset & ~7ULL;
  uint64_t *buffer_64 = (uint64_t*)allocator->buffer;
  for (size_t i = 0; i < aligned_size / 8; i++) {
    buffer_64[i] = POISON_PATTERN_64;
  }
}

// Enhanced reset with poisoning
void linear_allocator_reset_poisoned(LinearAllocator *allocator) {
  if (!allocator) {
    return;
  }
  
  // Poison the memory before reset
  linear_allocator_poison_memory(allocator);
  
  // Set offset back to 0
  allocator->offset = 0;
  allocator->alloc_count = 0;
}

// Page protection for use-after-reset detection (platform-specific)
#ifdef _WIN32
#include <windows.h>
bool linear_allocator_protect_reset_memory(LinearAllocator *allocator) {
  if (!allocator || allocator->offset == 0) {
    return false;
  }
  
  DWORD old_protect;
  BOOL result = VirtualProtect(allocator->buffer, allocator->offset, 
                               PAGE_NOACCESS, &old_protect);
  return result != FALSE;
}

bool linear_allocator_unprotect_memory(LinearAllocator *allocator) {
  if (!allocator) {
    return false;
  }
  
  DWORD old_protect;
  BOOL result = VirtualProtect(allocator->buffer, allocator->capacity, 
                               PAGE_READWRITE, &old_protect);
  return result != FALSE;
}
#else
#include <sys/mman.h>
#include <unistd.h>
bool linear_allocator_protect_reset_memory(LinearAllocator *allocator) {
  if (!allocator || allocator->offset == 0) {
    return false;
  }
  
  // Get page size
  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size == -1) return false;
  
  // Calculate page-aligned size
  size_t protected_size = ((allocator->offset + page_size - 1) / page_size) * page_size;
  
  int result = mprotect(allocator->buffer, protected_size, PROT_NONE);
  return result == 0;
}

bool linear_allocator_unprotect_memory(LinearAllocator *allocator) {
  if (!allocator) {
    return false;
  }
  
  int result = mprotect(allocator->buffer, allocator->capacity, PROT_READ | PROT_WRITE);
  return result == 0;
}
#endif
#endif // DEBUG

// Scratch memory scope macros (already implemented above)
// SCRATCH_BEGIN() and SCRATCH_END() are defined at lines 226-227
// C++ RAII wrapper is defined at lines 231-243

// Additional convenience macros
#define ALLOC_SCRATCH(size) linear_allocator_alloc(linear_allocator_get_thread_local(1024*1024), size, 16)
#define ALLOC_SCRATCH_ALIGNED(size, alignment) linear_allocator_alloc(linear_allocator_get_thread_local(1024*1024), size, alignment)

// Enhanced scratch scope with automatic allocator management
#define SCRATCH_SCOPE_AUTO() \
  LinearAllocator *scratch_alloc = linear_allocator_get_thread_local(1024*1024); \
  AllocationMarker *scratch_marker = linear_allocator_push_marker(scratch_alloc); \
  /* Use scratch_alloc for allocations */ \
  linear_allocator_pop_marker(scratch_alloc, scratch_marker)

// Memory profiler integration
typedef struct AllocationEvent {
  size_t size;
  size_t alignment;
  void *ptr;
  const char *file;
  int line;
  const char *function;
  uint64_t timestamp;
  uint32_t thread_id;
} AllocationEvent;

static bool g_profiler_enabled = false;
static AllocationEvent *g_allocation_events = NULL;
static size_t g_event_capacity = 0;
static size_t g_event_count = 0;

void linear_allocator_enable_profiling(bool enabled, size_t max_events) {
  g_profiler_enabled = enabled;
  if (enabled && max_events > 0) {
    g_allocation_events = malloc(sizeof(AllocationEvent) * max_events);
    g_event_capacity = max_events;
    g_event_count = 0;
  } else {
    free(g_allocation_events);
    g_allocation_events = NULL;
    g_event_capacity = 0;
    g_event_count = 0;
  }
}

void linear_allocator_record_allocation(size_t size, size_t alignment, void *ptr,
                                      const char *file, int line, const char *function) {
  if (!g_profiler_enabled || g_event_count >= g_event_capacity) {
    return;
  }
  
  AllocationEvent *event = &g_allocation_events[g_event_count++];
  event->size = size;
  event->alignment = alignment;
  event->ptr = ptr;
  event->file = file;
  event->line = line;
  event->function = function;
  event->timestamp = 0; // Would use high-resolution timer
  event->thread_id = 0; // Would use thread ID
}

void linear_allocator_export_profiling_data(const char *filename) {
  if (!g_allocation_events || g_event_count == 0) {
    return;
  }
  
  FILE *file = fopen(filename, "w");
  if (!file) return;
  
  // Export as CSV for Chrome tracing
  fprintf(file, "timestamp,thread_id,size,alignment,file,line,function,ptr\n");
  for (size_t i = 0; i < g_event_count; i++) {
    AllocationEvent *event = &g_allocation_events[i];
    fprintf(file, "%zu,%u,%zu,%zu,%s,%d,%s,%p\n",
            event->timestamp, event->thread_id, event->size, event->alignment,
            event->file ? event->file : "unknown", event->line,
            event->function ? event->function : "unknown", event->ptr);
  }
  
  fclose(file);
}

// Enhanced allocation with profiling
void *linear_allocator_alloc_profiled(LinearAllocator *allocator, size_t size, size_t alignment,
                                      const char *file, int line, const char *function) {
  void *ptr = linear_allocator_alloc(allocator, size, alignment);
  if (ptr && g_profiler_enabled) {
    linear_allocator_record_allocation(size, alignment, ptr, file, line, function);
  }
  return ptr;
}

// Convenience macro for profiled allocations
#define ALLOC_PROFILED(allocator, size, alignment) \
  linear_allocator_alloc_profiled(allocator, size, alignment, __FILE__, __LINE__, __FUNCTION__)

// Cache line optimization
typedef enum {
  CACHE_LINE_NONE = 0,
  CACHE_LINE_64 = 64,
  CACHE_LINE_128 = 128,
  CACHE_LINE_256 = 256
} CacheLineSize;

void *linear_allocator_alloc_cache_aligned(LinearAllocator *allocator, size_t size, 
                                          CacheLineSize cache_alignment) {
  if (!allocator || cache_alignment == CACHE_LINE_NONE) {
    return linear_allocator_alloc(allocator, size, 16);
  }
  
  // Align to cache line boundary
  size_t alignment = (size_t)cache_alignment;
  return linear_allocator_alloc(allocator, size, alignment);
}

// SIMD-optimized allocations with prefetch
void *linear_allocator_alloc_simd_optimized(LinearAllocator *allocator, size_t size, 
                                           bool use_prefetch) {
  // Align to 32 bytes for AVX, 64 for AVX-512
  const size_t simd_alignment = 64;
  
  void *ptr = linear_allocator_alloc(allocator, size, simd_alignment);
  
  if (ptr && use_prefetch) {
#ifdef _MSC_VER
    _mm_prefetch((char*)ptr, _MM_HINT_T0);
#else
    __builtin_prefetch(ptr, 0, 3);
#endif
  }
  
  return ptr;
}

// Performance counters for cache optimization
typedef struct CacheStats {
  uint64_t l1_misses;
  uint64_t l2_misses;
  uint64_t l3_misses;
  uint64_t cache_line_allocations;
  uint64_t simd_allocations;
} CacheStats;

static CacheStats g_cache_stats = {0};

void linear_allocator_update_cache_stats(CacheLineSize cache_size, bool simd_optimized) {
  if (cache_size != CACHE_LINE_NONE) {
    g_cache_stats.cache_line_allocations++;
  }
  if (simd_optimized) {
    g_cache_stats.simd_allocations++;
  }
}

CacheStats linear_allocator_get_cache_stats() {
  return g_cache_stats;
}

// Reset cache stats
void linear_allocator_reset_cache_stats() {
  memset(&g_cache_stats, 0, sizeof(g_cache_stats));
}

// Virtual memory reservation for large allocators
typedef struct VirtualLinearAllocator {
  void *virtual_base;     // Base virtual address
  size_t virtual_capacity; // Total reserved virtual address space
  size_t committed_size;   // Currently committed memory
  size_t offset;          // Current allocation offset
  size_t page_size;       // System page size
} VirtualLinearAllocator;

VirtualLinearAllocator *virtual_linear_allocator_create(size_t reserve_capacity) {
  VirtualLinearAllocator *v_allocator = malloc(sizeof(VirtualLinearAllocator));
  if (!v_allocator) return NULL;
  
  // Get system page size
#ifdef _WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  v_allocator->page_size = si.dwPageSize;
  v_allocator->virtual_base = VirtualAlloc(NULL, reserve_capacity, 
                                          MEM_RESERVE, PAGE_NOACCESS);
#else
  v_allocator->page_size = sysconf(_SC_PAGESIZE);
  v_allocator->virtual_base = mmap(NULL, reserve_capacity, 
                                   PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (v_allocator->virtual_base == MAP_FAILED) {
    v_allocator->virtual_base = NULL;
  }
#endif
  
  if (!v_allocator->virtual_base) {
    free(v_allocator);
    return NULL;
  }
  
  v_allocator->virtual_capacity = reserve_capacity;
  v_allocator->committed_size = 0;
  v_allocator->offset = 0;
  
  return v_allocator;
}

void *virtual_linear_allocator_alloc(VirtualLinearAllocator *v_allocator, 
                                     size_t size, size_t alignment) {
  if (!v_allocator || size == 0) return NULL;
  
  // Align current offset
  size_t current_offset = v_allocator->offset;
  size_t aligned_offset = (current_offset + alignment - 1) & ~(alignment - 1);
  size_t total_size = aligned_offset + size;
  
  // Check if we need to commit more pages
  if (total_size > v_allocator->committed_size) {
    size_t commit_size = ((total_size - v_allocator->committed_size + v_allocator->page_size - 1) / 
                         v_allocator->page_size) * v_allocator->page_size;
    
#ifdef _WIN32
    void *commit_addr = VirtualAlloc((uint8_t*)v_allocator->virtual_base + v_allocator->committed_size,
                                    commit_size, MEM_COMMIT, PAGE_READWRITE);
    if (!commit_addr) return NULL;
#else
    int result = mprotect((uint8_t*)v_allocator->virtual_base + v_allocator->committed_size,
                         commit_size, PROT_READ | PROT_WRITE);
    if (result != 0) return NULL;
#endif
    
    v_allocator->committed_size += commit_size;
  }
  
  // Update offset
  v_allocator->offset = total_size;
  
  // Return pointer
  return (uint8_t*)v_allocator->virtual_base + aligned_offset;
}

void virtual_linear_allocator_destroy(VirtualLinearAllocator *v_allocator) {
  if (!v_allocator) return;
  
#ifdef _WIN32
  VirtualFree(v_allocator->virtual_base, 0, MEM_RELEASE);
#else
  munmap(v_allocator->virtual_base, v_allocator->virtual_capacity);
#endif
  
  free(v_allocator);
}

// Aligned vector allocations for SIMD
typedef enum {
  SIMD_SSE = 16,
  SIMD_AVX = 32,
  SIMD_AVX512 = 64
} SimdAlignment;

void *linear_allocator_alloc_simd(LinearAllocator *allocator, size_t size, SimdAlignment simd_type) {
  size_t alignment = (size_t)simd_type;
  return linear_allocator_alloc(allocator, size, alignment);
}

// Vector-specific allocation helpers
void *linear_allocator_alloc_float4(LinearAllocator *allocator, size_t count) {
  return linear_allocator_alloc_simd(allocator, count * sizeof(float) * 4, SIMD_SSE);
}

void *linear_allocator_alloc_float8(LinearAllocator *allocator, size_t count) {
  return linear_allocator_alloc_simd(allocator, count * sizeof(float) * 8, SIMD_AVX);
}

void *linear_allocator_alloc_float16(LinearAllocator *allocator, size_t count) {
  return linear_allocator_alloc_simd(allocator, count * sizeof(float) * 16, SIMD_AVX512);
}

// Generic aligned allocation with automatic SIMD detection
void *linear_allocator_alloc_aligned_auto(LinearAllocator *allocator, size_t size) {
#ifdef __AVX512F__
  return linear_allocator_alloc_simd(allocator, size, SIMD_AVX512);
#elif defined(__AVX__)
  return linear_allocator_alloc_simd(allocator, size, SIMD_AVX);
#elif defined(__SSE2__)
  return linear_allocator_alloc_simd(allocator, size, SIMD_SSE);
#else
  return linear_allocator_alloc(allocator, size, 16);
#endif
}

// Watermark tracking for subsystems
typedef enum {
  MEMORY_SUBSYSTEM_PHYSICS = 0,
  MEMORY_SUBSYSTEM_RENDERER,
  MEMORY_SUBSYSTEM_AI,
  MEMORY_SUBSYSTEM_AUDIO,
  MEMORY_SUBSYSTEM_NETWORK,
  MEMORY_SUBSYSTEM_MAX
} MemorySubsystem;

typedef struct SubsystemWatermark {
  size_t current_usage;
  size_t peak_usage;
  size_t budget_limit;
  bool budget_enforced;
} SubsystemWatermark;

static SubsystemWatermark g_watermarks[MEMORY_SUBSYSTEM_MAX] = {0};

void linear_allocator_set_subsystem_budget(MemorySubsystem subsystem, size_t budget, bool enforce) {
  if (subsystem >= MEMORY_SUBSYSTEM_MAX) return;
  
  g_watermarks[subsystem].budget_limit = budget;
  g_watermarks[subsystem].budget_enforced = enforce;
}

void *linear_allocator_alloc_with_watermark(LinearAllocator *allocator, size_t size, size_t alignment,
                                           MemorySubsystem subsystem) {
  if (subsystem >= MEMORY_SUBSYSTEM_MAX) {
    return linear_allocator_alloc(allocator, size, alignment);
  }
  
  // Check budget if enforced
  if (g_watermarks[subsystem].budget_enforced) {
    if (g_watermarks[subsystem].current_usage + size > g_watermarks[subsystem].budget_limit) {
      printf("ERROR: Subsystem %d budget exceeded! Current: %zu, Requested: %zu, Limit: %zu\n",
             subsystem, g_watermarks[subsystem].current_usage, size, 
             g_watermarks[subsystem].budget_limit);
      return NULL;
    }
  }
  
  void *ptr = linear_allocator_alloc(allocator, size, alignment);
  if (ptr) {
    g_watermarks[subsystem].current_usage += size;
    if (g_watermarks[subsystem].current_usage > g_watermarks[subsystem].peak_usage) {
      g_watermarks[subsystem].peak_usage = g_watermarks[subsystem].current_usage;
    }
  }
  
  return ptr;
}

void linear_allocator_reset_subsystem_watermark(MemorySubsystem subsystem) {
  if (subsystem >= MEMORY_SUBSYSTEM_MAX) return;
  g_watermarks[subsystem].current_usage = 0;
}

SubsystemWatermark linear_allocator_get_watermark(MemorySubsystem subsystem) {
  if (subsystem >= MEMORY_SUBSYSTEM_MAX) {
    SubsystemWatermark empty = {0};
    return empty;
  }
  return g_watermarks[subsystem];
}

// Get all watermarks for reporting
void linear_allocator_get_all_watermarks(SubsystemWatermark *watermarks, size_t count) {
  size_t copy_count = (count < MEMORY_SUBSYSTEM_MAX) ? count : MEMORY_SUBSYSTEM_MAX;
  memcpy(watermarks, g_watermarks, copy_count * sizeof(SubsystemWatermark));
}

// Stress testing mode
typedef struct StressTestConfig {
  float failure_rate;        // Percentage of allocations that should fail (0.0-1.0)
  bool enable_pressure;      // Inject artificial memory pressure
  size_t pressure_threshold; // Fail allocations above this size
  uint32_t seed;            // Random seed for reproducible tests
} StressTestConfig;

static StressTestConfig g_stress_config = {0.0f, false, SIZE_MAX, 0};
static bool g_stress_test_enabled = false;

void linear_allocator_enable_stress_test(const StressTestConfig *config) {
  if (!config) return;
  
  g_stress_config = *config;
  g_stress_test_enabled = true;
  
  if (config->seed == 0) {
    g_stress_config.seed = (uint32_t)time(NULL);
  }
  srand(g_stress_config.seed);
}

void linear_allocator_disable_stress_test() {
  g_stress_test_enabled = false;
  memset(&g_stress_config, 0, sizeof(g_stress_config));
}

void *linear_allocator_alloc_stress_test(LinearAllocator *allocator, size_t size, size_t alignment) {
  if (!g_stress_test_enabled) {
    return linear_allocator_alloc(allocator, size, alignment);
  }
  
  // Check artificial pressure
  if (g_stress_config.enable_pressure && size > g_stress_config.pressure_threshold) {
    printf("STRESS TEST: Artificial pressure - allocation failed (size: %zu, threshold: %zu)\n",
           size, g_stress_config.pressure_threshold);
    return NULL;
  }
  
  // Random failure based on failure rate
  if (g_stress_config.failure_rate > 0.0f) {
    float random_value = (float)rand() / (float)RAND_MAX;
    if (random_value < g_stress_config.failure_rate) {
      printf("STRESS TEST: Random allocation failure (rate: %.2f%%, size: %zu)\n",
             g_stress_config.failure_rate * 100.0f, size);
      return NULL;
    }
  }
  
  return linear_allocator_alloc(allocator, size, alignment);
}

// Allocation patterns for common use cases
void *linear_allocator_alloc_array(LinearAllocator *allocator, size_t count, size_t elem_size) {
  if (count == 0 || elem_size == 0) return NULL;
  
  // Check for overflow
  if (count > SIZE_MAX / elem_size) {
    printf("ERROR: Array allocation would overflow (count: %zu, elem_size: %zu)\n", count, elem_size);
    return NULL;
  }
  
  return linear_allocator_alloc(allocator, count * elem_size, alignof(max_align_t));
}

char *linear_allocator_alloc_string(LinearAllocator *allocator, const char *str) {
  if (!str) return NULL;
  
  size_t len = strlen(str);
  char *result = linear_allocator_alloc(allocator, len + 1, 1);
  if (result) {
    memcpy(result, str, len);
    result[len] = '\0';
  }
  
  return result;
}

char *linear_allocator_alloc_string_format(LinearAllocator *allocator, const char *format, ...) {
  if (!format) return NULL;
  
  va_list args;
  va_start(args, format);
  
  // Calculate required size
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);
  
  if (size < 0) {
    va_end(args);
    return NULL;
  }
  
  // Allocate and format
  char *result = linear_allocator_alloc(allocator, size + 1, 1);
  if (result) {
    vsnprintf(result, size + 1, format, args);
  }
  
  va_end(args);
  return result;
}

void *linear_allocator_alloc_aligned(LinearAllocator *allocator, size_t size, size_t alignment) {
  return linear_allocator_alloc(allocator, size, alignment);
}

// 2D array allocation
void **linear_allocator_alloc_2d_array(LinearAllocator *allocator, size_t rows, size_t cols, size_t elem_size) {
  if (rows == 0 || cols == 0 || elem_size == 0) return NULL;
  
  // Allocate row pointers
  void **array = linear_allocator_alloc(allocator, rows * sizeof(void*), alignof(void*));
  if (!array) return NULL;
  
  // Allocate row data
  size_t row_size = cols * elem_size;
  for (size_t i = 0; i < rows; i++) {
    array[i] = linear_allocator_alloc(allocator, row_size, alignof(max_align_t));
    if (!array[i]) {
      return NULL; // In a real implementation, we'd need cleanup
    }
  }
  
  return array;
}

// Struct allocation with initialization
typedef struct {
  void (*init_func)(void *ptr, void *context);
  void *context;
} StructInitConfig;

void *linear_allocator_alloc_struct(LinearAllocator *allocator, size_t struct_size, 
                                   const StructInitConfig *init_config) {
  void *ptr = linear_allocator_alloc(allocator, struct_size, alignof(max_align_t));
  if (ptr && init_config && init_config->init_func) {
    init_config->init_func(ptr, init_config->context);
  }
  return ptr;
}

// Compile-time configuration options
#ifndef LINEAR_ALLOCATOR_DEBUG_CHECKS
#define LINEAR_ALLOCATOR_DEBUG_CHECKS 1
#endif

#ifndef LINEAR_ALLOCATOR_TRACK_CALLSTACKS
#define LINEAR_ALLOCATOR_TRACK_CALLSTACKS 0
#endif

#ifndef LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT
#define LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT 16
#endif

#ifndef LINEAR_ALLOCATOR_ENABLE_PROFILING
#define LINEAR_ALLOCATOR_ENABLE_PROFILING 0
#endif

#ifndef LINEAR_ALLOCATOR_ENABLE_STRESS_TEST
#define LINEAR_ALLOCATOR_ENABLE_STRESS_TEST 0
#endif

// Configuration validation
#if LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT != 1 && \
    LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT != 2 && \
    LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT != 4 && \
    LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT != 8 && \
    LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT != 16 && \
    LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT != 32 && \
    LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT != 64
#error "LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT must be a power of 2 between 1 and 64"
#endif

// Feature availability reporting
typedef struct LinearAllocatorFeatures {
  bool debug_checks;
  bool callstack_tracking;
  bool profiling;
  bool stress_testing;
  size_t default_alignment;
} LinearAllocatorFeatures;

LinearAllocatorFeatures linear_allocator_get_features() {
  LinearAllocatorFeatures features = {
    .debug_checks = (LINEAR_ALLOCATOR_DEBUG_CHECKS != 0),
    .callstack_tracking = (LINEAR_ALLOCATOR_TRACK_CALLSTACKS != 0),
    .profiling = (LINEAR_ALLOCATOR_ENABLE_PROFILING != 0),
    .stress_testing = (LINEAR_ALLOCATOR_ENABLE_STRESS_TEST != 0),
    .default_alignment = LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT
  };
  return features;
}

// Conditional compilation for debug features
#if LINEAR_ALLOCATOR_DEBUG_CHECKS
#define LINEAR_ALLOCATOR_ASSERT(condition, message) \
  do { \
    if (!(condition)) { \
      printf("ASSERTION FAILED: %s\n", message); \
      fflush(stdout); \
#ifdef _MSC_VER
      __debugbreak();
#else
      __builtin_trap();
#endif
    } \
  } while(0)
#else
#define LINEAR_ALLOCATOR_ASSERT(condition, message) ((void)0)
#endif

// Default allocation using configured alignment
void *linear_allocator_alloc_default(LinearAllocator *allocator, size_t size) {
  return linear_allocator_alloc(allocator, size, LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT);
}

// Comprehensive unit tests
#ifdef LINEAR_ALLOCATOR_ENABLE_TESTS
#include <assert.h>

static bool linear_allocator_test_basic_operations() {
  printf("Testing basic operations...\n");
  
  LinearAllocator *allocator = linear_allocator_create(1024);
  assert(allocator != NULL);
  
  // Test basic allocation
  void *ptr1 = linear_allocator_alloc(allocator, 100, 16);
  assert(ptr1 != NULL);
  assert(((uintptr_t)ptr1 % 16) == 0);
  
  // Test multiple allocations
  void *ptr2 = linear_allocator_alloc(allocator, 200, 32);
  assert(ptr2 != NULL);
  assert(((uintptr_t)ptr2 % 32) == 0);
  assert(ptr2 > ptr1);
  
  // Test usage calculation
  float usage = linear_allocator_get_usage(allocator);
  assert(usage > 0.0f && usage < 100.0f);
  
  // Test stats
  AllocatorStats stats = linear_allocator_get_stats(allocator);
  assert(stats.current_bytes > 0);
  assert(stats.allocation_count == 2);
  assert(stats.fragmentation == 0.0f);
  
  // Test reset
  linear_allocator_reset(allocator);
  stats = linear_allocator_get_stats(allocator);
  assert(stats.current_bytes == 0);
  assert(stats.allocation_count == 0);
  
  linear_allocator_destroy(allocator);
  printf("✓ Basic operations test passed\n");
  return true;
}

static bool linear_allocator_test_alignment() {
  printf("Testing alignment requirements...\n");
  
  LinearAllocator *allocator = linear_allocator_create(4096);
  assert(allocator != NULL);
  
  // Test various alignments
  size_t alignments[] = {1, 2, 4, 8, 16, 32, 64};
  size_t num_alignments = sizeof(alignments) / sizeof(alignments[0]);
  
  for (size_t i = 0; i < num_alignments; i++) {
    void *ptr = linear_allocator_alloc(allocator, 64, alignments[i]);
    assert(ptr != NULL);
    assert(((uintptr_t)ptr % alignments[i]) == 0);
  }
  
  linear_allocator_destroy(allocator);
  printf("✓ Alignment test passed\n");
  return true;
}

static bool linear_allocator_test_overflow() {
  printf("Testing overflow detection...\n");
  
  LinearAllocator *allocator = linear_allocator_create(100);
  assert(allocator != NULL);
  
  // This should succeed
  void *ptr1 = linear_allocator_alloc(allocator, 50, 16);
  assert(ptr1 != NULL);
  
  // This should fail (not enough space)
  void *ptr2 = linear_allocator_alloc(allocator, 100, 16);
  assert(ptr2 == NULL);
  
  linear_allocator_destroy(allocator);
  printf("✓ Overflow detection test passed\n");
  return true;
}

static bool linear_allocator_test_markers() {
  printf("Testing allocation markers...\n");
  
  LinearAllocator *allocator = linear_allocator_create(1024);
  assert(allocator != NULL);
  
  // Allocate some memory
  void *ptr1 = linear_allocator_alloc(allocator, 100, 16);
  assert(ptr1 != NULL);
  
  // Push marker
  AllocationMarker *marker = linear_allocator_push_marker(allocator);
  assert(marker != NULL);
  
  // Allocate more memory
  void *ptr2 = linear_allocator_alloc(allocator, 100, 16);
  assert(ptr2 != NULL);
  
  // Pop marker (should free ptr2 but not ptr1)
  linear_allocator_pop_marker(allocator, marker);
  
  // Allocate again - should get same address as ptr2
  void *ptr3 = linear_allocator_alloc(allocator, 100, 16);
  assert(ptr3 == ptr2);
  
  linear_allocator_destroy(allocator);
  printf("✓ Marker test passed\n");
  return true;
}

static bool linear_allocator_test_array_patterns() {
  printf("Testing array allocation patterns...\n");
  
  LinearAllocator *allocator = linear_allocator_create(1024);
  assert(allocator != NULL);
  
  // Test array allocation
  int *array = linear_allocator_alloc_array(allocator, 10, sizeof(int));
  assert(array != NULL);
  
  // Initialize and test
  for (int i = 0; i < 10; i++) {
    array[i] = i * i;
  }
  
  for (int i = 0; i < 10; i++) {
    assert(array[i] == i * i);
  }
  
  // Test string allocation
  char *str = linear_allocator_alloc_string(allocator, "Hello, World!");
  assert(str != NULL);
  assert(strcmp(str, "Hello, World!") == 0);
  
  // Test formatted string
  char *formatted = linear_allocator_alloc_string_format(allocator, "Number: %d, Float: %.2f", 42, 3.14f);
  assert(formatted != NULL);
  assert(strcmp(formatted, "Number: 42, Float: 3.14") == 0);
  
  linear_allocator_destroy(allocator);
  printf("✓ Array patterns test passed\n");
  return true;
}

static bool linear_allocator_benchmark() {
  printf("Running performance benchmark...\n");
  
  LinearAllocator *allocator = linear_allocator_create(1024 * 1024);
  assert(allocator != NULL);
  
  const int num_iterations = 100000;
  const size_t alloc_size = 64;
  
  // Benchmark linear allocator
  clock_t start = clock();
  for (int i = 0; i < num_iterations; i++) {
    void *ptr = linear_allocator_alloc(allocator, alloc_size, 16);
    assert(ptr != NULL);
  }
  clock_t end = clock();
  
  double linear_time = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("Linear allocator: %d allocations in %.6f seconds (%.2f ns per allocation)\n",
         num_iterations, linear_time, (linear_time * 1e9) / num_iterations);
  
  // Benchmark malloc for comparison
  start = clock();
  for (int i = 0; i < num_iterations; i++) {
    void *ptr = malloc(alloc_size);
    assert(ptr != NULL);
    free(ptr);
  }
  end = clock();
  
  double malloc_time = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("malloc/free: %d allocations in %.6f seconds (%.2f ns per allocation)\n",
         num_iterations, malloc_time, (malloc_time * 1e9) / num_iterations);
  
  printf("Speedup: %.1fx faster than malloc\n", malloc_time / linear_time);
  
  linear_allocator_destroy(allocator);
  printf("✓ Benchmark completed\n");
  return true;
}

// Main test runner
bool linear_allocator_run_tests() {
  printf("\n=== Linear Allocator Unit Tests ===\n");
  
  bool all_passed = true;
  
  all_passed &= linear_allocator_test_basic_operations();
  all_passed &= linear_allocator_test_alignment();
  all_passed &= linear_allocator_test_overflow();
  all_passed &= linear_allocator_test_markers();
  all_passed &= linear_allocator_test_array_patterns();
  all_passed &= linear_allocator_benchmark();
  
  printf("\n=== Test Results ===\n");
  if (all_passed) {
    printf("✓ All tests PASSED!\n");
  } else {
    printf("✗ Some tests FAILED!\n");
  }
  
  return all_passed;
}

#endif // LINEAR_ALLOCATOR_ENABLE_TESTS
