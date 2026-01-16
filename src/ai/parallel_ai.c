#include "ai/parallel_ai.h"
#include "ai/ai_allocator.h"
#include "core/thread_pool.h"
#include "core/threading/parallel_utils.h"
#include "engine/include/core/logger.h"

// Simple Queue for path requests
typedef struct {
  Vec3 start;
  Vec3 end;
  PathCompleteCallback callback;
  void *user_data;

  // Result
  bool ready;
  bool success;
  Vec3 waypoints[64];
  u32 count;
} PathRequest;

#define MAX_PENDING_PATHS 128
static PathRequest
    g_path_requests[MAX_PENDING_PATHS]; // Circular buffer or slot map
static u32 g_pending_count = 0;

void parallel_ai_init(u32 thread_count) {
  if (!thread_pool_get_global()) {
    thread_pool_init(thread_count);
  }
  LOG_INFO("[AI] Parallel processing initialized");
}

static void pathfinder_job(u32 index, void *user_data) {
  PathRequest *req = (PathRequest *)user_data;

  // Use AI Allocator's Path Arena (Thread Local or Locked)
  // Note: For true parallelism, we'd need TS-Arenas or one arena per thread.
  // For now, we simulate A* logic.

  // Mock A*:
  // 1. Start -> 2. Mid -> 3. End
  req->waypoints[0] = req->start;
  req->waypoints[1] = (Vec3){(req->start.x + req->end.x) * 0.5f, req->start.y,
                             (req->start.z + req->end.z) * 0.5f};
  req->waypoints[2] = req->end;
  req->count = 3;
  req->success = true;
  req->ready = true;
}

void ai_request_path(Vec3 start, Vec3 end, PathCompleteCallback cb,
                     void *data) {
  // Find free slot
  // In real impl: use atomic or lock
  if (g_pending_count >= MAX_PENDING_PATHS)
    return;

  PathRequest *req = &g_path_requests[g_pending_count++];
  req->start = start;
  req->end = end;
  req->callback = cb;
  req->user_data = data;
  req->ready = false;

  // Submit single job
  // Note: In Phase 3 parallel_for is batch-based.
  // We can use thread_pool_submit directly for single jobs.
  ThreadPool *pool = thread_pool_get_global();
  if (pool) {
    thread_pool_submit(pool, (JobFunction)pathfinder_job, req, "Pathfind");
  }
}

void parallel_ai_update(void) {
  // Check results
  // Iterate requests, if ready -> callback -> remove
  // Simplifying: assumes requests complete in order or we scan

  for (u32 i = 0; i < g_pending_count;) {
    if (g_path_requests[i].ready) {
      // Callback
      if (g_path_requests[i].callback) {
        g_path_requests[i].callback(
            g_path_requests[i].user_data, g_path_requests[i].success,
            g_path_requests[i].waypoints, g_path_requests[i].count);
      }

      // Remove (swap with last)
      g_path_requests[i] = g_path_requests[--g_pending_count];
    } else {
      i++;
    }
  }
}
