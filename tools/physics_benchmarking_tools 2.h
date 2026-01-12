/**
 * @file physics_benchmarking_tools.h
 * @brief Physics benchmarking tools
 *
 * Comprehensive benchmarking suite for physics systems including
 * performance measurement, profiling, and analysis tools.
 */

#ifndef PHYSICS_BENCHMARKING_TOOLS_H
#define PHYSICS_BENCHMARKING_TOOLS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Benchmark Types
// ========================================

typedef enum BenchmarkType {
    BENCHMARK_TYPE_THROUGHPUT,       // Throughput benchmark
    BENCHMARK_TYPE_LATENCY,           // Latency benchmark
    BENCHMARK_TYPE_STRESS,            // Stress benchmark
    BENCHMARK_TYPE_SCALABILITY,       // Scalability benchmark
    BENCHMARK_TYPE_MEMORY,            // Memory benchmark
    BENCHMARK_TYPE_ACCURACY,          // Accuracy benchmark
    BENCHMARK_TYPE_STABILITY,         // Stability benchmark
    BENCHMARK_TYPE_REGRESSION,        // Regression benchmark
    BENCHMARK_TYPE_CUSTOM = 1000      // Custom benchmark
} BenchmarkType;

// ========================================
// Benchmark Categories
// ========================================

typedef enum BenchmarkCategory {
    BENCHMARK_CATEGORY_COLLISION,      // Collision detection
    BENCHMARK_CATEGORY_SOLVER,         // Constraint solver
    BENCHMARK_CATEGORY_INTEGRATION,    // Integration
    BENCHMARK_CATEGORY_BROADPHASE,     // Broadphase
    BENCHMARK_CATEGORY_NARROWPHASE,    // Narrowphase
    BENCHMARK_CATEGORY_CLOTH,          // Cloth simulation
    BENCHMARK_CATEGORY_FLUID,          // Fluid simulation
    BENCHMARK_CATEGORY_DESTRUCTION,    // Destruction system
    BENCHMARK_CATEGORY_NETWORKING,     // Networking
    BENCHMARK_CATEGORY_SERIALIZATION,  // Serialization
    BENCHMARK_CATEGORY_ALL             // All categories
} BenchmarkCategory;

// ========================================
// Benchmark Result
// ========================================

typedef struct BenchmarkResult {
    char name[128];                  // Benchmark name
    char category[64];                // Benchmark category
    BenchmarkType type;               // Benchmark type
    bool passed;                      // Benchmark passed
    float score;                      // Performance score
    float baseline_score;             // Baseline score
    float improvement_percent;        // Improvement percentage
    
    // Timing metrics
    float total_time_seconds;         // Total execution time
    float average_time_seconds;        // Average time per iteration
    float min_time_seconds;           // Minimum time
    float max_time_seconds;           // Maximum time
    float median_time_seconds;        // Median time
    float std_deviation_seconds;       // Standard deviation
    
    // Throughput metrics
    float operations_per_second;      // Operations per second
    float frames_per_second;          // Frames per second
    float objects_per_second;         // Objects processed per second
    float contacts_per_second;        // Contacts processed per second
    
    // Memory metrics
    size_t peak_memory_usage;         // Peak memory usage
    size_t average_memory_usage;      // Average memory usage
    size_t memory_allocations;        // Memory allocations
    size_t memory_deallocations;      // Memory deallocations
    float memory_efficiency;          // Memory efficiency score
    
    // Quality metrics
    float accuracy_score;             // Accuracy score
    float stability_score;            // Stability score
    float convergence_rate;          // Convergence rate
    int iteration_count;              // Number of iterations
    
    // System metrics
    float cpu_usage_percent;          // CPU usage percentage
    float gpu_usage_percent;          // GPU usage percentage
    float cache_hit_rate;             // Cache hit rate
    float branch_miss_rate;            // Branch miss rate
    
    // Metadata
    uint64_t timestamp;               // Benchmark timestamp
    char hardware_info[256];          // Hardware information
    char software_info[256];          // Software information
    char notes[512];                  // Additional notes
    
} BenchmarkResult;

// ========================================
// Benchmark Configuration
// ========================================

typedef struct BenchmarkConfig {
    int iterations;                    // Number of iterations
    float warmup_time_seconds;         // Warmup time
    float measurement_time_seconds;    // Measurement time
    int thread_count;                 // Thread count
    bool enable_profiling;            // Enable profiling
    bool enable_memory_tracking;       // Enable memory tracking
    bool enable_cpu_monitoring;        // Enable CPU monitoring
    bool enable_gpu_monitoring;        // Enable GPU monitoring
    float tolerance;                   // Result tolerance
    char baseline_file[256];          // Baseline file path
    char output_file[256];            // Output file path
    bool verbose_output;               // Verbose output
    bool save_detailed_results;        // Save detailed results
} BenchmarkConfig;

// ========================================
// Benchmark Suite
// ========================================

typedef struct BenchmarkSuite {
    char name[128];                  // Suite name
    char description[256];            // Suite description
    BenchmarkCategory category;        // Suite category
    BenchmarkResult *results;          // Results array
    int result_count;                 // Number of results
    int result_capacity;              // Result capacity
    BenchmarkConfig config;            // Configuration
    bool enabled;                     // Suite enabled
    float overall_score;              // Overall score
    uint64_t start_time;              // Start time
    uint64_t end_time;                // End time
    void *user_data;                 // User data
} BenchmarkSuite;

// ========================================
// Benchmark Runner
// ========================================

typedef struct BenchmarkRunner {
    BenchmarkSuite *suites;           // Suite array
    int suite_count;                  // Number of suites
    int suite_capacity;               // Suite capacity
    BenchmarkConfig global_config;    // Global configuration
    bool running;                     // Currently running
    uint64_t start_time;              // Start time
    uint64_t end_time;                // End time
    float overall_score;              // Overall score
    char output_directory[512];        // Output directory
    bool save_results;                 // Save results
    bool generate_reports;             // Generate reports
    void *user_data;                 // User data
} BenchmarkRunner;

// ========================================
// Performance Profiler
// ========================================

typedef struct PerformanceProfiler {
    bool enabled;                     // Profiler enabled
    uint64_t start_time;              // Start time
    uint64_t end_time;                // End time
    
    // Timing data
    float *frame_times;               // Frame times array
    int frame_count;                  // Frame count
    int frame_capacity;               // Frame capacity
    
    // Memory data
    size_t *memory_usage;             // Memory usage array
    int memory_sample_count;          // Memory sample count
    int memory_sample_capacity;       // Memory sample capacity
    
    // CPU data
    float *cpu_usage;                 // CPU usage array
    int cpu_sample_count;             // CPU sample count
    int cpu_sample_capacity;          // CPU sample capacity
    
    // GPU data
    float *gpu_usage;                 // GPU usage array
    int gpu_sample_count;             // GPU sample count
    int gpu_sample_capacity;          // GPU sample capacity
    
    // Statistics
    float average_frame_time;         // Average frame time
    float min_frame_time;             // Minimum frame time
    float max_frame_time;             // Maximum frame time
    float average_memory_usage;       // Average memory usage
    float peak_memory_usage;          // Peak memory usage
    float average_cpu_usage;          // Average CPU usage
    float average_gpu_usage;          // Average GPU usage
    
} PerformanceProfiler;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create benchmark runner
 * @param max_suites Maximum number of suites
 * @return Benchmark runner or NULL on failure
 */
BenchmarkRunner* benchmark_runner_create(int max_suites);

/**
 * Destroy benchmark runner
 * @param runner Benchmark runner
 */
void benchmark_runner_destroy(BenchmarkRunner *runner);

/**
 * Create performance profiler
 * @param max_samples Maximum number of samples
 * @return Profiler or NULL on failure
 */
PerformanceProfiler* performance_profiler_create(int max_samples);

/**
 * Destroy performance profiler
 * @param profiler Profiler
 */
void performance_profiler_destroy(PerformanceProfiler *profiler);

// ========================================
// Suite Management
// ========================================

/**
 * Create benchmark suite
 * @param runner Benchmark runner
 * @param name Suite name
 * @param description Suite description
 * @param category Suite category
 * @return Suite pointer or NULL on failure
 */
BenchmarkSuite* benchmark_runner_create_suite(BenchmarkRunner *runner, const char *name,
                                              const char *description, BenchmarkCategory category);

/**
 * Add benchmark result to suite
 * @param suite Benchmark suite
 * @param result Benchmark result
 * @return True if successful
 */
bool benchmark_suite_add_result(BenchmarkSuite *suite, const BenchmarkResult *result);

/**
 * Remove benchmark result from suite
 * @param suite Benchmark suite
 * @param name Result name
 * @return True if successful
 */
bool benchmark_suite_remove_result(BenchmarkSuite *suite, const char *name);

/**
 * Get benchmark result by name
 * @param suite Benchmark suite
 * @param name Result name
 * @return Result pointer or NULL if not found
 */
BenchmarkResult* benchmark_suite_get_result(BenchmarkSuite *suite, const char *name);

// ========================================
// Benchmark Execution
// ========================================

/**
 * Run all benchmarks
 * @param runner Benchmark runner
 * @return True if all benchmarks passed
 */
bool benchmark_runner_run_all(BenchmarkRunner *runner);

/**
 * Run benchmark suite
 * @param runner Benchmark runner
 * @param suite_name Suite name
 * @return True if suite passed
 */
bool benchmark_runner_run_suite(BenchmarkRunner *runner, const char *suite_name);

/**
 * Run benchmark by name
 * @param runner Benchmark runner
 * @param suite_name Suite name
 * @param benchmark_name Benchmark name
 * @return True if benchmark passed
 */
bool benchmark_runner_run_benchmark(BenchmarkRunner *runner, const char *suite_name,
                                     const char *benchmark_name);

/**
 * Run benchmarks by category
 * @param runner Benchmark runner
 * @param category Benchmark category
 * @return True if category passed
 */
bool benchmark_runner_run_category(BenchmarkRunner *runner, BenchmarkCategory category);

// ========================================
// Built-in Benchmarks
// ========================================

/**
 * Run collision detection benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_collision_detection(BenchmarkSuite *suite);

/**
 * Run constraint solver benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_constraint_solver(BenchmarkSuite *suite);

/**
 * Run broadphase benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_broadphase(BenchmarkSuite *suite);

/**
 * Run narrowphase benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_narrowphase(BenchmarkSuite *suite);

/**
 * Run integration benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_integration(BenchmarkSuite *suite);

/**
 * Run cloth simulation benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_cloth_simulation(BenchmarkSuite *suite);

/**
 * Run fluid simulation benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_fluid_simulation(BenchmarkSuite *suite);

/**
 * Run destruction system benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_destruction_system(BenchmarkSuite *suite);

/**
 * Run networking benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_networking(BenchmarkSuite *suite);

/**
 * Run serialization benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_serialization(BenchmarkSuite *suite);

/**
 * Run memory benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_memory(BenchmarkSuite *suite);

/**
 * Run scalability benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_scalability(BenchmarkSuite *suite);

/**
 * Run stress benchmarks
 * @param suite Benchmark suite
 * @return True if successful
 */
bool benchmark_run_stress(BenchmarkSuite *suite);

// ========================================
// Performance Profiling
// ========================================

/**
 * Start performance profiling
 * @param profiler Profiler
 */
void performance_profiler_start(PerformanceProfiler *profiler);

/**
 * Stop performance profiling
 * @param profiler Profiler
 */
void performance_profiler_stop(PerformanceProfiler *profiler);

/**
 * Sample frame time
 * @param profiler Profiler
 * @param frame_time Frame time in seconds
 */
void performance_profiler_sample_frame_time(PerformanceProfiler *profiler, float frame_time);

/**
 * Sample memory usage
 * @param profiler Profiler
 * @param memory_usage Memory usage in bytes
 */
void performance_profiler_sample_memory_usage(PerformanceProfiler *profiler, size_t memory_usage);

/**
 * Sample CPU usage
 * @param profiler Profiler
 * @param cpu_usage CPU usage percentage
 */
void performance_profiler_sample_cpu_usage(PerformanceProfiler *profiler, float cpu_usage);

/**
 * Sample GPU usage
 * @param profiler Profiler
 * @param gpu_usage GPU usage percentage
 */
void performance_profiler_sample_gpu_usage(PerformanceProfiler *profiler, float gpu_usage);

/**
 * Calculate profiler statistics
 * @param profiler Profiler
 */
void performance_profiler_calculate_stats(PerformanceProfiler *profiler);

// ========================================
// Benchmark Utilities
// ========================================

/**
 * Measure execution time
 * @param function Function to measure
 * @param iterations Number of iterations
 * @param result Output benchmark result
 * @return True if successful
 */
bool benchmark_measure_execution_time(bool (*function)(void), int iterations, BenchmarkResult *result);

/**
 * Measure throughput
 * @param function Function to measure
 * @param duration_seconds Duration in seconds
 * @param result Output benchmark result
 * @return True if successful
 */
bool benchmark_measure_throughput(bool (*function)(void), float duration_seconds, BenchmarkResult *result);

/**
 * Measure memory usage
 * @param function Function to measure
 * @param result Output benchmark result
 * @return True if successful
 */
bool benchmark_measure_memory_usage(bool (*function)(void), BenchmarkResult *result);

/**
 * Measure scalability
 * @param function Function to measure
 * @param thread_counts Thread count array
 * @param thread_count_count Number of thread counts
 * @param result Output benchmark result
 * @return True if successful
 */
bool benchmark_measure_scalability(bool (*function)(int), const int *thread_counts,
                                   int thread_count_count, BenchmarkResult *result);

/**
 * Generate test data
 * @param object_count Number of objects
 * @param complexity Complexity level (1-10)
 * @return Test data or NULL on failure
 */
void* benchmark_generate_test_data(int object_count, int complexity);

/**
 * Cleanup test data
 * @param test_data Test data
 */
void benchmark_cleanup_test_data(void *test_data);

/**
 * Calculate performance score
 * @param result Benchmark result
 * @param baseline Baseline score
 * @return Performance score
 */
float benchmark_calculate_score(const BenchmarkResult *result, float baseline);

/**
 * Compare benchmark results
 * @param result1 First result
 * @param result2 Second result
 * @return Comparison score (>0 if result1 is better)
 */
float benchmark_compare_results(const BenchmarkResult *result1, const BenchmarkResult *result2);

// ========================================
// Results and Reporting
// ========================================

/**
 * Save benchmark results
 * @param runner Benchmark runner
 * @param filename Output filename
 * @param format Output format (json, xml, csv)
 * @return True if successful
 */
bool benchmark_save_results(BenchmarkRunner *runner, const char *filename, const char *format);

/**
 * Load benchmark results
 * @param filename Input filename
 * @param format Input format (json, xml, csv)
 * @return Loaded results or NULL on failure
 */
BenchmarkRunner* benchmark_load_results(const char *filename, const char *format);

/**
 * Generate benchmark report
 * @param runner Benchmark runner
 * @param filename Output filename
 * @param format Report format (html, pdf, text)
 * @return True if successful
 */
bool benchmark_generate_report(BenchmarkRunner *runner, const char *filename, const char *format);

/**
 * Print benchmark results
 * @param runner Benchmark runner
 */
void benchmark_print_results(BenchmarkRunner *runner);

/**
 * Print suite results
 * @param suite Benchmark suite
 */
void benchmark_print_suite_results(BenchmarkSuite *suite);

/**
 * Get overall statistics
 * @param runner Benchmark runner
 * @param total_benchmarks Total benchmarks
 * @param passed_benchmarks Passed benchmarks
 * @param failed_benchmarks Failed benchmarks
 * @param average_score Average score
 */
void benchmark_get_overall_stats(BenchmarkRunner *runner, int *total_benchmarks,
                                  int *passed_benchmarks, int *failed_benchmarks,
                                  float *average_score);

// ========================================
// Configuration
// ========================================

/**
 * Set benchmark configuration
 * @param runner Benchmark runner
 * @param config Configuration
 */
void benchmark_runner_set_config(BenchmarkRunner *runner, const BenchmarkConfig *config);

/**
 * Get benchmark configuration
 * @param runner Benchmark runner
 * @return Configuration
 */
BenchmarkConfig benchmark_runner_get_config(BenchmarkRunner *runner);

/**
 * Set output directory
 * @param runner Benchmark runner
 * @param directory Output directory
 */
void benchmark_set_output_directory(BenchmarkRunner *runner, const char *directory);

/**
 * Enable/disable result saving
 * @param runner Benchmark runner
 * @param enabled Enable saving
 */
void benchmark_set_save_results(BenchmarkRunner *runner, bool enabled);

/**
 * Enable/disable report generation
 * @param runner Benchmark runner
 * @param enabled Enable reports
 */
void benchmark_set_generate_reports(BenchmarkRunner *runner, bool enabled);

// ========================================
// Baseline Management
// ========================================

/**
 * Save baseline
 * @param runner Benchmark runner
 * @param filename Baseline filename
 * @return True if successful
 */
bool benchmark_save_baseline(BenchmarkRunner *runner, const char *filename);

/**
 * Load baseline
 * @param filename Baseline filename
 * @return Loaded baseline or NULL on failure
 */
BenchmarkRunner* benchmark_load_baseline(const char *filename);

/**
 * Compare with baseline
 * @param current Current results
 * @param baseline Baseline results
 * @param comparison Output comparison results
 * @return True if successful
 */
bool benchmark_compare_with_baseline(BenchmarkRunner *current, BenchmarkRunner *baseline,
                                      BenchmarkRunner *comparison);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_BENCHMARKING_TOOLS_H */
