/*
 * benchmark_raytracing.c
 * Performance benchmarks for Ray Tracing subsystem
 */

#include "../acceleration_structures/blas_builder.h"
#include "../acceleration_structures/tlas_builder.h"
#include <time.h>
#include <stdio.h>

#define ITERATIONS 100

void benchmark_blas_creation() {
    printf("Benchmarking BLAS Creation (%d iterations)...\n", ITERATIONS);
    
    acceleration_structures_blas_builder_init();
    
    clock_t start = clock();
    for(int i = 0; i < ITERATIONS; i++) {
        acceleration_structures_blas_builder_handle_t handle;
        acceleration_structures_blas_builder_desc_t desc = {0};
        acceleration_structures_blas_builder_create(&handle, &desc);
        acceleration_structures_blas_builder_destroy(handle);
    }
    clock_t end = clock();
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("BLAS Creation: %f seconds total, %f seconds average\n", time_taken, time_taken / ITERATIONS);
    
    acceleration_structures_blas_builder_shutdown();
}

void benchmark_tlas_creation() {
    printf("Benchmarking TLAS Creation (%d iterations)...\n", ITERATIONS);
    
    acceleration_structures_tlas_builder_init();
    
    clock_t start = clock();
    for(int i = 0; i < ITERATIONS; i++) {
        acceleration_structures_tlas_builder_handle_t handle;
        acceleration_structures_tlas_builder_desc_t desc = {0};
        acceleration_structures_tlas_builder_create(&handle, &desc);
        acceleration_structures_tlas_builder_destroy(handle);
    }
    clock_t end = clock();
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("TLAS Creation: %f seconds total, %f seconds average\n", time_taken, time_taken / ITERATIONS);
    
    acceleration_structures_tlas_builder_shutdown();
}

int main() {
    printf("=== Starting Ray Tracing Benchmarks ===\n");
    benchmark_blas_creation();
    benchmark_tlas_creation();
    printf("=== Benchmarks Completed ===\n");
    return 0;
}
