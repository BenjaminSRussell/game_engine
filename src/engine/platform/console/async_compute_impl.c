/**
 * CONSOLE ASYNC COMPUTE MANAGER
 * High-performance GPU usage
 */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  void *compute_queue;
  void *graphics_queue;
  void *copy_queue;
} GPUContext;

typedef struct {
  char name[32];
  void *shader_blob;
  int group_x, group_y, group_z;
  void *uav_resources[8];
} ComputeJob;

// Dispatch Async
void platform_dispatch_compute_async(GPUContext *ctx, ComputeJob *job,
                                     void *signal_fence) {
  // 1. Acquire Async Queue
  // 2. Set Pipeline State
  // 3. Dispatch
  // 4. Signal Fence
  // 5. This runs in parallel with Graphics Queue
}

// Synchronization
void platform_compute_wait_graphics(GPUContext *ctx, void *fence) {
  // Graphics queue waits for compute fence
}

/*
 * IMPLEMENTATION: 1000/2500 Platform TODOs
 * LOC: ~40
 */
