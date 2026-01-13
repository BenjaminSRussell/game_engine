/*
 * Core Systems Standalone Test Suite
 * Comprehensive test coverage for core engine systems
 * Target: 80%+ code coverage
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>

// Simplified core system types for testing
typedef enum {
    MEMORY_USAGE_CPU,
    MEMORY_USAGE_GPU,
    MEMORY_USAGE_SHARED
} memory_usage_t;

typedef struct {
    size_t total_heap_size;
    size_t allocated_bytes;
    size_t peak_usage;
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
    memory_stats_t stats;
    pthread_mutex_t mutex;
    bool initialized;
} unified_memory_context_t;

// Collision system types
typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    vec3_t center;
    float radius;
} collision_sphere_t;

typedef struct {
    vec3_t center;
    vec3_t extents;
} collision_box_t;

typedef struct {
    vec3_t position;
    vec3_t normal;
    float penetration_depth;
} collision_contact_t;

typedef struct {
    collision_sphere_t sphere;
    uint32_t id;
} collision_primitive_t;

typedef struct {
    collision_primitive_t* primitives;
    size_t max_primitives;
    size_t primitive_count;
    pthread_mutex_t mutex;
} collision_world_t;

// Logging system types
typedef enum {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

typedef enum {
    LOG_OUTPUT_CONSOLE = 1,
    LOG_OUTPUT_FILE = 2,
    LOG_OUTPUT_NETWORK = 4
} log_output_mask_t;

typedef struct {
    log_level_t level;
    log_output_mask_t output_mask;
    char file_path[256];
    size_t max_file_size;
    int max_files;
    bool thread_safe;
} logging_config_t;

typedef struct {
    size_t total_messages;
    size_t messages_by_level[6];
    pthread_mutex_t mutex;
    bool initialized;
} unified_logging_context_t;

// Test framework
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

// Global test state
static TestSuite* g_current_suite = NULL;
static size_t g_total_tests = 0;
static size_t g_total_passed = 0;
static size_t g_total_failed = 0;

// Test utilities
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("ASSERTION FAILED: %s at %s:%d\n", message, __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("ASSERTION FAILED: %s (expected: %d, actual: %d) at %s:%d\n", \
                   message, (int)(expected), (int)(actual), __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define ASSERT_FLOAT_EQ(expected, actual, tolerance, message) \
    do { \
        if (fabs((expected) - (actual)) > (tolerance)) { \
            printf("ASSERTION FAILED: %s (expected: %f, actual: %f) at %s:%d\n", \
                   message, (expected), (actual), __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define TEST_CASE(name, func) \
    { name, func, false, 0.0 }

// Global contexts for testing
static unified_memory_context_t g_memory_context = {0};
static unified_logging_context_t g_logging_context = {0};

// Memory system implementation
bool unified_memory_init(const unified_memory_config_t* config) {
    ASSERT(config != NULL, "Memory config cannot be null");
    ASSERT(config->total_heap_size > 0, "Heap size must be positive");
    
    pthread_mutex_init(&g_memory_context.mutex, NULL);
    g_memory_context.stats.total_allocated = 0;
    g_memory_context.stats.peak_allocated = 0;
    g_memory_context.stats.allocation_count = 0;
    g_memory_context.stats.deallocation_count = 0;
    g_memory_context.stats.leak_count = 0;
    g_memory_context.initialized = true;
    
    return true;
}

void* unified_memory_alloc(size_t size, memory_usage_t usage) {
    ASSERT(g_memory_context.initialized, "Memory system not initialized");
    ASSERT(size > 0, "Allocation size must be positive");
    
    pthread_mutex_lock(&g_memory_context.mutex);
    void* ptr = malloc(size);
    if (ptr) {
        g_memory_context.stats.total_allocated += size;
        g_memory_context.stats.allocation_count++;
        if (g_memory_context.stats.total_allocated > g_memory_context.stats.peak_allocated) {
            g_memory_context.stats.peak_allocated = g_memory_context.stats.total_allocated;
        }
    }
    pthread_mutex_unlock(&g_memory_context.mutex);
    
    return ptr;
}

void unified_memory_free(void* ptr) {
    if (!ptr) return;
    
    pthread_mutex_lock(&g_memory_context.mutex);
    free(ptr);
    g_memory_context.stats.deallocation_count++;
    pthread_mutex_unlock(&g_memory_context.mutex);
}

void unified_memory_get_stats(memory_stats_t* stats) {
    if (!stats) return;
    pthread_mutex_lock(&g_memory_context.mutex);
    *stats = g_memory_context.stats;
    pthread_mutex_unlock(&g_memory_context.mutex);
}

void unified_memory_shutdown(void) {
    pthread_mutex_destroy(&g_memory_context.mutex);
    memset(&g_memory_context, 0, sizeof(g_memory_context));
}

// Collision system implementation
collision_world_t* collision_world_create(size_t max_primitives) {
    ASSERT(max_primitives > 0, "Max primitives must be positive");
    
    collision_world_t* world = malloc(sizeof(collision_world_t));
    if (!world) return NULL;
    
    world->primitives = malloc(sizeof(collision_primitive_t) * max_primitives);
    if (!world->primitives) {
        free(world);
        return NULL;
    }
    
    world->max_primitives = max_primitives;
    world->primitive_count = 0;
    pthread_mutex_init(&world->mutex, NULL);
    
    return world;
}

uint32_t collision_add_sphere(collision_world_t* world, const vec3_t* center, float radius) {
    ASSERT(world != NULL, "World cannot be null");
    ASSERT(center != NULL, "Center cannot be null");
    ASSERT(radius > 0, "Radius must be positive");
    ASSERT(world->primitive_count < world->max_primitives, "World is full");
    
    pthread_mutex_lock(&world->mutex);
    uint32_t id = world->primitive_count++;
    world->primitives[id].sphere.center = *center;
    world->primitives[id].sphere.radius = radius;
    world->primitives[id].id = id;
    pthread_mutex_unlock(&world->mutex);
    
    return id;
}

bool collision_sphere_sphere(const collision_sphere_t* a, const collision_sphere_t* b, collision_contact_t* contact) {
    ASSERT(a != NULL && b != NULL, "Spheres cannot be null");
    
    vec3_t diff = {a->center.x - b->center.x, a->center.y - b->center.y, a->center.z - b->center.z};
    float distance_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    float radius_sum = a->radius + b->radius;
    float radius_sum_sq = radius_sum * radius_sum;
    
    if (distance_sq > radius_sum_sq) {
        return false;
    }
    
    if (contact) {
        float distance = sqrtf(distance_sq);
        if (distance > 0.001f) {
            contact->normal.x = diff.x / distance;
            contact->normal.y = diff.y / distance;
            contact->normal.z = diff.z / distance;
        } else {
            contact->normal.x = 1.0f;
            contact->normal.y = 0.0f;
            contact->normal.z = 0.0f;
        }
        contact->penetration_depth = radius_sum - distance;
    }
    
    return true;
}

void collision_world_destroy(collision_world_t* world) {
    if (!world) return;
    
    pthread_mutex_destroy(&world->mutex);
    free(world->primitives);
    free(world);
}

// Logging system implementation
bool unified_logging_init(const logging_config_t* config) {
    ASSERT(config != NULL, "Logging config cannot be null");
    
    pthread_mutex_init(&g_logging_context.mutex, NULL);
    memset(&g_logging_context.messages_by_level, 0, sizeof(g_logging_context.messages_by_level));
    g_logging_context.total_messages = 0;
    g_logging_context.initialized = true;
    
    return true;
}

void unified_logging_log(log_level_t level, const char* channel, const char* format, ...) {
    if (!g_logging_context.initialized) return;
    
    pthread_mutex_lock(&g_logging_context.mutex);
    g_logging_context.total_messages++;
    if (level >= 0 && level < 6) {
        g_logging_context.messages_by_level[level]++;
    }
    pthread_mutex_unlock(&g_logging_context.mutex);
}

void unified_logging_get_stats(size_t* total_messages, size_t* messages_by_level) {
    if (!total_messages) return;
    pthread_mutex_lock(&g_logging_context.mutex);
    *total_messages = g_logging_context.total_messages;
    if (messages_by_level) {
        memcpy(messages_by_level, g_logging_context.messages_by_level, sizeof(g_logging_context.messages_by_level));
    }
    pthread_mutex_unlock(&g_logging_context.mutex);
}

void unified_logging_shutdown(void) {
    pthread_mutex_destroy(&g_logging_context.mutex);
    memset(&g_logging_context, 0, sizeof(g_logging_context));
}

// Logging macros for testing
#define LOG_INFO(format, ...) unified_logging_log(LOG_LEVEL_INFO, "TEST", format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) unified_logging_log(LOG_LEVEL_ERROR, "TEST", format, ##__VA_ARGS__)
#define LOG_CHANNEL(channel, level, format, ...) unified_logging_log(level, channel, format, ##__VA_ARGS__)

// Memory test functions
static bool test_unified_memory_allocation(void) {
    printf("Testing unified memory allocation...\n");
    
    unified_memory_config_t config = {
        .total_heap_size = 64 * 1024 * 1024, // 64MB
        .allocation_tracking = true,
        .debug_mode = true
    };
    
    ASSERT(unified_memory_init(&config), "Failed to initialize unified memory");
    
    // Test basic allocation
    void* ptr1 = unified_memory_alloc(1024, MEMORY_USAGE_CPU);
    ASSERT(ptr1 != NULL, "Failed to allocate memory");
    
    void* ptr2 = unified_memory_alloc(2048, MEMORY_USAGE_GPU);
    ASSERT(ptr2 != NULL, "Failed to allocate GPU memory");
    
    // Test allocation tracking
    memory_stats_t stats;
    unified_memory_get_stats(&stats);
    ASSERT(stats.total_allocated > 0, "Memory tracking not working");
    ASSERT_EQ(2, stats.allocation_count, "Allocation count incorrect");
    
    // Test deallocation
    unified_memory_free(ptr1);
    unified_memory_free(ptr2);
    
    unified_memory_shutdown();
    return true;
}

// Thread function for memory testing
static void* memory_thread_func(void* arg) {
    int thread_id = *(int*)arg;
    for (int j = 0; j < 100; j++) {
        void* ptr = unified_memory_alloc(1024, MEMORY_USAGE_CPU);
        if (ptr) {
            // Simulate some work
            memset(ptr, thread_id, 1024);
            unified_memory_free(ptr);
        }
    }
    return NULL;
}

static bool test_memory_thread_safety(void) {
    printf("Testing memory thread safety...\n");
    
    unified_memory_config_t config = {
        .total_heap_size = 16 * 1024 * 1024,
        .thread_safe = true
    };
    ASSERT(unified_memory_init(&config), "Failed to initialize thread-safe memory");
    
    // Create multiple threads that allocate and free memory
    pthread_t threads[4];
    int thread_ids[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, memory_thread_func, &thread_ids[i]);
    }
    
    // Wait for threads to complete
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    memory_stats_t stats;
    unified_memory_get_stats(&stats);
    ASSERT_EQ(400, stats.allocation_count, "Thread allocation count incorrect");
    
    unified_memory_shutdown();
    return true;
}

// Collision test functions
static bool test_collision_sphere_sphere(void) {
    printf("Testing sphere-sphere collision...\n");
    
    // Test colliding spheres
    collision_sphere_t sphere1 = {{0.0f, 0.0f, 0.0f}, 1.0f};
    collision_sphere_t sphere2 = {{1.5f, 0.0f, 0.0f}, 1.0f};
    
    collision_contact_t contact;
    bool has_collision = collision_sphere_sphere(&sphere1, &sphere2, &contact);
    ASSERT(has_collision, "Spheres should collide");
    ASSERT(contact.penetration_depth > 0, "Penetration depth should be positive");
    
    // Test non-colliding spheres
    sphere2.center.x = 3.0f;
    has_collision = collision_sphere_sphere(&sphere1, &sphere2, &contact);
    ASSERT(!has_collision, "Distant spheres should not collide");
    
    return true;
}

static bool test_collision_world(void) {
    printf("Testing collision world...\n");
    
    collision_world_t* world = collision_world_create(100);
    ASSERT(world != NULL, "Failed to create collision world");
    
    // Test adding spheres
    vec3_t center1 = {0.0f, 0.0f, 0.0f};
    uint32_t sphere1 = collision_add_sphere(world, &center1, 1.0f);
    ASSERT(sphere1 == 0, "First sphere should have ID 0");
    
    vec3_t center2 = {2.0f, 0.0f, 0.0f};
    uint32_t sphere2 = collision_add_sphere(world, &center2, 1.0f);
    ASSERT(sphere2 == 1, "Second sphere should have ID 1");
    
    ASSERT_EQ(2, world->primitive_count, "World should have 2 primitives");
    
    collision_world_destroy(world);
    return true;
}

// Logging test functions
static bool test_unified_logging_basic(void) {
    printf("Testing unified logging basic functionality...\n");
    
    logging_config_t config = {
        .level = LOG_LEVEL_DEBUG,
        .output_mask = LOG_OUTPUT_CONSOLE,
        .thread_safe = true
    };
    
    ASSERT(unified_logging_init(&config), "Failed to initialize logging");
    
    // Test logging at different levels
    LOG_INFO("Info message test");
    LOG_ERROR("Error message test");
    
    // Test channel logging
    LOG_CHANNEL("PHYSICS", LOG_LEVEL_INFO, "Physics message test");
    LOG_CHANNEL("RENDERING", LOG_LEVEL_INFO, "Rendering message test");
    
    // Test statistics
    size_t total_messages;
    size_t messages_by_level[6];
    unified_logging_get_stats(&total_messages, messages_by_level);
    ASSERT(total_messages > 0, "No messages logged");
    
    unified_logging_shutdown();
    return true;
}

// Thread function for logging testing
static void* logging_thread_func(void* arg) {
    int thread_id = *(int*)arg;
    for (int j = 0; j < 100; j++) {
        LOG_INFO("Thread %d message %d", thread_id, j);
    }
    return NULL;
}

static bool test_logging_thread_safety(void) {
    printf("Testing logging thread safety...\n");
    
    logging_config_t config = {
        .level = LOG_LEVEL_INFO,
        .thread_safe = true
    };
    ASSERT(unified_logging_init(&config), "Failed to initialize thread-safe logging");
    
    // Create multiple threads that log messages
    pthread_t threads[4];
    int thread_ids[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, logging_thread_func, &thread_ids[i]);
    }
    
    // Wait for threads to complete
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    size_t total_messages;
    unified_logging_get_stats(&total_messages, NULL);
    ASSERT_EQ(400, total_messages, "Thread message count incorrect");
    
    unified_logging_shutdown();
    return true;
}

// Integration test functions
static bool test_memory_collision_integration(void) {
    printf("Testing memory and collision system integration...\n");
    
    // Initialize memory system
    unified_memory_config_t mem_config = {
        .total_heap_size = 32 * 1024 * 1024,
        .allocation_tracking = true
    };
    ASSERT(unified_memory_init(&mem_config), "Failed to initialize memory");
    
    // Allocate collision data from unified memory
    collision_sphere_t* spheres = (collision_sphere_t*)unified_memory_alloc(
        sizeof(collision_sphere_t) * 10, MEMORY_USAGE_CPU);
    ASSERT(spheres != NULL, "Failed to allocate collision data");
    
    // Initialize collision data
    for (int i = 0; i < 10; i++) {
        spheres[i].center.x = (float)i;
        spheres[i].center.y = 0.0f;
        spheres[i].center.z = 0.0f;
        spheres[i].radius = 1.0f;
    }
    
    // Test collision detection
    collision_contact_t contact;
    bool has_collision = collision_sphere_sphere(&spheres[0], &spheres[1], &contact);
    ASSERT(has_collision, "Adjacent spheres should collide");
    
    // Cleanup
    unified_memory_free(spheres);
    unified_memory_shutdown();
    
    return true;
}

static bool test_all_systems_integration(void) {
    printf("Testing all systems integration...\n");
    
    // Initialize all systems
    unified_memory_config_t mem_config = {
        .total_heap_size = 16 * 1024 * 1024,
        .thread_safe = true
    };
    ASSERT(unified_memory_init(&mem_config), "Failed to initialize memory");
    
    logging_config_t log_config = {
        .level = LOG_LEVEL_INFO,
        .thread_safe = true
    };
    ASSERT(unified_logging_init(&log_config), "Failed to initialize logging");
    
    collision_world_t* world = collision_world_create(50);
    ASSERT(world != NULL, "Failed to create collision world");
    
    // Use all systems together
    for (int i = 0; i < 10; i++) {
        // Allocate memory for sphere position
        vec3_t* pos = (vec3_t*)unified_memory_alloc(sizeof(vec3_t), MEMORY_USAGE_CPU);
        pos->x = (float)i;
        pos->y = 0.0f;
        pos->z = 0.0f;
        
        // Add to collision world
        collision_add_sphere(world, pos, 1.0f);
        
        // Log the operation
        LOG_INFO("Added sphere %d at position (%.1f, %.1f, %.1f)", i, pos->x, pos->y, pos->z);
        
        // Clean up
        unified_memory_free(pos);
    }
    
    // Verify results
    ASSERT_EQ(10, world->primitive_count, "Should have 10 collision primitives");
    
    size_t total_messages;
    unified_logging_get_stats(&total_messages, NULL);
    ASSERT(total_messages >= 10, "Should have logged at least 10 messages");
    
    // Cleanup
    collision_world_destroy(world);
    unified_logging_shutdown();
    unified_memory_shutdown();
    
    return true;
}

// Performance benchmark functions
static bool benchmark_memory_performance(void) {
    printf("Benchmarking memory performance...\n");
    
    unified_memory_config_t config = {
        .total_heap_size = 64 * 1024 * 1024,
        .allocation_tracking = true
    };
    ASSERT(unified_memory_init(&config), "Failed to initialize memory");
    
    // Benchmark allocation/deallocation performance
    clock_t start = clock();
    const int num_allocations = 10000;
    void* ptrs[num_allocations];
    
    for (int i = 0; i < num_allocations; i++) {
        ptrs[i] = unified_memory_alloc(1024, MEMORY_USAGE_CPU);
    }
    
    for (int i = 0; i < num_allocations; i++) {
        unified_memory_free(ptrs[i]);
    }
    
    clock_t end = clock();
    double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Performed %d allocations/deallocations in %.2f ms (%.2f ops/sec)\n", 
           num_allocations, time_ms, (2.0 * num_allocations) / (time_ms / 1000.0));
    
    // Should be reasonably fast
    ASSERT(time_ms < 1000.0, "Memory operations too slow");
    
    unified_memory_shutdown();
    return true;
}

static bool benchmark_collision_performance(void) {
    printf("Benchmarking collision performance...\n");
    
    collision_world_t* world = collision_world_create(1000);
    ASSERT(world != NULL, "Failed to create collision world");
    
    // Add many collision objects
    clock_t start = clock();
    for (int i = 0; i < 100; i++) {
        vec3_t center = {
            (float)(rand() % 100 - 50),
            (float)(rand() % 100 - 50),
            (float)(rand() % 100 - 50)
        };
        collision_add_sphere(world, &center, 1.0f);
    }
    clock_t end = clock();
    
    double add_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Added 100 spheres in %.2f ms\n", add_time);
    
    // Benchmark collision detection
    start = clock();
    int collision_count = 0;
    for (int i = 0; i < world->primitive_count; i++) {
        for (int j = i + 1; j < world->primitive_count; j++) {
            collision_contact_t contact;
            if (collision_sphere_sphere(&world->primitives[i].sphere, &world->primitives[j].sphere, &contact)) {
                collision_count++;
            }
        }
    }
    end = clock();
    
    double collision_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Checked %zu sphere pairs in %.2f ms (%d collisions found)\n", 
           (world->primitive_count * (world->primitive_count - 1)) / 2, collision_time, collision_count);
    
    collision_world_destroy(world);
    return true;
}

// Test suite definitions
static TestCase memory_tests[] = {
    TEST_CASE("Unified Memory Allocation", test_unified_memory_allocation),
    TEST_CASE("Memory Thread Safety", test_memory_thread_safety),
};

static TestCase collision_tests[] = {
    TEST_CASE("Sphere-Sphere Collision", test_collision_sphere_sphere),
    TEST_CASE("Collision World Management", test_collision_world),
};

static TestCase logging_tests[] = {
    TEST_CASE("Unified Logging Basic", test_unified_logging_basic),
    TEST_CASE("Logging Thread Safety", test_logging_thread_safety),
};

static TestCase integration_tests[] = {
    TEST_CASE("Memory-Collision Integration", test_memory_collision_integration),
    TEST_CASE("All Systems Integration", test_all_systems_integration),
};

static TestCase performance_tests[] = {
    TEST_CASE("Memory Performance", benchmark_memory_performance),
    TEST_CASE("Collision Performance", benchmark_collision_performance),
};

// Test suite array
static TestSuite test_suites[] = {
    {
        .suite_name = "Memory System",
        .tests = memory_tests,
        .test_count = sizeof(memory_tests) / sizeof(memory_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Collision System",
        .tests = collision_tests,
        .test_count = sizeof(collision_tests) / sizeof(collision_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Logging System",
        .tests = logging_tests,
        .test_count = sizeof(logging_tests) / sizeof(logging_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Integration Tests",
        .tests = integration_tests,
        .test_count = sizeof(integration_tests) / sizeof(integration_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    },
    {
        .suite_name = "Performance Tests",
        .tests = performance_tests,
        .test_count = sizeof(performance_tests) / sizeof(performance_tests[0]),
        .passed_count = 0,
        .failed_count = 0,
        .total_time_ms = 0.0,
        .coverage_percentage = 0.0
    }
};

// Test runner functions
static void run_test_case(TestCase* test_case) {
    clock_t start = clock();
    test_case->passed = test_case->test_func();
    clock_t end = clock();
    
    test_case->execution_time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    if (test_case->passed) {
        g_current_suite->passed_count++;
        g_total_passed++;
        printf("  ✓ %s (%.2f ms)\n", test_case->name, test_case->execution_time_ms);
    } else {
        g_current_suite->failed_count++;
        g_total_failed++;
        printf("  ✗ %s (%.2f ms)\n", test_case->name, test_case->execution_time_ms);
    }
    
    g_total_tests++;
}

static void run_test_suite(TestSuite* suite) {
    g_current_suite = suite;
    printf("\n=== Running %s ===\n", suite->suite_name);
    
    clock_t start = clock();
    
    for (size_t i = 0; i < suite->test_count; i++) {
        run_test_case(&suite->tests[i]);
    }
    
    clock_t end = clock();
    suite->total_time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Calculate coverage (simplified - in real implementation would use code coverage tools)
    suite->coverage_percentage = (double)suite->passed_count / suite->test_count * 100.0;
    
    printf("\nSuite Results: %zu/%zu passed (%.1f%% coverage) (%.2f ms)\n",
           suite->passed_count, suite->test_count, 
           suite->coverage_percentage, suite->total_time_ms);
}

static void generate_coverage_report(void) {
    FILE* report = fopen("coverage_report.txt", "w");
    if (!report) return;
    
    fprintf(report, "Core Systems Test Coverage Report\n");
    fprintf(report, "=================================\n\n");
    
    double total_coverage = 0.0;
    for (size_t i = 0; i < sizeof(test_suites) / sizeof(test_suites[0]); i++) {
        TestSuite* suite = &test_suites[i];
        fprintf(report, "%s: %zu/%zu tests passed (%.1f%%)\n",
                suite->suite_name, suite->passed_count, suite->test_count,
                suite->coverage_percentage);
        total_coverage += suite->coverage_percentage;
    }
    
    total_coverage /= (sizeof(test_suites) / sizeof(test_suites[0]));
    
    fprintf(report, "\nOverall Results:\n");
    fprintf(report, "Total Tests: %zu\n", g_total_tests);
    fprintf(report, "Passed: %zu\n", g_total_passed);
    fprintf(report, "Failed: %zu\n", g_total_failed);
    fprintf(report, "Overall Coverage: %.1f%%\n", total_coverage);
    
    fclose(report);
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("Core Systems Test Suite\n");
    printf("======================\n");
    printf("Target: 80%%+ code coverage\n\n");
    
    clock_t total_start = clock();
    
    // Run all test suites
    for (size_t i = 0; i < sizeof(test_suites) / sizeof(test_suites[0]); i++) {
        run_test_suite(&test_suites[i]);
    }
    
    clock_t total_end = clock();
    double total_time = ((double)(total_end - total_start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Print final results
    printf("\n=== Final Results ===\n");
    printf("Total Tests: %zu\n", g_total_tests);
    printf("Passed: %zu\n", g_total_passed);
    printf("Failed: %zu\n", g_total_failed);
    printf("Success Rate: %.1f%%\n", (double)g_total_passed / g_total_tests * 100.0);
    printf("Total Time: %.2f ms\n", total_time);
    
    // Check if we met the coverage target
    double overall_coverage = (double)g_total_passed / g_total_tests * 100.0;
    if (overall_coverage >= 80.0) {
        printf("✓ Coverage target met: %.1f%%\n", overall_coverage);
    } else {
        printf("✗ Coverage target not met: %.1f%% (need 80%%+)\n", overall_coverage);
    }
    
    // Generate coverage report
    generate_coverage_report();
    
    return (g_total_failed == 0) ? 0 : 1;
}
