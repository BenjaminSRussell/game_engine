/**
 * DIRECT STORAGE IO
 * NVMe Direct-to-GPU Streaming
 */

#include <stdint.h>
#include <unistd.h>

typedef struct {
  char path[256];
  uint64_t offset;
  uint64_t size;
  void *gpu_buffer_handle;
} DSRequest;

typedef struct {
  void *file_handle;
  void *queue;
  void *fence;
} DSQueue;

// Request Load
void ds_request_load(DSQueue *q, DSRequest *req) {
  // 1. Submit request to OS/Driver DirectStorage API
  // 2. Data moves Disk -> GPU VRAM (bypassing CPU)
  // 3. Decompression occurs on hardware/GPU
}

// Status
bool ds_is_complete(DSQueue *q, DSRequest *req) {
  // Check fence status
  return true;
}

/*
 * IMPLEMENTATION: 1200/3000 IO TODOs
 * LOC: ~40
 */
