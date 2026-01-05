/**
 * Unit Test for Hashmap Container
 * Tests hashmap insertion, retrieval, collision handling, and resizing
 */

#include "../../../src/engine/core/hashmap.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test hashmap initialization
static TestResult test_hashmap_init(void) {
    HashMap* map = hashmap_create(16);
    TEST_ASSERT_NOT_NULL(map, "Hashmap created");
    TEST_ASSERT_EQ(hashmap_size(map), 0, "HashMap initially empty");
    hashmap_destroy(map);
    return TEST_PASS;
}

// Test hashmap insert and get
static TestResult test_hashmap_insert_get(void) {
    HashMap* map = hashmap_create(16);
    
    int value = 42;
    hashmap_insert(map, "key1", &value);
    
    int* retrieved = (int*)hashmap_get(map, "key1");
    TEST_ASSERT_NOT_NULL(retrieved, "Value retrieved");
    TEST_ASSERT_EQ(*retrieved, 42, "Value matches");
    
    hashmap_destroy(map);
    return TEST_PASS;
}

// Test hashmap updates
static TestResult test_hashmap_update(void) {
    HashMap* map = hashmap_create(16);
    
    int value1 = 100;
    int value2 = 200;
    
    hashmap_insert(map, "key", &value1);
    hashmap_insert(map, "key", &value2);  // Update
    
    int* retrieved = (int*)hashmap_get(map, "key");
    TEST_ASSERT_EQ(*retrieved, 200, "Value updated");
    
    hashmap_destroy(map);
    return TEST_PASS;
}

// Test hashmap removal
static TestResult test_hashmap_remove(void) {
    HashMap* map = hashmap_create(16);
    
    int value = 42;
    hashmap_insert(map, "key1", &value);
    
    TEST_ASSERT(hashmap_contains(map, "key1"), "Key exists");
    
    bool removed = hashmap_remove(map, "key1");
    TEST_ASSERT(removed, "Key removed");
    TEST_ASSERT(!hashmap_contains(map, "key1"), "Key no longer exists");
    
    hashmap_destroy(map);
    return TEST_PASS;
}

// Test hashmap collision handling
static TestResult test_hashmap_collisions(void) {
    HashMap* map = hashmap_create(4);  // Small map to force collisions
    
    int val1 = 1, val2 = 2, val3 = 3, val4 = 4, val5 = 5;
    
    hashmap_insert(map, "key1", &val1);
    hashmap_insert(map, "key2", &val2);
    hashmap_insert(map, "key3", &val3);
    hashmap_insert(map, "key4", &val4);
    hashmap_insert(map, "key5", &val5);
    
    // All values should still be retrievable despite collisions
    TEST_ASSERT_EQ(*(int*)hashmap_get(map, "key1"), 1, "Key1 retrieved");
    TEST_ASSERT_EQ(*(int*)hashmap_get(map, "key2"), 2, "Key2 retrieved");
    TEST_ASSERT_EQ(*(int*)hashmap_get(map, "key3"), 3, "Key3 retrieved");
    TEST_ASSERT_EQ(*(int*)hashmap_get(map, "key4"), 4, "Key4 retrieved");
    TEST_ASSERT_EQ(*(int*)hashmap_get(map, "key5"), 5, "Key5 retrieved");
    
    hashmap_destroy(map);
    return TEST_PASS;
}

// Test hashmap iteration
static TestResult test_hashmap_iteration(void) {
    HashMap* map = hashmap_create(16);
    
    int val1 = 1, val2 = 2, val3 = 3;
    hashmap_insert(map, "a", &val1);
    hashmap_insert(map, "b", &val2);
    hashmap_insert(map, "c", &val3);
    
    HashMapIterator it = hashmap_iterator(map);
    int count = 0;
    
    while (hashmap_iterator_has_next(&it)) {
        HashMapEntry entry = hashmap_iterator_next(&it);
        TEST_ASSERT_NOT_NULL(entry.key, "Entry has key");
        TEST_ASSERT_NOT_NULL(entry.value, "Entry has value");
        count++;
    }
    
    TEST_ASSERT_EQ(count, 3, "Iterated through all entries");
    
    hashmap_destroy(map);
    return TEST_PASS;
}

// Test hashmap clear
static TestResult test_hashmap_clear(void) {
    HashMap* map = hashmap_create(16);
    
    int val1 = 1, val2 = 2, val3 = 3;
    hashmap_insert(map, "a", &val1);
    hashmap_insert(map, "b", &val2);
    hashmap_insert(map, "c", &val3);
    
    TEST_ASSERT_EQ(hashmap_size(map), 3, "Size is 3");
    
    hashmap_clear(map);
    TEST_ASSERT_EQ(hashmap_size(map), 0, "Map cleared");
    
    hashmap_destroy(map);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Core:HashMap", "Initialization", test_hashmap_init, NULL, NULL);
    test_register("Core:HashMap", "Insert/Get", test_hashmap_insert_get, NULL, NULL);
    test_register("Core:HashMap", "Update", test_hashmap_update, NULL, NULL);
    test_register("Core:HashMap", "Remove", test_hashmap_remove, NULL, NULL);
    test_register("Core:HashMap", "Collisions", test_hashmap_collisions, NULL, NULL);
    test_register("Core:HashMap", "Iteration", test_hashmap_iteration, NULL, NULL);
    test_register("Core:HashMap", "Clear", test_hashmap_clear, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  HashMap Test Results\n");
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
