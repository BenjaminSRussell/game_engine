#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H

#include <stdbool.h>
#include <stddef.h>

// Memory allocation tracking for leak detection
typedef struct AllocationInfo {
  void *ptr;
  size_t size;
  const char *file;
  int line;
  int tag;
  struct AllocationInfo *next;
} AllocationInfo;

typedef struct MemoryTracker {
  AllocationInfo *allocations;
  size_t total_allocations;
  size_t total_frees;
  size_t current_bytes;
  size_t peak_bytes;
  bool enabled;
} MemoryTracker;

// Create memory tracker
MemoryTracker *memory_tracker_create(void);

// Track allocation
void memory_tracker_add(MemoryTracker *tracker, void *ptr, size_t size,
                        const char *file, int line, int tag);

// Track deallocation
void memory_tracker_remove(MemoryTracker *tracker, void *ptr);

// Generate leak report
void memory_tracker_report(MemoryTracker *tracker);

// Get statistics
size_t memory_tracker_get_current_bytes(MemoryTracker *tracker);
size_t memory_tracker_get_peak_bytes(MemoryTracker *tracker);
size_t memory_tracker_get_allocation_count(MemoryTracker *tracker);

// Enable/disable tracking
void memory_tracker_enable(MemoryTracker *tracker);
void memory_tracker_disable(MemoryTracker *tracker);

// Destroy tracker
void memory_tracker_destroy(MemoryTracker *tracker);

#endif // MEMORY_TRACKER_H
