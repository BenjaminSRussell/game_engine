/**
 * @file thread_safety_verifier.c
 * @brief Thread safety verification and analysis system implementation
 */

#include "thread_safety_verifier.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// ========================================
// Internal Constants
// ========================================

#define MAX_THREADS 1024
#define MAX_EVENTS_PER_THREAD 10000
#define MAX_RACE_CONDITIONS 1000
#define MAX_DEADLOCK_CYCLES 100
#define HASH_TABLE_SIZE 65536

// ========================================
// Internal State
// ========================================

static ThreadSafetyConfig g_config = {0};
static ThreadSafetyStatistics g_statistics = {0};
static bool g_initialized = false;
static bool g_debug_mode = false;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread tracking
static ThreadInfo *g_threads[MAX_THREADS] = {NULL};
static uint32_t g_thread_count = 0;

// Event tracking
static ThreadSafetyRecord *g_events_head = NULL;
static ThreadSafetyRecord *g_events_tail = NULL;
static uint64_t g_total_events = 0;

// Race condition tracking
static RaceCondition *g_race_conditions[MAX_RACE_CONDITIONS] = {NULL};
static uint32_t g_race_condition_count = 0;

// Deadlock tracking
static DeadlockCycle *g_deadlock_cycles[MAX_DEADLOCK_CYCLES] = {NULL};
static uint32_t g_deadlock_cycle_count = 0;

// Memory access tracking
typedef struct MemoryAccessRecord {
    void *address;
    size_t size;
    bool is_write;
    uint32_t thread_id;
    uint64_t timestamp;
    const char *function;
    const char *file;
    int line;
    struct MemoryAccessRecord *next;
} MemoryAccessRecord;

static MemoryAccessRecord *g_memory_access_table[HASH_TABLE_SIZE] = {NULL};

// Callbacks
static void (*g_event_callback)(ThreadSafetyRecord*) = NULL;

// ========================================
// Internal Helper Functions
// ========================================

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t get_thread_id(void) {
    return (uint32_t)pthread_self();
}

static uint32_t hash_pointer(void *ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (uint32_t)(addr ^ (addr >> 16)) % HASH_TABLE_SIZE;
}

static ThreadInfo* find_or_create_thread(uint32_t thread_id) {
    // Search existing threads
    for (uint32_t i = 0; i < g_thread_count; i++) {
        if (g_threads[i] && g_threads[i]->thread_id == thread_id) {
            return g_threads[i];
        }
    }
    
    // Create new thread info
    if (g_thread_count >= MAX_THREADS) {
        return NULL;
    }
    
    ThreadInfo *thread = calloc(1, sizeof(ThreadInfo));
    if (!thread) return NULL;
    
    thread->thread_id = thread_id;
    thread->pthread_handle = pthread_self();
    thread->creation_time = get_timestamp_ns();
    thread->last_activity = thread->creation_time;
    thread->is_active = true;
    
    g_threads[g_thread_count++] = thread;
    g_statistics.total_threads++;
    g_statistics.active_threads++;
    
    return thread;
}

static ThreadSafetyRecord* create_event_record(ThreadSafetyEventType event_type, void *resource,
                                              const char *function, const char *file, int line,
                                              const char *description) {
    ThreadSafetyRecord *record = malloc(sizeof(ThreadSafetyRecord));
    if (!record) return NULL;
    
    memset(record, 0, sizeof(ThreadSafetyRecord));
    
    record->event_type = event_type;
    record->thread_id = get_thread_id();
    record->timestamp = get_timestamp_ns();
    record->resource = resource;
    record->function = function;
    record->file = file;
    record->line = line;
    
    if (description) {
        strncpy(record->description, description, sizeof(record->description) - 1);
    }
    
    return record;
}

static void add_event_record(ThreadSafetyRecord *record) {
    if (!g_events_head) {
        g_events_head = g_events_tail = record;
    } else {
        g_events_tail->next = record;
        g_events_tail = record;
    }
    
    g_total_events++;
    g_statistics.total_events++;
}

static void update_thread_activity(uint32_t thread_id) {
    ThreadInfo *thread = find_or_create_thread(thread_id);
    if (thread) {
        thread->last_activity = get_timestamp_ns();
    }
}

static bool should_ignore_event(const char *file) {
    if (!g_config.ignore_pattern_count) return false;
    
    for (int i = 0; i < g_config.ignore_pattern_count; i++) {
        if (strstr(file, g_config.ignore_patterns[i])) {
            return true;
        }
    }
    return false;
}

static void update_lock_statistics(ThreadSafetyRecord *record) {
    switch (record->event_type) {
        case THREAD_EVENT_LOCK_ACQUIRE:
            g_statistics.lock_acquisitions++;
            break;
        case THREAD_EVENT_LOCK_RELEASE:
            g_statistics.lock_releases++;
            break;
        default:
            break;
    }
}

static void detect_race_condition(MemoryAccessRecord *access1, MemoryAccessRecord *access2) {
    if (!g_config.detect_race_conditions) return;
    
    // Check if it's a race condition
    bool is_race = false;
    char description[512] = {0};
    
    if (access1->is_write && access2->is_write) {
        is_race = true;
        snprintf(description, sizeof(description), 
                "Write-write race between threads %u and %u at address %p",
                access1->thread_id, access2->thread_id, access1->address);
    } else if (access1->is_write || access2->is_write) {
        is_race = true;
        snprintf(description, sizeof(description),
                "Read-write race between threads %u and %u at address %p",
                access1->thread_id, access2->thread_id, access1->address);
    }
    
    if (is_race && g_race_condition_count < MAX_RACE_CONDITIONS) {
        RaceCondition *race = malloc(sizeof(RaceCondition));
        if (race) {
            race->memory_address = access1->address;
            race->thread1_id = access1->thread_id;
            race->thread2_id = access2->thread_id;
            race->is_write_conflict = (access1->is_write && access2->is_write);
            race->detection_time = get_timestamp_ns();
            strncpy(race->description, description, sizeof(race->description) - 1);
            
            g_race_conditions[g_race_condition_count++] = race;
            g_statistics.race_conditions_detected++;
            
            if (g_config.break_on_race_condition) {
                // Breakpoint for debugging
                __builtin_trap();
            }
        }
    }
}

static void check_memory_access_race(MemoryAccessRecord *new_access) {
    uint32_t hash = hash_pointer(new_access->address);
    MemoryAccessRecord *existing = g_memory_access_table[hash];
    
    while (existing) {
        // Check if accesses overlap in memory
        void *end1 = (char*)existing->address + existing->size;
        void *end2 = (char*)new_access->address + new_access->size;
        
        bool overlaps = (existing->address <= new_access->address && new_access->address < end1) ||
                       (new_access->address <= existing->address && existing->address < end2);
        
        if (overlaps && existing->thread_id != new_access->thread_id) {
            // Check time window for race condition
            uint64_t time_diff = (new_access->timestamp > existing->timestamp) ?
                                (new_access->timestamp - existing->timestamp) :
                                (existing->timestamp - new_access->timestamp);
            
            if (time_diff <= g_config.race_condition_window_ns) {
                detect_race_condition(existing, new_access);
            }
        }
        
        existing = existing->next;
    }
    
    // Add to hash table
    new_access->next = g_memory_access_table[hash];
    g_memory_access_table[hash] = new_access;
}

// ========================================
// Public API Implementation
// ========================================

bool thread_safety_verifier_init(const ThreadSafetyConfig *config) {
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
        g_config.detect_race_conditions = true;
        g_config.detect_deadlocks = true;
        g_config.track_memory_accesses = false;
        g_config.track_lock_performance = true;
        g_config.generate_warnings = true;
        g_config.break_on_race_condition = false;
        g_config.break_on_deadlock = false;
        g_config.max_records_per_thread = 1000;
        g_config.race_condition_window_ns = 1000000000ULL; // 1 second
        g_config.deadlock_timeout_ns = 5000000000ULL; // 5 seconds
        g_config.report_filename = "thread_safety_report.txt";
    }
    
    // Initialize statistics
    memset(&g_statistics, 0, sizeof(g_statistics));
    
    g_initialized = true;
    
    pthread_mutex_unlock(&g_mutex);
    return true;
}

void thread_safety_verifier_shutdown(bool generate_report) {
    pthread_mutex_lock(&g_mutex);
    
    if (!g_initialized) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    
    if (generate_report) {
        thread_safety_print_summary();
        thread_safety_export_report(g_config.report_filename, "text");
    }
    
    // Cleanup threads
    for (uint32_t i = 0; i < g_thread_count; i++) {
        if (g_threads[i]) {
            free(g_threads[i]);
            g_threads[i] = NULL;
        }
    }
    g_thread_count = 0;
    
    // Cleanup events
    ThreadSafetyRecord *current = g_events_head;
    while (current) {
        ThreadSafetyRecord *next = current->next;
        free(current);
        current = next;
    }
    g_events_head = g_events_tail = NULL;
    g_total_events = 0;
    
    // Cleanup race conditions
    for (uint32_t i = 0; i < g_race_condition_count; i++) {
        if (g_race_conditions[i]) {
            free(g_race_conditions[i]);
            g_race_conditions[i] = NULL;
        }
    }
    g_race_condition_count = 0;
    
    // Cleanup deadlock cycles
    for (uint32_t i = 0; i < g_deadlock_cycle_count; i++) {
        if (g_deadlock_cycles[i]) {
            free(g_deadlock_cycles[i]);
            g_deadlock_cycles[i] = NULL;
        }
    }
    g_deadlock_cycle_count = 0;
    
    // Cleanup memory access records
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        MemoryAccessRecord *current = g_memory_access_table[i];
        while (current) {
            MemoryAccessRecord *next = current->next;
            free(current);
            current = next;
        }
        g_memory_access_table[i] = NULL;
    }
    
    g_initialized = false;
    
    pthread_mutex_unlock(&g_mutex);
}

ThreadSafetyConfig thread_safety_verifier_get_config(void) {
    pthread_mutex_lock(&g_mutex);
    ThreadSafetyConfig config = g_config;
    pthread_mutex_unlock(&g_mutex);
    return config;
}

void thread_safety_verifier_set_config(const ThreadSafetyConfig *config) {
    pthread_mutex_lock(&g_mutex);
    if (config) {
        g_config = *config;
    }
    pthread_mutex_unlock(&g_mutex);
}

bool thread_safety_register_thread(uint32_t thread_id, const char *thread_name) {
    if (!g_initialized || !g_config.enabled) return false;
    
    pthread_mutex_lock(&g_mutex);
    
    ThreadInfo *thread = find_or_create_thread(thread_id);
    if (!thread) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    
    if (thread_name) {
        thread->thread_name = thread_name;
    }
    
    pthread_mutex_unlock(&g_mutex);
    return true;
}

void thread_safety_unregister_thread(uint32_t thread_id) {
    if (!g_initialized || !g_config.enabled) return;
    
    pthread_mutex_lock(&g_mutex);
    
    for (uint32_t i = 0; i < g_thread_count; i++) {
        if (g_threads[i] && g_threads[i]->thread_id == thread_id) {
            g_threads[i]->is_active = false;
            g_statistics.active_threads--;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_mutex);
}

ThreadInfo* thread_safety_get_thread_info(uint32_t thread_id) {
    if (!g_initialized) return NULL;
    
    pthread_mutex_lock(&g_mutex);
    
    ThreadInfo *thread = NULL;
    for (uint32_t i = 0; i < g_thread_count; i++) {
        if (g_threads[i] && g_threads[i]->thread_id == thread_id) {
            thread = g_threads[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&g_mutex);
    return thread;
}

bool thread_safety_record_event(ThreadSafetyEventType event_type, void *resource,
                                const char *function, const char *file, int line,
                                const char *description) {
    if (!g_initialized || !g_config.enabled) return false;
    
    if (should_ignore_event(file)) return false;
    
    pthread_mutex_lock(&g_mutex);
    
    ThreadSafetyRecord *record = create_event_record(event_type, resource, function, file, line, description);
    if (!record) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    
    add_event_record(record);
    update_thread_activity(record->thread_id);
    update_lock_statistics(record);
    
    // Call callback if set
    if (g_event_callback) {
        g_event_callback(record);
    }
    
    pthread_mutex_unlock(&g_mutex);
    return true;
}

bool thread_safety_record_memory_access(void *address, size_t size, bool is_write,
                                        const char *function, const char *file, int line) {
    if (!g_initialized || !g_config.enabled || !g_config.track_memory_accesses) {
        return false;
    }
    
    if (should_ignore_event(file)) return false;
    
    pthread_mutex_lock(&g_mutex);
    
    MemoryAccessRecord *access = malloc(sizeof(MemoryAccessRecord));
    if (!access) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    
    access->address = address;
    access->size = size;
    access->is_write = is_write;
    access->thread_id = get_thread_id();
    access->timestamp = get_timestamp_ns();
    access->function = function;
    access->file = file;
    access->line = line;
    
    check_memory_access_race(access);
    g_statistics.memory_accesses_tracked++;
    
    pthread_mutex_unlock(&g_mutex);
    return true;
}

bool thread_safety_record_lock_acquire(pthread_mutex_t *mutex,
                                      const char *function, const char *file, int line) {
    return thread_safety_record_event(THREAD_EVENT_LOCK_ACQUIRE, mutex, function, file, line, NULL);
}

bool thread_safety_record_lock_release(pthread_mutex_t *mutex,
                                      const char *function, const char *file, int line) {
    return thread_safety_record_event(THREAD_EVENT_LOCK_RELEASE, mutex, function, file, line, NULL);
}

ThreadSafetyStatistics thread_safety_get_statistics(void) {
    pthread_mutex_lock(&g_mutex);
    ThreadSafetyStatistics stats = g_statistics;
    pthread_mutex_unlock(&g_mutex);
    return stats;
}

bool thread_safety_check_race_conditions(uint32_t *race_count) {
    pthread_mutex_lock(&g_mutex);
    
    bool has_races = g_race_condition_count > 0;
    if (race_count) {
        *race_count = g_race_condition_count;
    }
    
    pthread_mutex_unlock(&g_mutex);
    return has_races;
}

uint32_t thread_safety_get_race_conditions(RaceCondition *races, uint32_t max_races) {
    if (!races) return 0;
    
    pthread_mutex_lock(&g_mutex);
    
    uint32_t count = (max_races < g_race_condition_count) ? max_races : g_race_condition_count;
    for (uint32_t i = 0; i < count; i++) {
        races[i] = *g_race_conditions[i];
    }
    
    pthread_mutex_unlock(&g_mutex);
    return count;
}

bool thread_safety_check_deadlocks(uint32_t *deadlock_count) {
    pthread_mutex_lock(&g_mutex);
    
    bool has_deadlocks = g_deadlock_cycle_count > 0;
    if (deadlock_count) {
        *deadlock_count = g_deadlock_cycle_count;
    }
    
    pthread_mutex_unlock(&g_mutex);
    return has_deadlocks;
}

void thread_safety_print_summary(void) {
    pthread_mutex_lock(&g_mutex);
    
    uint32_t race_count, deadlock_count;
    thread_safety_check_race_conditions(&race_count);
    thread_safety_check_deadlocks(&deadlock_count);
    
    printf("\n=== Thread Safety Verification Summary ===\n");
    printf("Total Threads: %u\n", g_statistics.total_threads);
    printf("Active Threads: %u\n", g_statistics.active_threads);
    printf("Total Events: %llu\n", (unsigned long long)g_statistics.total_events);
    printf("Lock Acquisitions: %llu\n", (unsigned long long)g_statistics.lock_acquisitions);
    printf("Lock Releases: %llu\n", (unsigned long long)g_statistics.lock_releases);
    printf("Race Conditions Detected: %u\n", g_statistics.race_conditions_detected);
    printf("Deadlocks Detected: %u\n", g_statistics.deadlocks_detected);
    printf("Lock Contentions: %u\n", g_statistics.lock_contentions);
    printf("Memory Accesses Tracked: %llu\n", (unsigned long long)g_statistics.memory_accesses_tracked);
    
    if (race_count > 0) {
        printf("\n*** RACE CONDITIONS DETECTED ***\n");
        printf("Total Race Conditions: %u\n", race_count);
    }
    
    if (deadlock_count > 0) {
        printf("\n*** DEADLOCKS DETECTED ***\n");
        printf("Total Deadlock Cycles: %u\n", deadlock_count);
    }
    
    printf("========================================\n\n");
    
    pthread_mutex_unlock(&g_mutex);
}

bool thread_safety_export_report(const char *filename, const char *format) {
    if (!filename || !format) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    pthread_mutex_lock(&g_mutex);
    
    if (strcmp(format, "json") == 0) {
        fprintf(file, "{\n");
        fprintf(file, "  \"statistics\": {\n");
        fprintf(file, "    \"total_threads\": %u,\n", g_statistics.total_threads);
        fprintf(file, "    \"active_threads\": %u,\n", g_statistics.active_threads);
        fprintf(file, "    \"total_events\": %llu,\n", (unsigned long long)g_statistics.total_events);
        fprintf(file, "    \"lock_acquisitions\": %llu,\n", (unsigned long long)g_statistics.lock_acquisitions);
        fprintf(file, "    \"lock_releases\": %llu,\n", (unsigned long long)g_statistics.lock_releases);
        fprintf(file, "    \"race_conditions_detected\": %u,\n", g_statistics.race_conditions_detected);
        fprintf(file, "    \"deadlocks_detected\": %u,\n", g_statistics.deadlocks_detected);
        fprintf(file, "    \"lock_contentions\": %u,\n", g_statistics.lock_contentions);
        fprintf(file, "    \"memory_accesses_tracked\": %llu\n", (unsigned long long)g_statistics.memory_accesses_tracked);
        fprintf(file, "  },\n");
        
        fprintf(file, "  \"race_conditions\": [\n");
        for (uint32_t i = 0; i < g_race_condition_count; i++) {
            if (i > 0) fprintf(file, ",\n");
            fprintf(file, "    {\n");
            fprintf(file, "      \"memory_address\": \"%p\",\n", g_race_conditions[i]->memory_address);
            fprintf(file, "      \"thread1_id\": %u,\n", g_race_conditions[i]->thread1_id);
            fprintf(file, "      \"thread2_id\": %u,\n", g_race_conditions[i]->thread2_id);
            fprintf(file, "      \"is_write_conflict\": %s,\n", g_race_conditions[i]->is_write_conflict ? "true" : "false");
            fprintf(file, "      \"description\": \"%s\",\n", g_race_conditions[i]->description);
            fprintf(file, "      \"detection_time\": %llu\n", (unsigned long long)g_race_conditions[i]->detection_time);
            fprintf(file, "    }");
        }
        fprintf(file, "\n  ]\n");
        fprintf(file, "}\n");
    } else {
        // Text format
        fprintf(file, "Thread Safety Verification Report\n");
        fprintf(file, "================================\n\n");
        
        fprintf(file, "Statistics:\n");
        fprintf(file, "  Total Threads: %u\n", g_statistics.total_threads);
        fprintf(file, "  Active Threads: %u\n", g_statistics.active_threads);
        fprintf(file, "  Total Events: %llu\n", (unsigned long long)g_statistics.total_events);
        fprintf(file, "  Lock Acquisitions: %llu\n", (unsigned long long)g_statistics.lock_acquisitions);
        fprintf(file, "  Lock Releases: %llu\n", (unsigned long long)g_statistics.lock_releases);
        fprintf(file, "  Race Conditions Detected: %u\n", g_statistics.race_conditions_detected);
        fprintf(file, "  Deadlocks Detected: %u\n", g_statistics.deadlocks_detected);
        fprintf(file, "  Lock Contentions: %u\n", g_statistics.lock_contentions);
        fprintf(file, "  Memory Accesses Tracked: %llu\n\n", (unsigned long long)g_statistics.memory_accesses_tracked);
        
        if (g_race_condition_count > 0) {
            fprintf(file, "Race Conditions:\n");
            for (uint32_t i = 0; i < g_race_condition_count; i++) {
                fprintf(file, "  Race Condition #%u:\n", i + 1);
                fprintf(file, "    Memory Address: %p\n", g_race_conditions[i]->memory_address);
                fprintf(file, "    Thread IDs: %u, %u\n", g_race_conditions[i]->thread1_id, g_race_conditions[i]->thread2_id);
                fprintf(file, "    Write Conflict: %s\n", g_race_conditions[i]->is_write_conflict ? "Yes" : "No");
                fprintf(file, "    Description: %s\n\n", g_race_conditions[i]->description);
            }
        }
        
        if (g_deadlock_cycle_count > 0) {
            fprintf(file, "Deadlock Cycles:\n");
            for (uint32_t i = 0; i < g_deadlock_cycle_count; i++) {
                fprintf(file, "  Deadlock Cycle #%u:\n", i + 1);
                fprintf(file, "    Description: %s\n\n", g_deadlock_cycles[i]->description);
            }
        }
    }
    
    pthread_mutex_unlock(&g_mutex);
    fclose(file);
    return true;
}

void thread_safety_set_debug_mode(bool enabled) {
    pthread_mutex_lock(&g_mutex);
    g_debug_mode = enabled;
    pthread_mutex_unlock(&g_mutex);
}

void thread_safety_set_event_callback(void (*callback)(ThreadSafetyRecord*)) {
    pthread_mutex_lock(&g_mutex);
    g_event_callback = callback;
    pthread_mutex_unlock(&g_mutex);
}

bool thread_safety_validate_tracking(void) {
    pthread_mutex_lock(&g_mutex);
    
    // Basic validation: check that thread counts match
    uint32_t active_count = 0;
    for (uint32_t i = 0; i < g_thread_count; i++) {
        if (g_threads[i] && g_threads[i]->is_active) {
            active_count++;
        }
    }
    
    bool valid = (active_count == g_statistics.active_threads);
    
    pthread_mutex_unlock(&g_mutex);
    return valid;
}
