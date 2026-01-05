/**
 * Unit Test for Cache System
 * Tests caching functionality and cache eviction policies
 */

#include "../../../src/engine/core/cache.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test cache initialization
static TestResult test_cache_init(void) {
    Cache* cache = cache_create(10);  // 10 entries
    TEST_ASSERT_NOT_NULL(cache, "Cache created");
    TEST_ASSERT_EQ(cache_get_size(cache), 0, "Cache initially empty");
    cache_destroy(cache);
    return TEST_PASS;
}

// Test cache insert and retrieve
static TestResult test_cache_insert_get(void) {
    Cache* cache = cache_create(10);
    
    const char* key = "test_key";
    const char* value = "test_value";
    
    bool inserted = cache_insert(cache, key, (void*)value, strlen(value) + 1);
    TEST_ASSERT(inserted, "Value inserted");
    
    void* retrieved = cache_get(cache, key);
    TEST_ASSERT_NOT_NULL(retrieved, "Value retrieved");
    TEST_ASSERT_STRING_EQ((char*)retrieved, value, "Retrieved value matches");
    
    cache_destroy(cache);
    return TEST_PASS;
}

// Test cache eviction (LRU)
static TestResult test_cache_eviction(void) {
    Cache* cache = cache_create(3);  // Small cache for eviction testing
    
    cache_insert(cache, "key1", "value1", 7);
    cache_insert(cache, "key2", "value2", 7);
    cache_insert(cache, "key3", "value3", 7);
    
    TEST_ASSERT_EQ(cache_get_size(cache), 3, "Cache full");
    
    // Insert 4th item should evict oldest
    cache_insert(cache, "key4", "value4", 7);
    
    TEST_ASSERT_EQ(cache_get_size(cache), 3, "Cache still at max size");
    TEST_ASSERT_NULL(cache_get(cache, "key1"), "Oldest entry evicted");
    TEST_ASSERT_NOT_NULL(cache_get(cache, "key4"), "New entry exists");
    
    cache_destroy(cache);
    return TEST_PASS;
}

// Test cache removal
static TestResult test_cache_remove(void) {
    Cache* cache = cache_create(10);
    
    cache_insert(cache, "key1", "value1", 7);
    TEST_ASSERT_NOT_NULL(cache_get(cache, "key1"), "Entry exists");
    
    bool removed = cache_remove(cache, "key1");
    TEST_ASSERT(removed, "Entry removed");
    TEST_ASSERT_NULL(cache_get(cache, "key1"), "Entry no longer exists");
    
    cache_destroy(cache);
    return TEST_PASS;
}

// Test cache clear
static TestResult test_cache_clear(void) {
    Cache* cache = cache_create(10);
    
    cache_insert(cache, "key1", "value1", 7);
    cache_insert(cache, "key2", "value2", 7);
    cache_insert(cache, "key3", "value3", 7);
    
    TEST_ASSERT_EQ(cache_get_size(cache), 3, "3 entries");
    
    cache_clear(cache);
    TEST_ASSERT_EQ(cache_get_size(cache), 0, "Cache cleared");
    
    cache_destroy(cache);
    return TEST_PASS;
}

// Test cache hit rate
static TestResult test_cache_hit_rate(void) {
    Cache* cache = cache_create(10);
    
    cache_insert(cache, "key1", "value1", 7);
    
    // Hit
    cache_get(cache, "key1");
    
    // Miss
    cache_get(cache, "nonexistent");
    
    float hit_rate = cache_get_hit_rate(cache);
    TEST_ASSERT_FLOAT_EQ(hit_rate, 0.5f, 0.01f, "Hit rate is 50%");
    
    cache_destroy(cache);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Core:Cache", "Initialization", test_cache_init, NULL, NULL);
    test_register("Core:Cache", "Insert/Get", test_cache_insert_get, NULL, NULL);
    test_register("Core:Cache", "Eviction", test_cache_eviction, NULL, NULL);
    test_register("Core:Cache", "Remove", test_cache_remove, NULL, NULL);
    test_register("Core:Cache", "Clear", test_cache_clear, NULL, NULL);
    test_register("Core:Cache", "Hit Rate", test_cache_hit_rate, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Cache System Test Results\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Total:   %u\n", stats.total);
    printf("  Passed:  %u\n", stats.passed);
    printf("  Failed:  %u\n", stats.failed);
    printf("  Skipped: %u\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    printf("════════════════════════════════════════════════════════\n");
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}
