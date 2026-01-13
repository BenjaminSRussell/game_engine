#include "core/logger/unified_logger.h"
#include "core/memory/unified_memory_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#undef memory_alloc
#undef memory_realloc
#undef memory_free

// Memory stubs
void* memory_alloc(size_t size) {
    return calloc(1, size);
}
void* memory_realloc(void* block, size_t size) {
    return realloc(block, size);
}
void memory_free(void* block) {
    free(block);
}

// Unified Memory Stubs
void* unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags,
                        const char* file, int line, const char* function) {
    return calloc(1, size);
}

void* unified_memory_realloc(void* ptr, size_t new_size, MemoryFlags flags,
                           const char* file, int line, const char* function) {
    return realloc(ptr, new_size);
}

void unified_memory_free(void* ptr, const char* file, int line, const char* function) {
    free(ptr);
}


// Logger stubs
void unified_logger_log(LogLevel level, LogCategory category, const char* file, int line,
                     const char* function, const char* format, ...) {
    va_list args;
    va_start(args, format);
    // vprintf(format, args);
    // printf("\n");
    va_end(args);
}
