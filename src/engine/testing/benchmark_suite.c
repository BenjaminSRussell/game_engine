#include "testing/render_tests.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct benchmark_result {
    char name[64];
    double min_ms;
    double max_ms;
    double avg_ms;
    uint32_t sample_count;
} benchmark_result_t;

// Timer helper
static double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

benchmark_result_t benchmark_run(render_test_context_t* ctx, const char* name,
                                  void (*bench_func)(void*), void* user_data,
                                  uint32_t iterations) {
    benchmark_result_t result;
    strncpy(result.name, name, sizeof(result.name));
    result.sample_count = iterations;
    result.min_ms = 1e9; // Large start
    result.max_ms = 0;
    result.avg_ms = 0;

    double total_ms = 0;

    // Warmup
    for(int i=0; i<3; i++) {
        if (bench_func) bench_func(user_data);
    }

    printf("Running Benchmark: %s (%d samples)\n", name, iterations);

    for (uint32_t i = 0; i < iterations; i++) {
        double start = get_time_ms();
        
        if (bench_func) bench_func(user_data);
        
        double end = get_time_ms();
        double duration = end - start;

        if (duration < result.min_ms) result.min_ms = duration;
        if (duration > result.max_ms) result.max_ms = duration;
        total_ms += duration;
    }

    result.avg_ms = total_ms / iterations;

    printf("  Result: Avg: %.2fms | Min: %.2fms | Max: %.2fms\n", 
           result.avg_ms, result.min_ms, result.max_ms);

    return result;
}
