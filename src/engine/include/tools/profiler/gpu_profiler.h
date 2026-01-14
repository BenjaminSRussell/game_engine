// tools/profiler/gpu_profiler.h
// Metal GPU profiling with Instruments integration
#ifndef GPU_PROFILER_H
#define GPU_PROFILER_H

#include <common.h>
#include <Metal/Metal.h>

#define GPU_PROFILER_MAX_MARKERS 256
#define GPU_PROFILER_HISTORY_SIZE 120 // 120 frames

// GPU timing marker
typedef struct {
  char name[64];
  f64 start_time;
  f64 end_time;
  f64 duration_ms;
  u32 draw_calls;
  u32 triangles;
  u32 vertices;
} GPUMarker;

// Frame stats
typedef struct {
  f64 frame_time_ms;
  f64 cpu_time_ms;
  f64 gpu_time_ms;

  u32 total_draw_calls;
  u32 total_triangles;
  u32 total_vertices;
  u32 texture_memory_mb;
  u32 buffer_memory_mb;

  // Per-stage timings
  f64 geometry_pass_ms;
  f64 lighting_pass_ms;
  f64 post_process_ms;
  f64 ui_pass_ms;

  GPUMarker markers[GPU_PROFILER_MAX_MARKERS];
  u32 marker_count;

} GPUFrameStats;

typedef struct {
  id<MTLDevice> device;
  id<MTLCounterSampleBuffer> counter_buffer;

  // Frame history
  GPUFrameStats frame_history[GPU_PROFILER_HISTORY_SIZE];
  u32 current_frame_index;

  // Current frame
  GPUFrameStats *current_frame;
  u32 current_marker_depth;

  // Capture
  bool is_capturing;
  char capture_path[512];

} GPUProfiler;

#ifdef __cplusplus
extern "C" {
#endif

// Profiler lifecycle
GPUProfiler *gpu_profiler_create(id<MTLDevice> device);
void gpu_profiler_destroy(GPUProfiler *profiler);

// Frame markers
void gpu_profiler_begin_frame(GPUProfiler *profiler, id<MTLCommandBuffer> cmd);
void gpu_profiler_end_frame(GPUProfiler *profiler);

// GPU markers (push/pop)
void gpu_profiler_push(GPUProfiler *profiler, id<MTLCommandEncoder> encoder,
                       const char *name);
void gpu_profiler_pop(GPUProfiler *profiler, id<MTLCommandEncoder> encoder);

// Stats recording
void gpu_profiler_record_draw(GPUProfiler *profiler, u32 triangles,
                              u32 vertices);

// Analysis
const GPUFrameStats *gpu_profiler_get_frame(GPUProfiler *profiler,
                                            u32 frame_offset);
f64 gpu_profiler_get_avg_frame_time(GPUProfiler *profiler, u32 num_frames);
const GPUMarker *gpu_profiler_find_bottleneck(GPUProfiler *profiler);

// Capture (save to file for Instruments)
void gpu_profiler_start_capture(GPUProfiler *profiler, const char *path);
void gpu_profiler_stop_capture(GPUProfiler *profiler);

// Live visualization data
void gpu_profiler_get_timeline_data(GPUProfiler *profiler, f32 *out_data,
                                    u32 max_points);

#ifdef __cplusplus
}
#endif

#endif // GPU_PROFILER_H
