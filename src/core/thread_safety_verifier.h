/**
 * @file thread_safety_verifier.h
 * @brief Thread safety verification and analysis system
 *
 * Comprehensive thread safety verification system for the Minecraft v2 Engine.
 * Detects race conditions, deadlocks, and provides thread usage analysis.
 */

#ifndef THREAD_SAFETY_VERIFIER_H
#define THREAD_SAFETY_VERIFIER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Thread Safety Event Types
// ========================================

typedef enum ThreadSafetyEventType {
    THREAD_EVENT_LOCK_ACQUIRE,         // Mutex lock acquired
    THREAD_EVENT_LOCK_RELEASE,         // Mutex lock released
    THREAD_EVENT_LOCK_TRY,              // Mutex lock attempted
    THREAD_EVENT_LOCK_TIMEOUT,          // Mutex lock timeout
    THREAD_EVENT_RWLOCK_READ,           // Read lock acquired
    THREAD_EVENT_RWLOCK_WRITE,          // Write lock acquired
    THREAD_EVENT_RWLOCK_RELEASE,        // RW lock released
    THREAD_EVENT_COND_WAIT,             // Condition variable wait
    THREAD_EVENT_COND_SIGNAL,           // Condition variable signal
    THREAD_EVENT_COND_BROADCAST,        // Condition variable broadcast
    THREAD_EVENT_THREAD_CREATE,         // Thread created
    THREAD_EVENT_THREAD_JOIN,           // Thread joined
    THREAD_EVENT_THREAD_EXIT,           // Thread exited
    THREAD_EVENT_MEMORY_ACCESS,         // Memory access
    THREAD_EVENT_CRITICAL_SECTION,     // Critical section entry/exit
    THREAD_EVENT_ATOMIC_OPERATION,      // Atomic operation
    THREAD_EVENT_BARRIER_WAIT,          // Barrier wait
    THREAD_EVENT_SEMAPHORE_WAIT,        // Semaphore wait
    THREAD_EVENT_SEMAPHORE_POST         // Semaphore post
} ThreadSafetyEventType;

// ========================================
// Thread Safety Record
// ========================================

typedef struct ThreadSafetyRecord {
    ThreadSafetyEventType event_type;   // Type of event
    uint32_t thread_id;                 // Thread ID
    uint64_t timestamp;                 // Event timestamp
    const char *function;               // Function name
    const char *file;                    // Source file
    int line;                           // Source line
    void *resource;                     // Resource (mutex, memory, etc.)
    size_t resource_size;               // Size of resource (for memory)
    uint64_t duration_ns;               // Duration for held locks
    uint32_t nesting_level;             // Lock nesting level
    bool is_read_lock;                  // Whether it's a read lock
    struct ThreadSafetyRecord *next;    // Next record in list
    char description[256];              // Event description
} ThreadSafetyRecord;

// ========================================
// Thread Information
// ========================================

typedef struct ThreadInfo {
    uint32_t thread_id;                 // Thread ID
    pthread_t pthread_handle;           // Pthread handle
    const char *thread_name;            // Thread name
    uint64_t creation_time;             // Thread creation time
    uint64_t last_activity;             // Last activity time
    uint32_t lock_count;                // Number of locks held
    uint32_t total_locks_acquired;      // Total locks acquired
    uint64_t total_lock_time_ns;        // Total time holding locks
    uint32_t race_condition_count;      // Number of race conditions detected
    uint32_t deadlock_warnings;         // Number of deadlock warnings
    bool is_active;                     // Thread is currently active
    ThreadSafetyRecord *current_locks;  // Currently held locks
} ThreadInfo;

// ========================================
// Race Condition Detection
// ========================================

typedef struct RaceCondition {
    void *memory_address;               // Memory address involved
    uint32_t thread1_id;                // First thread ID
    uint32_t thread2_id;                // Second thread ID
    ThreadSafetyRecord *access1;        // First access record
    ThreadSafetyRecord *access2;        // Second access record
    bool is_write_conflict;             // Whether it's a write-write conflict
    uint64_t detection_time;            // When race condition was detected
    char description[512];               // Description of the race condition
} RaceCondition;

// ========================================
// Deadlock Detection
// ========================================

typedef struct DeadlockCycle {
    uint32_t thread_ids[16];            // Thread IDs in cycle
    int thread_count;                   // Number of threads in cycle
    void *resources[16];                // Resources in cycle
    int resource_count;                 // Number of resources in cycle
    uint64_t detection_time;            // When deadlock was detected
    char description[1024];             // Description of the deadlock
} DeadlockCycle;

// ========================================
// Thread Safety Configuration
// ========================================

typedef struct ThreadSafetyConfig {
    bool enabled;                       // Enable thread safety verification
    bool detect_race_conditions;        // Enable race condition detection
    bool detect_deadlocks;              // Enable deadlock detection
    bool track_memory_accesses;         // Track memory accesses
    bool track_lock_performance;        // Track lock performance
    bool generate_warnings;             // Generate warnings
    bool break_on_race_condition;      // Break on race condition
    bool break_on_deadlock;             // Break on deadlock
    uint32_t max_records_per_thread;    // Maximum records per thread
    uint64_t race_condition_window_ns; // Time window for race detection
    uint64_t deadlock_timeout_ns;       // Timeout for deadlock detection
    const char *report_filename;        // Report output filename
    const char *ignore_patterns[16];    // Patterns to ignore
    int ignore_pattern_count;           // Number of ignore patterns
} ThreadSafetyConfig;

// ========================================
// Thread Safety Statistics
// ========================================

typedef struct ThreadSafetyStatistics {
    uint32_t total_threads;             // Total threads created
    uint32_t active_threads;            // Currently active threads
    uint64_t total_events;              // Total events recorded
    uint64_t lock_acquisitions;          // Total lock acquisitions
    uint64_t lock_releases;             // Total lock releases
    uint64_t total_lock_time_ns;        // Total time holding locks
    uint64_t average_lock_time_ns;      // Average lock hold time
    uint32_t race_conditions_detected;  // Race conditions detected
    uint32_t deadlocks_detected;        // Deadlocks detected
    uint32_t lock_contentions;          // Lock contentions detected
    uint32_t priority_inversions;       // Priority inversions detected
    double lock_contention_rate;        // Lock contention rate
    uint64_t memory_accesses_tracked;   // Memory accesses tracked
} ThreadSafetyStatistics;

// ========================================
// Thread Safety Report
// ========================================

typedef struct ThreadSafetyReport {
    ThreadSafetyStatistics statistics;   // Thread safety statistics
    RaceCondition *race_conditions;     // Array of race conditions
    uint32_t race_condition_count;      // Number of race conditions
    DeadlockCycle *deadlock_cycles;     // Array of deadlock cycles
    uint32_t deadlock_cycle_count;      // Number of deadlock cycles
    ThreadInfo *thread_info;            // Array of thread information
    uint32_t thread_count;              // Number of threads
    char summary[2048];                 // Report summary
    char detailed_report[131072];        // Detailed report text
    uint64_t report_timestamp;          // When report was generated
    double generation_time_ms;           // Time to generate report
} ThreadSafetyReport;

// ========================================
// Initialization and Configuration
// ========================================

/**
 * Initialize thread safety verifier
 * @param config Configuration settings
 * @return True if initialization successful
 */
bool thread_safety_verifier_init(const ThreadSafetyConfig *config);

/**
 * Shutdown thread safety verifier
 * @param generate_report Whether to generate final report
 */
void thread_safety_verifier_shutdown(bool generate_report);

/**
 * Get current configuration
 * @return Current configuration
 */
ThreadSafetyConfig thread_safety_verifier_get_config(void);

/**
 * Update configuration
 * @param config New configuration
 */
void thread_safety_verifier_set_config(const ThreadSafetyConfig *config);

// ========================================
// Thread Tracking
// ========================================

/**
 * Register thread creation
 * @param thread_id Thread ID
 * @param thread_name Thread name (optional)
 * @return True if registration successful
 */
bool thread_safety_register_thread(uint32_t thread_id, const char *thread_name);

/**
 * Unregister thread
 * @param thread_id Thread ID
 */
void thread_safety_unregister_thread(uint32_t thread_id);

/**
 * Get thread information
 * @param thread_id Thread ID
 * @return Thread information or NULL if not found
 */
ThreadInfo* thread_safety_get_thread_info(uint32_t thread_id);

/**
 * Set thread name
 * @param thread_id Thread ID
 * @param thread_name Thread name
 */
void thread_safety_set_thread_name(uint32_t thread_id, const char *thread_name);

// ========================================
// Event Recording
// ========================================

/**
 * Record thread safety event
 * @param event_type Type of event
 * @param resource Resource involved (mutex, memory, etc.)
 * @param function Function name
 * @param file Source file
 * @param line Source line
 * @param description Event description
 * @return True if recording successful
 */
bool thread_safety_record_event(ThreadSafetyEventType event_type, void *resource,
                                const char *function, const char *file, int line,
                                const char *description);

/**
 * Record memory access
 * @param address Memory address
 * @param size Size of access
 * @param is_write Whether it's a write access
 * @param function Function name
 * @param file Source file
 * @param line Source line
 * @return True if recording successful
 */
bool thread_safety_record_memory_access(void *address, size_t size, bool is_write,
                                        const char *function, const char *file, int line);

/**
 * Record lock acquisition
 * @param mutex Mutex pointer
 * @param function Function name
 * @param file Source file
 * @param line Source line
 * @return True if recording successful
 */
bool thread_safety_record_lock_acquire(pthread_mutex_t *mutex,
                                      const char *function, const char *file, int line);

/**
 * Record lock release
 * @param mutex Mutex pointer
 * @param function Function name
 * @param file Source file
 * @param line Source line
 * @return True if recording successful
 */
bool thread_safety_record_lock_release(pthread_mutex_t *mutex,
                                      const char *function, const char *file, int line);

// ========================================
// Race Condition Detection
// ========================================

/**
 * Check for race conditions
 * @param race_count Output number of race conditions
 * @return True if race conditions detected
 */
bool thread_safety_check_race_conditions(uint32_t *race_count);

/**
 * Get race conditions
 * @param races Output array for race conditions
 * @param max_races Maximum number of races to return
 * @return Number of race conditions found
 */
uint32_t thread_safety_get_race_conditions(RaceCondition *races, uint32_t max_races);

/**
 * Clear detected race conditions
 */
void thread_safety_clear_race_conditions(void);

// ========================================
// Deadlock Detection
// ========================================

/**
 * Check for potential deadlocks
 * @param deadlock_count Output number of deadlocks
 * @return True if deadlocks detected
 */
bool thread_safety_check_deadlocks(uint32_t *deadlock_count);

/**
 * Get deadlock cycles
 * @param cycles Output array for deadlock cycles
 * @param max_cycles Maximum number of cycles to return
 * @return Number of deadlock cycles found
 */
uint32_t thread_safety_get_deadlock_cycles(DeadlockCycle *cycles, uint32_t max_cycles);

/**
 * Analyze lock dependency graph
 * @return True if analysis successful
 */
bool thread_safety_analyze_dependencies(void);

// ========================================
// Statistics and Monitoring
// ========================================

/**
 * Get current thread safety statistics
 * @return Thread safety statistics
 */
ThreadSafetyStatistics thread_safety_get_statistics(void);

/**
 * Reset thread safety statistics
 */
void thread_safety_reset_statistics(void);

/**
 * Get lock contention rate
 * @return Lock contention rate (0.0-1.0)
 */
double thread_safety_get_contention_rate(void);

/**
 * Get average lock hold time
 * @return Average lock hold time in nanoseconds
 */
uint64_t thread_safety_get_average_lock_time(void);

// ========================================
// Reporting and Analysis
// ========================================

/**
 * Generate thread safety report
 * @return Generated report or NULL on failure
 */
ThreadSafetyReport* thread_safety_generate_report(void);

/**
 * Export report to file
 * @param filename Output filename
 * @param format Export format (text, json, xml, csv)
 * @return True if export successful
 */
bool thread_safety_export_report(const char *filename, const char *format);

/**
 * Print thread safety summary to console
 */
void thread_safety_print_summary(void);

/**
 * Print detailed thread safety report
 */
void thread_safety_print_detailed_report(void);

/**
 * Print race condition analysis
 */
void thread_safety_print_race_analysis(void);

/**
 * Print deadlock analysis
 */
void thread_safety_print_deadlock_analysis(void);

/**
 * Print thread performance analysis
 */
void thread_safety_print_performance_analysis(void);

// ========================================
// Advanced Analysis
// ========================================

/**
 * Detect priority inversions
 * @param inversion_count Output number of inversions
 * @return True if priority inversions detected
 */
bool thread_safety_detect_priority_inversions(uint32_t *inversion_count);

/**
 * Analyze lock usage patterns
 * @return Analysis report string
 */
char* thread_safety_analyze_lock_patterns(void);

/**
 * Find thread safety hotspots
 * @param hotspots Output array of hotspot locations
 * @param max_hotspots Maximum hotspots to return
 * @return Number of hotspots found
 */
uint32_t thread_safety_find_hotspots(ThreadSafetyRecord **hotspots, uint32_t max_hotspots);

/**
 * Validate thread safety of specific function
 * @param function Function name to validate
 * @return True if function appears thread-safe
 */
bool thread_safety_validate_function(const char *function);

// ========================================
// Debugging and Utilities
// ========================================

/**
 * Enable/disable debug mode
 * @param enabled Enable debug mode
 */
void thread_safety_set_debug_mode(bool enabled);

/**
 * Set custom event callback
 * @param callback Custom callback function
 */
void thread_safety_set_event_callback(void (*callback)(ThreadSafetyRecord*));

/**
 * Dump all thread events to log
 */
void thread_safety_dump_events(void);

/**
 * Validate thread tracking integrity
 * @return True if tracking is consistent
 */
bool thread_safety_validate_tracking(void);

/**
 * Force garbage collection of tracking data
 */
void thread_safety_force_gc(void);

// ========================================
// Macros for Easy Integration
// ========================================

#ifdef THREAD_SAFETY_VERIFICATION_ENABLED
    #define THREAD_SAFETY_RECORD_LOCK(mutex) \
        thread_safety_record_lock_acquire(mutex, __FUNCTION__, __FILE__, __LINE__)
    #define THREAD_SAFETY_RECORD_UNLOCK(mutex) \
        thread_safety_record_lock_release(mutex, __FUNCTION__, __FILE__, __LINE__)
    #define THREAD_SAFETY_RECORD_MEMORY_READ(addr, size) \
        thread_safety_record_memory_access(addr, size, false, __FUNCTION__, __FILE__, __LINE__)
    #define THREAD_SAFETY_RECORD_MEMORY_WRITE(addr, size) \
        thread_safety_record_memory_access(addr, size, true, __FUNCTION__, __FILE__, __LINE__)
    #define THREAD_SAFETY_REGISTER_THREAD(name) \
        thread_safety_register_thread((uint32_t)pthread_self(), name)
    #define THREAD_SAFETY_UNREGISTER_THREAD() \
        thread_safety_unregister_thread((uint32_t)pthread_self())
#else
    #define THREAD_SAFETY_RECORD_LOCK(mutex) do {} while(0)
    #define THREAD_SAFETY_RECORD_UNLOCK(mutex) do {} while(0)
    #define THREAD_SAFETY_RECORD_MEMORY_READ(addr, size) do {} while(0)
    #define THREAD_SAFETY_RECORD_MEMORY_WRITE(addr, size) do {} while(0)
    #define THREAD_SAFETY_REGISTER_THREAD(name) do {} while(0)
    #define THREAD_SAFETY_UNREGISTER_THREAD() do {} while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* THREAD_SAFETY_VERIFIER_H */
