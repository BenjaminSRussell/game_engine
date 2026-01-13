#ifndef PHYSICS_PERFORMANCE_PROFILER_H
#define PHYSICS_PERFORMANCE_PROFILER_H

#include <stdbool.h>
#include <stdint.h>
#include "math/vec3.h"
#include "physics/physics.h"

#ifdef __cplusplus
extern "C" {
#endif

// Profiler Categories
typedef enum {
    PROFILER_CATEGORY_PHYSICS = 0,
    PROFILER_CATEGORY_COLLISION_DETECTION,
    PROFILER_CATEGORY_BROAD_PHASE,
    PROFILER_CATEGORY_NARROW_PHASE,
    PROFILER_CATEGORY_CONSTRAINT_SOLVING,
    PROFILER_CATEGORY_INTEGRATION,
    PROFILER_CATEGORY_SLEEP_SYSTEM,
    PROFILER_CATEGORY_ISLAND_SOLVING,
    PROFILER_CATEGORY_SOFT_BODY,
    PROFILER_CATEGORY_CONTINUOUS_COLLISION,
    PROFILER_CATEGORY_SPATIAL_HASH,
    PROFILER_CATEGORY_CONTACT_CACHE,
    PROFILER_CATEGORY_TIME_STEP,
    PROFILER_CATEGORY_DEBUG_VISUALIZATION,
    PROFILER_CATEGORY_COUNT
} ProfilerCategory;

// Performance Metrics
typedef struct {
    float total_time;
    float average_time;
    float min_time;
    float max_time;
    float last_time;
    uint32_t call_count;
    uint32_t samples_collected;
    bool enabled;
} PerformanceMetric;

// Performance Sample
typedef struct {
    float timestamp;
    float duration;
    uint32_t call_count;
    uint32_t object_count;
    uint32_t collision_count;
    float memory_usage;
} PerformanceSample;

// Performance Threshold
typedef struct {
    float warning_threshold;
    float critical_threshold;
    uint32_t max_samples;
    bool enable_alerts;
} PerformanceThreshold;

// Performance Alert
typedef struct {
    ProfilerCategory category;
    float current_value;
    float threshold_value;
    uint32_t alert_type; // 0 = warning, 1 = critical
    uint64_t timestamp;
    char message[256];
} PerformanceAlert;

// Performance Profiler Context
typedef struct {
    // Metrics for each category
    PerformanceMetric metrics[PROFILER_CATEGORY_COUNT];
    
    // Performance samples
    PerformanceSample* samples;
    uint32_t sample_count;
    uint32_t sample_capacity;
    uint32_t current_sample_index;
    
    // Performance thresholds
    PerformanceThreshold thresholds[PROFILER_CATEGORY_COUNT];
    
    // Performance alerts
    PerformanceAlert* alerts;
    uint32_t alert_count;
    uint32_t alert_capacity;
    
    // Global statistics
    struct {
        float total_physics_time;
        float frame_time;
        float physics_frame_ratio;
        uint32_t total_objects;
        uint32_t active_objects;
        uint32_t sleeping_objects;
        uint32_t total_collisions;
        uint32_t broad_phase_tests;
        uint32_t narrow_phase_tests;
        float memory_usage;
        uint32_t frames_analyzed;
    } global_stats;
    
    // Profiling state
    bool enabled;
    bool real_time_mode;
    float sampling_interval;
    float last_sample_time;
    uint32_t frame_counter;
    
    // Bottleneck detection
    struct {
        ProfilerCategory bottleneck_category;
        float bottleneck_percentage;
        uint32_t consecutive_frames;
        bool bottleneck_detected;
    } bottleneck;
    
    // Callbacks
    void (*on_performance_alert)(const PerformanceAlert* alert);
    void (*on_bottleneck_detected)(ProfilerCategory category, float percentage);
    void (*on_performance_sample)(ProfilerCategory category, float duration);
    
    // Memory tracking
    struct {
        size_t total_allocated;
        size_t peak_usage;
        uint32_t allocation_count;
        uint32_t deallocation_count;
    } memory_stats;
} PhysicsProfiler;

// Profiler Functions
bool physics_profiler_init(PhysicsProfiler* profiler, uint32_t max_samples, uint32_t max_alerts);
void physics_profiler_cleanup(PhysicsProfiler* profiler);
void physics_profiler_reset(PhysicsProfiler* profiler);
void physics_profiler_update(PhysicsProfiler* profiler, float delta_time);

// Profiling Control
void physics_profiler_enable(PhysicsProfiler* profiler, bool enabled);
void physics_profiler_enable_category(PhysicsProfiler* profiler, ProfilerCategory category, bool enabled);
void physics_profiler_set_sampling_interval(PhysicsProfiler* profiler, float interval);
void physics_profiler_set_real_time_mode(PhysicsProfiler* profiler, bool real_time);

// Performance Measurement
void physics_profiler_begin_sample(PhysicsProfiler* profiler, ProfilerCategory category);
void physics_profiler_end_sample(PhysicsProfiler* profiler, ProfilerCategory category);
void physics_profiler_record_metric(PhysicsProfiler* profiler, ProfilerCategory category, float duration);
void physics_profiler_record_frame_metrics(PhysicsProfiler* profiler, float frame_time, uint32_t object_count,
                                        uint32_t collision_count, float memory_usage);

// Performance Analysis
void physics_profiler_analyze_performance(PhysicsProfiler* profiler);
void physics_profiler_detect_bottlenecks(PhysicsProfiler* profiler);
void physics_profiler_generate_report(PhysicsProfiler* profiler);
float physics_profiler_get_category_time(const PhysicsProfiler* profiler, ProfilerCategory category);
float physics_profiler_get_category_percentage(const PhysicsProfiler* profiler, ProfilerCategory category);

// Threshold Management
void physics_profiler_set_threshold(PhysicsProfiler* profiler, ProfilerCategory category, 
                                   float warning_threshold, float critical_threshold);
void physics_profiler_enable_alerts(PhysicsProfiler* profiler, ProfilerCategory category, bool enable);
bool physics_profiler_check_thresholds(PhysicsProfiler* profiler, ProfilerCategory category, float value);

// Alert Management
void physics_profiler_add_alert(PhysicsProfiler* profiler, ProfilerCategory category, uint32_t alert_type,
                                float current_value, float threshold_value, const char* message);
void physics_profiler_clear_alerts(PhysicsProfiler* profiler);
PerformanceAlert* physics_profiler_get_alerts(PhysicsProfiler* profiler, uint32_t* count);

// Sample Management
void physics_profiler_add_sample(PhysicsProfiler* profiler, ProfilerCategory category, float duration,
                               uint32_t call_count, uint32_t object_count, uint32_t collision_count, float memory_usage);
PerformanceSample* physics_profiler_get_samples(PhysicsProfiler* profiler, ProfilerCategory category, uint32_t* count);
void physics_profiler_clear_samples(PhysicsProfiler* profiler);

// Statistics and Reporting
void physics_profiler_get_statistics(const PhysicsProfiler* profiler, uint32_t* total_objects, uint32_t* active_objects,
                                    uint32_t* total_collisions, float* physics_time, float* frame_ratio);
void physics_profiler_get_category_statistics(const PhysicsProfiler* profiler, ProfilerCategory category,
                                            float* total_time, float* average_time, float* min_time, float* max_time);
void physics_profiler_export_performance_data(PhysicsProfiler* profiler, const char* filename);

// Debug and Visualization
void physics_profiler_debug_draw_performance_graph(PhysicsProfiler* profiler);
void physics_profiler_debug_draw_category_breakdown(PhysicsProfiler* profiler);
void physics_profiler_debug_draw_bottleneck_indicator(PhysicsProfiler* profiler);
void physics_profiler_debug_print_statistics(PhysicsProfiler* profiler);

// Memory Profiling
void physics_profiler_track_allocation(PhysicsProfiler* profiler, size_t size);
void physics_profiler_track_deallocation(PhysicsProfiler* profiler, size_t size);
void physics_profiler_get_memory_statistics(const PhysicsProfiler* profiler, size_t* total_allocated,
                                          size_t* peak_usage, uint32_t* allocation_count);

// Performance Optimization Suggestions
typedef struct {
    ProfilerCategory category;
    char suggestion[256];
    float potential_improvement;
    uint32_t priority; // 1 = highest, 3 = lowest
} OptimizationSuggestion;

void physics_profiler_generate_optimization_suggestions(PhysicsProfiler* profiler);
OptimizationSuggestion* physics_profiler_get_suggestions(PhysicsProfiler* profiler, uint32_t* count);

// Utility Functions
const char* physics_profiler_category_get_name(ProfilerCategory category);
float physics_profiler_calculate_fps(float frame_time);
float physics_profiler_calculate_frame_budget(float target_fps);
bool physics_profiler_is_performance_acceptable(const PhysicsProfiler* profiler);
uint32_t physics_profiler_estimate_optimal_sub_steps(const PhysicsProfiler* profiler, float target_time);

// Validation and Testing
bool physics_profiler_validate(const PhysicsProfiler* profiler);
bool physics_profiler_test_performance_detection(PhysicsProfiler* profiler, uint32_t iterations);
void physics_profiler_run_benchmark(PhysicsProfiler* profiler, uint32_t object_count, uint32_t duration_seconds);

// Callback Management
void physics_profiler_set_alert_callback(PhysicsProfiler* profiler, void (*callback)(const PerformanceAlert*));
void physics_profiler_set_bottleneck_callback(PhysicsProfiler* profiler, void (*callback)(ProfilerCategory, float));
void physics_profiler_set_sample_callback(PhysicsProfiler* profiler, void (*callback)(ProfilerCategory, float));

// Helper Macros for Easy Profiling
#define PHYSICS_PROFILE_BEGIN(profiler, category) physics_profiler_begin_sample(profiler, category)
#define PHYSICS_PROFILE_END(profiler, category) physics_profiler_end_sample(profiler, category)
#define PHYSICS_PROFILE_SCOPE(profiler, category) \
    physics_profiler_begin_sample(profiler, category); \
    /* code to profile */ \
    physics_profiler_end_sample(profiler, category)

// RAII-style profiling helper
typedef struct {
    PhysicsProfiler* profiler;
    ProfilerCategory category;
    bool active;
} ProfileScope;

ProfileScope physics_profiler_create_scope(PhysicsProfiler* profiler, ProfilerCategory category);
void physics_profiler_destroy_scope(ProfileScope* scope);

#define PHYSICS_PROFILE_SCOPE_AUTO(profiler, category) \
    ProfileScope _scope = physics_profiler_create_scope(profiler, category); \
    /* scope automatically ends when function returns */

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_PERFORMANCE_PROFILER_H
