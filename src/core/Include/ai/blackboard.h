// include/engine/ai/blackboard.h
//
// Purpose: Blackboard data storage system for AI behavior trees
// This system provides efficient key-value data storage and sharing between AI nodes
//
// Key Features:
// - Type-safe key-value storage (Float, Int, Vector, Entity, String, etc.)
// - Fast hash table lookup for O(1) access
// - Memory pooling for efficient allocation
// - Multiplayer synchronization support
// - Blackboard decorators for conditional execution
// - Persistent and temporary data storage
// - Debug visualization and monitoring
//
// Performance Targets:
// - <1s lookup time for cached entries
// - <10s insertion time for new entries
// - <100KB memory overhead per AI agent
// - Support for 1000+ concurrent entries
//
// Ownership: Each AI agent owns its blackboard instance
// Invariants: Keys must be unique, data types must match operations
//
#ifndef AI_BLACKBOARD_H
#define AI_BLACKBOARD_H

#include <common.h>
#include "include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// BLACKBOARD DATA TYPES
// ============================================================================

typedef enum {
    BLACKBOARD_TYPE_FLOAT = 0,
    BLACKBOARD_TYPE_INT,
    BLACKBOARD_TYPE_BOOL,
    BLACKBOARD_TYPE_VECTOR3,
    BLACKBOARD_TYPE_VECTOR4,
    BLACKBOARD_TYPE_ENTITY,
    BLACKBOARD_TYPE_STRING,
    BLACKBOARD_TYPE_TRANSFORM,
    BLACKBOARD_TYPE_COLOR,
    BLACKBOARD_TYPE_QUATERNION,
    BLACKBOARD_TYPE_ARRAY,
    BLACKBOARD_TYPE_CUSTOM
} BlackboardType;

// ============================================================================
// BLACKBOARD ENTRY STRUCTURE
// ============================================================================

typedef struct {
    char key[64];                    // Key name (max 63 chars + null)
    BlackboardType type;             // Data type
    union {
        f32 float_value;
        i32 int_value;
        struct {
            f32 x, y, z;
        } vector3_value;
        struct {
            f32 x, y, z, w;
        } vector4_value;
        u32 entity_value;
        char string_value[256];
        struct {
            struct {
                f32 x, y, z;
            } position;
            struct {
                f32 x, y, z, w;
            } rotation;
            struct {
                f32 x, y, z;
            } scale;
        } transform_value;
        struct {
            f32 r, g, b, a;
        } color_value;
        struct {
            f32 x, y, z, w;
        } quaternion_value;
        void *custom_value;
    } data;
    
    // Metadata
    f64 last_access_time;            // Last time this entry was accessed
    f64 creation_time;               // When this entry was created
    u32 access_count;                // Number of times accessed
    bool is_persistent;              // Should persist across behavior tree resets
    bool is_network_synced;          // Should sync across multiplayer
    bool is_read_only;               // Cannot be modified after creation
    u32 version;                     // For change detection
} BlackboardEntry;

// ============================================================================
// BLACKBOARD DECORATORS
// ============================================================================

typedef enum {
    BLACKBOARD_DECORATOR_EXISTS = 0,     // Execute only if key exists
    BLACKBOARD_DECORATOR_NOT_EXISTS,     // Execute only if key doesn't exist
    BLACKBOARD_DECORATOR_EQUALS,         // Execute only if key equals value
    BLACKBOARD_DECORATOR_NOT_EQUALS,     // Execute only if key doesn't equal value
    BLACKBOARD_DECORATOR_GREATER,        // Execute only if key > value
    BLACKBOARD_DECORATOR_LESS,           // Execute only if key < value
    BLACKBOARD_DECORATOR_CHANGED,        // Execute only if key changed since last check
    BLACKBOARD_DECORATOR_TIMEOUT         // Execute only if timeout expired
} BlackboardDecoratorType;

typedef struct {
    BlackboardDecoratorType type;
    char key[64];
    union {
        f32 float_threshold;
        i32 int_threshold;
        bool bool_threshold;
        f64 timeout_seconds;
    } threshold;
    bool negate_result;                  // Invert the decorator result
    u32 last_check_version;              // For change detection
    f64 last_check_time;                 // For timeout detection
} BlackboardDecorator;

// ============================================================================
// BLACKBOARD SYSTEM
// ============================================================================

typedef struct {
    BlackboardEntry *entries;            // Array of entries
    u32 entry_count;                     // Current number of entries
    u32 max_entries;                     // Maximum number of entries
    
    // Hash table for fast lookup
    u32 hash_table_size;                 // Size of hash table
    BlackboardEntry **hash_table;        // Hash table buckets
    u32 *hash_collision_count;           // Collision statistics
    
    // Memory management
    void *memory_pool;                    // Pre-allocated memory pool
    size_t pool_size;                     // Size of memory pool
    size_t pool_used;                     // Currently used pool memory
    
    // Decorators
    BlackboardDecorator *decorators;     // Array of decorators
    u32 decorator_count;                  // Current number of decorators
    u32 max_decorators;                  // Maximum number of decorators
    
    // Statistics
    u32 total_lookups;                   // Total lookup operations
    u32 cache_hits;                      // Cache hit count
    u32 cache_misses;                    // Cache miss count
    f64 total_lookup_time;               // Total time spent in lookups
    
    // Multiplayer synchronization
    bool is_network_enabled;             // Network sync enabled
    u32 sync_version;                    // Current sync version
    void *network_context;               // Network sync context
    
    // Debug information
    bool is_debug_enabled;               // Debug logging enabled
    char owner_name[64];                 // Name of blackboard owner
} Blackboard;

// ============================================================================
// PUBLIC API - BLACKBOARD MANAGEMENT
// ============================================================================

// Blackboard creation and destruction
Blackboard *blackboard_create(u32 max_entries, const char *owner_name);
void blackboard_destroy(Blackboard *blackboard);
bool blackboard_initialize(Blackboard *blackboard);
void blackboard_shutdown(Blackboard *blackboard);

// Configuration
void blackboard_enable_network_sync(Blackboard *blackboard, bool enable);
void blackboard_enable_debug(Blackboard *blackboard, bool enable);
void blackboard_set_memory_pool_size(Blackboard *blackboard, size_t pool_size);

// ============================================================================
// PUBLIC API - DATA OPERATIONS
// ============================================================================

// Basic type operations
bool blackboard_set_float(Blackboard *blackboard, const char *key, f32 value, bool persistent);
bool blackboard_set_int(Blackboard *blackboard, const char *key, i32 value, bool persistent);
bool blackboard_set_bool(Blackboard *blackboard, const char *key, bool value, bool persistent);
bool blackboard_set_vector3(Blackboard *blackboard, const char *key, f32 x, f32 y, f32 z, bool persistent);
bool blackboard_set_vector4(Blackboard *blackboard, const char *key, f32 x, f32 y, f32 z, f32 w, bool persistent);
bool blackboard_set_entity(Blackboard *blackboard, const char *key, u32 entity, bool persistent);
bool blackboard_set_string(Blackboard *blackboard, const char *key, const char *value, bool persistent);
bool blackboard_set_custom(Blackboard *blackboard, const char *key, void *value, size_t size, bool persistent);

bool blackboard_get_float(Blackboard *blackboard, const char *key, f32 *value);
bool blackboard_get_int(Blackboard *blackboard, const char *key, i32 *value);
bool blackboard_get_bool(Blackboard *blackboard, const char *key, bool *value);
bool blackboard_get_vector3(Blackboard *blackboard, const char *key, f32 *x, f32 *y, f32 *z);
bool blackboard_get_vector4(Blackboard *blackboard, const char *key, f32 *x, f32 *y, f32 *z, f32 *w);
bool blackboard_get_entity(Blackboard *blackboard, const char *key, u32 *entity);
bool blackboard_get_string(Blackboard *blackboard, const char *key, char *value, size_t max_length);
bool blackboard_get_custom(Blackboard *blackboard, const char *key, void **value, size_t *size);

// Type checking and conversion
BlackboardType blackboard_get_type(Blackboard *blackboard, const char *key);
bool blackboard_has_key(Blackboard *blackboard, const char *key);
bool blackboard_remove_key(Blackboard *blackboard, const char *key);
void blackboard_clear_non_persistent(Blackboard *blackboard);
void blackboard_clear_all(Blackboard *blackboard);

// ============================================================================
// PUBLIC API - DECORATORS
// ============================================================================

// Decorator management
bool blackboard_add_decorator_exists(Blackboard *blackboard, const char *key, bool negate);
bool blackboard_add_decorator_equals(Blackboard *blackboard, const char *key, f32 value, bool negate);
bool blackboard_add_decorator_greater(Blackboard *blackboard, const char *key, f32 threshold, bool negate);
bool blackboard_add_decorator_changed(Blackboard *blackboard, const char *key, bool negate);
bool blackboard_add_decorator_timeout(Blackboard *blackboard, const char *key, f64 timeout_seconds, bool negate);

bool blackboard_remove_decorator(Blackboard *blackboard, u32 decorator_index);
bool blackboard_evaluate_decorators(Blackboard *blackboard);

// ============================================================================
// PUBLIC API - SYNCHRONIZATION
// ============================================================================

// Multiplayer synchronization
bool blackboard_sync_to_network(Blackboard *blackboard);
bool blackboard_sync_from_network(Blackboard *blackboard, const void *network_data, size_t data_size);
void blackboard_mark_dirty(Blackboard *blackboard, const char *key);
u32 blackboard_get_sync_version(Blackboard *blackboard);

// ============================================================================
// PUBLIC API - DEBUGGING AND MONITORING
// ============================================================================

// Statistics and monitoring
typedef struct {
    u32 entry_count;
    u32 decorator_count;
    u32 total_lookups;
    u32 cache_hits;
    u32 cache_misses;
    f32 cache_hit_rate;
    f64 average_lookup_time;
    size_t memory_usage;
    u32 network_sync_count;
    f64 total_sync_time;
} BlackboardStats;

BlackboardStats *blackboard_get_statistics(Blackboard *blackboard);
void blackboard_reset_statistics(Blackboard *blackboard);
void blackboard_print_statistics(Blackboard *blackboard);

// Debug information
void blackboard_dump_contents(Blackboard *blackboard);
void blackboard_dump_decorators(Blackboard *blackboard);
void blackboard_validate_integrity(Blackboard *blackboard);

// ============================================================================
// PUBLIC API - UTILITY FUNCTIONS
// ============================================================================

// Type utilities
const char *blackboard_get_type_name(BlackboardType type);
size_t blackboard_get_type_size(BlackboardType type);
bool blackboard_can_convert_types(BlackboardType from, BlackboardType to);

// Key utilities
bool blackboard_is_valid_key(const char *key);
u32 blackboard_hash_key(const char *key);

// Memory utilities
size_t blackboard_estimate_memory_usage(u32 max_entries, u32 max_decorators);
void blackboard_defragment(Blackboard *blackboard);

// ============================================================================
// ERROR HANDLING
// ============================================================================

typedef enum {
    BLACKBOARD_ERROR_NONE = 0,
    BLACKBOARD_ERROR_INVALID_PARAMETER,
    BLACKBOARD_ERROR_KEY_NOT_FOUND,
    BLACKBOARD_ERROR_TYPE_MISMATCH,
    BLACKBOARD_ERROR_OUT_OF_MEMORY,
    BLACKBOARD_ERROR_KEY_EXISTS,
    BLACKBOARD_ERROR_READ_ONLY,
    BLACKBOARD_ERROR_NETWORK_ERROR
} BlackboardError;

const char *blackboard_get_error_string(BlackboardError error);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Type-safe getter macros
#define BLACKBOARD_GET_FLOAT(bb, key, default_val) \
    ({ f32 val; blackboard_get_float(bb, key, &val) ? val : (default_val); })

#define BLACKBOARD_GET_INT(bb, key, default_val) \
    ({ i32 val; blackboard_get_int(bb, key, &val) ? val : (default_val); })

#define BLACKBOARD_GET_BOOL(bb, key, default_val) \
    ({ bool val; blackboard_get_bool(bb, key, &val) ? val : (default_val); })

#define BLACKBOARD_GET_VECTOR3(bb, key, default_x, default_y, default_z) \
    ({ f32 x, y, z; \
       if (blackboard_get_vector3(bb, key, &x, &y, &z)) { /* use x,y,z */ } \
       else { x = (default_x); y = (default_y); z = (default_z); } \
       /* x,y,z contain the result */ })

// Error checking macros
#define BLACKBOARD_CHECK_ERROR(condition, error) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("Blackboard Error: %s", blackboard_get_error_string(error)); \
            return error; \
        } \
    } while(0)

#define BLACKBOARD_CHECK_NULL_PARAM(param) \
    BLACKBOARD_CHECK_ERROR((param) != NULL, BLACKBOARD_ERROR_INVALID_PARAMETER)

// Performance macros
#define BLACKBOARD_START_TIMER(bb) \
    f64 start_time = performance_get_time_ms()

#define BLACKBOARD_END_TIMER(bb, lookup_time_ptr) \
    do { \
        f64 end_time = performance_get_time_ms(); \
        f64 duration = end_time - start_time; \
        void* _ptr = (void*)(lookup_time_ptr); \
        if (_ptr) *((f64*)_ptr) = duration; \
        bb->total_lookup_time += duration; \
        bb->total_lookups++; \
    } while(0)

#endif // AI_BLACKBOARD_H
