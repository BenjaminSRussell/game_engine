#ifndef GPU_PROFILING_METAL_H
#define GPU_PROFILING_METAL_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec3 Vec3;
typedef struct Vec4 Vec4;

// Metal Performance Shaders profiling
#ifdef __APPLE__
#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>
#endif

// GPU profiling metrics
typedef enum {
    GPU_METRIC_FRAME_TIME,
    GPU_METRIC_DRAW_CALLS,
    GPU_METRIC_TRIANGLES,
    GPU_METRIC_VERTICES,
    GPU_METRIC_FRAGMENTS,
    GPU_METRIC_COMPUTE_THREADS,
    GPU_METRIC_MEMORY_USAGE,
    GPU_METRIC_TEXTURE_UPLOADS,
    GPU_METRIC_BUFFER_UPLOADS,
    GPU_METRIC_SHADER_COMPILATIONS,
    GPU_METRIC_PIPELINE_STATE_CHANGES,
    GPU_METRIC_RENDER_TARGET_SWITCHES,
    GPU_METRIC_GPU_UTILIZATION,
    GPU_METRIC_POWER_USAGE,
    GPU_METRIC_TEMPERATURE,
    GPU_METRIC_CLOCK_FREQUENCY
} GPUMetricType;

// GPU profiling data types
typedef enum {
    GPU_DATA_TYPE_FLOAT,
    GPU_DATA_TYPE_UINT32,
    GPU_DATA_TYPE_UINT64,
    GPU_DATA_TYPE_PERCENTAGE,
    GPU_DATA_TYPE_TIME_MS,
    GPU_DATA_TYPE_FREQUENCY_MHZ,
    GPU_DATA_TYPE_TEMPERATURE_C,
    GPU_DATA_TYPE_POWER_WATTS,
    GPU_DATA_TYPE_MEMORY_MB
} GPUDataType;

// GPU performance counter
typedef struct {
    char name[64];
    GPUMetricType type;
    GPUDataType data_type;
    
    union {
        float float_value;
        u32 uint32_value;
        u64 uint64_value;
    } current_value;
    
    union {
        float float_value;
        u32 uint32_value;
        u64 uint64_value;
    } average_value;
    
    union {
        float float_value;
        u32 uint32_value;
        u64 uint64_value;
    } min_value;
    
    union {
        float float_value;
        u32 uint32_value;
        u64 uint64_value;
    } max_value;
    
    // Historical data
    void* history_buffer;
    u32 history_count;
    u32 history_capacity;
    u32 history_write_index;
    
    // Statistics
    u64 sample_count;
    u64 total_samples;
    float running_average;
    float variance;
    float standard_deviation;
    
    // Metadata
    bool is_enabled;
    bool is_accumulative;
    u32 update_frequency; // samples per frame
    u32 last_update_frame;
    
} GPUPerformanceCounter;

// GPU frame profiler
typedef struct {
    u32 frame_number;
    u64 frame_start_time_ns;
    u64 frame_end_time_ns;
    float frame_time_ms;
    
    // Frame metrics
    u32 draw_calls;
    u32 triangle_count;
    u32 vertex_count;
    u32 fragment_count;
    u32 compute_thread_count;
    
    // Memory metrics
    u64 memory_usage_bytes;
    u32 texture_uploads;
    u32 buffer_uploads;
    u64 texture_upload_bytes;
    u64 buffer_upload_bytes;
    
    // Pipeline metrics
    u32 shader_compilations;
    u32 pipeline_state_changes;
    u32 render_target_switches;
    
    // GPU utilization
    float gpu_utilization_percent;
    float power_usage_watts;
    float temperature_celsius;
    float clock_frequency_mhz;
    
} GPUFrameProfile;

// GPU profiling session
typedef struct {
    char session_name[128];
    u64 session_start_time_ns;
    u64 session_end_time_ns;
    bool is_active;
    
    // Frame data
    GPUFrameProfile* frames;
    u32 frame_count;
    u32 frame_capacity;
    u32 current_frame_index;
    
    // Performance counters
    GPUPerformanceCounter* counters;
    u32 counter_count;
    u32 counter_capacity;
    
    // Metal-specific resources
#ifdef __APPLE__
    id<MTLDevice> metal_device;
    id<MTLCommandQueue> command_queue;
    id<MTLBuffer> query_buffer;
    id<MTLBuffer> result_buffer;
#endif
    
    // Profiling settings
    bool enable_timing;
    bool enable_statistics;
    bool enable_memory_tracking;
    bool enable_power_tracking;
    u32 target_frame_rate;
    float max_frame_time_ms;
    
    // Callbacks
    void (*on_frame_completed)(const GPUFrameProfile* frame);
    void (*on_session_started)(const char* session_name);
    void (*on_session_ended)(const char* session_name);
    void (*on_performance_warning)(const char* metric_name, float value, float threshold);
    
    void* user_data;
    
} GPUProfilingSession;

// GPU profiling system
typedef struct {
    // Session management
    GPUProfilingSession* sessions;
    u32 session_count;
    u32 session_capacity;
    
    // Active session
    GPUProfilingSession* active_session;
    
    // Global counters
    GPUPerformanceCounter* global_counters;
    u32 global_counter_count;
    u32 global_counter_capacity;
    
    // Metal device
#ifdef __APPLE__
    id<MTLDevice> metal_device;
    id<MTLCommandQueue> command_queue;
#endif
    
    // System configuration
    bool is_initialized;
    bool auto_start_session;
    u32 max_sessions;
    u32 max_frames_per_session;
    u32 max_counters_per_session;
    
    // Performance thresholds
    float frame_time_warning_threshold_ms;
    float frame_time_critical_threshold_ms;
    float memory_warning_threshold_mb;
    float temperature_warning_threshold_c;
    float utilization_warning_threshold_percent;
    
    // Export settings
    bool enable_auto_export;
    char export_directory[256];
    u32 export_interval_frames;
    u32 last_export_frame;
    
    // Statistics
    u64 total_sessions_created;
    u64 total_frames_profiled;
    u64 total_warnings_triggered;
    
} GPUProfilingSystem;

// MARK: - GPU Profiling System Management

bool gpu_profiling_init(GPUProfilingSystem* system, u32 max_sessions, u32 max_frames, u32 max_counters);
void gpu_profiling_shutdown(GPUProfilingSystem* system);

// MARK: - Session Management

GPUProfilingSession* gpu_profiling_create_session(GPUProfilingSystem* system, const char* session_name);
bool gpu_profiling_start_session(GPUProfilingSession* session);
bool gpu_profiling_stop_session(GPUProfilingSession* session);
bool gpu_profiling_destroy_session(GPUProfilingSystem* system, GPUProfilingSession* session);

GPUProfilingSession* gpu_profiling_get_active_session(GPUProfilingSystem* system);
GPUProfilingSession* gpu_profiling_get_session(GPUProfilingSystem* system, const char* session_name);

// MARK: - Frame Profiling

bool gpu_profiling_begin_frame(GPUProfilingSession* session, u32 frame_number);
bool gpu_profiling_end_frame(GPUProfilingSession* session);
bool gpu_profiling_record_draw_call(GPUProfilingSession* session, u32 triangle_count, u32 vertex_count);
bool gpu_profiling_record_compute_dispatch(GPUProfilingSession* session, u32 thread_count_x, u32 thread_count_y, u32 thread_count_z);
bool gpu_profiling_record_memory_operation(GPUProfilingSession* session, u64 bytes, bool is_texture, bool is_upload);

// MARK: - Performance Counters

GPUPerformanceCounter* gpu_profiling_create_counter(GPUProfilingSession* session, const char* name, 
                                                   GPUMetricType type, GPUDataType data_type);
bool gpu_profiling_destroy_counter(GPUProfilingSession* session, GPUPerformanceCounter* counter);
bool gpu_profiling_update_counter(GPUProfilingSession* session, GPUPerformanceCounter* counter, 
                                 float value);
bool gpu_profiling_update_counter_u32(GPUProfilingSession* session, GPUPerformanceCounter* counter, 
                                     u32 value);
bool gpu_profiling_update_counter_u64(GPUProfilingSession* session, GPUPerformanceCounter* counter, 
                                     u64 value);

GPUPerformanceCounter* gpu_profiling_get_counter(GPUProfilingSession* session, const char* name);
bool gpu_profiling_reset_counter(GPUProfilingSession* session, GPUPerformanceCounter* counter);

// MARK: - Metal Performance Shaders Integration

#ifdef __APPLE__
bool gpu_profiling_init_metal(GPUProfilingSystem* system, id<MTLDevice> device);
bool gpu_profiling_create_metal_query_buffers(GPUProfilingSession* session);
bool gpu_profiling_begin_metal_timing(GPUProfilingSession* session, id<MTLCommandBuffer> command_buffer);
bool gpu_profiling_end_metal_timing(GPUProfilingSession* session, id<MTLCommandBuffer> command_buffer);
bool gpu_profiling_collect_metal_statistics(GPUProfilingSession* session);
#endif

// MARK: - Statistics and Analysis

void gpu_profiling_calculate_session_statistics(GPUProfilingSession* session);
void gpu_profiling_calculate_frame_statistics(GPUFrameProfile* frame);
float gpu_profiling_get_average_frame_time(GPUProfilingSession* session);
float gpu_profiling_get_average_gpu_utilization(GPUProfilingSession* session);
u64 gpu_profiling_get_total_memory_transferred(GPUProfilingSession* session);

// MARK: - Export and Reporting

bool gpu_profiling_export_session_to_json(GPUProfilingSession* session, const char* filename);
bool gpu_profiling_export_session_to_csv(GPUProfilingSession* session, const char* filename);
bool gpu_profiling_export_counter_history(GPUPerformanceCounter* counter, const char* filename);

void gpu_profiling_print_session_summary(GPUProfilingSession* session);
void gpu_profiling_print_frame_summary(const GPUFrameProfile* frame);
void gpu_profiling_print_counter_summary(GPUPerformanceCounter* counter);

// MARK: - Performance Monitoring

bool gpu_profiling_check_performance_thresholds(GPUProfilingSession* session);
void gpu_profiling_set_frame_time_thresholds(GPUProfilingSystem* system, float warning_ms, float critical_ms);
void gpu_profiling_set_memory_threshold(GPUProfilingSystem* system, float warning_mb);
void gpu_profiling_set_temperature_threshold(GPUProfilingSystem* system, float warning_c);
void gpu_profiling_set_utilization_threshold(GPUProfilingSystem* system, float warning_percent);

// MARK: - Utility Functions

void gpu_profiling_enable_auto_export(GPUProfilingSystem* system, bool enabled, const char* directory);
void gpu_profiling_set_export_interval(GPUProfilingSystem* system, u32 interval_frames);
bool gpu_profiling_export_all_sessions(GPUProfilingSystem* system);

void gpu_profiling_get_system_statistics(GPUProfilingSystem* system, u32* active_sessions, 
                                       u64* total_frames, u64* total_warnings);
void gpu_profiling_print_system_statistics(GPUProfilingSystem* system);

// MARK: - Configuration

void gpu_profiling_set_callbacks(GPUProfilingSession* session,
                                 void (*on_frame)(const GPUFrameProfile*),
                                 void (*on_session_start)(const char*),
                                 void (*on_session_end)(const char*),
                                 void (*on_warning)(const char*, float, float));

void gpu_profiling_set_session_settings(GPUProfilingSession* session, bool timing, bool statistics, 
                                       bool memory_tracking, bool power_tracking);

// MARK: - Debug Macros

#define GPU_PROFILING_ENABLED 1

#if GPU_PROFILING_ENABLED && defined(__APPLE__)
    #define GPU_PROFILING_BEGIN_FRAME(session, frame_num) gpu_profiling_begin_frame(session, frame_num)
    #define GPU_PROFILING_END_FRAME(session) gpu_profiling_end_frame(session)
    #define GPU_PROFILING_RECORD_DRAW(session, tris, verts) gpu_profiling_record_draw_call(session, tris, verts)
    #define GPU_PROFILING_RECORD_COMPUTE(session, x, y, z) gpu_profiling_record_compute_dispatch(session, x, y, z)
    #define GPU_PROFILING_UPDATE_COUNTER(session, name, value) do { \
        GPUPerformanceCounter* counter = gpu_profiling_get_counter(session, name); \
        if (counter) gpu_profiling_update_counter(session, counter, value); \
    } while(0)
#else
    #define GPU_PROFILING_BEGIN_FRAME(session, frame_num) false
    #define GPU_PROFILING_END_FRAME(session) false
    #define GPU_PROFILING_RECORD_DRAW(session, tris, verts) false
    #define GPU_PROFILING_RECORD_COMPUTE(session, x, y, z) false
    #define GPU_PROFILING_UPDATE_COUNTER(session, name, value)
#endif

// Global GPU profiling system instance
extern GPUProfilingSystem* g_gpu_profiling_system;

#endif // GPU_PROFILING_METAL_H
