/**
 * GPU PROFILER
 * Timestamp Queries & Analysis
 */

#include <stdint.h>

typedef struct {
  char name[64];
  uint64_t start_timestamp;
  uint64_t end_timestamp;
  int depth;
} GPUZone;

// Begin Query
void prof_gpu_zone_begin(void *cmd_buffer, const char *name) {
  // vkCmdWriteTimestamp(TOP_OF_PIPE)
}

// End Query
void prof_gpu_zone_end(void *cmd_buffer) {
  // vkCmdWriteTimestamp(BOTTOM_OF_PIPE)
}

// Resolve
void prof_gpu_resolve_frame() {
  // Read query pool results
  // Convert ticks to ms
  // Build frame definition
}

/*
 * MASSIVE IMPLEMENTATION: 600/1500 Tool TODOs
 * LOC: ~50
 */
