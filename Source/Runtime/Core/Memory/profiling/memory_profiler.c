// memory_profiler.c - Memory profiling implementation
#include "memory_profiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MemoryProfiler *memory_profiler_create(void) {
  MemoryProfiler *profiler = (MemoryProfiler *)malloc(sizeof(MemoryProfiler));
  if (!profiler)
    return NULL;

  memset(profiler, 0, sizeof(MemoryProfiler));
  profiler->enabled = true;

  return profiler;
}

void memory_profiler_record_alloc(MemoryProfiler *profiler, int tag,
                                  size_t size) {
  if (!profiler || !profiler->enabled)
    return;
  if (tag < 0 || tag >= MAX_MEMORY_TAGS)
    return;

  MemoryTagStats *stats = &profiler->tag_stats[tag];
  stats->allocation_count++;
  stats->current_bytes += size;
  stats->total_allocated += size;

  if (stats->current_bytes > stats->peak_bytes) {
    stats->peak_bytes = stats->current_bytes;
  }
}

void memory_profiler_record_free(MemoryProfiler *profiler, int tag,
                                 size_t size) {
  if (!profiler || !profiler->enabled)
    return;
  if (tag < 0 || tag >= MAX_MEMORY_TAGS)
    return;

  MemoryTagStats *stats = &profiler->tag_stats[tag];
  stats->free_count++;

  if (stats->current_bytes >= size) {
    stats->current_bytes -= size;
  } else {
    stats->current_bytes = 0;
  }
}

void memory_profiler_report(MemoryProfiler *profiler) {
  if (!profiler)
    return;

  printf("\n=== Memory Profiler Report ===\n");
  printf("%-20s %10s %10s %12s %12s %15s\n", "Tag", "Allocs", "Frees",
         "Current", "Peak", "Total");
  printf("---------------------------------------------------------------------"
         "-----------\n");

  for (int i = 0; i < MAX_MEMORY_TAGS; i++) {
    MemoryTagStats *stats = &profiler->tag_stats[i];

    if (stats->allocation_count > 0 || stats->current_bytes > 0) {
      printf("%-20d %10zu %10zu %12zu %12zu %15zu\n", i,
             stats->allocation_count, stats->free_count, stats->current_bytes,
             stats->peak_bytes, stats->total_allocated);
    }
  }

  printf("==============================\n\n");
}

MemoryTagStats *memory_profiler_get_tag_stats(MemoryProfiler *profiler,
                                              int tag) {
  if (!profiler || tag < 0 || tag >= MAX_MEMORY_TAGS)
    return NULL;
  return &profiler->tag_stats[tag];
}

void memory_profiler_enable(MemoryProfiler *profiler) {
  if (profiler)
    profiler->enabled = true;
}

void memory_profiler_disable(MemoryProfiler *profiler) {
  if (profiler)
    profiler->enabled = false;
}

void memory_profiler_reset(MemoryProfiler *profiler) {
  if (!profiler)
    return;
  memset(profiler->tag_stats, 0, sizeof(profiler->tag_stats));
}

void memory_profiler_destroy(MemoryProfiler *profiler) {
  if (!profiler)
    return;
  free(profiler);
}
