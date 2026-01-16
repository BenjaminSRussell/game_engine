#ifndef MEMORY_PROFILER_H
#define MEMORY_PROFILER_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_MEMORY_TAGS 64

// Memory profiling by tag
typedef struct MemoryTagStats {
  size_t allocation_count;
  size_t free_count;
  size_t current_bytes;
  size_t peak_bytes;
  size_t total_allocated;
} MemoryTagStats;

typedef struct MemoryProfiler {
  MemoryTagStats tag_stats[MAX_MEMORY_TAGS];
  bool enabled;
} MemoryProfiler;

// Create memory profiler
MemoryProfiler *memory_profiler_create(void);

// Record allocation
void memory_profiler_record_alloc(MemoryProfiler *profiler, int tag,
                                  size_t size);

// Record deallocation
void memory_profiler_record_free(MemoryProfiler *profiler, int tag,
                                 size_t size);

// Generate profiling report
void memory_profiler_report(MemoryProfiler *profiler);

// Get tag statistics
MemoryTagStats *memory_profiler_get_tag_stats(MemoryProfiler *profiler,
                                              int tag);

// Enable/disable profiling
void memory_profiler_enable(MemoryProfiler *profiler);
void memory_profiler_disable(MemoryProfiler *profiler);

// Reset statistics
void memory_profiler_reset(MemoryProfiler *profiler);

// Destroy profiler
void memory_profiler_destroy(MemoryProfiler *profiler);

#endif // MEMORY_PROFILER_H
