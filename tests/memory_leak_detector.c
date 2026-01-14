/**
 * @file memory_leak_detector.c
 * @brief Implementation of advanced memory leak detection system
 * 
 * This file implements a comprehensive memory leak detection system that tracks
 * all allocations, detects leaks, validates memory integrity, and provides
 * detailed reporting for debugging purposes.
 * 
 * @author Minecraft v2 Engine Team
 * @date 2026-01-13
 */

#include "memory_leak_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

// ========================================
// Constants and Magic Numbers
// ========================================

#define MEMORY_LEAK_MAGIC_VALID       0xDEADBEEF
#define MEMORY_LEAK_MAGIC_FREED       0xFEEDFACE
#define MEMORY_LEAK_GUARD_PATTERN     0xBAADF00D
#define MEMORY_LEAK_HASH_TABLE_SIZE   16384
#define MEMORY_LEAK_MAX_ALLOCATIONS   1000000

// ========================================
// Global State
// ========================================

typedef struct {
    MemoryLeakConfig config;
    MemoryAllocation** hash_table;
    MemoryLeakStats stats;
    pthread_mutex_t global_mutex;
    uint64_t next_allocation_id;
    bool initialized;
    bool auto_validation_enabled;
    uint32_t leak_threshold;
    uint64_t byte_threshold;
    bool performance_monitoring_enabled;
    uint64_t tracking_time_ns;
    uint64_t validation_time_ns;
    uint64_t start_time_ns;
} MemoryLeakDetectorState;

static MemoryLeakDetectorState g_detector_state = {0};

// ========================================
// Utility Functions
// ========================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t hash_pointer(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (uint32_t)((addr >> 3) ^ (addr >> 13) ^ (addr >> 23)) % g_detector_state.config.hash_table_size;
}

static void log_message(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

static bool validate_guard_patterns(const MemoryAllocation* alloc) {
    if (!g_detector_state.config.enable_guard_pages) {
        return true;
    }
    
    // Check guard patterns before and after allocation
    uint32_t* guard_before = (uint32_t*)((uint8_t*)alloc->ptr - sizeof(uint32_t));
    uint32_t* guard_after = (uint32_t*)((uint8_t*)alloc->ptr + alloc->size);
    
    return (*guard_before == g_detector_state.config.guard_pattern) &&
           (*guard_after == g_detector_state.config.guard_pattern);
}

static void set_guard_patterns(MemoryAllocation* alloc) {
    if (!g_detector_state.config.enable_guard_pages) {
        return;
    }
    
    uint32_t* guard_before = (uint32_t*)((uint8_t*)alloc->ptr - sizeof(uint32_t));
    uint32_t* guard_after = (uint32_t*)((uint8_t*)alloc->ptr + alloc->size);
    
    *guard_before = g_detector_state.config.guard_pattern;
    *guard_after = g_detector_state.config.guard_pattern;
}

static void update_performance_tracking(uint64_t start_time, bool is_validation) {
    if (!g_detector_state.performance_monitoring_enabled) {
        return;
    }
    
    uint64_t elapsed = get_time_ns() - start_time;
    if (is_validation) {
        g_detector_state.validation_time_ns += elapsed;
    } else {
        g_detector_state.tracking_time_ns += elapsed;
    }
}

// ========================================
// Core Implementation
// ========================================

bool memory_leak_detector_init(const MemoryLeakConfig* config) {
    if (g_detector_state.initialized) {
        return false;
    }
    
    memset(&g_detector_state, 0, sizeof(g_detector_state));
    
    // Set configuration
    if (config) {
        g_detector_state.config = *config;
    } else {
        // Default configuration
        g_detector_state.config = (MemoryLeakConfig){
            .enable_tracking = true,
            .enable_guard_pages = true,
            .enable_magic_numbers = true,
            .enable_stack_traces = false,
            .hash_table_size = MEMORY_LEAK_HASH_TABLE_SIZE,
            .max_allocations = MEMORY_LEAK_MAX_ALLOCATIONS,
            .guard_pattern = MEMORY_LEAK_GUARD_PATTERN,
            .magic_number = MEMORY_LEAK_MAGIC_VALID,
            .enable_thread_safety = true,
            .enable_performance_monitoring = false
        };
    }
    
    // Initialize hash table
    g_detector_state.hash_table = calloc(g_detector_state.config.hash_table_size, 
                                        sizeof(MemoryAllocation*));
    if (!g_detector_state.hash_table) {
        log_message("Failed to allocate memory leak detector hash table");
        return false;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&g_detector_state.global_mutex, NULL) != 0) {
        free(g_detector_state.hash_table);
        log_message("Failed to initialize memory leak detector mutex");
        return false;
    }
    
    g_detector_state.next_allocation_id = 1;
    g_detector_state.start_time_ns = get_time_ns();
    g_detector_state.auto_validation_enabled = true;
    g_detector_state.leak_threshold = 100;
    g_detector_state.byte_threshold = 1024 * 1024; // 1MB
    g_detector_state.initialized = true;
    
    log_message("Memory leak detector initialized successfully");
    return true;
}

bool memory_leak_detector_shutdown(const char* report_file) {
    if (!g_detector_state.initialized) {
        return false;
    }
    
    // Generate final report
    memory_leak_detector_print_report(report_file, true, false);
    
    // Clean up hash table
    pthread_mutex_lock(&g_detector_state.global_mutex);
    
    for (uint32_t i = 0; i < g_detector_state.config.hash_table_size; i++) {
        MemoryAllocation* current = g_detector_state.hash_table[i];
        while (current) {
            MemoryAllocation* next = current->next;
            free(current);
            current = next;
        }
    }
    
    free(g_detector_state.hash_table);
    g_detector_state.hash_table = NULL;
    
    pthread_mutex_unlock(&g_detector_state.global_mutex);
    pthread_mutex_destroy(&g_detector_state.global_mutex);
    
    g_detector_state.initialized = false;
    
    log_message("Memory leak detector shutdown complete");
    return true;
}

bool memory_leak_detector_track_allocation(void* ptr, size_t size, 
                                          const char* file, int line, 
                                          const char* function) {
    if (!ptr || !g_detector_state.initialized || !g_detector_state.config.enable_tracking) {
        return false;
    }
    
    uint64_t start_time = get_time_ns();
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    // Check allocation limit
    if (g_detector_state.stats.active_allocations >= g_detector_state.config.max_allocations) {
        if (g_detector_state.config.enable_thread_safety) {
            pthread_mutex_unlock(&g_detector_state.global_mutex);
        }
        return false;
    }
    
    // Create allocation record
    MemoryAllocation* alloc = malloc(sizeof(MemoryAllocation));
    if (!alloc) {
        if (g_detector_state.config.enable_thread_safety) {
            pthread_mutex_unlock(&g_detector_state.global_mutex);
        }
        return false;
    }
    
    memset(alloc, 0, sizeof(MemoryAllocation));
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->requested_size = size;
    alloc->line = line;
    alloc->thread_id = pthread_self();
    alloc->timestamp = get_time_ns();
    alloc->allocation_id = g_detector_state.next_allocation_id++;
    alloc->magic = g_detector_state.config.magic_number;
    
    if (file) {
        strncpy(alloc->file, file, sizeof(alloc->file) - 1);
        alloc->file[sizeof(alloc->file) - 1] = '\0';
    }
    
    if (function) {
        strncpy(alloc->function, function, sizeof(alloc->function) - 1);
        alloc->function[sizeof(alloc->function) - 1] = '\0';
    }
    
    // Add to hash table
    uint32_t hash = hash_pointer(ptr);
    alloc->next = g_detector_state.hash_table[hash];
    g_detector_state.hash_table[hash] = alloc;
    
    // Update statistics
    g_detector_state.stats.total_allocations++;
    g_detector_state.stats.active_allocations++;
    g_detector_state.stats.total_allocated_bytes += size;
    g_detector_state.stats.active_allocated_bytes += size;
    
    if (g_detector_state.stats.active_allocated_bytes > g_detector_state.stats.peak_allocated_bytes) {
        g_detector_state.stats.peak_allocated_bytes = g_detector_state.stats.active_allocated_bytes;
    }
    
    // Set guard patterns
    set_guard_patterns(alloc);
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    update_performance_tracking(start_time, false);
    return true;
}

bool memory_leak_detector_track_deallocation(void* ptr, 
                                           const char* file, int line, 
                                           const char* function) {
    if (!ptr || !g_detector_state.initialized || !g_detector_state.config.enable_tracking) {
        return false;
    }
    
    uint64_t start_time = get_time_ns();
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    uint32_t hash = hash_pointer(ptr);
    MemoryAllocation** current = &g_detector_state.hash_table[hash];
    
    while (*current) {
        if ((*current)->ptr == ptr) {
            MemoryAllocation* alloc = *current;
            
            // Validate guard patterns
            if (g_detector_state.auto_validation_enabled && !validate_guard_patterns(alloc)) {
                g_detector_state.stats.corruption_detected++;
                log_message("Memory corruption detected in allocation %p from %s:%d", 
                           ptr, alloc->file, alloc->line);
            }
            
            // Remove from hash table
            *current = (*current)->next;
            
            // Update statistics
            g_detector_state.stats.active_allocations--;
            g_detector_state.stats.active_allocated_bytes -= alloc->size;
            g_detector_state.stats.total_deallocations++;
            
            // Mark as freed
            alloc->magic = MEMORY_LEAK_MAGIC_FREED;
            free(alloc);
            
            if (g_detector_state.config.enable_thread_safety) {
                pthread_mutex_unlock(&g_detector_state.global_mutex);
            }
            
            update_performance_tracking(start_time, false);
            return true;
        }
        current = &(*current)->next;
    }
    
    // Allocation not found - invalid free
    g_detector_state.stats.invalid_frees_detected++;
    log_message("Invalid free detected for pointer %p at %s:%d", ptr, file, line);
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    update_performance_tracking(start_time, false);
    return false;
}

bool memory_leak_detector_validate_allocation(void* ptr) {
    if (!ptr || !g_detector_state.initialized) {
        return false;
    }
    
    uint64_t start_time = get_time_ns();
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    uint32_t hash = hash_pointer(ptr);
    MemoryAllocation* current = g_detector_state.hash_table[hash];
    
    while (current) {
        if (current->ptr == ptr) {
            bool valid = validate_guard_patterns(current);
            
            if (g_detector_state.config.enable_thread_safety) {
                pthread_mutex_unlock(&g_detector_state.global_mutex);
            }
            
            update_performance_tracking(start_time, true);
            return valid;
        }
        current = current->next;
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    update_performance_tracking(start_time, true);
    return false;
}

uint32_t memory_leak_detector_validate_all(void) {
    if (!g_detector_state.initialized) {
        return 0;
    }
    
    uint64_t start_time = get_time_ns();
    uint32_t corrupted_count = 0;
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    for (uint32_t i = 0; i < g_detector_state.config.hash_table_size; i++) {
        MemoryAllocation* current = g_detector_state.hash_table[i];
        while (current) {
            if (!validate_guard_patterns(current)) {
                corrupted_count++;
                log_message("Memory corruption detected in allocation %p from %s:%d", 
                           current->ptr, current->file, current->line);
            }
            current = current->next;
        }
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    update_performance_tracking(start_time, true);
    return corrupted_count;
}

// ========================================
// Statistics and Reporting
// ========================================

MemoryLeakStats memory_leak_detector_get_stats(void) {
    MemoryLeakStats stats = {0};
    
    if (!g_detector_state.initialized) {
        return stats;
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    stats = g_detector_state.stats;
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    return stats;
}

uint32_t memory_leak_detector_generate_report(MemoryLeakReport* report, 
                                             uint32_t max_entries,
                                             bool include_active) {
    if (!report || !g_detector_state.initialized || max_entries == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    uint64_t current_time = get_time_ns();
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    for (uint32_t i = 0; i < g_detector_state.config.hash_table_size && count < max_entries; i++) {
        MemoryAllocation* current = g_detector_state.hash_table[i];
        while (current && count < max_entries) {
            if (include_active || current->magic == MEMORY_LEAK_MAGIC_VALID) {
                report[count].allocation_id = current->allocation_id;
                report[count].ptr = current->ptr;
                report[count].size = current->size;
                strncpy(report[count].file, current->file, sizeof(report[count].file) - 1);
                report[count].line = current->line;
                strncpy(report[count].function, current->function, sizeof(report[count].function) - 1);
                report[count].thread_id = current->thread_id;
                report[count].timestamp = current->timestamp;
                report[count].age_seconds = (current_time - current->timestamp) / 1000000000.0;
                
                count++;
            }
            current = current->next;
        }
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    return count;
}

void memory_leak_detector_print_stats(const char* output_file, bool verbose) {
    FILE* output = stdout;
    if (output_file) {
        output = fopen(output_file, "w");
        if (!output) {
            log_message("Failed to open stats output file: %s", output_file);
            return;
        }
    }
    
    MemoryLeakStats stats = memory_leak_detector_get_stats();
    
    fprintf(output, "=== MEMORY LEAK DETECTION STATISTICS ===\n");
    fprintf(output, "Total Allocations: %llu\n", (unsigned long long)stats.total_allocations);
    fprintf(output, "Active Allocations: %llu\n", (unsigned long long)stats.active_allocations);
    fprintf(output, "Total Deallocations: %llu\n", (unsigned long long)stats.total_deallocations);
    fprintf(output, "Total Allocated Bytes: %llu\n", (unsigned long long)stats.total_allocated_bytes);
    fprintf(output, "Active Allocated Bytes: %llu\n", (unsigned long long)stats.active_allocated_bytes);
    fprintf(output, "Peak Allocated Bytes: %llu\n", (unsigned long long)stats.peak_allocated_bytes);
    fprintf(output, "Failed Allocations: %llu\n", (unsigned long long)stats.failed_allocations);
    fprintf(output, "Corruption Detected: %llu\n", (unsigned long long)stats.corruption_detected);
    fprintf(output, "Double Frees Detected: %llu\n", (unsigned long long)stats.double_frees_detected);
    fprintf(output, "Invalid Frees Detected: %llu\n", (unsigned long long)stats.invalid_frees_detected);
    
    if (verbose) {
        uint64_t tracking_ms, validation_ms, memory_overhead;
        double fragmentation;
        memory_leak_detector_get_performance_impact(&tracking_ms, &validation_ms, &memory_overhead, &fragmentation);
        
        fprintf(output, "\n=== PERFORMANCE IMPACT ===\n");
        fprintf(output, "Tracking Overhead: %llu ms\n", (unsigned long long)tracking_ms);
        fprintf(output, "Validation Overhead: %llu ms\n", (unsigned long long)validation_ms);
        fprintf(output, "Memory Overhead: %llu bytes\n", (unsigned long long)memory_overhead);
        fprintf(output, "Fragmentation Ratio: %.2f%%\n", fragmentation * 100.0);
    }
    
    if (output_file) {
        fclose(output);
    }
}

void memory_leak_detector_print_report(const char* output_file, 
                                       bool sort_by_age,
                                       bool include_stack_traces) {
    FILE* output = stdout;
    if (output_file) {
        output = fopen(output_file, "w");
        if (!output) {
            log_message("Failed to open report output file: %s", output_file);
            return;
        }
    }
    
    // Get all active allocations (leaks)
    const uint32_t max_entries = 10000;
    MemoryLeakReport* report = malloc(max_entries * sizeof(MemoryLeakReport));
    if (!report) {
        if (output_file) {
            fclose(output);
        }
        return;
    }
    
    uint32_t count = memory_leak_detector_generate_report(report, max_entries, true);
    
    // Sort by age if requested
    if (sort_by_age) {
        for (uint32_t i = 0; i < count - 1; i++) {
            for (uint32_t j = 0; j < count - i - 1; j++) {
                if (report[j].age_seconds < report[j + 1].age_seconds) {
                    MemoryLeakReport temp = report[j];
                    report[j] = report[j + 1];
                    report[j + 1] = temp;
                }
            }
        }
    }
    
    fprintf(output, "=== MEMORY LEAK REPORT ===\n");
    fprintf(output, "Total Leaks Detected: %u\n", count);
    fprintf(output, "Generated: %s", ctime(&(time_t){time(NULL)}));
    fprintf(output, "\n");
    
    for (uint32_t i = 0; i < count; i++) {
        fprintf(output, "Leak #%u:\n", i + 1);
        fprintf(output, "  Allocation ID: %llu\n", (unsigned long long)report[i].allocation_id);
        fprintf(output, "  Pointer: %p\n", report[i].ptr);
        fprintf(output, "  Size: %zu bytes\n", report[i].size);
        fprintf(output, "  Location: %s:%d in %s()\n", report[i].file, report[i].line, report[i].function);
        fprintf(output, "  Thread ID: %lu\n", (unsigned long)report[i].thread_id);
        fprintf(output, "  Age: %.2f seconds\n", report[i].age_seconds);
        
        if (include_stack_traces && strlen(report[i].stack_trace) > 0) {
            fprintf(output, "  Stack Trace:\n%s\n", report[i].stack_trace);
        }
        
        fprintf(output, "\n");
    }
    
    free(report);
    
    if (output_file) {
        fclose(output);
    }
}

void memory_leak_detector_reset_stats(void) {
    if (!g_detector_state.initialized) {
        return;
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    // Reset counters but keep allocations
    uint64_t active_allocations = g_detector_state.stats.active_allocations;
    uint64_t active_allocated_bytes = g_detector_state.stats.active_allocated_bytes;
    
    memset(&g_detector_state.stats, 0, sizeof(g_detector_state.stats));
    
    g_detector_state.stats.active_allocations = active_allocations;
    g_detector_state.stats.active_allocated_bytes = active_allocated_bytes;
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
}

void memory_leak_detector_clear_all(void) {
    if (!g_detector_state.initialized) {
        return;
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    // Clear hash table
    for (uint32_t i = 0; i < g_detector_state.config.hash_table_size; i++) {
        MemoryAllocation* current = g_detector_state.hash_table[i];
        while (current) {
            MemoryAllocation* next = current->next;
            free(current);
            current = next;
        }
        g_detector_state.hash_table[i] = NULL;
    }
    
    // Reset statistics
    memset(&g_detector_state.stats, 0, sizeof(g_detector_state.stats));
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
}

// ========================================
// Advanced Features
// ========================================

void memory_leak_detector_set_threshold(uint32_t leak_threshold, 
                                       uint64_t byte_threshold) {
    if (!g_detector_state.initialized) {
        return;
    }
    
    g_detector_state.leak_threshold = leak_threshold;
    g_detector_state.byte_threshold = byte_threshold;
}

void memory_leak_detector_set_auto_validation(bool enable) {
    if (!g_detector_state.initialized) {
        return;
    }
    
    g_detector_state.auto_validation_enabled = enable;
}

const MemoryAllocation* memory_leak_detector_get_allocation(void* ptr) {
    if (!ptr || !g_detector_state.initialized) {
        return NULL;
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    uint32_t hash = hash_pointer(ptr);
    MemoryAllocation* current = g_detector_state.hash_table[hash];
    
    while (current) {
        if (current->ptr == ptr) {
            if (g_detector_state.config.enable_thread_safety) {
                pthread_mutex_unlock(&g_detector_state.global_mutex);
            }
            return current;
        }
        current = current->next;
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    return NULL;
}

uint32_t memory_leak_detector_get_allocations_by_thread(pthread_t thread_id,
                                                       MemoryAllocation* allocations,
                                                       uint32_t max_allocations) {
    if (!allocations || !g_detector_state.initialized || max_allocations == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    for (uint32_t i = 0; i < g_detector_state.config.hash_table_size && count < max_allocations; i++) {
        MemoryAllocation* current = g_detector_state.hash_table[i];
        while (current && count < max_allocations) {
            if (current->thread_id == thread_id) {
                allocations[count] = *current;
                count++;
            }
            current = current->next;
        }
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    return count;
}

uint32_t memory_leak_detector_get_allocations_by_size(size_t min_size,
                                                     size_t max_size,
                                                     MemoryAllocation* allocations,
                                                     uint32_t max_allocations) {
    if (!allocations || !g_detector_state.initialized || max_allocations == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    for (uint32_t i = 0; i < g_detector_state.config.hash_table_size && count < max_allocations; i++) {
        MemoryAllocation* current = g_detector_state.hash_table[i];
        while (current && count < max_allocations) {
            if (current->size >= min_size && current->size <= max_size) {
                allocations[count] = *current;
                count++;
            }
            current = current->next;
        }
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    return count;
}

void memory_leak_detector_dump_memory_map(const char* output_file,
                                          bool include_details) {
    FILE* output = stdout;
    if (output_file) {
        output = fopen(output_file, "w");
        if (!output) {
            log_message("Failed to open memory map output file: %s", output_file);
            return;
        }
    }
    
    fprintf(output, "=== MEMORY ALLOCATION MAP ===\n");
    fprintf(output, "Generated: %s", ctime(&(time_t){time(NULL)}));
    fprintf(output, "\n");
    
    if (!g_detector_state.initialized) {
        fprintf(output, "Memory leak detector not initialized\n");
        if (output_file) {
            fclose(output);
        }
        return;
    }
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_lock(&g_detector_state.global_mutex);
    }
    
    uint32_t total_slots = 0;
    uint32_t used_slots = 0;
    
    for (uint32_t i = 0; i < g_detector_state.config.hash_table_size; i++) {
        MemoryAllocation* current = g_detector_state.hash_table[i];
        if (current) {
            used_slots++;
            total_slots++;
            
            fprintf(output, "Hash slot %u: %p", i, current->ptr);
            
            if (include_details) {
                fprintf(output, " (%zu bytes, %s:%d)", 
                       current->size, current->file, current->line);
            }
            
            fprintf(output, "\n");
            
            // Count chain length
            uint32_t chain_length = 1;
            current = current->next;
            while (current) {
                chain_length++;
                total_slots++;
                
                if (include_details) {
                    fprintf(output, "  -> %p (%zu bytes, %s:%d)\n", 
                           current->ptr, current->size, current->file, current->line);
                }
                
                current = current->next;
            }
            
            if (chain_length > 1) {
                fprintf(output, "  Chain length: %u\n", chain_length);
            }
        }
    }
    
    fprintf(output, "\nHash table statistics:\n");
    fprintf(output, "Total slots: %u\n", g_detector_state.config.hash_table_size);
    fprintf(output, "Used slots: %u\n", used_slots);
    fprintf(output, "Total allocations: %u\n", total_slots);
    fprintf(output, "Load factor: %.2f%%\n", 
           (double)used_slots / g_detector_state.config.hash_table_size * 100.0);
    
    if (g_detector_state.config.enable_thread_safety) {
        pthread_mutex_unlock(&g_detector_state.global_mutex);
    }
    
    if (output_file) {
        fclose(output);
    }
}

// ========================================
// Performance Monitoring
// ========================================

void memory_leak_detector_get_performance_impact(uint64_t* tracking_overhead_ms,
                                                uint64_t* validation_overhead_ms,
                                                uint64_t* memory_overhead_bytes,
                                                double* fragmentation_ratio) {
    if (!g_detector_state.initialized) {
        if (tracking_overhead_ms) *tracking_overhead_ms = 0;
        if (validation_overhead_ms) *validation_overhead_ms = 0;
        if (memory_overhead_bytes) *memory_overhead_bytes = 0;
        if (fragmentation_ratio) *fragmentation_ratio = 0.0;
        return;
    }
    
    if (tracking_overhead_ms) {
        *tracking_overhead_ms = g_detector_state.tracking_time_ns / 1000000;
    }
    
    if (validation_overhead_ms) {
        *validation_overhead_ms = g_detector_state.validation_time_ns / 1000000;
    }
    
    if (memory_overhead_bytes) {
        uint64_t overhead = g_detector_state.config.hash_table_size * sizeof(MemoryAllocation*);
        overhead += g_detector_state.stats.active_allocations * sizeof(MemoryAllocation);
        *memory_overhead_bytes = overhead;
    }
    
    if (fragmentation_ratio) {
        if (g_detector_state.stats.total_allocated_bytes > 0) {
            *fragmentation_ratio = 1.0 - ((double)g_detector_state.stats.active_allocated_bytes / 
                                          g_detector_state.stats.total_allocated_bytes);
        } else {
            *fragmentation_ratio = 0.0;
        }
    }
}

void memory_leak_detector_set_performance_monitoring(bool enable) {
    if (!g_detector_state.initialized) {
        return;
    }
    
    g_detector_state.performance_monitoring_enabled = enable;
}
