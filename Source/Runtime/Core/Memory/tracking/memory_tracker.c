// memory_tracker.c - Memory allocation tracking implementation
#include "memory_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MemoryTracker *memory_tracker_create(void) {
  MemoryTracker *tracker = (MemoryTracker *)malloc(sizeof(MemoryTracker));
  if (!tracker)
    return NULL;

  tracker->allocations = NULL;
  tracker->total_allocations = 0;
  tracker->total_frees = 0;
  tracker->current_bytes = 0;
  tracker->peak_bytes = 0;
  tracker->enabled = true;

  return tracker;
}

void memory_tracker_add(MemoryTracker *tracker, void *ptr, size_t size,
                        const char *file, int line, int tag) {
  if (!tracker || !tracker->enabled || !ptr)
    return;

  AllocationInfo *info = (AllocationInfo *)malloc(sizeof(AllocationInfo));
  if (!info)
    return;

  info->ptr = ptr;
  info->size = size;
  info->file = file;
  info->line = line;
  info->tag = tag;
  info->next = tracker->allocations;

  tracker->allocations = info;
  tracker->total_allocations++;
  tracker->current_bytes += size;

  if (tracker->current_bytes > tracker->peak_bytes) {
    tracker->peak_bytes = tracker->current_bytes;
  }
}

void memory_tracker_remove(MemoryTracker *tracker, void *ptr) {
  if (!tracker || !tracker->enabled || !ptr)
    return;

  AllocationInfo **current = &tracker->allocations;
  while (*current) {
    if ((*current)->ptr == ptr) {
      AllocationInfo *to_free = *current;
      *current = (*current)->next;

      tracker->total_frees++;
      tracker->current_bytes -= to_free->size;

      free(to_free);
      return;
    }
    current = &(*current)->next;
  }

  // Pointer not found - potential double free or invalid pointer
  fprintf(stderr, "Warning: Attempted to free untracked pointer %p\n", ptr);
}

void memory_tracker_report(MemoryTracker *tracker) {
  if (!tracker)
    return;

  printf("\n=== Memory Tracker Report ===\n");
  printf("Total Allocations: %zu\n", tracker->total_allocations);
  printf("Total Frees: %zu\n", tracker->total_frees);
  printf("Current Bytes: %zu\n", tracker->current_bytes);
  printf("Peak Bytes: %zu\n", tracker->peak_bytes);

  if (tracker->allocations) {
    printf("\n=== Memory Leaks Detected ===\n");
    AllocationInfo *current = tracker->allocations;
    size_t leak_count = 0;
    size_t leak_bytes = 0;

    while (current) {
      printf("Leak #%zu: %zu bytes at %p\n", leak_count + 1, current->size,
             current->ptr);
      if (current->file) {
        printf("  Allocated at: %s:%d\n", current->file, current->line);
      }
      printf("  Tag: %d\n", current->tag);

      leak_count++;
      leak_bytes += current->size;
      current = current->next;
    }

    printf("\nTotal Leaks: %zu allocations, %zu bytes\n", leak_count,
           leak_bytes);
  } else {
    printf("\nNo memory leaks detected!\n");
  }

  printf("=============================\n\n");
}

size_t memory_tracker_get_current_bytes(MemoryTracker *tracker) {
  return tracker ? tracker->current_bytes : 0;
}

size_t memory_tracker_get_peak_bytes(MemoryTracker *tracker) {
  return tracker ? tracker->peak_bytes : 0;
}

size_t memory_tracker_get_allocation_count(MemoryTracker *tracker) {
  if (!tracker)
    return 0;

  size_t count = 0;
  AllocationInfo *current = tracker->allocations;
  while (current) {
    count++;
    current = current->next;
  }
  return count;
}

void memory_tracker_enable(MemoryTracker *tracker) {
  if (tracker)
    tracker->enabled = true;
}

void memory_tracker_disable(MemoryTracker *tracker) {
  if (tracker)
    tracker->enabled = false;
}

void memory_tracker_destroy(MemoryTracker *tracker) {
  if (!tracker)
    return;

  // Free all tracked allocations (report leaks first)
  AllocationInfo *current = tracker->allocations;
  while (current) {
    AllocationInfo *next = current->next;
    free(current);
    current = next;
  }

  free(tracker);
}
