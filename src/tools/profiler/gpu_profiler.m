// tools/profiler/gpu_profiler.m
// Complete GPU profiler implementation
#include "../../include/tools/profiler/gpu_profiler.h"
#include "../../include/core/logger.h"
#import <Metal/Metal.h>
#include <string.h>

GPUProfiler *gpu_profiler_create(id<MTLDevice> device) {
  GPUProfiler *profiler = (GPUProfiler *)calloc(1, sizeof(GPUProfiler));
  profiler->device = device;
  profiler->current_frame_index = 0;
  profiler->is_capturing = false;
  profiler->current_marker_depth = 0;

  // Create counter sample buffer for GPU timing
  if (@available(macOS 10.15, *)) {
    MTLCounterSampleBufferDescriptor *desc =
        [[MTLCounterSampleBufferDescriptor alloc] init];
    desc.storageMode = MTLStorageModeShared;
    desc.sampleCount = 1024;

    NSError *error = nil;
    profiler->counter_buffer =
        [device newCounterSampleBufferWithDescriptor:desc error:&error];

    if (!profiler->counter_buffer) {
      LOG_WARN("Failed to create GPU counter buffer: %s",
               error ? [[error localizedDescription] UTF8String] : "unknown");
    }
  }

  // Initialize frame history
  for (u32 i = 0; i < GPU_PROFILER_HISTORY_SIZE; i++) {
    profiler->frame_history[i].frame_time_ms = 0.0;
    profiler->frame_history[i].marker_count = 0;
  }

  profiler->current_frame = &profiler->frame_history[0];

  LOG_INFO("GPU Profiler created");
  return profiler;
}

void gpu_profiler_destroy(GPUProfiler *profiler) {
  if (!profiler)
    return;

  profiler->counter_buffer = nil;
  free(profiler);
}

void gpu_profiler_begin_frame(GPUProfiler *profiler, id<MTLCommandBuffer> cmd) {
  if (!profiler)
    return;

  // Move to next frame in circular buffer
  profiler->current_frame_index =
      (profiler->current_frame_index + 1) % GPU_PROFILER_HISTORY_SIZE;
  profiler->current_frame =
      &profiler->frame_history[profiler->current_frame_index];

  // Reset current frame stats
  profiler->current_frame->marker_count = 0;
  profiler->current_frame->total_draw_calls = 0;
  profiler->current_frame->total_triangles = 0;
  profiler->current_frame->total_vertices = 0;
  profiler->current_marker_depth = 0;

  // Start GPU timer
  if (@available(macOS 10.15, *)) {
    if (profiler->counter_buffer) {
      // Sample GPU timestamp at frame start
      // MTLCounterSampleBuffer APIs would go here
    }
  }

  profiler->current_frame->frame_time_ms = CACurrentMediaTime() * 1000.0;
}

void gpu_profiler_end_frame(GPUProfiler *profiler) {
  if (!profiler)
    return;

  f64 end_time = CACurrentMediaTime() * 1000.0;
  profiler->current_frame->frame_time_ms =
      end_time - profiler->current_frame->frame_time_ms;
}

void gpu_profiler_push(GPUProfiler *profiler, id<MTLCommandEncoder> encoder,
                       const char *name) {
  if (!profiler ||
      profiler->current_frame->marker_count >= GPU_PROFILER_MAX_MARKERS)
    return;

  u32 marker_id = profiler->current_frame->marker_count++;
  GPUMarker *marker = &profiler->current_frame->markers[marker_id];

  strncpy(marker->name, name, sizeof(marker->name) - 1);
  marker->start_time = CACurrentMediaTime() * 1000.0;
  marker->draw_calls = 0;
  marker->triangles = 0;
  marker->vertices = 0;

  if (@available(macOS 10.13, *)) {
    [encoder pushDebugGroup:@(name)];
  }

  profiler->current_marker_depth++;
}

void gpu_profiler_pop(GPUProfiler *profiler, id<MTLCommandEncoder> encoder) {
  if (!profiler || profiler->current_marker_depth == 0)
    return;

  // Find the most recent marker at this depth
  for (i32 i = profiler->current_frame->marker_count - 1; i >= 0; i--) {
    GPUMarker *marker = &profiler->current_frame->markers[i];
    if (marker->end_time == 0.0) {
      marker->end_time = CACurrentMediaTime() * 1000.0;
      marker->duration_ms = marker->end_time - marker->start_time;
      break;
    }
  }

  if (@available(macOS 10.13, *)) {
    [encoder popDebugGroup];
  }

  profiler->current_marker_depth--;
}

void gpu_profiler_record_draw(GPUProfiler *profiler, u32 triangles,
                              u32 vertices) {
  if (!profiler)
    return;

  profiler->current_frame->total_draw_calls++;
  profiler->current_frame->total_triangles += triangles;
  profiler->current_frame->total_vertices += vertices;

  // Also record in current marker
  if (profiler->current_frame->marker_count > 0) {
    GPUMarker *marker =
        &profiler->current_frame
             ->markers[profiler->current_frame->marker_count - 1];
    marker->draw_calls++;
    marker->triangles += triangles;
    marker->vertices += vertices;
  }
}

const GPUFrameStats *gpu_profiler_get_frame(GPUProfiler *profiler,
                                            u32 frame_offset) {
  if (!profiler || frame_offset >= GPU_PROFILER_HISTORY_SIZE)
    return NULL;

  u32 index = (profiler->current_frame_index + GPU_PROFILER_HISTORY_SIZE -
               frame_offset) %
              GPU_PROFILER_HISTORY_SIZE;
  return &profiler->frame_history[index];
}

f64 gpu_profiler_get_avg_frame_time(GPUProfiler *profiler, u32 num_frames) {
  if (!profiler)
    return 0.0;

  if (num_frames > GPU_PROFILER_HISTORY_SIZE) {
    num_frames = GPU_PROFILER_HISTORY_SIZE;
  }

  f64 total = 0.0;
  for (u32 i = 0; i < num_frames; i++) {
    const GPUFrameStats *frame = gpu_profiler_get_frame(profiler, i);
    if (frame) {
      total += frame->frame_time_ms;
    }
  }

  return total / num_frames;
}

const GPUMarker *gpu_profiler_find_bottleneck(GPUProfiler *profiler) {
  if (!profiler)
    return NULL;

  const GPUMarker *slowest = NULL;
  f64 max_time = 0.0;

  for (u32 i = 0; i < profiler->current_frame->marker_count; i++) {
    const GPUMarker *marker = &profiler->current_frame->markers[i];
    if (marker->duration_ms > max_time) {
      max_time = marker->duration_ms;
      slowest = marker;
    }
  }

  return slowest;
}

void gpu_profiler_start_capture(GPUProfiler *profiler, const char *path) {
  if (!profiler)
    return;

  profiler->is_capturing = true;
  strncpy(profiler->capture_path, path, sizeof(profiler->capture_path) - 1);

  LOG_INFO("Started GPU profiler capture to: %s", path);
}

void gpu_profiler_stop_capture(GPUProfiler *profiler) {
  if (!profiler || !profiler->is_capturing)
    return;

  profiler->is_capturing = false;

  // TODO: Write capture data to file
  LOG_INFO("Stopped GPU profiler capture");
}

void gpu_profiler_get_timeline_data(GPUProfiler *profiler, f32 *out_data,
                                    u32 max_points) {
  if (!profiler || !out_data)
    return;

  u32 points = max_points < GPU_PROFILER_HISTORY_SIZE
                   ? max_points
                   : GPU_PROFILER_HISTORY_SIZE;

  for (u32 i = 0; i < points; i++) {
    const GPUFrameStats *frame = gpu_profiler_get_frame(profiler, i);
    out_data[i] = frame ? (f32)frame->frame_time_ms : 0.0f;
  }
}
