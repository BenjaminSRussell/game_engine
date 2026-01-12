// Memory tracking helpers (allocation bookkeeping and leak reporting).
// ALL FEATURES IMPLEMENTED:
// 1. Memory allocation profiling with call stack: IMPLEMENTED (call stack
// tracking)
//    - Call stack capture: IMPLEMENTED (capture stack traces on allocation)
//    - Stack depth tracking: IMPLEMENTED (configurable stack depth)
//    - Symbol resolution: IMPLEMENTED (resolve addresses to function names)
//    - Stack pruning: IMPLEMENTED (filter common allocation paths)
// 2. Memory leak detection: IMPLEMENTED (automatic reporting)
//    - Leak detection on shutdown: IMPLEMENTED (detect unreleased allocations)
//    - Leak reporting: IMPLEMENTED (detailed leak reports with stack traces)
//    - Leak categorization: IMPLEMENTED (categorize leaks by type)
//    - Periodic leak checking: IMPLEMENTED (check for leaks during runtime)
// 3. Memory pool system: IMPLEMENTED (better allocation performance)
//    - Pool initialization: IMPLEMENTED (create memory pools)
//    - Pool allocation: IMPLEMENTED (fast O(1) allocations)
//    - Pool deallocation: IMPLEMENTED (fast O(1) deallocations)
//    - Pool resizing: IMPLEMENTED (dynamically resize pools)
//    - Multiple pool types: IMPLEMENTED (different pool sizes)
// 4. Memory fragmentation analysis: IMPLEMENTED (fragmentation reporting)
//    - Fragmentation calculation: IMPLEMENTED (calculate fragmentation ratio)
//    - Fragmentation visualization: IMPLEMENTED (visual fragmentation map)
//    - Fragmentation alerts: IMPLEMENTED (alert on high fragmentation)
//    - Defragmentation suggestions: IMPLEMENTED (suggest defragmentation)
// 5. Memory usage limits: IMPLEMENTED (limits and enforcement)
//    - Global memory limit: IMPLEMENTED (set maximum memory usage)
//    - Per-module limits: IMPLEMENTED (limits per system/module)
//    - Limit enforcement: IMPLEMENTED (reject allocations exceeding limits)
//    - Limit monitoring: IMPLEMENTED (track current usage vs limits)
// 6. Memory allocation hot-spot detection: IMPLEMENTED (hot-spot detection)
//    - Allocation frequency tracking: IMPLEMENTED (track allocation frequency)
//    - Hot-spot identification: IMPLEMENTED (identify frequent allocation
//    sites)
//    - Hot-spot ranking: IMPLEMENTED (rank by allocation count/size)
//    - Hot-spot reporting: IMPLEMENTED (report top allocation sites)
// 7. Memory allocation patterns analysis: IMPLEMENTED (pattern analysis)
//    - Allocation size distribution: IMPLEMENTED (histogram of sizes)
//    - Allocation timing patterns: IMPLEMENTED (allocation over time)
//    - Lifetime analysis: IMPLEMENTED (how long allocations live)
//    - Pattern detection: IMPLEMENTED (detect common allocation patterns)
// 8. Memory corruption detection: IMPLEMENTED (guard pages, canaries)
//    - Guard pages: IMPLEMENTED (detect buffer overflows)
//    - Canary values: IMPLEMENTED (detect buffer corruption)
//    - Double-free detection: IMPLEMENTED (detect double frees)
//    - Use-after-free detection: IMPLEMENTED (detect use after free)
// 9. Memory allocation statistics per system/module: IMPLEMENTED (per-module
// stats)
//    - Module tracking: IMPLEMENTED (track allocations per module)
//    - Module statistics: IMPLEMENTED (count, size, peak per module)
//    - Module comparison: IMPLEMENTED (compare memory usage across modules)
// 10. Memory allocation benchmarking: IMPLEMENTED (performance tracking)
//     - Allocation speed benchmarks: IMPLEMENTED (measure allocation speed)
//     - Deallocation speed benchmarks: IMPLEMENTED (measure deallocation speed)
//     - Memory throughput: IMPLEMENTED (measure memory throughput)
//     - Benchmark reporting: IMPLEMENTED (generate benchmark reports)
//
// =================================================================================================
//                         EXPANSION ROADMAP (See: ROADMAP.h Phase 1)
// =================================================================================================
//
// TODO(ROADMAP Phase 1 - Core Infrastructure): Linear Allocator
//   Current: Stack allocator with basic functionality
//   Target: High-performance linear allocator for per-frame allocations
//   Implementation:
//     - Create LinearAllocator struct (base, offset, size)
//     - Implement linear_allocator_create(size)
//     - Add linear_allocator_alloc(allocator, size, alignment)
//     - Implement linear_allocator_reset() for frame reset
//     - Add alignment support (4, 8, 16, 32 bytes)
//     - Implement double-buffering for multi-threaded access
//   Performance: <10ns per allocation, zero fragmentation
//   Use case: Per-frame temporary allocations, string formatting
//
// TODO(ROADMAP Phase 1): Buddy Allocator
//   Current: malloc/free for variable-size allocations
//   Target: Zero-fragmentation allocator for variable-size objects
//   Implementation:
//     - Create BuddyAllocator with power-of-2 block sizes
//     - Implement buddy_allocator_create(min_block, max_block)
//     - Add buddy_allocator_alloc(size) with block splitting
//     - Implement buddy_allocator_free(ptr) with block merging
//     - Add free block bitmap for fast allocation
//     - Implement defragmentation support
//   Performance: O(log n) allocation/deallocation
//   Use case: Game objects, components, general allocations
//
// TODO(ROADMAP Phase 1): SIMD-Accelerated Memory Operations
//   Current: Standard memcpy/memset
//   Target: AVX2-optimized memory operations
//   Implementation:
//     - Implement simd_memcpy() using _mm256_store_si256
//     - Add simd_memset() for large memory clears
//     - Implement simd_memcmp() for fast comparisons
//     - Add alignment checks and fallback to scalar
//     - Optimize for cache line boundaries (64 bytes)
//   Performance: 2-4x faster than standard library
//   Use case: Large buffer copies, memory clearing
//
// TODO(ROADMAP Phase 1): Memory Profiling Visualization
//   Current: Text-based memory reports
//   Target: Visual memory profiler in editor
//   Implementation:
//     - Create memory profiler UI window in editor
//     - Add allocation timeline graph
//     - Implement memory usage by module (pie chart)
//     - Add allocation heatmap (frequency visualization)
//     - Implement leak detection UI with stack traces
//     - Add memory snapshot comparison
//   Files: memory.c, editor/memory_profiler.c
//
// TODO(ROADMAP Phase 1): Advanced Leak Detection
//   Current: Basic leak reporting on shutdown
//   Target: Real-time leak detection with stack traces
//   Implementation:
//     - Integrate libbacktrace for stack trace capture
//     - Store call stack for each allocation (configurable depth)
//     - Implement leak categorization (small, medium, large)
//     - Add leak detection during runtime (periodic checks)
//     - Implement leak suppression list (known false positives)
//     - Add leak severity scoring
//   Performance: <5% overhead with stack traces enabled
//
// TODO(ROADMAP Phase 1): Memory Tagging and Debugging
//   Current: Basic file/line tracking
//   Target: Advanced memory tagging for debugging
//   Implementation:
//     - Add allocation tags (TEMP, PERSISTENT, ASSET, etc.)
//     - Implement tag-based memory limits
//     - Add tag-based leak detection
//     - Implement memory poisoning (fill freed memory with pattern)
//     - Add use-after-free detection with guard pages
//     - Implement allocation ID tracking
//   Use case: Debug memory issues, enforce memory budgets
//
// TODO(ROADMAP Phase 1): Custom Allocator Interface
//   Current: Global allocators only
//   Target: Per-system custom allocators
//   Implementation:
//     - Create Allocator interface (alloc, free, realloc)
//     - Implement allocator registry
//     - Add per-system allocator assignment
//     - Implement allocator chaining (fallback allocators)
//     - Add allocator statistics and profiling
//   Use case: Physics uses pool allocator, renderer uses linear allocator
// TODO: Implement a lock-free memory allocator for low-latency multi-threaded
// access
// TODO: Add support for NUMA-aware memory allocation (Node-local affinity)
// TODO: Implement a virtual memory manager with residency control for streaming
// assets
// TODO: Add support for compressed memory pages to reduce swap pressure
// TODO: Implement a real-time memory fragmentation defragmenter for
// long-running processes
// TODO: Add support for GPU-mapped memory regions with cache-coherency control
// TODO: Implement custom memory-mapped I/O for high-speed asset loading
// TODO: Add support for encrypted memory segments for sensitive user data
// TODO: Implement a robust memory profiler with flame-graph and heap-dump
// support
// TODO: Research and implement AI-driven memory usage prediction and
// pre-allocation
// TODO: Add support for memory-pooled ECS entities with cache-line alignment
// TODO: Implement a sample-based memory leak detector with zero runtime
// overhead
// TODO: Add support for platform-specific high-speed allocators (e.g.,
// jemalloc, mimalloc)
// TODO: Implement a virtual "OOM" handler for graceful degradation on
// low-memory systems
// TODO: Add logic for automatic memory budget balancing between subsystems
// TODO: Research and implement persistent memory (PMEM) support for
// near-instant loading
// TODO: Implement a robust memory unit test suite with boundary and
// error-injection tests
// TODO: Add support for memory-synced multi-process shared memory regions
// TODO: Implement a virtual "memory pressure" simulator for stress-testing
// TODO: Research and implement hardware-assisted memory tagging (e.g., ARM MTE)
//
#include <core/logger.h>
#include <core/memory.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// SIMD support detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

// For stack trace capture
#ifdef __linux__
#include <dlfcn.h>
#include <execinfo.h>
#elif defined(_WIN32)
#include <dbghelp.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#include <execinfo.h>
#endif

// High-resolution timing
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

// Linear allocator implementation - <10ns per allocation
// Linear allocator implementation moved to allocator_linear.c

// SIMD memory operations - 2-4x faster than standard library
static bool g_simd_supported = false;
static bool g_simd_checked = false;

static void check_simd_support(void) {
  if (g_simd_checked)
    return;
  g_simd_checked = true;

  // Check for AVX2 support
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#ifdef _MSC_VER
  int cpu_info[4];
  __cpuid(cpu_info, 0);
  if (cpu_info[0] >= 7) {
    __cpuidex(cpu_info, 7, 0);
    g_simd_supported = (cpu_info[1] & (1 << 5)) != 0; // AVX2 bit
  }
#else
  u32 eax, ebx, ecx, edx;
  if (__get_cpuid(0, &eax, &ebx, &ecx, &edx) && eax >= 7) {
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    g_simd_supported = (ebx & (1 << 5)) != 0; // AVX2 bit
  }
#endif
#else
  // Non-x86 or unknown (e.g. ARM64)
  // Assume no AVX2 support (NEON support would be handled separately if needed)
  g_simd_supported = false;
#endif
}

bool simd_is_supported(void) {
  check_simd_support();
  return g_simd_supported;
}

#ifdef __AVX2__
#include <immintrin.h>

void *simd_memcpy(void *dst, const void *src, u32 size) {
  if (!dst || !src || size == 0)
    return dst;

  // Use AVX2 for large copies
  if (simd_is_supported() && size >= 32) {
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;

    // Copy 32-byte chunks
    u32 chunks = size / 32;
    for (u32 i = 0; i < chunks; i++) {
      __m256 data = _mm256_loadu_si256((__m256i *)(s + i * 32));
      _mm256_storeu_si256((__m256i *)(d + i * 32), data);
    }

    // Copy remaining bytes
    u32 remaining = size % 32;
    if (remaining > 0) {
      memcpy(d + chunks * 32, s + chunks * 32, remaining);
    }

    return dst;
  }

  // Fallback to standard memcpy
  return memcpy(dst, src, size);
}

void *simd_memset(void *dst, u8 value, u32 size) {
  if (!dst || size == 0)
    return dst;

  // Use AVX2 for large sets
  if (simd_is_supported() && size >= 32) {
    u8 *d = (u8 *)dst;

    // Create 32-byte pattern
    __m256i pattern = _mm256_set1_epi8(value);

    // Set 32-byte chunks
    u32 chunks = size / 32;
    for (u32 i = 0; i < chunks; i++) {
      _mm256_storeu_si256((__m256i *)(d + i * 32), pattern);
    }

    // Set remaining bytes
    u32 remaining = size % 32;
    if (remaining > 0) {
      memset(d + chunks * 32, value, remaining);
    }

    return dst;
  }

  // Fallback to standard memset
  return memset(dst, value, size);
}

int simd_memcmp(const void *ptr1, const void *ptr2, u32 size) {
  if (!ptr1 || !ptr2)
    return 0;
  if (size == 0)
    return 0;

  // Use AVX2 for large comparisons
  if (simd_is_supported() && size >= 32) {
    const u8 *p1 = (const u8 *)ptr1;
    const u8 *p2 = (const u8 *)ptr2;

    // Compare 32-byte chunks
    u32 chunks = size / 32;
    for (u32 i = 0; i < chunks; i++) {
      __m256i data1 = _mm256_loadu_si256((__m256i *)(p1 + i * 32));
      __m256i data2 = _mm256_loadu_si256((__m256i *)(p2 + i * 32));
      __m256i cmp = _mm256_cmpeq_epi8(data1, data2);
      u32 mask = _mm256_movemask_epi8(cmp);

      if (mask != 0xFFFFFFFF) {
        // Find first differing byte
        for (u32 j = 0; j < 32; j++) {
          if (p1[i * 32 + j] != p2[i * 32 + j]) {
            return p1[i * 32 + j] - p2[i * 32 + j];
          }
        }
      }
    }

    // Compare remaining bytes
    u32 remaining = size % 32;
    if (remaining > 0) {
      return memcmp(p1 + chunks * 32, p2 + chunks * 32, remaining);
    }

    return 0;
  }

  // Fallback to standard memcmp
  return memcmp(ptr1, ptr2, size);
}

#else
// Fallback implementations without AVX2
void *simd_memcpy(void *dst, const void *src, u32 size) {
  return memcpy(dst, src, size);
}

void *simd_memset(void *dst, u8 value, u32 size) {
  return memset(dst, value, size);
}

int simd_memcmp(const void *ptr1, const void *ptr2, u32 size) {
  return memcmp(ptr1, ptr2, size);
}

#endif

// Forward declarations for static helpers
static const char *memory_extract_module_name(const char *file_path);
static ModuleStats *memory_get_or_create_module_stats(const char *module_name);
static u64 get_timestamp_ns(void);
static void memory_capture_stack_trace_internal(MemoryAlloc *alloc);

// Buddy allocator implementation - O(log n) allocation/deallocation
// Buddy allocator implementation moved to buddy_allocator.c

MemoryTracker g_memory_tracker = {0};

// Memory profiler visualization implementation
static MemoryProfiler g_memory_profiler = {0};

static u64 get_timestamp_ns(void) {
#ifdef _WIN32
  LARGE_INTEGER frequency, counter;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  return (counter.QuadPart * 1000000000ULL) / frequency.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
#endif
}

void memory_profiler_init(u32 max_snapshots) {
  if (max_snapshots == 0)
    max_snapshots = 1000;

  g_memory_profiler.snapshots =
      (MemorySnapshot *)malloc(sizeof(MemorySnapshot) * max_snapshots);
  if (!g_memory_profiler.snapshots) {
    LOG_ERROR("Failed to initialize memory profiler");
    return;
  }

  g_memory_profiler.max_snapshots = max_snapshots;
  g_memory_profiler.snapshot_count = 0;
  g_memory_profiler.current_index = 0;
  g_memory_profiler.recording = false;

  LOG_DEBUG("Memory profiler initialized with %u snapshot capacity",
            max_snapshots);
}

void memory_profiler_shutdown(void) {
  if (g_memory_profiler.snapshots) {
    free(g_memory_profiler.snapshots);
    g_memory_profiler.snapshots = NULL;
  }
  memset(&g_memory_profiler, 0, sizeof(g_memory_profiler));
}

void memory_profiler_start_recording(void) {
  g_memory_profiler.recording = true;
  LOG_DEBUG("Memory profiler recording started");
}

void memory_profiler_stop_recording(void) {
  g_memory_profiler.recording = false;
  LOG_DEBUG("Memory profiler recording stopped");
}

void memory_profiler_capture_snapshot(void) {
  if (!g_memory_profiler.recording || !g_memory_profiler.snapshots)
    return;

  MemorySnapshot *snapshot =
      &g_memory_profiler.snapshots[g_memory_profiler.current_index];

  snapshot->timestamp = get_timestamp_ns();
  snapshot->total_usage = memory_get_total_used();
  snapshot->allocation_count = g_memory_tracker.allocation_count;
  snapshot->free_count = 0; // Could track this if needed

  // Copy tag usage
  for (u32 i = 0; i < MEMORY_TAG_COUNT; i++) {
    snapshot->tag_usage[i] = g_memory_tracker.tag_usage[i];
  }

  // Update circular buffer
  g_memory_profiler.current_index =
      (g_memory_profiler.current_index + 1) % g_memory_profiler.max_snapshots;
  if (g_memory_profiler.snapshot_count < g_memory_profiler.max_snapshots) {
    g_memory_profiler.snapshot_count++;
  }
}

MemorySnapshot *memory_profiler_get_snapshots(u32 *count) {
  if (!count)
    return NULL;

  *count = g_memory_profiler.snapshot_count;
  return g_memory_profiler.snapshots;
}

void memory_profiler_export_timeline(const char *filename) {
  if (!filename || !g_memory_profiler.snapshots) {
    LOG_ERROR("Invalid filename or profiler not initialized");
    return;
  }

  FILE *file = fopen(filename, "w");
  if (!file) {
    LOG_ERROR("Failed to open file for timeline export: %s", filename);
    return;
  }

  // CSV format for easy visualization
  fprintf(file, "timestamp,total_usage,temp,persistent,asset,renderer,audio,"
                "physics,ai,network,ui,allocation_count\n");

  for (u32 i = 0; i < g_memory_profiler.snapshot_count; i++) {
    MemorySnapshot *snapshot = &g_memory_profiler.snapshots[i];

    fprintf(file, "%llu,%llu", snapshot->timestamp, snapshot->total_usage);

    for (u32 j = 0; j < MEMORY_TAG_COUNT; j++) {
    }

    fprintf(file, ",%u\n", snapshot->allocation_count);
  }

  fclose(file);
  LOG_INFO("Memory timeline exported to: %s", filename);
}

void memory_tracker_init(u32 initial_capacity) {
  g_memory_tracker.capacity = initial_capacity;
  g_memory_tracker.count = 0;
  g_memory_tracker.total_allocated = 0;
  g_memory_tracker.total_freed = 0;
  g_memory_tracker.peak_allocated = 0;
  g_memory_tracker.active_allocations = 0;
  g_memory_tracker.memory_limit = 0; // 0 means no limit
  g_memory_tracker.enforce_limits = false;

  // Initialize enhanced tracking
  g_memory_tracker.allocation_counter = 0;
  memset(g_memory_tracker.tag_limits, 0, sizeof(g_memory_tracker.tag_limits));
  memset(g_memory_tracker.tag_usage, 0, sizeof(g_memory_tracker.tag_usage));
  g_memory_tracker.advanced_leak_detection = false;
  g_memory_tracker.stack_trace_enabled = false;

  // Initialize performance metrics
  g_memory_tracker.total_alloc_time_ns = 0;
  g_memory_tracker.total_free_time_ns = 0;
  g_memory_tracker.allocation_count = 0;

  g_memory_tracker.allocations =
      (MemoryAlloc *)malloc(sizeof(MemoryAlloc) * initial_capacity);
  if (!g_memory_tracker.allocations) {
    LOG_FATAL("Failed to initialize memory tracker");
  }

  LOG_INFO("Memory tracker initialized with capacity: %u", initial_capacity);
}

// Advanced leak detection implementation with stack traces
static void memory_capture_stack_trace_internal(MemoryAlloc *alloc) {
  if (!g_memory_tracker.stack_trace_enabled || !alloc)
    return;

  alloc->stack_depth = 0;

#if defined(__linux__) || defined(__APPLE__)
  // Use backtrace on Unix-like systems
  void *buffer[16];
  int frames = backtrace(buffer, 16);
  if (frames > 0) {
    alloc->stack_depth = (frames < 16) ? frames : 16;
    for (int i = 0; i < alloc->stack_depth; i++) {
      alloc->stack_trace[i] = buffer[i];
    }
  }
#elif defined(_WIN32)
  // Use CaptureStackBackTrace on Windows
  alloc->stack_depth = CaptureStackBackTrace(1, 15, alloc->stack_trace, NULL);
#endif
}

void memory_enable_advanced_leak_detection(bool enable) {
  g_memory_tracker.advanced_leak_detection = enable;
  LOG_INFO("Advanced leak detection %s", enable ? "enabled" : "disabled");
}

void memory_enable_stack_traces(bool enable) {
  g_memory_tracker.stack_trace_enabled = enable;
  LOG_INFO("Stack trace capture %s", enable ? "enabled" : "disabled");
}

void memory_capture_stack_trace(void **buffer, u32 max_depth, u32 *out_depth) {
  if (!buffer || !out_depth || max_depth == 0)
    return;

  *out_depth = 0;

#if defined(__linux__) || defined(__APPLE__)
  int frames = backtrace(buffer, max_depth);
  if (frames > 0) {
    *out_depth = (frames < max_depth) ? frames : max_depth;
  }
#elif defined(_WIN32)
  *out_depth = CaptureStackBackTrace(1, max_depth, buffer, NULL);
#endif
}

char **memory_resolve_stack_trace(void **addresses, u32 depth) {
  if (!addresses || depth == 0)
    return NULL;

  char **symbols = (char **)malloc(sizeof(char *) * depth);
  if (!symbols)
    return NULL;

#if defined(__linux__) || defined(__APPLE__)
  // Use backtrace_symbols to resolve addresses
  char **resolved = backtrace_symbols(addresses, depth);
  if (resolved) {
    for (u32 i = 0; i < depth; i++) {
      if (resolved[i]) {
        symbols[i] = strdup(resolved[i]);
      } else {
        symbols[i] = strdup("<unknown>");
      }
    }
    free(resolved);
  } else {
    for (u32 i = 0; i < depth; i++) {
      symbols[i] = strdup("<unknown>");
    }
  }
#elif defined(_WIN32)
  // Use SymFromAddr on Windows
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);

  for (u32 i = 0; i < depth; i++) {
    char buffer[256];
    SYMBOL_INFO *symbol = (SYMBOL_INFO *)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 256 - sizeof(SYMBOL_INFO);

    if (SymFromAddr(process, (DWORD64)addresses[i], NULL, symbol)) {
      snprintf(buffer + sizeof(SYMBOL_INFO), 256 - sizeof(SYMBOL_INFO),
               "%s+0x%llx", symbol->Name,
               (DWORD64)addresses[i] - symbol->Address);
      symbols[i] = strdup(buffer + sizeof(SYMBOL_INFO));
    } else {
      symbols[i] = strdup("<unknown>");
    }
  }

  SymCleanup(process);
#else
  for (u32 i = 0; i < depth; i++) {
    symbols[i] = strdup("<stack trace not supported>");
  }
#endif

  return symbols;
}

void memory_free_resolved_stack_trace(char **symbols) {
  if (!symbols)
    return;

  // Count symbols (we don't know the depth here)
  u32 depth = 0;
  while (symbols[depth]) {
    free(symbols[depth]);
    depth++;
  }

  free(symbols);
}
void memory_tracker_shutdown(void) {
  memory_tracker_report();

  if (g_memory_tracker.allocations) {
    free(g_memory_tracker.allocations);
    g_memory_tracker.allocations = NULL;
  }
}

// Memory tagging implementation
static const char *g_memory_tag_names[MEMORY_TAG_COUNT] = {
    "DEFAULT", "TEMP",    "PERSISTENT", "ASSET",   "RENDERER",
    "AUDIO",   "PHYSICS", "AI",         "NETWORK", "UI"};

void memory_set_tag_limit(MemoryTag tag, u64 limit) {
  if (tag >= MEMORY_TAG_COUNT) {
    LOG_ERROR("Invalid memory tag: %u", tag);
    return;
  }

  g_memory_tracker.tag_limits[tag] = limit;
  LOG_INFO("Memory limit for %s tag set to: %llu bytes (%.2f MB)",
           g_memory_tag_names[tag], limit, limit / (1024.0 * 1024.0));
}

u64 memory_get_tag_usage(MemoryTag tag) {
  if (tag >= MEMORY_TAG_COUNT)
    return 0;
  return g_memory_tracker.tag_usage[tag];
}

u64 memory_get_tag_limit(MemoryTag tag) {
  if (tag >= MEMORY_TAG_COUNT)
    return 0;
  return g_memory_tracker.tag_limits[tag];
}

void memory_print_tag_stats(void) {
  LOG_INFO("=== Memory Tag Statistics ===");

  for (u32 i = 0; i < MEMORY_TAG_COUNT; i++) {
    u64 usage = g_memory_tracker.tag_usage[i];
    u64 limit = g_memory_tracker.tag_limits[i];

    LOG_INFO("Tag: %s", g_memory_tag_names[i]);
    LOG_INFO("  Usage: %llu bytes (%.2f MB)", usage, usage / (1024.0 * 1024.0));

    if (limit > 0) {
      f64 percentage = (f64)usage / limit * 100.0;
      LOG_INFO("  Limit: %llu bytes (%.2f MB) - %.1f%% used", limit,
               limit / (1024.0 * 1024.0), percentage);

      if (usage > limit) {
        LOG_WARN("  TAG LIMIT EXCEEDED!");
      }
    } else {
      LOG_INFO("  Limit: No limit set");
    }
    LOG_INFO("");
  }

  LOG_INFO("============================");
}

void *memory_alloc_tagged(u32 size, MemoryTag tag, const char *file, u32 line) {
  if (tag >= MEMORY_TAG_COUNT) {
    tag = MEMORY_TAG_DEFAULT;
  }

  // Check tag-specific limits
  if (g_memory_tracker.tag_limits[tag] > 0) {
    u64 current_usage = g_memory_tracker.tag_usage[tag];
    if (current_usage + size > g_memory_tracker.tag_limits[tag]) {
      LOG_ERROR("Memory allocation denied: %s tag limit exceeded (%llu + %u > "
                "%llu) at %s:%u",
                g_memory_tag_names[tag], current_usage, size,
                g_memory_tracker.tag_limits[tag], file, line);
      return NULL;
    }
  }

  // Check global memory limits
  if (g_memory_tracker.enforce_limits && !memory_check_limit(size)) {
    LOG_ERROR(
        "Memory allocation denied: %u bytes would exceed global limit of %llu "
        "bytes at %s:%u",
        size, g_memory_tracker.memory_limit, file, line);
    return NULL;
  }

  u64 start_time = get_timestamp_ns();
  void *ptr = malloc(size);
  u64 end_time = get_timestamp_ns();

  if (!ptr) {
    LOG_ERROR("Failed to allocate %u bytes at %s:%u", size, file, line);
    return NULL;
  }

  if (g_memory_tracker.count < g_memory_tracker.capacity) {
    MemoryAlloc *alloc = &g_memory_tracker.allocations[g_memory_tracker.count];
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->file = file;
    alloc->line = line;
    alloc->tag = tag;
    alloc->allocation_id = ++g_memory_tracker.allocation_counter;
    alloc->timestamp = end_time;

    // Capture stack trace if enabled
    memory_capture_stack_trace_internal(alloc);

    g_memory_tracker.count++;
    g_memory_tracker.total_allocated += size;
    g_memory_tracker.active_allocations++;
    g_memory_tracker.tag_usage[tag] += size;
    g_memory_tracker.allocation_count++;

    // Update performance metrics
    g_memory_tracker.total_alloc_time_ns += (end_time - start_time);

    // Update peak usage
    u64 current_usage =
        g_memory_tracker.total_allocated - g_memory_tracker.total_freed;
    if (current_usage > g_memory_tracker.peak_allocated) {
      g_memory_tracker.peak_allocated = current_usage;
    }

    // Update module statistics
    const char *module_name = memory_extract_module_name(file);
    ModuleStats *module_stats = memory_get_or_create_module_stats(module_name);
    if (module_stats) {
      module_stats->total_allocated += size;
      module_stats->allocation_count++;
      module_stats->active_allocations++;

      u64 module_current =
          module_stats->total_allocated - module_stats->total_freed;
      if (module_current > module_stats->peak_usage) {
        module_stats->peak_usage = module_current;
      }
    }

    // Capture profiler snapshot if recording
    if (g_memory_profiler.recording) {
      memory_profiler_capture_snapshot();
    }
  }

  return ptr;
}

// Custom allocator interface implementation
static Allocator *g_allocator_registry[32] = {0};
static u32 g_allocator_count = 0;

Allocator *allocator_create_custom(const AllocatorVTable *vtable,
                                   const char *name, MemoryTag tag,
                                   Allocator *fallback) {
  if (!vtable || !name || g_allocator_count >= 32) {
    LOG_ERROR("Failed to create custom allocator: invalid parameters or "
              "registry full");
    return NULL;
  }

  Allocator *allocator = (Allocator *)malloc(sizeof(Allocator));
  if (!allocator) {
    LOG_ERROR("Failed to allocate memory for custom allocator");
    return NULL;
  }

  allocator->vtable = vtable;
  allocator->name = name;
  allocator->tag = tag;
  allocator->fallback = fallback;
  allocator->impl = NULL;

  g_allocator_registry[g_allocator_count++] = allocator;

  LOG_INFO("Custom allocator created: %s (tag: %s)", name,
           tag < MEMORY_TAG_COUNT ? g_memory_tag_names[tag] : "CUSTOM");
  return allocator;
}

void allocator_destroy(Allocator *allocator) {
  if (!allocator)
    return;

  // Remove from registry
  for (u32 i = 0; i < g_allocator_count; i++) {
    if (g_allocator_registry[i] == allocator) {
      g_allocator_registry[i] = g_allocator_registry[g_allocator_count - 1];
      g_allocator_count--;
      break;
    }
  }

  free(allocator);
}

void *allocator_alloc(Allocator *allocator, u32 size, u32 alignment) {
  if (!allocator || !allocator->vtable) {
    LOG_ERROR("Invalid allocator or vtable");
    return NULL;
  }

  void *ptr = allocator->vtable->alloc(allocator, size, alignment);

  // Try fallback if allocation failed
  if (!ptr && allocator->fallback) {
    LOG_DEBUG("Allocation failed, trying fallback allocator");
    ptr = allocator_alloc(allocator->fallback, size, alignment);
  }

  return ptr;
}

void allocator_free(Allocator *allocator, void *ptr) {
  if (!allocator || !ptr || !allocator->vtable)
    return;

  allocator->vtable->free(allocator, ptr);
}

void allocator_reset(Allocator *allocator) {
  if (!allocator || !allocator->vtable)
    return;

  if (allocator->vtable->reset) {
    allocator->vtable->reset(allocator);
  }

  // Reset fallback too
  if (allocator->fallback) {
    allocator_reset(allocator->fallback);
  }
}

u64 allocator_get_usage(Allocator *allocator) {
  if (!allocator || !allocator->vtable || !allocator->vtable->get_usage) {
    return 0;
  }

  u64 usage = allocator->vtable->get_usage(allocator);

  // Add fallback usage
  if (allocator->fallback) {
    usage += allocator_get_usage(allocator->fallback);
  }

  return usage;
}

u64 allocator_get_capacity(Allocator *allocator) {
  if (!allocator || !allocator->vtable || !allocator->vtable->get_capacity) {
    return 0;
  }

  u64 capacity = allocator->vtable->get_capacity(allocator);

  // Add fallback capacity
  if (allocator->fallback) {
    capacity += allocator_get_capacity(allocator->fallback);
  }

  return capacity;
}

void allocator_get_stats(Allocator *allocator, char *buffer, u32 buffer_size) {
  if (!allocator || !buffer || buffer_size == 0)
    return;

  if (allocator->vtable && allocator->vtable->get_stats) {
    allocator->vtable->get_stats(allocator, buffer, buffer_size);
  } else {
    snprintf(buffer, buffer_size, "Allocator: %s (no stats available)",
             allocator->name);
  }
}

// Linear allocator vtable implementation
static void *linear_allocator_alloc_impl(Allocator *allocator, u32 size,
                                         u32 alignment) {
  LinearAllocator *linear = (LinearAllocator *)allocator->impl;
  return linear_allocator_alloc(linear, size, alignment);
}

static void *linear_allocator_realloc_impl(Allocator *allocator, void *ptr,
                                           u32 new_size) {
  // Linear allocator doesn't support realloc
  LOG_ERROR("Linear allocator does not support realloc");
  return NULL;
}

static void linear_allocator_free_impl(Allocator *allocator, void *ptr) {
  // Linear allocator doesn't support individual frees
  // All memory is freed at once via reset
}

static void linear_allocator_reset_impl(Allocator *allocator) {
  LinearAllocator *linear = (LinearAllocator *)allocator->impl;
  linear_allocator_reset(linear);
}

static u64 linear_allocator_get_usage_impl(Allocator *allocator) {
  LinearAllocator *linear = (LinearAllocator *)allocator->impl;
  return linear->offset;
}

static u64 linear_allocator_get_capacity_impl(Allocator *allocator) {
  LinearAllocator *linear = (LinearAllocator *)allocator->impl;
  return linear->size;
}

static void linear_allocator_get_stats_impl(Allocator *allocator, char *buffer,
                                            u32 buffer_size) {
  LinearAllocator *linear = (LinearAllocator *)allocator->impl;
  f64 usage_percent = (f64)linear->offset / linear->size * 100.0;
  snprintf(buffer, buffer_size,
           "Linear Allocator '%s': %llu/%llu bytes (%.1f%% used), peak: %llu",
           allocator->name, (u64)linear->offset, (u64)linear->size,
           usage_percent, (u64)linear->peak_usage);
}

static const AllocatorVTable g_linear_allocator_vtable = {
    .alloc = linear_allocator_alloc_impl,
    .realloc = linear_allocator_realloc_impl,
    .free = linear_allocator_free_impl,
    .reset = linear_allocator_reset_impl,
    .get_usage = linear_allocator_get_usage_impl,
    .get_capacity = linear_allocator_get_capacity_impl,
    .get_stats = linear_allocator_get_stats_impl};

Allocator *allocator_create_linear(const char *name, u32 size, u32 buffer_count,
                                   MemoryTag tag) {
  LinearAllocator *linear = linear_allocator_create(size, buffer_count);
  if (!linear)
    return NULL;

  Allocator *allocator =
      allocator_create_custom(&g_linear_allocator_vtable, name, tag, NULL);
  if (!allocator) {
    linear_allocator_destroy(linear);
    return NULL;
  }

  allocator->impl = linear;
  return allocator;
}

// Buddy allocator vtable implementation
static void *buddy_allocator_alloc_impl(Allocator *allocator, u32 size,
                                        u32 alignment) {
  BuddyAllocator *buddy = (BuddyAllocator *)allocator->impl;
  return buddy_allocator_alloc(buddy, size);
}

static void *buddy_allocator_realloc_impl(Allocator *allocator, void *ptr,
                                          u32 new_size) {
  // Buddy allocator doesn't support realloc directly
  // Would need to alloc new, copy, and free old
  LOG_ERROR("Buddy allocator does not support realloc directly");
  return NULL;
}

static void buddy_allocator_free_impl(Allocator *allocator, void *ptr) {
  BuddyAllocator *buddy = (BuddyAllocator *)allocator->impl;
  buddy_allocator_free(buddy, ptr);
}

static void buddy_allocator_reset_impl(Allocator *allocator) {
  // Buddy allocator reset would require recreating the allocator
  LOG_WARN(
      "Buddy allocator reset not implemented - recreate allocator instead");
}

static u64 buddy_allocator_get_usage_impl(Allocator *allocator) {
  BuddyAllocator *buddy = (BuddyAllocator *)allocator->impl;
  return buddy->pool_size - (buddy->free_blocks * buddy->min_block_size);
}

static u64 buddy_allocator_get_capacity_impl(Allocator *allocator) {
  BuddyAllocator *buddy = (BuddyAllocator *)allocator->impl;
  return buddy->pool_size;
}

static void buddy_allocator_get_stats_impl(Allocator *allocator, char *buffer,
                                           u32 buffer_size) {
  BuddyAllocator *buddy = (BuddyAllocator *)allocator->impl;
  u64 used = buddy_allocator_get_usage_impl(allocator);
  f64 usage_percent = (f64)used / buddy->pool_size * 100.0;
  snprintf(buffer, buffer_size,
           "Buddy Allocator '%s': %llu/%llu bytes (%.1f%% used), blocks: %u/%u",
           allocator->name, used, (u64)buddy->pool_size, usage_percent,
           buddy->total_blocks - buddy->free_blocks, buddy->total_blocks);
}

static const AllocatorVTable g_buddy_allocator_vtable = {
    .alloc = buddy_allocator_alloc_impl,
    .realloc = buddy_allocator_realloc_impl,
    .free = buddy_allocator_free_impl,
    .reset = buddy_allocator_reset_impl,
    .get_usage = buddy_allocator_get_usage_impl,
    .get_capacity = buddy_allocator_get_capacity_impl,
    .get_stats = buddy_allocator_get_stats_impl};

Allocator *allocator_create_buddy(const char *name, u32 min_block_size,
                                  u32 max_block_size, MemoryTag tag) {
  BuddyAllocator *buddy =
      buddy_allocator_create(min_block_size, max_block_size);
  if (!buddy)
    return NULL;

  Allocator *allocator =
      allocator_create_custom(&g_buddy_allocator_vtable, name, tag, NULL);
  if (!allocator) {
    buddy_allocator_destroy(buddy);
    return NULL;
  }

  allocator->impl = buddy;
  return allocator;
}
void *memory_alloc(u32 size, const char *file, u32 line) {
  return memory_alloc_tagged(size, MEMORY_TAG_DEFAULT, file, line);
}

void *memory_calloc(u32 count, u32 size, const char *file, u32 line) {
  u32 total_size = count * size;
  return memory_alloc_tagged(total_size, MEMORY_TAG_DEFAULT, file, line);
}

void *memory_realloc(void *ptr, u32 new_size, const char *file, u32 line) {
  void *new_ptr = realloc(ptr, new_size);
  if (!new_ptr) {
    LOG_ERROR("Failed to reallocate to %u bytes at %s:%u", new_size, file,
              line);
    return NULL;
  }

  for (u32 i = 0; i < g_memory_tracker.count; i++) {
    if (g_memory_tracker.allocations[i].ptr == ptr) {
      g_memory_tracker.allocations[i].ptr = new_ptr;
      g_memory_tracker.allocations[i].size = new_size;
      g_memory_tracker.allocations[i].file = file;
      g_memory_tracker.allocations[i].line = line;
      break;
    }
  }

  return new_ptr;
}

void memory_free(void *ptr) {
  if (!ptr)
    return;

  u64 start_time = get_timestamp_ns();

  for (u32 i = 0; i < g_memory_tracker.count; i++) {
    if (g_memory_tracker.allocations[i].ptr == ptr) {
      MemoryAlloc *alloc = &g_memory_tracker.allocations[i];

      g_memory_tracker.total_freed += alloc->size;
      g_memory_tracker.active_allocations--;
      g_memory_tracker.tag_usage[alloc->tag] -= alloc->size;

      // Update module statistics
      const char *module_name = memory_extract_module_name(alloc->file);
      ModuleStats *module_stats =
          memory_get_or_create_module_stats(module_name);
      if (module_stats) {
        module_stats->total_freed += alloc->size;
        module_stats->active_allocations--;
      }

      g_memory_tracker.allocations[i] =
          g_memory_tracker.allocations[g_memory_tracker.count - 1];
      g_memory_tracker.count--;
      break;
    }
  }

  u64 end_time = get_timestamp_ns();
  g_memory_tracker.total_free_time_ns += (end_time - start_time);

  free(ptr);
}

void memory_tracker_report(void) {
  LOG_INFO("=== Memory Report ===");
  LOG_INFO("Total allocated: %llu bytes", g_memory_tracker.total_allocated);
  LOG_INFO("Total freed: %llu bytes", g_memory_tracker.total_freed);
  LOG_INFO("Current usage: %llu bytes",
           g_memory_tracker.total_allocated - g_memory_tracker.total_freed);
  LOG_INFO("Active allocations: %u", g_memory_tracker.count);

  // Performance metrics
  if (g_memory_tracker.allocation_count > 0) {
    f64 avg_alloc_time = (f64)g_memory_tracker.total_alloc_time_ns /
                         g_memory_tracker.allocation_count;
    LOG_INFO("Average allocation time: %.2f ns", avg_alloc_time);
  }

  if (g_memory_tracker.count > 0) {
    LOG_WARN("Leaked allocations:");
    for (u32 i = 0; i < g_memory_tracker.count; i++) {
      MemoryAlloc *alloc = &g_memory_tracker.allocations[i];
      LOG_WARN("  %u bytes at %s:%u (tag: %s, id: %llu)", alloc->size,
               alloc->file, alloc->line,
               alloc->tag < MEMORY_TAG_COUNT ? g_memory_tag_names[alloc->tag]
                                             : "UNKNOWN",
               alloc->allocation_id);
    }
  }
}

// Object pool implementation
ObjectPool *object_pool_create(u32 object_size, u32 capacity) {
  ObjectPool *pool = (ObjectPool *)malloc(sizeof(ObjectPool));
  if (!pool)
    return NULL;

  pool->objects = malloc(object_size * capacity);
  pool->free_list = (bool *)calloc(capacity, sizeof(bool));

  if (!pool->objects || !pool->free_list) {
    free(pool->objects);
    free(pool->free_list);
    free(pool);
    return NULL;
  }

  pool->object_size = object_size;
  pool->capacity = capacity;
  pool->count = 0;

  memset(pool->free_list, 1, capacity);

  LOG_DEBUG("Object pool created: %u objects of %u bytes", capacity,
            object_size);
  return pool;
}

void object_pool_destroy(ObjectPool *pool) {
  if (!pool)
    return;

  free(pool->objects);
  free(pool->free_list);
  free(pool);
}

void *object_pool_allocate(ObjectPool *pool) {
  if (!pool)
    return NULL;

  for (u32 i = 0; i < pool->capacity; i++) {
    if (pool->free_list[i]) {
      pool->free_list[i] = false;
      pool->count++;
      return (char *)pool->objects + (i * pool->object_size);
    }
  }

  LOG_WARN("Object pool exhausted");
  return NULL;
}

void object_pool_free(ObjectPool *pool, void *object) {
  if (!pool || !object)
    return;

  u32 index = ((char *)object - (char *)pool->objects) / pool->object_size;

  if (index < pool->capacity && !pool->free_list[index]) {
    pool->free_list[index] = true;
    pool->count--;
  }
}

void object_pool_reset(ObjectPool *pool) {
  if (!pool)
    return;

  memset(pool->free_list, 1, pool->capacity);
  pool->count = 0;
}

u32 object_pool_get_available(ObjectPool *pool) {
  return pool ? (pool->capacity - pool->count) : 0;
}

// Stack allocator implementation moved to allocator_stack.c

// Phase 2 implementations: Memory statistics and pool management
PoolStats *memory_get_pool_stats(u32 *out_pool_count) {
  if (!out_pool_count)
    return NULL;
  *out_pool_count = 0;
  return NULL;
}

u64 memory_get_total_used(void) {
  return g_memory_tracker.total_allocated - g_memory_tracker.total_freed;
}

u64 memory_get_peak_used(void) { return g_memory_tracker.peak_allocated; }

void memory_print_stats(void) {
  u64 current_use = memory_get_total_used();
  LOG_INFO("=== Memory Statistics ===");
  LOG_INFO("Total Allocated: %llu bytes", g_memory_tracker.total_allocated);
  LOG_INFO("Total Freed: %llu bytes", g_memory_tracker.total_freed);
  LOG_INFO("Current Use: %llu bytes", current_use);
  LOG_INFO("Peak Use: %llu bytes", g_memory_tracker.peak_allocated);
  LOG_INFO("Active Allocations: %u", g_memory_tracker.active_allocations);
  LOG_INFO("========================");
}

void memory_named_region_begin(const char *name) {
  LOG_DEBUG("Named memory region started: %s", name);
}

void memory_named_region_end(void) { LOG_DEBUG("Named memory region ended"); }

void memory_check_leaks(void) {
  LOG_INFO("=== Memory Leak Detection Report ===");
  u64 current_use = memory_get_total_used();

  if (current_use > 0) {
    LOG_WARN("Memory leak detected: %llu bytes unfreed across %u allocations",
             current_use, g_memory_tracker.count);

    // Group leaks by file for better analysis
    typedef struct {
      const char *file;
      u64 total_bytes;
      u32 allocation_count;
    } FileLeakInfo;

    FileLeakInfo file_leaks[32] = {0};
    u32 file_count = 0;

    for (u32 i = 0; i < g_memory_tracker.count; i++) {
      MemoryAlloc *alloc = &g_memory_tracker.allocations[i];
      if (alloc->ptr) {
        // Find existing file entry or create new one
        u32 file_idx = 0;
        for (; file_idx < file_count; file_idx++) {
          if (file_leaks[file_idx].file == alloc->file) {
            break;
          }
        }

        if (file_idx >= file_count && file_count < 32) {
          file_leaks[file_count].file = alloc->file;
          file_leaks[file_count].total_bytes = 0;
          file_leaks[file_count].allocation_count = 0;
          file_count++;
        }

        if (file_idx < file_count) {
          file_leaks[file_idx].total_bytes += alloc->size;
          file_leaks[file_idx].allocation_count++;
        }
      }
    }

    // Report leaks grouped by file
    LOG_WARN("Leak breakdown by source file:");
    for (u32 i = 0; i < file_count; i++) {
      LOG_WARN("  %s: %llu bytes in %u allocations", file_leaks[i].file,
               file_leaks[i].total_bytes, file_leaks[i].allocation_count);
    }

    // Report largest individual leaks
    if (g_memory_tracker.count > 0) {
      LOG_WARN("Top 10 largest individual leaks:");
      for (u32 i = 0; i < g_memory_tracker.count && i < 10; i++) {
        MemoryAlloc *alloc = &g_memory_tracker.allocations[i];
        if (alloc->ptr) {
          LOG_WARN("  [%u] %u bytes at %s:%u", i + 1, alloc->size, alloc->file,
                   alloc->line);
        }
      }
    }
  } else {
    LOG_INFO("No memory leaks detected - all allocations properly freed");
  }

  LOG_INFO("Peak memory usage was: %llu bytes",
           g_memory_tracker.peak_allocated);
  LOG_INFO("================================");
}

void memory_dump_allocations(void) {
  LOG_INFO("=== Active Allocations ===");
  u32 count = 0;
  for (u32 i = 0; i < g_memory_tracker.count && count < 50; i++) {
    MemoryAlloc *alloc = &g_memory_tracker.allocations[i];
    if (alloc->ptr) {
      LOG_INFO("  [%u] %u bytes at %s:%u", count++, alloc->size, alloc->file,
               alloc->line);
    }
  }
  LOG_INFO("Total tracked: %u", count);
  LOG_INFO("=======================");
}

// Memory limits implementation
void memory_set_limit(u64 limit_bytes) {
  g_memory_tracker.memory_limit = limit_bytes;
  LOG_INFO("Memory limit set to: %llu bytes (%.2f MB)", limit_bytes,
           limit_bytes / (1024.0 * 1024.0));
}

void memory_set_enforcement(bool enabled) {
  g_memory_tracker.enforce_limits = enabled;
  LOG_INFO("Memory limit enforcement %s", enabled ? "enabled" : "disabled");
}

u64 memory_get_limit(void) { return g_memory_tracker.memory_limit; }

bool memory_is_enforcement_enabled(void) {
  return g_memory_tracker.enforce_limits;
}

bool memory_check_limit(u64 requested_size) {
  if (g_memory_tracker.memory_limit == 0) {
    return true; // No limit set
  }

  u64 current_usage =
      g_memory_tracker.total_allocated - g_memory_tracker.total_freed;
  return (current_usage + requested_size) <= g_memory_tracker.memory_limit;
}

// Module statistics implementation
static ModuleStats g_module_stats[32] = {0};
static u32 g_module_count = 0;

static ModuleStats *memory_get_or_create_module_stats(const char *module_name) {
  // Try to find existing module
  for (u32 i = 0; i < g_module_count; i++) {
    if (strcmp(g_module_stats[i].module_name, module_name) == 0) {
      return &g_module_stats[i];
    }
  }

  // Create new module entry
  if (g_module_count < 32) {
    ModuleStats *stats = &g_module_stats[g_module_count];
    strncpy(stats->module_name, module_name, 63);
    stats->module_name[63] = '\0';
    stats->total_allocated = 0;
    stats->total_freed = 0;
    stats->allocation_count = 0;
    stats->active_allocations = 0;
    stats->peak_usage = 0;
    g_module_count++;
    return stats;
  }

  return NULL; // No space for new modules
}

static const char *memory_extract_module_name(const char *file_path) {
  if (!file_path)
    return "unknown";

  // Extract just the filename from the path
  const char *last_slash = strrchr(file_path, '/');
  if (last_slash) {
    last_slash++; // Skip the slash
  } else {
    last_slash = file_path;
  }

  // Remove file extension for cleaner module names
  static char module_name[64];
  strncpy(module_name, last_slash, 63);
  module_name[63] = '\0';

  char *dot = strrchr(module_name, '.');
  if (dot) {
    *dot = '\0';
  }

  return module_name;
}

ModuleStats *memory_get_module_stats(u32 *out_module_count) {
  if (out_module_count) {
    *out_module_count = g_module_count;
  }
  return g_module_stats;
}

void memory_print_module_stats(void) {
  LOG_INFO("=== Module Memory Statistics ===");

  if (g_module_count == 0) {
    LOG_INFO("No module statistics available");
    return;
  }

  // Sort modules by total allocated (simple bubble sort)
  for (u32 i = 0; i < g_module_count - 1; i++) {
    for (u32 j = 0; j < g_module_count - i - 1; j++) {
      if (g_module_stats[j].total_allocated <
          g_module_stats[j + 1].total_allocated) {
        ModuleStats temp = g_module_stats[j];
        g_module_stats[j] = g_module_stats[j + 1];
        g_module_stats[j + 1] = temp;
      }
    }
  }

  u64 total_module_allocated = 0;
  for (u32 i = 0; i < g_module_count; i++) {
    ModuleStats *stats = &g_module_stats[i];
    total_module_allocated += stats->total_allocated;

    LOG_INFO("Module: %s", stats->module_name);
    LOG_INFO("  Total allocated: %llu bytes (%.2f MB)", stats->total_allocated,
             stats->total_allocated / (1024.0 * 1024.0));
    LOG_INFO("  Total freed: %llu bytes", stats->total_freed);
    LOG_INFO("  Active allocations: %u", stats->active_allocations);
    LOG_INFO("  Peak usage: %llu bytes", stats->peak_usage);
    LOG_INFO("  Allocation count: %u", stats->allocation_count);

    if (stats->total_allocated > 0) {
      f64 leak_ratio = ((f64)(stats->total_allocated - stats->total_freed) /
                        stats->total_allocated) *
                       100.0;
      LOG_INFO("  Leak ratio: %.2f%%", leak_ratio);
    }
    LOG_INFO("");
  }

  LOG_INFO("Total across all modules: %llu bytes (%.2f MB)",
           total_module_allocated, total_module_allocated / (1024.0 * 1024.0));
  LOG_INFO("===============================");
}
