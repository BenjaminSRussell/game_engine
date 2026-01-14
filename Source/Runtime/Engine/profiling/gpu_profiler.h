// src/engine/core/profiling/gpu_profiler.h
//
// Purpose: GPU timeline profiling system header
// Provides comprehensive GPU performance analysis with timeline visualization

#ifndef GPU_PROFILER_H
#define GPU_PROFILER_H

#include "core/math/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GPU PROFILE CATEGORIES
// ============================================================================

typedef enum GPUProfileCategory {
    GPU_PROFILE_CATEGORY_RENDERING = 0,
    GPU_PROFILE_CATEGORY_COMPUTE,
    GPU_PROFILE_CATEGORY_TRANSFER,
    GPU_PROFILE_CATEGORY_POST_PROCESS,
    GPU_PROFILE_CATEGORY_SHADOWS,
    GPU_PROFILE_CATEGORY_GEOMETRY,
    GPU_PROFILE_CATEGORY_ANIMATION,
    GPU_PROFILE_CATEGORY_PHYSICS,
    GPU_PROFILE_CATEGORY_AUDIO,
    GPU_PROFILE_CATEGORY_OTHER,
    GPU_PROFILE_CATEGORY_COUNT
} GPUProfileCategory;

// ============================================================================
// Opaque TYPES
// ============================================================================

typedef struct GPUProfilerContext GPUProfilerContext;
typedef u32 GPUProfileHandle;

// ============================================================================
// STATISTICS STRUCTURES
// ============================================================================

typedef struct GPUProfilerStats {
    f32 total_frame_time_ms;
    u32 current_frame;
    u32 entries_count;
    f32 category_times_ms[GPU_PROFILE_CATEGORY_COUNT];
    u32 category_counts[GPU_PROFILE_CATEGORY_COUNT];
} GPUProfilerStats;

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

// Create GPU profiler context
GPUProfilerContext* gpu_profiler_create(u32 max_queries_per_frame, u32 frame_delay);

// Destroy GPU profiler context
void gpu_profiler_destroy(GPUProfilerContext* ctx);

// ============================================================================
// GLOBAL PROFILER MANAGEMENT
// ============================================================================

// Initialize global GPU profiler
bool gpu_profiler_initialize(u32 max_queries_per_frame, u32 frame_delay);

// Shutdown global GPU profiler
void gpu_profiler_shutdown(void);

// Get global GPU profiler instance
GPUProfilerContext* gpu_profiler_get_global(void);

// ============================================================================
// PROFILING CONTROL
// ============================================================================

// Begin profiling capture
void gpu_profiler_begin_capture(void);

// End profiling capture
void gpu_profiler_end_capture(void);

// ============================================================================
// TIMELINE PROFILING
// ============================================================================

// Begin a profiled event
GPUProfileHandle gpu_profiler_begin_event(const char* name, GPUProfileCategory category);

// End a profiled event
void gpu_profiler_end_event(GPUProfileHandle handle);

// ============================================================================
// FRAME MANAGEMENT
// ============================================================================

// Called at the beginning of each frame
void gpu_profiler_begin_frame(void);

// Called at the end of each frame
void gpu_profiler_end_frame(void);

// ============================================================================
// STATISTICS AND REPORTING
// ============================================================================

// Get current profiler statistics
void gpu_profiler_get_statistics(GPUProfilerStats* out_stats);

// Print summary statistics to log
void gpu_profiler_print_summary(void);

// Print detailed timeline to log
void gpu_profiler_print_timeline(void);

// Export profiling data to CSV file
void gpu_profiler_export_csv(const char* filename);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

#define GPU_PROFILE_SCOPE(name, category) \
    GPUProfileHandle __profile_handle = gpu_profiler_begin_event(name, category); \
    /* Scope ends automatically when function returns */

#define GPU_PROFILE_END_SCOPE() \
    gpu_profiler_end_event(__profile_handle)

#define GPU_PROFILE_FUNCTION(category) \
    GPU_PROFILE_SCOPE(__FUNCTION__, category)

#define GPU_PROFILE_RENDERING(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_RENDERING)
#define GPU_PROFILE_COMPUTE(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_COMPUTE)
#define GPU_PROFILE_TRANSFER(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_TRANSFER)
#define GPU_PROFILE_POST_PROCESS(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_POST_PROCESS)
#define GPU_PROFILE_SHADOWS(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_SHADOWS)
#define GPU_PROFILE_GEOMETRY(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_GEOMETRY)
#define GPU_PROFILE_ANIMATION(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_ANIMATION)
#define GPU_PROFILE_PHYSICS(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_PHYSICS)
#define GPU_PROFILE_AUDIO(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_AUDIO)
#define GPU_PROFILE_OTHER(name) GPU_PROFILE_SCOPE(name, GPU_PROFILE_CATEGORY_OTHER)

// ============================================================================
// GPU BACKEND INTERFACE (to be implemented by graphics backend)
// ============================================================================

// Create timestamp query pool
u32 gpu_create_timestamp_query_pool(u32 query_count);

// Destroy timestamp query pool
void gpu_destroy_timestamp_query_pool(u32 pool);

// Issue timestamp query
void gpu_timestamp_query(u32 pool, u32 index);

// Get timestamp query results
void gpu_get_timestamp_query_results(u32 pool, u32 start_index, u32 count, u64* results);

#ifdef __cplusplus
}
#endif

#endif // GPU_PROFILER_H
