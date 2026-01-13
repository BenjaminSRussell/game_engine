#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <core/containers/dynamic_array.h>
#include <core/containers/hashmap.h>
#include <core/logger.h>

#define STRESS_COUNT_ARRAY 1000000
#define STRESS_COUNT_MAP 100000

bool run_dynamic_array_stress_test(void) {
    printf("  Testing DynamicArray with %d elements...\n", STRESS_COUNT_ARRAY);
    clock_t start = clock();

    DynamicArray *arr = dynamic_array_create(sizeof(u32), 16);
    if (!arr) {
        printf("    Failed to create array\n");
        return false;
    }

    // Push
    for (u32 i = 0; i < STRESS_COUNT_ARRAY; i++) {
        dynamic_array_push(arr, &i);
    }

    if (arr->count != STRESS_COUNT_ARRAY) {
        printf("    Count mismatch: expected %d, got %d\n", STRESS_COUNT_ARRAY, arr->count);
        return false;
    }

    // Access and Verify
    for (u32 i = 0; i < STRESS_COUNT_ARRAY; i += 1000) { // Check every 1000th to save time
        u32 *val = (u32*)dynamic_array_get(arr, i);
        if (!val || *val != i) {
             printf("    Value mismatch at %d\n", i);
             return false;
        }
    }

    // Pop half
    u32 target_count = STRESS_COUNT_ARRAY / 2;
    for (u32 i = 0; i < target_count; i++) {
        dynamic_array_pop(arr);
    }

    if (arr->count != STRESS_COUNT_ARRAY - target_count) {
        printf("    Count mismatch after pop: expected %d, got %d\n", STRESS_COUNT_ARRAY - target_count, arr->count);
        return false;
    }

    dynamic_array_destroy(arr);

    clock_t end = clock();
    double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("    DynamicArray test passed in %.2f ms\n", time_ms);
    return true;
}

bool run_hashmap_stress_test(void) {
    printf("  Testing HashMap with %d elements...\n", STRESS_COUNT_MAP);
    clock_t start = clock();

    // Use string keys "key_0", "key_1", ...
    // Key is char*, Value is u32
    HashMap *map = hashmap_create(1024, sizeof(char*), sizeof(u32), hash_string, equals_string);
    if (!map) {
        printf("    Failed to create map\n");
        return false;
    }

    char **keys = malloc(sizeof(char*) * STRESS_COUNT_MAP);
    if (!keys) return false;

    // Insert
    for (u32 i = 0; i < STRESS_COUNT_MAP; i++) {
        keys[i] = malloc(32);
        sprintf(keys[i], "key_%d", i);
        // hashmap_insert_string wraps insertion passing &key
        hashmap_insert_string(map, keys[i], &i);
    }

    if (hashmap_size(map) != STRESS_COUNT_MAP) {
        printf("    Map size mismatch: expected %d, got %d\n", STRESS_COUNT_MAP, hashmap_size(map));
        return false;
    }

    // Lookup
    for (u32 i = 0; i < STRESS_COUNT_MAP; i += 100) {
        u32 *val = (u32*)hashmap_get_string(map, keys[i]);
        if (!val || *val != i) {
            printf("    Map lookup failed for %s\n", keys[i]);
            return false;
        }
    }

    // Remove half
    u32 target_count = STRESS_COUNT_MAP / 2;
    for (u32 i = 0; i < target_count; i++) {
        // hashmap_remove takes const void *key.
        // For our string map using hash_string/equals_string which cast to char**, we must pass char**
        char *k = keys[i];
        hashmap_remove(map, &k);
    }

    if (hashmap_size(map) != STRESS_COUNT_MAP - target_count) {
        printf("    Map size mismatch after remove: expected %d, got %d\n", STRESS_COUNT_MAP - target_count, hashmap_size(map));
        return false;
    }

    // Cleanup keys
    for (u32 i = 0; i < STRESS_COUNT_MAP; i++) {
        free(keys[i]);
    }
    free(keys);

    hashmap_destroy(map);

    clock_t end = clock();
    double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("    HashMap test passed in %.2f ms\n", time_ms);
    return true;
}

int main(void) {
    printf("=== Data Structure Stress Tests ===\n");

    // Initialize logger with config
    LoggerConfig config = {0};
    config.min_level = LOG_LEVEL_INFO;
    config.enabled_channels = LOG_CHANNEL_CONSOLE;
    config.show_timestamp = false; // Clean output
    config.use_colors = false;
    for (int i = 0; i < LOG_CAT_COUNT; i++) config.enabled_categories[i] = true;

    logger_init(&config);

    bool passed = true;
    passed &= run_dynamic_array_stress_test();
    passed &= run_hashmap_stress_test();

    logger_shutdown();

    if (passed) {
        printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("TESTS FAILED\n");
        return 1;
    }
}
