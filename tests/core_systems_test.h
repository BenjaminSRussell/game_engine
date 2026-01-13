/*
 * Core Systems Test Header
 * Comprehensive test coverage for core engine systems
 * Target: 80%+ code coverage
 */

#ifndef CORE_SYSTEMS_TEST_H
#define CORE_SYSTEMS_TEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Forward declarations for core systems
typedef struct {
    size_t total_heap_size;
    bool allocation_tracking;
    bool debug_mode;
    bool thread_safe;
} unified_memory_config_t;

typedef struct {
    size_t total_allocated;
    size_t peak_allocated;
    size_t allocation_count;
    size_t deallocation_count;
    size_t leak_count;
} memory_stats_t;

typedef struct {
    void* base;
    size_t block_size;
    size_t block_count;
    size_t allocated_count;
    size_t free_count;
    const char* name;
} memory_pool_t;

typedef struct {
    size_t total_allocations;
    size_t total_deallocations;
    size_t peak_usage;
    size_t current_usage;
} pool_stats_t;

typedef enum {
    MEMORY_USAGE_CPU,
    MEMORY_USAGE_GPU,
    MEMORY_USAGE_SHARED
} memory_usage_type_t;

typedef struct {
    vec3_t center;
    float radius;
} collision_sphere_t;

typedef struct {
    vec3_t center;
    vec3_t extents;
} collision_box_t;

typedef struct {
    vec3_t point;
    vec3_t normal;
    float penetration_depth;
} collision_contact_t;

typedef struct {
    vec3_t origin;
    vec3_t direction;
} ray_t;

typedef struct {
    bool hit;
    vec3_t point;
    vec3_t normal;
    float distance;
    uint32_t object_id;
} raycast_result_t;

typedef struct {
    collision_sphere_t* spheres;
    collision_box_t* boxes;
    size_t sphere_count;
    size_t box_count;
    size_t max_primitives;
} collision_world_t;

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} log_level_t;

typedef enum {
    LOG_OUTPUT_CONSOLE = 1 << 0,
    LOG_OUTPUT_FILE = 1 << 1,
    LOG_OUTPUT_NETWORK = 1 << 2
} log_output_mask_t;

typedef struct {
    log_level_t level;
    uint32_t output_mask;
    const char* file_path;
    size_t max_file_size;
    size_t max_files;
    size_t buffer_size;
    bool thread_safe;
} logging_config_t;

typedef struct {
    size_t total_messages;
    size_t debug_messages;
    size_t info_messages;
    size_t warning_messages;
    size_t error_messages;
    size_t dropped_messages;
} logging_stats_t;

// Constants
#define INVALID_COLLISION_ID ((uint32_t)-1)

// Memory system functions
bool unified_memory_init(const unified_memory_config_t* config);
void unified_memory_shutdown(void);
void* unified_memory_alloc(size_t size, memory_usage_type_t usage);
void unified_memory_free(void* ptr);
void unified_memory_get_stats(memory_stats_t* stats);
void unified_memory_get_leak_report(memory_leak_report_t* report);

// Memory pool functions
bool memory_pool_create(memory_pool_t* pool, size_t block_size, size_t block_count, const char* name);
void memory_pool_destroy(memory_pool_t* pool);
void* memory_pool_alloc(memory_pool_t* pool);
void memory_pool_free(memory_pool_t* pool, void* ptr);
void memory_pool_get_stats(const memory_pool_t* pool, pool_stats_t* stats);

// Collision system functions
collision_world_t* collision_world_create(size_t max_primitives);
void collision_world_destroy(collision_world_t* world);
uint32_t collision_add_sphere(collision_world_t* world, const vec3_t* center, float radius);
uint32_t collision_add_box(collision_world_t* world, const vec3_t* center, const vec3_t* extents);
bool collision_sphere_sphere(const collision_sphere_t* a, const collision_sphere_t* b, collision_contact_t* contact);
bool collision_sphere_box(const collision_sphere_t* sphere, const collision_box_t* box, collision_contact_t* contact);
void collision_update_broadphase(collision_world_t* world);
void collision_update_narrowphase(collision_world_t* world);
void collision_resolve_contacts(collision_world_t* world, float dt);
bool collision_ray_cast(const collision_world_t* world, const ray_t* ray, float max_distance, raycast_result_t* result);

// Logging system functions
bool unified_logging_init(const logging_config_t* config);
void unified_logging_shutdown(void);
void unified_logging_log(log_level_t level, const char* channel, const char* format, ...);
void unified_logging_get_stats(logging_stats_t* stats);

// Logging macros
#define LOG_DEBUG(msg, ...) unified_logging_log(LOG_LEVEL_DEBUG, "CORE", msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) unified_logging_log(LOG_LEVEL_INFO, "CORE", msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) unified_logging_log(LOG_LEVEL_WARNING, "CORE", msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) unified_logging_log(LOG_LEVEL_ERROR, "CORE", msg, ##__VA_ARGS__)
#define LOG_CHANNEL(channel, level, msg, ...) unified_logging_log(level, channel, msg, ##__VA_ARGS__)

// Vector math utilities
typedef struct {
    float x, y, z;
} vec3_t;

static inline vec3_t vec3_make(float x, float y, float z) {
    return (vec3_t){x, y, z};
}

static inline vec3_t vec3_add(const vec3_t* a, const vec3_t* b) {
    return vec3_make(a->x + b->x, a->y + b->y, a->z + b->z);
}

static inline vec3_t vec3_sub(const vec3_t* a, const vec3_t* b) {
    return vec3_make(a->x - b->x, a->y - b->y, a->z - b->z);
}

static inline vec3_t vec3_mul(const vec3_t* v, float s) {
    return vec3_make(v->x * s, v->y * s, v->z * s);
}

static inline float vec3_length(const vec3_t* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

static inline float vec3_distance(const vec3_t* a, const vec3_t* b) {
    vec3_t diff = vec3_sub(a, b);
    return vec3_length(&diff);
}

// Test framework structures
typedef struct {
    const char* name;
    bool (*test_func)(void);
    bool passed;
    double execution_time_ms;
} TestCase;

typedef struct {
    const char* suite_name;
    TestCase* tests;
    size_t test_count;
    size_t passed_count;
    size_t failed_count;
    double total_time_ms;
    double coverage_percentage;
} TestSuite;

// Test configuration
typedef struct {
    bool enable_memory_tests;
    bool enable_collision_tests;
    bool enable_logging_tests;
    bool enable_integration_tests;
    bool enable_performance_tests;
    bool enable_verbose_output;
    bool generate_reports;
    const char* output_directory;
    size_t timeout_seconds;
} CoreTestConfig;

// Test runner functions
int core_systems_test_run(const CoreTestConfig* config);
void core_systems_test_generate_report(const char* filename);

// Individual test functions
bool test_unified_memory_allocation(void);
bool test_memory_pool_operations(void);
bool test_unified_collision_world(void);
bool test_collision_primitives(void);
bool test_unified_logging_basic(void);
bool test_logging_performance(void);
bool test_collision_memory_integration(void);
bool test_thread_safety(void);
bool benchmark_collision_performance(void);

// Utility functions
double get_time_ms(void);
double get_time_ns(void);
void sleep_ms(unsigned int ms);

#endif // CORE_SYSTEMS_TEST_H
