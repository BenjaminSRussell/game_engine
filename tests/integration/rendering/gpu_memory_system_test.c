#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

// Include necessary headers for types
// We need to define some types that might be missing or conflicting if we don't include correct headers
// But since we are compiling this as a standalone unit including the source, we rely on the source includes.

// Mock Unified Logger
// We need to match the signature in unified_logger.h
#include "core/logger/unified_logger.h"

void unified_logger_log(LogLevel level, LogCategory category, const char* file, int line,
                     const char* function, const char* format, ...) {
    va_list args;
    va_start(args, format);

    const char* level_str = "INFO";
    if (level == LOG_LEVEL_ERROR) level_str = "ERROR";
    else if (level == LOG_LEVEL_WARN) level_str = "WARN";
    else if (level == LOG_LEVEL_DEBUG) level_str = "DEBUG";

    printf("[%s] ", level_str);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

// Stub for structured logging if needed (gpu_memory.c likely doesn't use it, but just in case)
void unified_logger_log_structured(LogLevel level, LogCategory category, const char* file,
                              int line, const char* function, const char* message,
                              const char* context_json) {
    printf("[STRUCT] %s: %s\n", message, context_json);
}

// Include the source file directly to test internal state and functions
// This allows us to compile this test file as a single unit without linking against the whole engine.
// This white-box testing approach allows access to internal definitions and state without exposing them in headers.
#include "src/rendering/gpu_memory.c"

int main() {
    setbuf(stdout, NULL);
    printf("=== Real GPU Memory Validation Test ===\n");

    // 1. Initialize
    printf("Initializing GPU memory manager...\n");
    if (!gpu_memory_init()) {
        fprintf(stderr, "Failed to initialize GPU memory manager\n");
        return 1;
    }

    if (!gpu_memory_is_initialized()) {
        fprintf(stderr, "GPU memory manager should be initialized\n");
        return 1;
    }

    // 2. Enable validation
    gpu_memory_enable_validation(true);

    // 3. Allocate memory
    printf("Allocating memory...\n");
    GPUMemoryAllocation alloc1 = gpu_memory_allocate(MEMORY_POOL_VERTEX, 1024, "VertexBuf1");
    if (alloc1.allocation_id == 0) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    printf("Allocated ID: %u, Size: %llu\n", alloc1.allocation_id, alloc1.size);

    GPUMemoryAllocation alloc2 = gpu_memory_allocate(MEMORY_POOL_TEXTURE, 2048, "Texture1");
    if (alloc2.allocation_id == 0) {
        fprintf(stderr, "Failed to allocate memory 2\n");
        return 1;
    }
    printf("Allocated ID: %u, Size: %llu\n", alloc2.allocation_id, alloc2.size);

    // 4. Validate state
    printf("Validating state...\n");
    if (!gpu_memory_validate_state()) {
        fprintf(stderr, "Validation failed!\n");
        return 1;
    }

    // 5. Check stats
    GPUMemoryStats stats;
    gpu_memory_get_stats(&stats);
    printf("Used memory: %llu\n", stats.used_memory);
    printf("Total allocations: %u\n", stats.total_allocations);

    if (stats.total_allocations != 2) {
        fprintf(stderr, "Expected 2 allocations, got %u\n", stats.total_allocations);
        return 1;
    }

    // 6. Free memory
    printf("Freeing memory...\n");
    gpu_memory_free(&alloc1);

    // 7. Validate again
    if (!gpu_memory_validate_state()) {
        fprintf(stderr, "Validation failed after free!\n");
        return 1;
    }

    gpu_memory_get_stats(&stats);
    printf("Used memory after free: %llu\n", stats.used_memory);
    printf("Total frees: %u\n", stats.total_frees);

    if (stats.total_frees != 1) {
        fprintf(stderr, "Expected 1 free, got %u\n", stats.total_frees);
        return 1;
    }

    // 8. Free remaining
    gpu_memory_free(&alloc2);

    // 9. Shutdown
    printf("Shutting down...\n");
    gpu_memory_shutdown();

    // 10. Check if we can detect leaks (optional, by not freeing and checking logs, but that's harder to assert in this simple test)
    // To properly test leak detection, we would need to capture the log output or modify the test to inspect internal state.
    // For now, we trust the manual calls above.

    printf("Test Passed!\n");
    return 0;
}
