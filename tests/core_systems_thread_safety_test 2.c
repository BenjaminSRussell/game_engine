/**
 * Core Systems Thread Safety Verification
 *
 * This test verifies the thread safety of the core systems (Memory, Logger).
 * It spawns multiple threads that concurrently use these systems to ensure
 * no race conditions, crashes, or corruptions occur.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Include core system headers
// We assume -I src/engine/include/core is used
#include "logger/unified_logger.h"
#include "memory/unified_memory_allocator.h"

#define NUM_THREADS 8
#define ITERATIONS_PER_THREAD 1000

// Thread data structure
typedef struct {
    int thread_id;
    int allocations;
    int logs;
    int errors;
} ThreadData;

// Worker thread function
void* worker_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char thread_name[32];
    snprintf(thread_name, sizeof(thread_name), "Thread-%d", data->thread_id);

    // Random seed per thread
    unsigned int seed = time(NULL) + data->thread_id;

    for (int i = 0; i < ITERATIONS_PER_THREAD; i++) {
        // 1. Log something
        if (i % 10 == 0) {
            unified_logger_log(LOG_LEVEL_INFO, LOG_CAT_GENERAL, __FILE__, __LINE__, thread_name,
                             "Iteration %d started", i);
            data->logs++;
        }

        // 2. Allocate memory
        size_t size = (rand_r(&seed) % 1024) + 1;
        // Use MEMORY_FLAG_TRACK to test tracking logic and locking
        void* ptr = unified_memory_alloc(size, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_TRACK | MEMORY_FLAG_ZERO,
                                       __FILE__, __LINE__, thread_name);

        if (ptr) {
            data->allocations++;

            // 3. Write to memory to check for corruption/overlap (rudimentary)
            memset(ptr, (data->thread_id & 0xFF), size);

            // Simulate work
            if (i % 100 == 0) {
                usleep(100);
            }

            // 4. Verify memory content (rudimentary)
            unsigned char* bytes = (unsigned char*)ptr;
            if (bytes[0] != (data->thread_id & 0xFF)) {
                unified_logger_log(LOG_LEVEL_ERROR, LOG_CAT_MEMORY, __FILE__, __LINE__, thread_name,
                                 "Memory corruption detected at %p!", ptr);
                data->errors++;
            }

            // 5. Free memory
            unified_memory_free(ptr, __FILE__, __LINE__, thread_name);
        } else {
            unified_logger_log(LOG_LEVEL_ERROR, LOG_CAT_MEMORY, __FILE__, __LINE__, thread_name,
                             "Allocation failed size=%zu", size);
            data->errors++;
        }
    }

    unified_logger_log(LOG_LEVEL_INFO, LOG_CAT_GENERAL, __FILE__, __LINE__, thread_name,
                     "Thread finished. Allocations: %d, Logs: %d, Errors: %d",
                     data->allocations, data->logs, data->errors);

    return NULL;
}

int main(int argc, char* argv[]) {
    printf("Starting Core Systems Thread Safety Verification...\n");

    // 1. Initialize Logger
    LoggerConfig log_config = {0};
    log_config.min_level = LOG_LEVEL_INFO;
    log_config.enabled_channels = LOG_CHANNEL_CONSOLE; // | LOG_CHANNEL_FILE;
    // Enable all categories
    for (int i = 0; i < LOG_CAT_COUNT; i++) log_config.enabled_categories[i] = true;
    log_config.use_colors = true;
    log_config.use_buffering = false; // Direct output for better threading stress

    if (!unified_logger_init(&log_config)) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    // 2. Initialize Memory
    MemoryPolicy mem_policy = {0};
    mem_policy.global_limit = 1024 * 1024 * 1024; // 1GB
    mem_policy.max_allocations = 1000000;
    mem_policy.enable_leak_detection = true;
    mem_policy.enable_fragmentation_check = true;
    mem_policy.enable_canaries = true; // Test canaries

    if (!unified_memory_init(&mem_policy)) {
        fprintf(stderr, "Failed to initialize memory\n");
        return 1;
    }

    printf("Systems initialized. Spawning %d threads...\n", NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].allocations = 0;
        thread_data[i].logs = 0;
        thread_data[i].errors = 0;
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return 1;
        }
    }

    // Wait for completion
    int total_errors = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_errors += thread_data[i].errors;
    }

    printf("All threads joined.\n");

    // Print Stats
    unified_memory_print_stats();

    // Shutdown
    unified_memory_shutdown(); // Checks for leaks
    unified_logger_shutdown();

    if (total_errors == 0) {
        printf("✅ Core Systems Thread Safety Verification PASSED\n");
        return 0;
    } else {
        printf("❌ Core Systems Thread Safety Verification FAILED with %d errors\n", total_errors);
        return 1;
    }
}
