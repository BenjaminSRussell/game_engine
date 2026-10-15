// tests/unit/profiling/gpu_profiler_test.c
//
// Unit tests for the GPU timeline profiler's marker recording: begin/end events,
// timestamp query bookkeeping, per-category statistics and frame pruning.
//
// Not Engine-linked: Engine does not build on origin/main. This test is compiled
// together with src/engine/core/profiling/gpu_profiler.c, and the memory, logger and
// GPU backend symbols that unit needs are stubbed below. The fake backend stamps each
// timestamp query with a clock that advances 1 ms per query, so a marker that spans
// queries i..j lasts (j - i) ms.

#include "core/logging/unified_logger.h"
#include "core/memory/unified_allocator.h"
#include "core/profiling/gpu_profiler.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// STUBS: memory
// ============================================================================

Allocator *g_default_allocator = NULL;
Allocator *g_temp_allocator = NULL;
Allocator *g_persistent_allocator = NULL;

void *allocator_alloc(Allocator *allocator, size_t size) {
  (void)allocator;
  return calloc(1, size ? size : 1);
}

void allocator_free(Allocator *allocator, void *ptr) {
  (void)allocator;
  free(ptr);
}

// ============================================================================
// STUBS: logger. Formatted lines are kept so tests can read the timeline output.
// ============================================================================

#define CAPTURED_LINES 128
#define CAPTURED_WIDTH 192

static char g_lines[CAPTURED_LINES][CAPTURED_WIDTH];
static int g_line_count = 0;

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  if (g_line_count >= CAPTURED_LINES) {
    return;
  }
  va_list args;
  va_start(args, format);
  vsnprintf(g_lines[g_line_count], CAPTURED_WIDTH, format, args);
  va_end(args);
  g_line_count++;
}

static bool logged(const char *text) {
  for (int i = 0; i < g_line_count; i++) {
    if (strstr(g_lines[i], text) != NULL) {
      return true;
    }
  }
  return false;
}

// ============================================================================
// STUBS: GPU timestamp backend. Query k reads (k + 1) ms within a session.
// ============================================================================

#define QUERY_SLOTS 64

static u64 g_slot_ns[QUERY_SLOTS];
static u64 g_clock_ns = 1000000u; // never 0, so no stamp looks unset
static u32 g_queries_issued = 0;
static u32 g_pools_created = 0;
static u32 g_pools_destroyed = 0;

u32 gpu_create_timestamp_query_pool(u32 query_count) {
  (void)query_count;
  g_pools_created++;
  return 1;
}

void gpu_destroy_timestamp_query_pool(u32 pool) {
  (void)pool;
  g_pools_destroyed++;
}

void gpu_timestamp_query(u32 pool, u32 index) {
  (void)pool;
  if (index < QUERY_SLOTS) {
    g_slot_ns[index] = g_clock_ns;
  }
  g_clock_ns += 1000000u;
  g_queries_issued++;
}

void gpu_get_timestamp_query_results(u32 pool, u32 start_index, u32 count, u64 *results) {
  (void)pool;
  for (u32 i = 0; i < count; i++) {
    results[i] = g_slot_ns[start_index + i];
  }
}

// ============================================================================
// CHECKS
// ============================================================================

static int g_failed = 0;

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
    }                                                                          \
  } while (0)

static void check_ms(f32 actual, f32 expected, const char *what) {
  if (actual < expected - 0.001f || actual > expected + 0.001f) {
    fprintf(stderr, "FAIL %s: expected %.3f ms, got %.3f ms\n", what,
            (double)expected, (double)actual);
    g_failed++;
  }
}

// ============================================================================
// SESSION HELPERS
// ============================================================================

// Fresh global profiler with capture on and frame 1 open.
static void start_session(u32 max_queries_per_frame, u32 frame_delay) {
  gpu_profiler_shutdown();
  g_queries_issued = 0;
  g_line_count = 0;
  CHECK(gpu_profiler_initialize(max_queries_per_frame, frame_delay), "initialize");
  gpu_profiler_begin_capture();
  gpu_profiler_begin_frame();
}

static GPUProfilerStats stats_now(void) {
  GPUProfilerStats stats;
  memset(&stats, 0, sizeof(stats));
  gpu_profiler_get_statistics(&stats);
  return stats;
}

// ============================================================================
// TESTS
// ============================================================================

static void test_context_create_and_destroy(void) {
  g_pools_created = 0;
  g_pools_destroyed = 0;

  GPUProfilerContext *ctx = gpu_profiler_create(4, 0);
  CHECK(ctx != NULL, "create returns a context");
  CHECK(g_pools_created == 1, "create allocates one timestamp query pool");

  gpu_profiler_destroy(ctx);
  CHECK(g_pools_destroyed == 1, "destroy releases the query pool");

  gpu_profiler_destroy(NULL);
  CHECK(g_pools_destroyed == 1, "destroy(NULL) releases nothing");
}

static void test_global_lifecycle(void) {
  gpu_profiler_shutdown();
  g_pools_destroyed = 0;

  CHECK(gpu_profiler_get_global() == NULL, "no global profiler before initialize");
  CHECK(gpu_profiler_initialize(4, 0), "initialize succeeds");

  GPUProfilerContext *first = gpu_profiler_get_global();
  CHECK(first != NULL, "initialize installs a global profiler");
  CHECK(gpu_profiler_initialize(4, 0), "second initialize is accepted");
  CHECK(gpu_profiler_get_global() == first, "second initialize keeps the existing profiler");

  gpu_profiler_shutdown();
  CHECK(gpu_profiler_get_global() == NULL, "shutdown clears the global profiler");
  CHECK(g_pools_destroyed == 1, "shutdown releases the query pool");
}

static void test_markers_ignored_outside_capture(void) {
  gpu_profiler_shutdown();
  g_queries_issued = 0;
  CHECK(gpu_profiler_initialize(4, 0), "initialize");

  GPUProfileHandle before = gpu_profiler_begin_event("Idle", GPU_PROFILE_CATEGORY_RENDERING);
  CHECK(before == 0, "begin before begin_capture returns 0");
  gpu_profiler_end_event(before);
  CHECK(g_queries_issued == 0, "no timestamp queries before begin_capture");

  gpu_profiler_begin_capture();
  gpu_profiler_begin_frame();
  gpu_profiler_end_capture();
  GPUProfileHandle after = gpu_profiler_begin_event("Idle", GPU_PROFILE_CATEGORY_RENDERING);
  CHECK(after == 0, "begin after end_capture returns 0");
  CHECK(g_queries_issued == 0, "no timestamp queries after end_capture");
  CHECK(stats_now().entries_count == 0, "no markers recorded outside capture");

  gpu_profiler_shutdown();
}

static void test_sequential_markers_record_durations(void) {
  start_session(4, 0);

  GPUProfileHandle clear = gpu_profiler_begin_event("Clear", GPU_PROFILE_CATEGORY_RENDERING);
  gpu_profiler_end_event(clear);
  GPUProfileHandle lighting = gpu_profiler_begin_event("Lighting", GPU_PROFILE_CATEGORY_COMPUTE);
  gpu_profiler_end_event(lighting);
  gpu_profiler_end_capture();

  GPUProfilerStats stats = stats_now();
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_RENDERING], 1.0f, "Rendering total");
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_COMPUTE], 1.0f, "Compute total");
  CHECK(stats.category_counts[GPU_PROFILE_CATEGORY_RENDERING] == 1, "one Rendering marker");
  CHECK(stats.category_counts[GPU_PROFILE_CATEGORY_COMPUTE] == 1, "one Compute marker");
  CHECK(stats.entries_count == 2, "both markers are recorded");

  gpu_profiler_print_timeline();
  CHECK(logged("[Rendering] Clear: 1.00 ms (frame 1)"), "timeline lists Clear");
  CHECK(logged("[Compute] Lighting: 1.00 ms (frame 1)"), "timeline lists Lighting");

  gpu_profiler_shutdown();
}

static void shadows_scope(void) {
  GPU_PROFILE_SHADOWS("Shadows");
  GPU_PROFILE_END_SCOPE();
}

static void frame_scope(void) {
  GPU_PROFILE_RENDERING("Frame");
  shadows_scope();
  GPU_PROFILE_END_SCOPE();
}

static void test_nested_scopes_pair_with_their_own_end(void) {
  start_session(4, 0);

  // Queries: Frame begin (0), Shadows begin (1), Shadows end (2), Frame end (3).
  frame_scope();
  gpu_profiler_end_capture();

  GPUProfilerStats stats = stats_now();
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_RENDERING], 3.0f,
           "Frame spans its nested scope");
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_SHADOWS], 1.0f,
           "Shadows is only its own 1 ms");

  gpu_profiler_print_timeline();
  CHECK(logged("[Rendering] Frame: 3.00 ms (frame 1)"), "timeline reports Frame as 3 ms");
  CHECK(logged("[Shadows] Shadows: 1.00 ms (frame 1)"), "timeline reports Shadows as 1 ms");

  gpu_profiler_shutdown();
}

static void test_overlapping_markers_pair_by_handle(void) {
  start_session(4, 0);

  GPUProfileHandle a = gpu_profiler_begin_event("A", GPU_PROFILE_CATEGORY_RENDERING);
  GPUProfileHandle b = gpu_profiler_begin_event("B", GPU_PROFILE_CATEGORY_SHADOWS);
  CHECK(a != 0 && b != 0 && a != b, "overlapping markers get distinct non-zero handles");
  gpu_profiler_end_event(a); // A ends before B does
  gpu_profiler_end_event(b);
  gpu_profiler_end_capture();

  GPUProfilerStats stats = stats_now();
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_RENDERING], 2.0f,
           "A runs from its begin to its end");
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_SHADOWS], 2.0f,
           "B runs from its begin to its end");

  gpu_profiler_shutdown();
}

static void test_unterminated_marker_is_not_reported(void) {
  start_session(4, 0);

  gpu_profiler_begin_event("Open", GPU_PROFILE_CATEGORY_RENDERING); // never ended
  GPUProfileHandle closed = gpu_profiler_begin_event("Closed", GPU_PROFILE_CATEGORY_SHADOWS);
  gpu_profiler_end_event(closed);
  gpu_profiler_end_capture();

  GPUProfilerStats stats = stats_now();
  CHECK(stats.category_counts[GPU_PROFILE_CATEGORY_RENDERING] == 0, "open marker is not counted");
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_RENDERING], 0.0f,
           "open marker has no duration");
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_SHADOWS], 1.0f,
           "closed sibling still reports 1 ms");

  gpu_profiler_print_timeline();
  CHECK(!logged("Open:"), "open marker is absent from the timeline");
  CHECK(logged("[Shadows] Closed: 1.00 ms (frame 1)"), "closed marker is in the timeline");

  gpu_profiler_shutdown();
}

static void test_query_limit_refuses_markers_without_recording(void) {
  start_session(1, 0); // two timestamp queries per frame

  GPUProfileHandle a = gpu_profiler_begin_event("A", GPU_PROFILE_CATEGORY_RENDERING);
  gpu_profiler_end_event(a); // the frame's query budget is now used up
  GPUProfileHandle refused = gpu_profiler_begin_event("B", GPU_PROFILE_CATEGORY_RENDERING);
  CHECK(refused == 0, "begin past the query budget returns 0");
  gpu_profiler_end_event(refused);
  CHECK(g_queries_issued == 2, "refused marker issues no timestamp query");
  gpu_profiler_end_capture();

  GPUProfilerStats stats = stats_now();
  CHECK(stats.entries_count == 1, "only the accepted marker is recorded");
  CHECK(stats.category_counts[GPU_PROFILE_CATEGORY_RENDERING] == 1, "one Rendering marker");
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_RENDERING], 1.0f,
           "accepted marker keeps its 1 ms");

  gpu_profiler_shutdown();
}

static void test_end_capture_counts_each_marker_once(void) {
  start_session(4, 0);

  GPUProfileHandle h = gpu_profiler_begin_event("Once", GPU_PROFILE_CATEGORY_RENDERING);
  gpu_profiler_end_event(h);
  gpu_profiler_end_capture();
  gpu_profiler_end_capture(); // the same queries are read back again

  GPUProfilerStats stats = stats_now();
  CHECK(stats.category_counts[GPU_PROFILE_CATEGORY_RENDERING] == 1, "a marker is counted once");
  check_ms(stats.category_times_ms[GPU_PROFILE_CATEGORY_RENDERING], 1.0f,
           "a marker keeps its 1 ms");

  gpu_profiler_shutdown();
}

static void test_frames_older_than_delay_are_pruned(void) {
  start_session(4, 0); // frame 1

  GPUProfileHandle h = gpu_profiler_begin_event("Early", GPU_PROFILE_CATEGORY_RENDERING);
  gpu_profiler_end_event(h);
  gpu_profiler_end_frame();   // processes frame 1 and restarts capture
  gpu_profiler_begin_frame(); // frame 2 keeps frame 1 when frame_delay is 0
  CHECK(stats_now().entries_count == 1, "frame 1 marker kept during frame 2");

  gpu_profiler_end_frame();
  gpu_profiler_begin_frame(); // frame 3 drops frame 1
  CHECK(stats_now().entries_count == 0, "frame 1 marker pruned during frame 3");

  gpu_profiler_shutdown();
}

static void test_markers_match_their_own_frame(void) {
  start_session(4, 1); // frame_delay 1: frame 1 is not processed at its end

  GPUProfileHandle early = gpu_profiler_begin_event("Early", GPU_PROFILE_CATEGORY_RENDERING);
  gpu_profiler_end_event(early);
  gpu_profiler_end_frame();   // frame 1: not processed yet
  gpu_profiler_begin_frame(); // frame 2
  gpu_profiler_end_frame();   // frame 2: no markers
  gpu_profiler_begin_frame(); // frame 3

  GPUProfileHandle late = gpu_profiler_begin_event("Late", GPU_PROFILE_CATEGORY_RENDERING);
  gpu_profiler_end_event(late);
  gpu_profiler_end_frame();   // frame 3: processed against its own queries

  gpu_profiler_print_timeline();
  CHECK(logged("[Rendering] Late: 1.00 ms (frame 3)"), "frame 3 marker gets frame 3 timestamps");
  CHECK(!logged("Early:"), "frame 1 marker is not matched to frame 3 timestamps");

  gpu_profiler_shutdown();
}

int main(void) {
  test_context_create_and_destroy();
  test_global_lifecycle();
  test_markers_ignored_outside_capture();
  test_sequential_markers_record_durations();
  test_nested_scopes_pair_with_their_own_end();
  test_overlapping_markers_pair_by_handle();
  test_unterminated_marker_is_not_reported();
  test_query_limit_refuses_markers_without_recording();
  test_end_capture_counts_each_marker_once();
  test_frames_older_than_delay_are_pruned();
  test_markers_match_their_own_frame();

  if (g_failed != 0) {
    fprintf(stderr, "test_gpu_profiler: %d check(s) failed\n", g_failed);
    return 1;
  }
  printf("test_gpu_profiler: all checks passed\n");
  return 0;
}
