/**
 * MEMORY TRACKER & LEAK DETECTION
 * Allocation Tracing
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  void *ptr;
  size_t size;
  const char *file;
  int line;
  int frame;
} AllocInfo;

#define MAX_TRACKS 100000
AllocInfo g_allocs[MAX_TRACKS];
int g_alloc_count = 0;

// Hook
void *p_malloc(size_t size, const char *file, int line) {
  void *ptr = malloc(size);
  // Record
  g_allocs[g_alloc_count++] = (AllocInfo){ptr, size, file, line, 0};
  return ptr;
}

// Dump Leaks
void debug_dump_leaks() {
  printf("--- MEMORY LEAKS ---\n");
  for (int i = 0; i < g_alloc_count; i++) {
    if (g_allocs[i].ptr != NULL) {
      printf("Leak: %zu bytes at %s:%d\n",
             g_allocs[i].size, g_allocs[i].file, g_allocs[i].line);
    }
  }
}

/*
 * IMPLEMENTATION: 500/1000 Memory TODOs
 * LOC: ~40
 */
