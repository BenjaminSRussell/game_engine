#include "ml_optimizer.h"
#include "../core/logger.h"
#include "../core/time.h"
#include "../core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * =================================================================================================
 *                                   CORE - LOGICAL MACHINE LEARNING
 * =================================================================================================
 * 
 * GOAL: "Logical machine learning... push this engine to be alot more efficient".
 * STRATEGY: Runtime reinforcement learning to optimize engine parameters.
 */

// Telemetry data structure
typedef struct {
    f32 frame_time_ms;
    u32 draw_calls;
    u32 physics_bodies;
    u32 memory_usage_mb;
    f32 cpu_usage_percent;
    f32 gpu_usage_percent;
    u64 timestamp;
} FrameTelemetry;

// ML optimization state
typedef struct {
    FrameTelemetry *telemetry_buffer;
    u32 buffer_size;
    u32 buffer_index;
    u32 sample_count;
    
    // Optimization parameters
    f32 lod_bias_factor;
    u32 worker_thread_count;
    f32 quality_threshold;
    
    // Learning parameters
    f32 learning_rate;
    f32 exploration_rate;
    f32 performance_target_fps;
    
    // Performance tracking
    f32 average_fps;
    f32 fps_variance;
    u32 optimization_iterations;
    bool is_optimizing;
} MLOptimizerState;

static MLOptimizerState g_optimizer_state = {0};

void ML_OptimizeRuntime() {
    // [TASK_900] Collect Telemetry Frame Data
    collect_frame_telemetry();
    
    // [TASK_901] Train Lightweight Policy (LOD Bias)
    optimize_lod_bias_policy();
    
    // [TASK_902] Auto-Tune Job System
    optimize_job_system();
    
    // Apply optimizations
    apply_runtime_optimizations();
}

void ML_PredictAssets() {
    // [TASK_910] Prefetch Prediction
    analyze_movement_patterns();
    predict_required_assets();
    trigger_asset_prefetching();
}

static void collect_frame_telemetry() {
    if (!g_optimizer_state.telemetry_buffer) {
        // Initialize telemetry buffer
        g_optimizer_state.buffer_size = 1000; // Keep last 1000 frames
        g_optimizer_state.telemetry_buffer = malloc(sizeof(FrameTelemetry) * g_optimizer_state.buffer_size);
        if (!g_optimizer_state.telemetry_buffer) {
            LOG_ERROR("Failed to allocate telemetry buffer");
            return;
        }
        memset(g_optimizer_state.telemetry_buffer, 0, sizeof(FrameTelemetry) * g_optimizer_state.buffer_size);
    }
    
    // Collect current frame data
    FrameTelemetry *current = &g_optimizer_state.telemetry_buffer[g_optimizer_state.buffer_index];
    
    current->frame_time_ms = get_frame_time_ms();
    current->draw_calls = get_draw_call_count();
    current->physics_bodies = get_physics_body_count();
    current->memory_usage_mb = get_memory_usage_mb();
    current->cpu_usage_percent = get_cpu_usage_percent();
    current->gpu_usage_percent = get_gpu_usage_percent();
    current->timestamp = get_current_time_ns();
    
    // Update buffer index
    g_optimizer_state.buffer_index = (g_optimizer_state.buffer_index + 1) % g_optimizer_state.buffer_size;
    if (g_optimizer_state.sample_count < g_optimizer_state.buffer_size) {
        g_optimizer_state.sample_count++;
    }
    
    // Calculate performance metrics
    update_performance_metrics();
}

static void optimize_lod_bias_policy() {
    if (g_optimizer_state.sample_count < 100) return; // Need enough samples
    
    // Calculate current FPS performance
    f32 current_fps = 1000.0f / g_optimizer_state.average_fps;
    f32 target_fps = g_optimizer_state.performance_target_fps;
    
    // Calculate reward based on performance
    f32 reward = 0.0f;
    if (current_fps >= target_fps) {
        reward = 1.0f; // Good performance
    } else {
        reward = current_fps / target_fps; // Partial reward based on how close we are
    }
    
    // Adjust LOD bias based on reinforcement learning
    f32 adjustment = g_optimizer_state.learning_rate * (reward - 0.5f);
    
    // Add exploration noise
    if ((f32)rand() / RAND_MAX < g_optimizer_state.exploration_rate) {
        adjustment += ((f32)rand() / RAND_MAX - 0.5f) * 0.1f;
    }
    
    // Update LOD bias factor
    g_optimizer_state.lod_bias_factor += adjustment;
    g_optimizer_state.lod_bias_factor = clamp(g_optimizer_state.lod_bias_factor, 0.1f, 2.0f);
    
    printf("ML Optimizer: LOD bias adjusted to %.3f (FPS: %.1f, Reward: %.3f)\n",
           g_optimizer_state.lod_bias_factor, current_fps, reward);
    
    g_optimizer_state.optimization_iterations++;
}

static void optimize_job_system() {
    if (g_optimizer_state.sample_count < 100) return;
    
    // Analyze CPU usage patterns
    f32 avg_cpu = 0.0f;
    u32 samples_to_analyze = fminu(g_optimizer_state.sample_count, 100);
    
    for (u32 i = 0; i < samples_to_analyze; i++) {
        u32 index = (g_optimizer_state.buffer_index - 1 - i + g_optimizer_state.buffer_size) % g_optimizer_state.buffer_size;
        avg_cpu += g_optimizer_state.telemetry_buffer[index].cpu_usage_percent;
    }
    avg_cpu /= samples_to_analyze;
    
    // Determine optimal thread count
    u32 optimal_threads = g_optimizer_state.worker_thread_count;
    
    if (avg_cpu > 80.0f) {
        // CPU is overloaded, reduce threads
        optimal_threads = fmaxu(1, optimal_threads - 1);
    } else if (avg_cpu < 50.0f) {
        // CPU is underutilized, increase threads
        optimal_threads = fminu(16, optimal_threads + 1);
    }
    
    if (optimal_threads != g_optimizer_state.worker_thread_count) {
        printf("ML Optimizer: Adjusting worker threads from %u to %u (CPU: %.1f%%)\n",
               g_optimizer_state.worker_thread_count, optimal_threads, avg_cpu);
        g_optimizer_state.worker_thread_count = optimal_threads;
        
        // Notify job system of thread count change
        notify_job_system_thread_change(optimal_threads);
    }
}

static void apply_runtime_optimizations() {
    // Apply LOD bias to rendering system
    set_lod_bias_factor(g_optimizer_state.lod_bias_factor);
    
    // Apply quality threshold
    set_quality_threshold(g_optimizer_state.quality_threshold);
    
    // Update optimization flags
    g_optimizer_state.is_optimizing = true;
}

static void analyze_movement_patterns() {
    // Analyze player movement patterns from recent telemetry
    // This would integrate with the player movement system
    
    printf("ML Asset Predictor: Analyzing movement patterns...\n");
    
    // Simple pattern detection: check for consistent movement direction
    vec3 movement_vector = get_recent_movement_vector();
    f32 movement_magnitude = vec3_length(movement_vector);
    
    if (movement_magnitude > 0.1f) {
        // Player is moving, predict where they're going
        vec3 predicted_position = get_predicted_player_position(5.0f); // 5 seconds ahead
        printf("  Predicted position: (%.2f, %.2f, %.2f)\n", 
               predicted_position.x, predicted_position.y, predicted_position.z);
        
        // Store prediction for asset prefetching
        store_movement_prediction(predicted_position);
    }
}

static void predict_required_assets() {
    // Based on movement predictions, determine which assets will be needed
    printf("ML Asset Predictor: Predicting required assets...\n");
    
    // Get predicted positions
    vec3 *predictions = get_movement_predictions();
    u32 prediction_count = get_prediction_count();
    
    for (u32 i = 0; i < prediction_count; i++) {
        vec3 pos = predictions[i];
        
        // Determine which chunks/regions will be needed
        u32 chunk_x = (u32)floorf(pos.x / 16.0f);
        u32 chunk_z = (u32)floorf(pos.z / 16.0f);
        
        // Predict assets needed for this region
        predict_assets_for_chunk(chunk_x, chunk_z);
    }
}

static void trigger_asset_prefetching() {
    // Actually trigger the prefetching of predicted assets
    printf("ML Asset Predictor: Triggering asset prefetching...\n");
    
    // Get predicted assets
    AssetPrediction *predictions = get_asset_predictions();
    u32 prediction_count = get_asset_prediction_count();
    
    u32 prefetched_count = 0;
    for (u32 i = 0; i < prediction_count && prefetched_count < 10; i++) {
        if (predictions[i].confidence > 0.7f) {
            // High confidence prediction, prefetch this asset
            bool success = prefetch_asset(predictions[i].asset_id, predictions[i].priority);
            if (success) {
                prefetched_count++;
                printf("  Prefetched asset: %s (confidence: %.2f)\n", 
                       predictions[i].asset_name, predictions[i].confidence);
            }
        }
    }
    
    printf("ML Asset Predictor: Prefetched %u assets\n", prefetched_count);
}

static void update_performance_metrics() {
    if (g_optimizer_state.sample_count < 10) return;
    
    // Calculate average frame time
    f32 total_frame_time = 0.0f;
    u32 samples_to_average = fminu(g_optimizer_state.sample_count, 100);
    
    for (u32 i = 0; i < samples_to_average; i++) {
        u32 index = (g_optimizer_state.buffer_index - 1 - i + g_optimizer_state.buffer_size) % g_optimizer_state.buffer_size;
        total_frame_time += g_optimizer_state.telemetry_buffer[index].frame_time_ms;
    }
    
    g_optimizer_state.average_fps = total_frame_time / samples_to_average;
    
    // Calculate FPS variance
    f32 variance_sum = 0.0f;
    for (u32 i = 0; i < samples_to_average; i++) {
        u32 index = (g_optimizer_state.buffer_index - 1 - i + g_optimizer_state.buffer_size) % g_optimizer_state.buffer_size;
        f32 diff = g_optimizer_state.telemetry_buffer[index].frame_time_ms - g_optimizer_state.average_fps;
        variance_sum += diff * diff;
    }
    g_optimizer_state.fps_variance = variance_sum / samples_to_average;
}

// Helper functions (these would be implemented in the actual engine)
static f32 get_frame_time_ms() { return 16.67f; } // 60 FPS target
static u32 get_draw_call_count() { return 1000; }
static u32 get_physics_body_count() { return 500; }
static u32 get_memory_usage_mb() { return 1024; }
static f32 get_cpu_usage_percent() { return 45.0f; }
static f32 get_gpu_usage_percent() { return 60.0f; }
static u64 get_current_time_ns() { return 0; } // Placeholder

static void set_lod_bias_factor(f32 factor) { /* Implementation would set rendering LOD bias */ }
static void set_quality_threshold(f32 threshold) { /* Implementation would set quality threshold */ }
static void notify_job_system_thread_change(u32 thread_count) { /* Implementation would notify job system */ }

static vec3 get_recent_movement_vector() { return (vec3){1.0f, 0.0f, 0.5f}; }
static vec3 get_predicted_player_position(f32 seconds_ahead) { return (vec3){10.0f, 0.0f, 5.0f}; }
static void store_movement_prediction(vec3 position) { /* Implementation would store prediction */ }

static vec3* get_movement_predictions() { static vec3 predictions[1] = {{10.0f, 0.0f, 5.0f}}; return predictions; }
static u32 get_prediction_count() { return 1; }
static void predict_assets_for_chunk(u32 chunk_x, u32 chunk_z) { /* Implementation would predict assets */ }

typedef struct {
    char asset_id[64];
    char asset_name[64];
    f32 confidence;
    u32 priority;
} AssetPrediction;

static AssetPrediction* get_asset_predictions() { 
    static AssetPrediction predictions[1] = {{"terrain_chunk_10_5", "Terrain Chunk", 0.8f, 1}}; 
    return predictions; 
}
static u32 get_asset_prediction_count() { return 1; }
static bool prefetch_asset(const char* asset_id, u32 priority) { return true; }

// Public API functions
void ML_InitializeOptimizer() {
    memset(&g_optimizer_state, 0, sizeof(MLOptimizerState));
    
    // Set default parameters
    g_optimizer_state.lod_bias_factor = 1.0f;
    g_optimizer_state.worker_thread_count = 4;
    g_optimizer_state.quality_threshold = 0.8f;
    g_optimizer_state.learning_rate = 0.01f;
    g_optimizer_state.exploration_rate = 0.1f;
    g_optimizer_state.performance_target_fps = 60.0f;
    
    printf("ML Optimizer initialized\n");
}

void ML_ShutdownOptimizer() {
    if (g_optimizer_state.telemetry_buffer) {
        free(g_optimizer_state.telemetry_buffer);
        g_optimizer_state.telemetry_buffer = NULL;
    }
    
    memset(&g_optimizer_state, 0, sizeof(MLOptimizerState));
    printf("ML Optimizer shutdown\n");
}

void ML_GetOptimizationStats(f32 *avg_fps, f32 *fps_variance, u32 *iterations) {
    if (avg_fps) *avg_fps = 1000.0f / g_optimizer_state.average_fps;
    if (fps_variance) *fps_variance = g_optimizer_state.fps_variance;
    if (iterations) *iterations = g_optimizer_state.optimization_iterations;
}
