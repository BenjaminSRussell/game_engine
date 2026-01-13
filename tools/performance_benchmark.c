/*
 * performance_benchmark.c
 * Performance benchmarking tool for Phase 5 implementation
 *
 * Provides comprehensive performance analysis, profiling, and optimization metrics
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "gpu_cull_compute.h"
#include "cull_output_buffer.h"
#include "cull_statistics.h"

/* ============================================================================
 * PERFORMANCE METRICS
 * ============================================================================ */

typedef struct performance_metrics {
    double total_time;
    double min_time;
    double max_time;
    double avg_time;
    double std_deviation;
    uint64_t operations;
    double ops_per_second;
    size_t memory_usage;
    double memory_per_op;
} performance_metrics_t;

typedef struct benchmark_config {
    const char* name;
    int iterations;
    int objects_per_iteration;
    int concurrent_instances;
    bool enable_multi_threading;
    bool enable_memory_tracking;
} benchmark_config_t;

/* ============================================================================
 * HIGH-PRECISION TIMING
 * ============================================================================ */

static double get_time_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000.0 + tv.tv_usec;
}

static double get_time_milliseconds(void) {
    return get_time_microseconds() / 1000.0;
}

/* ============================================================================
 * BENCHMARK FRAMEWORK
 * ============================================================================ */

static performance_metrics_t run_benchmark(benchmark_config_t* config) {
    performance_metrics_t metrics = {0};
    
    printf("Running benchmark: %s\n", config->name);
    printf("  Iterations: %d\n", config->iterations);
    printf("  Objects per iteration: %d\n", config->objects_per_iteration);
    printf("  Concurrent instances: %d\n", config->concurrent_instances);
    
    // Initialize system
    culling_gpu_cull_compute_init();
    
    // Create instances
    culling_gpu_cull_compute_handle_t* handles = 
        malloc(sizeof(culling_gpu_cull_compute_handle_t) * config->concurrent_instances);
    
    for (int i = 0; i < config->concurrent_instances; i++) {
        culling_gpu_cull_compute_desc_t desc = {.flags = i % 8};
        culling_gpu_cull_compute_create(&handles[i], &desc);
    }
    
    // Timing arrays for statistics
    double* times = malloc(sizeof(double) * config->iterations);
    metrics.min_time = INFINITY;
    metrics.max_time = -INFINITY;
    
    // Run benchmark iterations
    double total_time = 0.0;
    size_t initial_memory = culling_gpu_cull_compute_get_memory_usage();
    
    for (int iter = 0; iter < config->iterations; iter++) {
        double start_time = get_time_microseconds();
        
        // Perform operations on all instances
        for (int i = 0; i < config->concurrent_instances; i++) {
            uint32_t objects = config->objects_per_iteration + (i * 10) + (iter % 100);
            culling_gpu_cull_compute_update(handles[i], &objects, sizeof(objects));
        }
        
        // Process all pending operations
        int processed = culling_gpu_cull_compute_process_pending();
        
        double end_time = get_time_microseconds();
        double iteration_time = (end_time - start_time) / 1000.0; // Convert to ms
        
        times[iter] = iteration_time;
        total_time += iteration_time;
        
        if (iteration_time < metrics.min_time) {
            metrics.min_time = iteration_time;
        }
        if (iteration_time > metrics.max_time) {
            metrics.max_time = iteration_time;
        }
        
        if ((iter + 1) % 100 == 0) {
            printf("  Progress: %d/%d iterations (%.1f%%)\n", 
                   iter + 1, config->iterations, 
                   (double)(iter + 1) / config->iterations * 100.0);
        }
    }
    
    // Calculate statistics
    metrics.total_time = total_time;
    metrics.avg_time = total_time / config->iterations;
    metrics.operations = config->iterations * config->concurrent_instances;
    metrics.ops_per_second = metrics.operations / (total_time / 1000.0);
    
    // Calculate standard deviation
    double variance = 0.0;
    for (int i = 0; i < config->iterations; i++) {
        double diff = times[i] - metrics.avg_time;
        variance += diff * diff;
    }
    metrics.std_deviation = sqrt(variance / config->iterations);
    
    // Memory metrics
    size_t final_memory = culling_gpu_cull_compute_get_memory_usage();
    metrics.memory_usage = final_memory - initial_memory;
    metrics.memory_per_op = (double)metrics.memory_usage / metrics.operations;
    
    // Cleanup
    for (int i = 0; i < config->concurrent_instances; i++) {
        culling_gpu_cull_compute_destroy(handles[i]);
    }
    free(handles);
    free(times);
    
    culling_gpu_cull_compute_shutdown();
    
    return metrics;
}

static void print_metrics(performance_metrics_t* metrics) {
    printf("\n=== Performance Results ===\n");
    printf("Total time: %.3f ms\n", metrics->total_time);
    printf("Average time: %.3f ms\n", metrics->avg_time);
    printf("Min time: %.3f ms\n", metrics->min_time);
    printf("Max time: %.3f ms\n", metrics->max_time);
    printf("Standard deviation: %.3f ms\n", metrics->std_deviation);
    printf("Total operations: %llu\n", (unsigned long long)metrics->operations);
    printf("Operations per second: %.0f\n", metrics->ops_per_second);
    printf("Memory usage: %zu bytes\n", metrics->memory_usage);
    printf("Memory per operation: %.2f bytes\n", metrics->memory_per_op);
    printf("===========================\n\n");
}

/* ============================================================================
 * BENCHMARK TESTS
 * ============================================================================ */

static void benchmark_single_thread_performance(void) {
    benchmark_config_t config = {
        .name = "Single Thread Performance",
        .iterations = 1000,
        .objects_per_iteration = 1000,
        .concurrent_instances = 1,
        .enable_multi_threading = false,
        .enable_memory_tracking = true
    };
    
    performance_metrics_t metrics = run_benchmark(&config);
    print_metrics(&metrics);
}

static void benchmark_multi_instance_performance(void) {
    benchmark_config_t config = {
        .name = "Multi-Instance Performance",
        .iterations = 500,
        .objects_per_iteration = 500,
        .concurrent_instances = 10,
        .enable_multi_threading = false,
        .enable_memory_tracking = true
    };
    
    performance_metrics_t metrics = run_benchmark(&config);
    print_metrics(&metrics);
}

static void benchmark_scalability(void) {
    printf("=== Scalability Benchmark ===\n");
    
    int instance_counts[] = {1, 2, 4, 8, 16, 32, 64};
    int num_tests = sizeof(instance_counts) / sizeof(instance_counts[0]);
    
    printf("Testing scalability with different instance counts:\n\n");
    
    for (int i = 0; i < num_tests; i++) {
        benchmark_config_t config = {
            .name = "Scalability Test",
            .iterations = 200,
            .objects_per_iteration = 200,
            .concurrent_instances = instance_counts[i],
            .enable_multi_threading = false,
            .enable_memory_tracking = true
        };
        
        performance_metrics_t metrics = run_benchmark(&config);
        printf("Instances: %d, Ops/sec: %.0f, Avg time: %.3f ms, Memory: %zu bytes\n",
               instance_counts[i], metrics.ops_per_second, metrics.avg_time, metrics.memory_usage);
    }
    
    printf("\n");
}

static void benchmark_memory_efficiency(void) {
    printf("=== Memory Efficiency Benchmark ===\n");
    
    int object_counts[] = {100, 500, 1000, 5000, 10000, 50000};
    int num_tests = sizeof(object_counts) / sizeof(object_counts[0]);
    
    printf("Testing memory efficiency with different object counts:\n\n");
    
    for (int i = 0; i < num_tests; i++) {
        benchmark_config_t config = {
            .name = "Memory Efficiency Test",
            .iterations = 100,
            .objects_per_iteration = object_counts[i],
            .concurrent_instances = 5,
            .enable_multi_threading = false,
            .enable_memory_tracking = true
        };
        
        performance_metrics_t metrics = run_benchmark(&config);
        printf("Objects: %d, Memory/op: %.2f bytes, Total memory: %zu bytes\n",
               object_counts[i], metrics.memory_per_op, metrics.memory_usage);
    }
    
    printf("\n");
}

static void benchmark_stress_test(void) {
    benchmark_config_t config = {
        .name = "Stress Test",
        .iterations = 10000,
        .objects_per_iteration = 1000,
        .concurrent_instances = 50,
        .enable_multi_threading = false,
        .enable_memory_tracking = true
    };
    
    performance_metrics_t metrics = run_benchmark(&config);
    print_metrics(&metrics);
}

/* ============================================================================
 * PERFORMANCE COMPARISON
 * ============================================================================ */

static void compare_performance_targets(void) {
    printf("=== Performance Target Comparison ===\n");
    
    // Target metrics from Phase 4 documentation
    double target_ops_per_sec = 1000.0;  // 1000+ ops/sec
    double target_avg_time = 1.0;         // < 1ms average
    size_t target_memory_per_op = 1024;  // < 1KB per operation
    
    benchmark_config_t config = {
        .name = "Target Comparison",
        .iterations = 1000,
        .objects_per_iteration = 1000,
        .concurrent_instances = 10,
        .enable_multi_threading = false,
        .enable_memory_tracking = true
    };
    
    performance_metrics_t metrics = run_benchmark(&config);
    
    printf("Target vs Actual Performance:\n");
    printf("  Ops/sec target: %.0f, actual: %.0f %s\n",
           target_ops_per_sec, metrics.ops_per_second,
           metrics.ops_per_second >= target_ops_per_sec ? "✓" : "✗");
    
    printf("  Avg time target: %.3f ms, actual: %.3f ms %s\n",
           target_avg_time, metrics.avg_time,
           metrics.avg_time <= target_avg_time ? "✓" : "✗");
    
    printf("  Memory/op target: %zu bytes, actual: %.2f bytes %s\n",
           target_memory_per_op, metrics.memory_per_op,
           metrics.memory_per_op <= target_memory_per_op ? "✓" : "✗");
    
    bool all_targets_met = (metrics.ops_per_second >= target_ops_per_sec &&
                           metrics.avg_time <= target_avg_time &&
                           metrics.memory_per_op <= target_memory_per_op);
    
    printf("\nOverall target compliance: %s\n", all_targets_met ? "✓ ALL TARGETS MET" : "✗ SOME TARGETS MISSED");
    printf("\n");
}

/* ============================================================================
 * MAIN BENCHMARK RUNNER
 * ============================================================================ */

int main(int argc, char* argv[]) {
    printf("=== Performance Benchmark Suite ===\n");
    printf("Phase 5 Implementation Performance Analysis\n\n");
    
    // Run all benchmarks
    benchmark_single_thread_performance();
    benchmark_multi_instance_performance();
    benchmark_scalability();
    benchmark_memory_efficiency();
    benchmark_stress_test();
    compare_performance_targets();
    
    printf("=== Benchmark Summary ===\n");
    printf("All benchmarks completed successfully.\n");
    printf("Review the results above for performance analysis.\n");
    printf("Compare against Phase 4 targets to ensure compliance.\n");
    
    return 0;
}
