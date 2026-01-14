/**
 * @file memory_leak_detector.c
 * @brief Memory leak detection and tracking system implementation
 */

#include "memory_leak_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

// ========================================
// Internal Constants
// ========================================

#define HASH_TABLE_SIZE 65536
#define MAX_MEMORY_RECORDS 1000000
#define STACK_TRACE_MAX_FRAMES 16
#define REPORT_BUFFER_SIZE 65536

// ========================================
// Internal State
// ========================================

static MemoryLeakConfig g_config = {0};
static MemoryStatistics g_statistics = {0};
static bool g_initialized = false;
static bool g_debug_mode = false;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hash table for fast lookup
static MemoryRecord *g_hash_table[HASH_TABLE_SIZE] = {NULL};

// Linked list for all records
static MemoryRecord *g_all_records = NULL;
static MemoryRecord *g_all_records_tail = NULL;

// Dynamic array for records
static MemoryRecord *g_records_array = NULL;
static uint32_t g_records_capacity = 0;
static uint32_t g_records_count = 0;

// Callbacks
static void (*g_allocation_callback)(MemoryRecord*) = NULL;
static void (*g_deallocation_callback)(MemoryRecord*) = NULL;

// ========================================
// Internal Helper Functions
// ========================================

static uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

static uint32_t get_thread_id(void) {
    return (uint32_t)pthread_self();
}

static uint32_t hash_pointer(void *ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (uint32_t)(addr ^ (addr >> 16)) % HASH_TABLE_SIZE;
}

static uint32_t get_next_allocation_id(void) {
    static uint32_t next_id = 1;
    return __sync_fetch_and_add(&next_id, 1);
}

static int get_size_bucket(size_t size) {
    if (size < 64) return 0;
    if (size < 128) return 1;
    if (size < 256) return 2;
    if (size < 512) return 3;
    if (size < 1024) return 4;
    if (size < 2048) return 5;
    if (size < 4096) return 6;
    if (size < 8192) return 7;
    if (size < 16384) return 8;
    if (size < 32768) return 9;
    if (size < 65536) return 10;
    if (size < 131072) return 11;
    if (size < 262144) return 12;
    if (size < 524288) return 13;
    if (size < 1048576) return 14;
    if (size < 2097152) return 15;
    if (size < 4194304) return 16;
    if (size < 8388608) return 17;
    if (size < 16777216) return 18;
    if (size < 33554432) return 19;
    if (size < 67108864) return 20;
    if (size < 134217728) return 21;
    if (size < 268435456) return 22;
    if (size < 536870912) return 23;
    if (size < 1073741824) return 24;
    if (size < 2147483648ULL) return 25;
    return 26;
}

static bool should_ignore_allocation(const char *file) {
    if (!g_config.ignore_pattern_count) return false;
    
    for (int i = 0; i < g_config.ignore_pattern_count; i++) {
        if (strstr(file, g_config.ignore_patterns[i])) {
            return true;
        }
    }
    return false;
}

static MemoryRecord* create_memory_record(void *ptr, size_t size, 
                                          const char *file, int line, const char *function) {
    if (g_records_count >= g_records_capacity) {
        // Expand capacity
        uint32_t new_capacity = g_records_capacity ? g_records_capacity * 2 : 1024;
        MemoryRecord *new_array = realloc(g_records_array, new_capacity * sizeof(MemoryRecord));
        if (!new_array) return NULL;
        
        g_records_array = new_array;
        g_records_capacity = new_capacity;
    }
    
    MemoryRecord *record = &g_records_array[g_records_count++];
    memset(record, 0, sizeof(MemoryRecord));
    
    record->ptr = ptr;
    record->size = size;
    record->file = file;
    record->line = line;
    record->function = function;
    record->timestamp = get_timestamp_ms();
    record->allocation_id = get_next_allocation_id();
    record->thread_id = get_thread_id();
    record->is_freed = false;
    
    return record;
}

static void add_to_hash_table(MemoryRecord *record) {
    uint32_t hash = hash_pointer(record->ptr);
    record->next = g_hash_table[hash];
    if (g_hash_table[hash]) {
        g_hash_table[hash]->prev = record;
    }
    g_hash_table[hash] = record;
}

static void remove_from_hash_table(MemoryRecord *record) {
    uint32_t hash = hash_pointer(record->ptr);
    
    if (record->next) {
        record->next->prev = record->prev;
    }
    if (record->prev) {
        record->prev->next = record->next;
    } else {
        g_hash_table[hash] = record->next;
    }
    
    record->next = NULL;
    record->prev = NULL;
}

static void update_statistics_allocation(MemoryRecord *record) {
    g_statistics.total_allocations++;
    g_statistics.current_allocations++;
    g_statistics.total_bytes_allocated += record->size;
    g_statistics.current_bytes_allocated += record->size;
    
    if (g_statistics.current_allocations > g_statistics.peak_allocations) {
        g_statistics.peak_allocations = g_statistics.current_allocations;
    }
    
    if (g_statistics.current_bytes_allocated > g_statistics.peak_bytes_allocated) {
        g_statistics.peak_bytes_allocated = g_statistics.current_bytes_allocated;
    }
    
    // Update size buckets
    int bucket = get_size_bucket(record->size);
    if (bucket < 32) {
        g_statistics.allocation_count_by_size[bucket]++;
        g_statistics.bytes_by_size_bucket[bucket] += record->size;
    }
    
    // Update thread statistics
    int thread_index = record->thread_id % 16;
    g_statistics.allocations_by_thread[thread_index]++;
    
    // Update average allocation size
    g_statistics.average_allocation_size = 
        (double)g_statistics.total_bytes_allocated / g_statistics.total_allocations;
}

static void update_statistics_deallocation(MemoryRecord *record) {
    g_statistics.total_deallocations++;
    g_statistics.current_allocations--;
    g_statistics.current_bytes_allocated -= record->size;
    
    // Update average deallocation rate
    static uint64_t last_update_time = 0;
    uint64_t current_time = get_timestamp_ms();
    if (last_update_time > 0) {
        double time_diff = (current_time - last_update_time) / 1000.0;
        if (time_diff > 0) {
            g_statistics.deallocation_rate = 1.0 / time_diff;
        }
    }
    last_update_time = current_time;
}

// ========================================
// Public API Implementation
// ========================================

bool memory_leak_detector_init(const MemoryLeakConfig *config) {
    pthread_mutex_lock(&g_mutex);
    
    if (g_initialized) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    
    // Copy configuration
    if (config) {
        g_config = *config;
    } else {
        // Default configuration
        memset(&g_config, 0, sizeof(g_config));
        g_config.enabled = true;
        g_config.track_stack_traces = false;
        g_config.track_thread_info = true;
        g_config.track_allocation_patterns = true;
        g_config.generate_reports_on_exit = true;
        g_config.break_on_leak = false;
        g_config.min_allocation_size = 0;
        g_config.max_stack_frames = 8;
        g_config.report_filename = "memory_leak_report.txt";
    }
    
    // Initialize hash table
    memset(g_hash_table, 0, sizeof(g_hash_table));
    
    // Initialize statistics
    memset(&g_statistics, 0, sizeof(g_statistics));
    
    g_initialized = true;
    
    pthread_mutex_unlock(&g_mutex);
    return true;
}

void memory_leak_detector_shutdown(bool generate_report) {
    pthread_mutex_lock(&g_mutex);
    
    if (!g_initialized) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    
    if (generate_report && g_config.generate_reports_on_exit) {
        memory_leak_print_summary();
        memory_leak_export_report(g_config.report_filename, "text");
    }
    
    // Cleanup
    if (g_records_array) {
        free(g_records_array);
        g_records_array = NULL;
    }
    
    g_records_capacity = 0;
    g_records_count = 0;
    g_initialized = false;
    
    pthread_mutex_unlock(&g_mutex);
}

MemoryLeakConfig memory_leak_detector_get_config(void) {
    pthread_mutex_lock(&g_mutex);
    MemoryLeakConfig config = g_config;
    pthread_mutex_unlock(&g_mutex);
    return config;
}

void memory_leak_detector_set_config(const MemoryLeakConfig *config) {
    pthread_mutex_lock(&g_mutex);
    if (config) {
        g_config = *config;
    }
    pthread_mutex_unlock(&g_mutex);
}

bool memory_leak_track_allocation(void *ptr, size_t size, 
                                 const char *file, int line, const char *function) {
    if (!g_initialized || !g_config.enabled || !ptr) {
        return false;
    }
    
    if (size < g_config.min_allocation_size) {
        return false;
    }
    
    if (should_ignore_allocation(file)) {
        return false;
    }
    
    pthread_mutex_lock(&g_mutex);
    
    // Check if already tracked
    if (memory_leak_is_tracked(ptr)) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    
    // Create record
    MemoryRecord *record = create_memory_record(ptr, size, file, line, function);
    if (!record) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    
    // Add to tracking structures
    add_to_hash_table(record);
    
    // Add to linked list
    if (!g_all_records) {
        g_all_records = g_all_records_tail = record;
    } else {
        g_all_records_tail->next = record;
        record->prev = g_all_records_tail;
        g_all_records_tail = record;
    }
    
    // Update statistics
    update_statistics_allocation(record);
    
    // Call callback if set
    if (g_allocation_callback) {
        g_allocation_callback(record);
    }
    
    pthread_mutex_unlock(&g_mutex);
    return true;
}

bool memory_leak_track_deallocation(void *ptr) {
    if (!g_initialized || !g_config.enabled || !ptr) {
        return false;
    }
    
    pthread_mutex_lock(&g_mutex);
    
    MemoryRecord *record = memory_leak_get_record(ptr);
    if (!record || record->is_freed) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    
    // Mark as freed
    record->is_freed = true;
    record->free_timestamp = get_timestamp_ms();
    
    // Remove from hash table
    remove_from_hash_table(record);
    
    // Update statistics
    update_statistics_deallocation(record);
    
    // Call callback if set
    if (g_deallocation_callback) {
        g_deallocation_callback(record);
    }
    
    pthread_mutex_unlock(&g_mutex);
    return true;
}

bool memory_leak_is_tracked(void *ptr) {
    if (!ptr) return false;
    
    uint32_t hash = hash_pointer(ptr);
    MemoryRecord *record = g_hash_table[hash];
    
    while (record) {
        if (record->ptr == ptr && !record->is_freed) {
            return true;
        }
        record = record->next;
    }
    
    return false;
}

MemoryRecord* memory_leak_get_record(void *ptr) {
    if (!ptr) return NULL;
    
    uint32_t hash = hash_pointer(ptr);
    MemoryRecord *record = g_hash_table[hash];
    
    while (record) {
        if (record->ptr == ptr && !record->is_freed) {
            return record;
        }
        record = record->next;
    }
    
    return NULL;
}

MemoryStatistics memory_leak_get_statistics(void) {
    pthread_mutex_lock(&g_mutex);
    MemoryStatistics stats = g_statistics;
    pthread_mutex_unlock(&g_mutex);
    return stats;
}

void memory_leak_reset_statistics(void) {
    pthread_mutex_lock(&g_mutex);
    memset(&g_statistics, 0, sizeof(g_statistics));
    pthread_mutex_unlock(&g_mutex);
}

bool memory_leak_check_leaks(uint32_t *leak_count, size_t *leaked_bytes) {
    pthread_mutex_lock(&g_mutex);
    
    uint32_t leaks = 0;
    size_t bytes = 0;
    
    for (uint32_t i = 0; i < g_records_count; i++) {
        MemoryRecord *record = &g_records_array[i];
        if (!record->is_freed) {
            leaks++;
            bytes += record->size;
        }
    }
    
    if (leak_count) *leak_count = leaks;
    if (leaked_bytes) *leaked_bytes = bytes;
    
    pthread_mutex_unlock(&g_mutex);
    return leaks > 0;
}

void memory_leak_print_summary(void) {
    pthread_mutex_lock(&g_mutex);
    
    uint32_t leak_count;
    size_t leaked_bytes;
    bool has_leaks = memory_leak_check_leaks(&leak_count, &leaked_bytes);
    
    printf("\n=== Memory Leak Detection Summary ===\n");
    printf("Total Allocations: %llu\n", (unsigned long long)g_statistics.total_allocations);
    printf("Total Deallocations: %llu\n", (unsigned long long)g_statistics.total_deallocations);
    printf("Current Allocations: %llu\n", (unsigned long long)g_statistics.current_allocations);
    printf("Peak Allocations: %llu\n", (unsigned long long)g_statistics.peak_allocations);
    printf("Total Bytes Allocated: %zu\n", g_statistics.total_bytes_allocated);
    printf("Current Bytes Allocated: %zu\n", g_statistics.current_bytes_allocated);
    printf("Peak Bytes Allocated: %zu\n", g_statistics.peak_bytes_allocated);
    printf("Average Allocation Size: %.2f bytes\n", g_statistics.average_allocation_size);
    
    if (has_leaks) {
        printf("\n*** MEMORY LEAKS DETECTED ***\n");
        printf("Leak Count: %u\n", leak_count);
        printf("Total Leaked Bytes: %zu\n", leaked_bytes);
    } else {
        printf("\nNo memory leaks detected.\n");
    }
    
    printf("===================================\n\n");
    
    pthread_mutex_unlock(&g_mutex);
}

bool memory_leak_export_report(const char *filename, const char *format) {
    if (!filename || !format) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    pthread_mutex_lock(&g_mutex);
    
    if (strcmp(format, "json") == 0) {
        fprintf(file, "{\n");
        fprintf(file, "  \"statistics\": {\n");
        fprintf(file, "    \"total_allocations\": %llu,\n", (unsigned long long)g_statistics.total_allocations);
        fprintf(file, "    \"total_deallocations\": %llu,\n", (unsigned long long)g_statistics.total_deallocations);
        fprintf(file, "    \"current_allocations\": %llu,\n", (unsigned long long)g_statistics.current_allocations);
        fprintf(file, "    \"peak_allocations\": %llu,\n", (unsigned long long)g_statistics.peak_allocations);
        fprintf(file, "    \"total_bytes_allocated\": %zu,\n", g_statistics.total_bytes_allocated);
        fprintf(file, "    \"current_bytes_allocated\": %zu,\n", g_statistics.current_bytes_allocated);
        fprintf(file, "    \"peak_bytes_allocated\": %zu,\n", g_statistics.peak_bytes_allocated);
        fprintf(file, "    \"average_allocation_size\": %.2f\n", g_statistics.average_allocation_size);
        fprintf(file, "  },\n");
        
        fprintf(file, "  \"leaks\": [\n");
        bool first = true;
        for (uint32_t i = 0; i < g_records_count; i++) {
            MemoryRecord *record = &g_records_array[i];
            if (!record->is_freed) {
                if (!first) fprintf(file, ",\n");
                fprintf(file, "    {\n");
                fprintf(file, "      \"allocation_id\": %u,\n", record->allocation_id);
                fprintf(file, "      \"pointer\": \"%p\",\n", record->ptr);
                fprintf(file, "      \"size\": %zu,\n", record->size);
                fprintf(file, "      \"file\": \"%s\",\n", record->file ? record->file : "unknown");
                fprintf(file, "      \"line\": %d,\n", record->line);
                fprintf(file, "      \"function\": \"%s\",\n", record->function ? record->function : "unknown");
                fprintf(file, "      \"thread_id\": %u,\n", record->thread_id);
                fprintf(file, "      \"timestamp\": %llu\n", (unsigned long long)record->timestamp);
                fprintf(file, "    }");
                first = false;
            }
        }
        fprintf(file, "\n  ]\n");
        fprintf(file, "}\n");
    } else {
        // Text format
        fprintf(file, "Memory Leak Detection Report\n");
        fprintf(file, "===========================\n\n");
        
        fprintf(file, "Statistics:\n");
        fprintf(file, "  Total Allocations: %llu\n", (unsigned long long)g_statistics.total_allocations);
        fprintf(file, "  Total Deallocations: %llu\n", (unsigned long long)g_statistics.total_deallocations);
        fprintf(file, "  Current Allocations: %llu\n", (unsigned long long)g_statistics.current_allocations);
        fprintf(file, "  Peak Allocations: %llu\n", (unsigned long long)g_statistics.peak_allocations);
        fprintf(file, "  Total Bytes Allocated: %zu\n", g_statistics.total_bytes_allocated);
        fprintf(file, "  Current Bytes Allocated: %zu\n", g_statistics.current_bytes_allocated);
        fprintf(file, "  Peak Bytes Allocated: %zu\n", g_statistics.peak_bytes_allocated);
        fprintf(file, "  Average Allocation Size: %.2f bytes\n\n", g_statistics.average_allocation_size);
        
        fprintf(file, "Memory Leaks:\n");
        uint32_t leak_count = 0;
        size_t leaked_bytes = 0;
        
        for (uint32_t i = 0; i < g_records_count; i++) {
            MemoryRecord *record = &g_records_array[i];
            if (!record->is_freed) {
                leak_count++;
                leaked_bytes += record->size;
                fprintf(file, "  Leak #%u:\n", leak_count);
                fprintf(file, "    Pointer: %p\n", record->ptr);
                fprintf(file, "    Size: %zu bytes\n", record->size);
                fprintf(file, "    Location: %s:%d in %s\n", 
                       record->file ? record->file : "unknown", 
                       record->line, 
                       record->function ? record->function : "unknown");
                fprintf(file, "    Thread ID: %u\n", record->thread_id);
                fprintf(file, "    Timestamp: %llu\n\n", (unsigned long long)record->timestamp);
            }
        }
        
        if (leak_count == 0) {
            fprintf(file, "  No memory leaks detected.\n");
        } else {
            fprintf(file, "  Total Leaks: %u\n", leak_count);
            fprintf(file, "  Total Leaked Bytes: %zu\n", leaked_bytes);
        }
    }
    
    pthread_mutex_unlock(&g_mutex);
    fclose(file);
    return true;
}

void memory_leak_set_debug_mode(bool enabled) {
    pthread_mutex_lock(&g_mutex);
    g_debug_mode = enabled;
    pthread_mutex_unlock(&g_mutex);
}

void memory_leak_set_allocation_callback(void (*callback)(MemoryRecord*)) {
    pthread_mutex_lock(&g_mutex);
    g_allocation_callback = callback;
    pthread_mutex_unlock(&g_mutex);
}

void memory_leak_set_deallocation_callback(void (*callback)(MemoryRecord*)) {
    pthread_mutex_lock(&g_mutex);
    g_deallocation_callback = callback;
    pthread_mutex_unlock(&g_mutex);
}

bool memory_leak_validate_tracking(void) {
    pthread_mutex_lock(&g_mutex);
    
    // Count active allocations in hash table
    uint32_t hash_count = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        MemoryRecord *record = g_hash_table[i];
        while (record) {
            if (!record->is_freed) {
                hash_count++;
            }
            record = record->next;
        }
    }
    
    // Count active allocations in array
    uint32_t array_count = 0;
    for (uint32_t i = 0; i < g_records_count; i++) {
        if (!g_records_array[i].is_freed) {
            array_count++;
        }
    }
    
    bool valid = (hash_count == array_count) && (array_count == g_statistics.current_allocations);
    
    pthread_mutex_unlock(&g_mutex);
    return valid;
}

// ========================================
// Wrapper Functions Implementation
// ========================================

void* memory_leak_malloc(size_t size, const char *file, int line, const char *function) {
    void *ptr = malloc(size);
    if (ptr && memory_leak_track_allocation(ptr, size, file, line, function)) {
        return ptr;
    }
    return ptr;
}

void memory_leak_free(void *ptr, const char *file, int line, const char *function) {
    if (ptr) {
        memory_leak_track_deallocation(ptr);
        free(ptr);
    }
}

void* memory_leak_calloc(size_t count, size_t size, const char *file, int line, const char *function) {
    void *ptr = calloc(count, size);
    if (ptr && memory_leak_track_allocation(ptr, count * size, file, line, function)) {
        return ptr;
    }
    return ptr;
}

void* memory_leak_realloc(void *ptr, size_t size, const char *file, int line, const char *function) {
    if (ptr) {
        memory_leak_track_deallocation(ptr);
    }
    
    void *new_ptr = realloc(ptr, size);
    if (new_ptr && memory_leak_track_allocation(new_ptr, size, file, line, function)) {
        return new_ptr;
    }
    return new_ptr;
}
