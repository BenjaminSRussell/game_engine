#include "rendering/parallel_rendering.h"
#include "core/thread_pool.h"
#include "core/threading/parallel_utils.h"
#include "engine/include/core/logger.h"
#include <time.h>

static ParallelRenderStats g_stats = {0};

static inline u64 get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

void parallel_rendering_init(u32 thread_count) {
  // Thread pool should already be initialized from Phase 3
  ThreadPool *pool = thread_pool_get_global();

  if (!pool) {
    thread_pool_init(thread_count);
    LOG_INFO("[Render] Initialized thread pool with %u threads", thread_count);
  }

  g_stats.worker_threads = thread_count;
  LOG_INFO("[Render] Parallel rendering initialized");
}

void parallel_rendering_shutdown(void) {
  LOG_INFO("[Render] Parallel rendering shutdown");
}

// Culling worker function
typedef struct {
  u32 start;
  u32 end;
  bool *visible;
} CullContext;

static void cull_batch(u32 index, void *user_data) {
  CullContext *ctx = (CullContext *)user_data;

  // Placeholder culling logic
  // In real implementation, check against frustum
  ctx->visible[index] = true; // Always visible for now
}

void parallel_cull_meshes(u32 mesh_count, bool *visible_out) {
  if (mesh_count == 0 || !visible_out)
    return;

  u64 start_time = get_time_ns();

  CullContext ctx = {.start = 0, .end = mesh_count, .visible = visible_out};

  // Use parallel-for from Phase 3
  parallel_for(0, mesh_count, cull_batch, &ctx);

  u64 end_time = get_time_ns();

  // Update statistics
  g_stats.meshes_culled = mesh_count;
  g_stats.meshes_visible = mesh_count; // Placeholder
  g_stats.culling_time_ms = (end_time - start_time) / 1000000.0;
}

// Command generation worker
static void generate_command_batch(u32 index, void *user_data) {
  // Placeholder command generation
  // In real implementation, create draw commands
}

void parallel_generate_commands(u32 mesh_count) {
  if (mesh_count == 0)
    return;

  u64 start_time = get_time_ns();

  // Use parallel-for from Phase 3
  parallel_for(0, mesh_count, generate_command_batch, NULL);

  u64 end_time = get_time_ns();

  g_stats.command_gen_time_ms = (end_time - start_time) / 1000000.0;
}

ParallelRenderStats parallel_rendering_get_stats(void) { return g_stats; }

void parallel_rendering_print_stats(void) {
  printf("\n=== Parallel Rendering Statistics ===\n");
  printf("Worker Threads: %u\n", g_stats.worker_threads);
  printf("Meshes Culled: %u\n", g_stats.meshes_culled);
  printf("Meshes Visible: %u\n", g_stats.meshes_visible);
  printf("Culling Time: %.3f ms\n", g_stats.culling_time_ms);
  printf("Command Gen Time: %.3f ms\n", g_stats.command_gen_time_ms);
  printf("====================================\n\n");
}
