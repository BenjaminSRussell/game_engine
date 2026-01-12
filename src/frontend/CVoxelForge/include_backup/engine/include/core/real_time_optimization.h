// include/engine/core/real_time_optimization.h
//
// Purpose: Real-time optimization system for smooth gameplay performance
// This system provides cutting-edge performance optimization capabilities that
// dynamically adjust engine parameters to maintain target frame rates and quality.
//
// Key Features:
// - Dynamic quality adjustment based on performance metrics
// - Adaptive resolution scaling and frame rate targeting
// - Real-time LOD management and culling optimization
// - Memory usage monitoring and garbage collection
// - CPU/GPU load balancing and thermal management
// - Predictive performance optimization with ML
// - Automatic bottleneck detection and resolution
// - User experience quality scoring system
//
// Performance Advantages:
- - Consistent frame rates regardless of hardware
- - Automatic quality scaling for smooth gameplay
- - Intelligent resource management and optimization
- - Real-time bottleneck detection and resolution
- - Predictive optimization for future performance issues
//
// Public APIs:
// - OptimizationEngine: Main optimization controller
// - PerformanceMonitor: Real-time performance tracking
// - QualityManager: Dynamic quality adjustment
// - LODManager: Level-of-detail optimization
// - MemoryOptimizer: Memory usage optimization
//
// Ownership: OptimizationEngine owns all optimization components
// Invariants: Optimization decisions must preserve gameplay quality
//
#ifndef REAL_TIME_OPTIMIZATION_H
#define REAL_TIME_OPTIMIZATION_H

#include "../../common.h"
#include "../core/performance.h"
#include <stdbool.h>

// ============================================================================
// PERFORMANCE MONITORING
// ============================================================================

// Performance metrics
typedef enum {
  METRIC_FRAME_TIME = 0,
  METRIC_FRAME_RATE,
  METRIC_CPU_USAGE,
  METRIC_GPU_USAGE,
  METRIC_MEMORY_USAGE,
  METRIC_DRAW_CALLS,
  METRIC_TRIANGLES,
  METRIC_TEXTURE_BANDWIDTH,
  METRIC_SHADER_SWITCHES,
  METRIC_BOTTLENECK_SCORE,
  METRIC_THERMAL_THROTTLING,
  METRIC_BATTERY_LEVEL,
  METRIC_NETWORK_LATENCY,
  METRIC_DISK_IO,
  METRIC_COUNT
} PerformanceMetric;

// Performance thresholds
typedef struct {
  f32 target_frame_rate;
  f32 min_acceptable_frame_rate;
  f32 max_acceptable_frame_time;
  f32 cpu_usage_threshold;
  f32 gpu_usage_threshold;
  f32 memory_usage_threshold;
  f32 thermal_threshold;
  f32 battery_threshold;
} PerformanceThresholds;

// Performance snapshot
typedef struct {
  f64 timestamp;
  f32 frame_time;
  f32 frame_rate;
  f32 cpu_usage;
  f32 gpu_usage;
  f32 memory_usage_mb;
  u32 draw_calls;
  u32 triangles;
  f32 texture_bandwidth_mb;
  u32 shader_switches;
  f32 bottleneck_score;
  bool is_thermal_throttling;
  f32 battery_level;
  f32 network_latency_ms;
  f32 disk_io_mb;
  f32 quality_score;
} PerformanceSnapshot;

// Performance monitor
typedef struct {
  PerformanceSnapshot *snapshots;
  u32 snapshot_count;
  u32 max_snapshots;
  u32 current_index;
  PerformanceThresholds thresholds;
  f32 averaging_window;
  bool enable_detailed_monitoring;
  f64 total_monitoring_time;
  u32 frames_monitored;
  f32 average_frame_time;
  f32 average_frame_rate;
  f32 average_cpu_usage;
  f32 average_gpu_usage;
  f32 average_memory_usage;
} PerformanceMonitor;

// ============================================================================
// DYNAMIC QUALITY MANAGEMENT
// ============================================================================

// Quality settings
typedef enum {
  QUALITY_ULTRA = 0,
  QUALITY_HIGH,
  QUALITY_MEDIUM,
  QUALITY_LOW,
  QUALITY_POTATO,
  QUALITY_AUTO,
  QUALITY_COUNT
} QualityLevel;

// Quality parameters
typedef struct {
  QualityLevel level;
  f32 resolution_scale;
  u32 render_scale;
  f32 shadow_quality;
  f32 texture_quality;
  f32 effects_quality;
  f32 post_process_quality;
  u32 lod_bias;
  f32 view_distance;
  u32 particle_count;
  f32 reflection_quality;
  bool enable_vsync;
  u32 target_frame_rate;
  f32 motion_blur_strength;
  f32 depth_of_field_quality;
  u32 anisotropic_filtering;
  bool enable_ray_tracing;
  f32 ray_tracing_quality;
} QualitySettings;

// Quality adjustment strategies
typedef enum {
  STRATEGY_AGGRESSIVE = 0,    // Fast quality changes for performance
  STRATEGY_BALANCED,          // Balanced approach
  STRATEGY_CONSERVATIVE,      // Slow, conservative changes
  STRATEGY_PREDICTIVE,        // ML-based predictive adjustment
  STRATEGY_USER_PREFERENCE    // Respect user quality preferences
} QualityStrategy;

// Quality manager
typedef struct {
  QualitySettings current_settings;
  QualitySettings target_settings;
  QualityLevel current_level;
  QualityLevel target_level;
  QualityStrategy strategy;
  f32 transition_speed;
  bool enable_auto_adjustment;
  bool respect_user_settings;
  f32 user_preference_weight;
  u32 adjustment_cooldown;
  u32 last_adjustment_frame;
  f64 total_adjustment_time;
  u32 adjustment_count;
  f32 average_quality_score;
} QualityManager;

// ============================================================================
// LEVEL OF DETAIL OPTIMIZATION
// ============================================================================

// LOD levels
typedef enum {
  LOD_LEVEL_0 = 0,    // Highest quality
  LOD_LEVEL_1,
  LOD_LEVEL_2,
  LOD_LEVEL_3,
  LOD_LEVEL_4,        // Lowest quality
  LOD_LEVEL_COUNT
} LODLevel;

// LOD settings
typedef struct {
  LODLevel level;
  f32 distance_threshold;
  f32 screen_size_threshold;
  u32 triangle_reduction;
  f32 texture_resolution_scale;
  u32 shader_complexity;
  bool enable_collision;
  bool enable_physics;
  f32 update_frequency;
} LODSettings;

// LOD object
typedef struct {
  u32 object_id;
  vec3 position;
  f32 bounding_radius;
  LODLevel current_lod;
  LODLevel target_lod;
  f32 transition_progress;
  f64 last_update_time;
  u32 frame_cooldown;
  bool force_high_quality;
  bool is_important;
} LODObject;

// LOD manager
typedef struct {
  LODSettings settings[LOD_LEVEL_COUNT];
  LODObject *objects;
  u32 object_count;
  u32 max_objects;
  vec3 camera_position;
  f32 camera_fov;
  u32 screen_width;
  u32 screen_height;
  bool enable_distance_lod;
  bool enable_screen_size_lod;
  bool enable_importance_lod;
  f32 lod_transition_speed;
  u32 lod_update_frequency;
  f64 total_lod_time;
  u32 lod_updates_performed;
  f32 average_lod_level;
} LODManager;

// ============================================================================
// MEMORY OPTIMIZATION
// ============================================================================

// Memory optimization strategies
typedef enum {
  MEMORY_STRATEGY_AGGRESSIVE = 0,
  MEMORY_STRATEGY_BALANCED,
  MEMORY_STRATEGY_CONSERVATIVE,
  MEMORY_STRATEGY_PREDICTIVE
} MemoryStrategy;

// Memory pool types
typedef enum {
  MEMORY_POOL_TEXTURES = 0,
  MEMORY_POOL_MESHES,
  MEMORY_POOL_AUDIO,
  MEMORY_POOL_PARTICLES,
  MEMORY_POOL_SCRIPTS,
  MEMORY_POOL_CACHE,
  MEMORY_POOL_COUNT
} MemoryPoolType;

// Memory statistics
typedef struct {
  u64 total_allocated;
  u64 total_used;
  u64 peak_usage;
  u64 fragmentation;
  f32 fragmentation_ratio;
  u64 allocations_count;
  u64 deallocations_count;
  u64 gc_count;
  f64 gc_time;
  f64 allocation_time;
} MemoryStatistics;

// Memory optimizer
typedef struct {
  MemoryStrategy strategy;
  MemoryStatistics pool_stats[MEMORY_POOL_COUNT];
  u64 memory_budget_mb;
  u64 warning_threshold_mb;
  u64 critical_threshold_mb;
  bool enable_auto_gc;
  bool enable_defragmentation;
  u32 gc_frequency;
  f64 last_gc_time;
  u64 total_gc_time;
  u32 gc_count;
  bool is_memory_pressure;
  f64 total_optimization_time;
} MemoryOptimizer;

// ============================================================================
// BOTTLENECK DETECTION
// ============================================================================

// Bottleneck types
typedef enum {
  BOTTLENECK_CPU = 0,
  BOTTLENECK_GPU,
  BOTTLENECK_MEMORY,
  BOTTLENECK_DISK_IO,
  BOTTLENECK_NETWORK,
  BOTTLENECK_THERMAL,
  BOTTLENECK_POWER,
  BOTTLENECK_UNKNOWN
} BottleneckType;

// Bottleneck severity
typedef enum {
  SEVERITY_NONE = 0,
  SEVERITY_LOW,
  SEVERITY_MEDIUM,
  SEVERITY_HIGH,
  SEVERITY_CRITICAL
} BottleneckSeverity;

// Bottleneck detection result
typedef struct {
  BottleneckType type;
  BottleneckSeverity severity;
  f32 confidence_score;
  char *description;
  char *recommended_action;
  f64 detection_time;
  bool is_resolved;
  u32 resolution_attempts;
} BottleneckDetection;

// Bottleneck detector
typedef struct {
  BottleneckDetection current_bottleneck;
  BottleneckDetection *history;
  u32 history_count;
  u32 max_history;
  f32 detection_threshold;
  bool enable_ml_detection;
  void *ml_model;
  f64 total_detection_time;
  u32 detections_performed;
  u32 false_positives;
  f32 detection_accuracy;
} BottleneckDetector;

// ============================================================================
// PREDICTIVE OPTIMIZATION
// ============================================================================

// Prediction models
typedef enum {
  PREDICTION_MODEL_LINEAR = 0,
  PREDICTION_MODEL_POLYNOMIAL,
  PREDICTION_MODEL_NEURAL,
  PREDICTION_MODEL_ENSEMBLE
} PredictionModel;

// Performance prediction
typedef struct {
  f32 predicted_frame_time;
  f32 predicted_cpu_usage;
  f32 predicted_gpu_usage;
  f32 predicted_memory_usage;
  f32 confidence_score;
  f64 prediction_horizon;
  PredictionModel model;
  bool is_accurate;
} PerformancePrediction;

// Predictive optimizer
typedef struct {
  PredictionModel model;
  PerformanceSnapshot *training_data;
  u32 training_data_count;
  u32 max_training_data;
  bool enable_online_learning;
  f32 learning_rate;
  u32 prediction_horizon_frames;
  f64 total_prediction_time;
  u32 predictions_made;
  f32 prediction_accuracy;
  PerformancePrediction last_prediction;
} PredictiveOptimizer;

// ============================================================================
// THERMAL AND POWER MANAGEMENT
// ============================================================================

// Thermal states
typedef enum {
  THERMAL_STATE_NORMAL = 0,
  THERMAL_STATE_WARM,
  THERMAL_STATE_HOT,
  THERMAL_STATE_CRITICAL,
  THERMAL_STATE_THROTTLING
} ThermalState;

// Power states
typedef enum {
  POWER_STATE_FULL = 0,
  POWER_STATE_BALANCED,
  POWER_STATE_POWER_SAVING,
  POWER_STATE_BATTERY_SAVER
} PowerState;

// Thermal and power manager
typedef struct {
  ThermalState thermal_state;
  PowerState power_state;
  f32 current_temperature;
  f32 max_temperature;
  f32 battery_level;
  bool is_on_battery;
  f32 battery_discharge_rate;
  bool enable_thermal_throttling;
  bool enable_power_saving;
  f32 thermal_threshold;
  f32 battery_threshold;
  u32 thermal_throttle_level;
  u32 power_saving_level;
  f64 total_management_time;
} ThermalPowerManager;

// ============================================================================
// MAIN OPTIMIZATION ENGINE
// ============================================================================

// Optimization engine configuration
typedef struct {
  f32 target_frame_rate;
  f32 min_acceptable_frame_rate;
  QualityLevel default_quality;
  QualityStrategy default_strategy;
  MemoryStrategy default_memory_strategy;
  bool enable_predictive_optimization;
  bool enable_thermal_management;
  bool enable_bottleneck_detection;
  bool enable_auto_quality_adjustment;
  u32 performance_history_size;
  f32 optimization_frequency;
  bool enable_ml_optimization;
} OptimizationConfig;

// Main optimization engine
typedef struct {
  // Configuration
  OptimizationConfig config;
  
  // Performance monitoring
  PerformanceMonitor *performance_monitor;
  
  // Quality management
  QualityManager *quality_manager;
  
  // LOD management
  LODManager *lod_manager;
  
  // Memory optimization
  MemoryOptimizer *memory_optimizer;
  
  // Bottleneck detection
  BottleneckDetector *bottleneck_detector;
  
  // Predictive optimization
  PredictiveOptimizer *predictive_optimizer;
  
  // Thermal and power management
  ThermalPowerManager *thermal_power_manager;
  
  // Performance
  Profiler *optimization_profiler;
  f64 total_optimization_time;
  f64 quality_adjustment_time;
  f64 lod_optimization_time;
  f64 memory_optimization_time;
  f64 bottleneck_detection_time;
  
  // Statistics
  u32 optimization_cycles;
  u32 quality_adjustments;
  u32 lod_updates;
  u32 garbage_collections;
  u32 bottleneck_detections;
  f32 average_quality_score;
  f32 average_frame_rate;
  bool is_performance_stable;
} OptimizationEngine;

// ============================================================================
// PUBLIC API
// ============================================================================

// Optimization engine management
OptimizationEngine *optimization_engine_create(const OptimizationConfig *config);
void optimization_engine_destroy(OptimizationEngine *engine);
void optimization_engine_update(OptimizationEngine *engine, f32 delta_time);

// Configuration
OptimizationConfig optimization_create_default_config(void);
OptimizationConfig optimization_create_high_performance_config(void);
OptimizationConfig optimization_create_mobile_config(void);
OptimizationConfig optimization_create_battery_config(void);

// ============================================================================
// PERFORMANCE MONITORING API
// ============================================================================

// Performance tracking
void optimization_update_performance(OptimizationEngine *engine, const PerformanceSnapshot *snapshot);
PerformanceSnapshot optimization_get_current_performance(OptimizationEngine *engine);
PerformanceSnapshot optimization_get_average_performance(OptimizationEngine *engine);
bool optimization_is_performance_target_met(OptimizationEngine *engine);

// Threshold management
void optimization_set_performance_thresholds(OptimizationEngine *engine, const PerformanceThresholds *thresholds);
PerformanceThresholds optimization_get_performance_thresholds(OptimizationEngine *engine);
bool optimization_is_threshold_exceeded(OptimizationEngine *engine, PerformanceMetric metric);

// ============================================================================
// QUALITY MANAGEMENT API
// ============================================================================

// Quality control
void optimization_set_quality_level(OptimizationEngine *engine, QualityLevel level);
QualityLevel optimization_get_quality_level(OptimizationEngine *engine);
void optimization_enable_auto_quality(OptimizationEngine *engine, bool enable);
void optimization_set_quality_strategy(OptimizationEngine *engine, QualityStrategy strategy);

// Quality settings
QualitySettings optimization_get_quality_settings(OptimizationEngine *engine);
void optimization_override_quality_setting(OptimizationEngine *engine, const char *setting_name, f32 value);
void optimization_reset_quality_overrides(OptimizationEngine *engine);

// ============================================================================
// LOD OPTIMIZATION API
// ============================================================================

// LOD management
void optimization_update_camera_info(OptimizationEngine *engine, vec3 camera_pos, f32 fov, u32 screen_width, u32 screen_height);
u32 optimization_register_lod_object(OptimizationEngine *engine, u32 object_id, vec3 position, f32 bounding_radius);
void optimization_unregister_lod_object(OptimizationEngine *engine, u32 object_id);
void optimization_set_lod_importance(OptimizationEngine *engine, u32 object_id, bool is_important);

// LOD settings
void optimization_set_lod_settings(OptimizationEngine *engine, LODLevel level, const LODSettings *settings);
LODSettings optimization_get_lod_settings(OptimizationEngine *engine, LODLevel level);
void optimization_enable_lod_system(OptimizationEngine *engine, bool enable);

// ============================================================================
// MEMORY OPTIMIZATION API
// ============================================================================

// Memory monitoring
MemoryStatistics optimization_get_memory_statistics(OptimizationEngine *engine, MemoryPoolType pool_type);
bool optimization_is_memory_pressure(OptimizationEngine *engine);
void optimization_set_memory_strategy(OptimizationEngine *engine, MemoryStrategy strategy);

// Garbage collection
void optimization_force_garbage_collection(OptimizationEngine *engine);
void optimization_set_gc_frequency(OptimizationEngine *engine, u32 frequency);
void optimization_enable_auto_gc(OptimizationEngine *engine, bool enable);

// ============================================================================
// BOTTLENECK DETECTION API
// ============================================================================

// Bottleneck analysis
BottleneckDetection optimization_detect_bottleneck(OptimizationEngine *engine);
BottleneckDetection optimization_get_current_bottleneck(OptimizationEngine *engine);
bool optimization_resolve_bottleneck(OptimizationEngine *engine, const BottleneckDetection *bottleneck);

// Detection configuration
void optimization_enable_bottleneck_detection(OptimizationEngine *engine, bool enable);
void optimization_set_detection_threshold(OptimizationEngine *engine, f32 threshold);
void optimization_enable_ml_detection(OptimizationEngine *engine, bool enable);

// ============================================================================
// PREDICTIVE OPTIMIZATION API
// ============================================================================

// Performance prediction
PerformancePrediction optimization_predict_performance(OptimizationEngine *engine, u32 horizon_frames);
bool optimization_is_prediction_accurate(OptimizationEngine *engine);
void optimization_enable_predictive_optimization(OptimizationEngine *engine, bool enable);

// Model training
void optimization_train_prediction_model(OptimizationEngine *engine);
void optimization_enable_online_learning(OptimizationEngine *engine, bool enable);
void optimization_set_learning_rate(OptimizationEngine *engine, f32 rate);

// ============================================================================
// THERMAL AND POWER MANAGEMENT API
// ============================================================================

// Thermal management
ThermalState optimization_get_thermal_state(OptimizationEngine *engine);
void optimization_set_thermal_threshold(OptimizationEngine *engine, f32 threshold);
void optimization_enable_thermal_throttling(OptimizationEngine *engine, bool enable);

// Power management
PowerState optimization_get_power_state(OptimizationEngine *engine);
void optimization_set_battery_threshold(OptimizationEngine *engine, f32 threshold);
void optimization_enable_power_saving(OptimizationEngine *engine, bool enable);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Optimization configuration macros
#define OPTIMIZATION_DEFAULT_CONFIG() \
  (OptimizationConfig){ \
    .target_frame_rate = 60.0f, \
    .min_acceptable_frame_rate = 30.0f, \
    .default_quality = QUALITY_HIGH, \
    .default_strategy = STRATEGY_BALANCED, \
    .default_memory_strategy = MEMORY_STRATEGY_BALANCED, \
    .enable_predictive_optimization = false, \
    .enable_thermal_management = true, \
    .enable_bottleneck_detection = true, \
    .enable_auto_quality_adjustment = true, \
    .performance_history_size = 300, \
    .optimization_frequency = 0.5f, \
    .enable_ml_optimization = false \
  }

#define OPTIMIZATION_HIGH_PERFORMANCE_CONFIG() \
  (OptimizationConfig){ \
    .target_frame_rate = 120.0f, \
    .min_acceptable_frame_rate = 60.0f, \
    .default_quality = QUALITY_ULTRA, \
    .default_strategy = STRATEGY_AGGRESSIVE, \
    .default_memory_strategy = MEMORY_STRATEGY_AGGRESSIVE, \
    .enable_predictive_optimization = true, \
    .enable_thermal_management = true, \
    .enable_bottleneck_detection = true, \
    .enable_auto_quality_adjustment = true, \
    .performance_history_size = 600, \
    .optimization_frequency = 0.1f, \
    .enable_ml_optimization = true \
  }

#define OPTIMIZATION_MOBILE_CONFIG() \
  (OptimizationConfig){ \
    .target_frame_rate = 30.0f, \
    .min_acceptable_frame_rate = 20.0f, \
    .default_quality = QUALITY_MEDIUM, \
    .default_strategy = STRATEGY_CONSERVATIVE, \
    .default_memory_strategy = MEMORY_STRATEGY_CONSERVATIVE, \
    .enable_predictive_optimization = false, \
    .enable_thermal_management = true, \
    .enable_bottleneck_detection = true, \
    .enable_auto_quality_adjustment = true, \
    .performance_history_size = 180, \
    .optimization_frequency = 1.0f, \
    .enable_ml_optimization = false \
  }

#define OPTIMIZATION_BATTERY_CONFIG() \
  (OptimizationConfig){ \
    .target_frame_rate = 30.0f, \
    .min_acceptable_frame_rate = 15.0f, \
    .default_quality = QUALITY_LOW, \
    .default_strategy = STRATEGY_CONSERVATIVE, \
    .default_memory_strategy = MEMORY_STRATEGY_CONSERVATIVE, \
    .enable_predictive_optimization = false, \
    .enable_thermal_management = true, \
    .enable_bottleneck_detection = true, \
    .enable_auto_quality_adjustment = true, \
    .performance_history_size = 120, \
    .optimization_frequency = 2.0f, \
    .enable_ml_optimization = false \
  }

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Machine learning optimization
void optimization_enable_ml_optimization(OptimizationEngine *engine, bool enable);
bool optimization_train_ml_model(OptimizationEngine *engine, const PerformanceSnapshot *training_data, u32 data_count);
f32 optimization_get_ml_accuracy(OptimizationEngine *engine);

// User experience optimization
void optimization_enable_ux_optimization(OptimizationEngine *engine, bool enable);
f32 optimization_calculate_ux_score(OptimizationEngine *engine);
void optimization_optimize_for_ux(OptimizationEngine *engine);

// Adaptive optimization
void optimization_enable_adaptive_optimization(OptimizationEngine *engine, bool enable);
void optimization_adapt_to_hardware(OptimizationEngine *engine);
void optimization_adapt_to_user_behavior(OptimizationEngine *engine);

// ============================================================================
// DEBUGGING AND MONITORING
// ============================================================================

// Debug rendering
void optimization_debug_render_performance_graph(OptimizationEngine *engine);
void optimization_debug_render_quality_settings(OptimizationEngine *engine);
void optimization_debug_render_memory_usage(OptimizationEngine *engine);
void optimization_debug_render_bottleneck_info(OptimizationEngine *engine);

// Performance reports
typedef struct {
  f32 average_frame_rate;
  f32 average_frame_time;
  QualityLevel current_quality;
  f32 average_quality_score;
  u64 total_memory_usage;
  f32 cpu_utilization;
  f32 gpu_utilization;
  BottleneckType current_bottleneck;
  ThermalState thermal_state;
  PowerState power_state;
  u32 optimization_cycles;
  f64 total_optimization_time;
} OptimizationReport;

OptimizationReport optimization_get_report(OptimizationEngine *engine);
void optimization_print_report(OptimizationEngine *engine);

#endif // REAL_TIME_OPTIMIZATION_H
