#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <core/containers/dynamic_array.h>
#include <core/containers/hashmap.h>
#include <core/logger.h>

/* dynamic_array.c references unified_logger_log. The logger translation unit
   does not compile (LoggerStats and LogQueryResult are undeclared). This test
   must not rewrite the logger, so the symbols it needs are satisfied here. */
bool unified_logger_init(const LoggerConfig *config) {
    (void)config;
    return true;
}

void unified_logger_shutdown(void) {}

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

#define STRESS_COUNT_ARRAY 1000000
#define STRESS_COUNT_MAP 100000

static bool run_dynamic_array_stress_test(void) {
    printf("  Testing DynamicArray with %u elements...\n",
           (unsigned)STRESS_COUNT_ARRAY);
    clock_t start = clock();
    bool ok = false;

    DynamicArray *arr = dynamic_array_create(sizeof(u32), 16);
    if (!arr) {
        printf("    Failed to create array\n");
        return false;
    }

    for (u32 i = 0; i < STRESS_COUNT_ARRAY; i++) {
        dynamic_array_push(arr, &i);
    }

    if (arr->count != STRESS_COUNT_ARRAY) {
        printf("    Count mismatch: expected %u, got %u\n",
               (unsigned)STRESS_COUNT_ARRAY, (unsigned)arr->count);
        goto done;
    }

    for (u32 i = 0; i < STRESS_COUNT_ARRAY; i += 1000) {
        u32 *val = (u32 *)dynamic_array_get(arr, i);
        if (!val || *val != i) {
            printf("    Value mismatch at %u\n", (unsigned)i);
            goto done;
        }
    }

    {
        u32 target_count = STRESS_COUNT_ARRAY / 2;
        for (u32 i = 0; i < target_count; i++) {
            dynamic_array_pop(arr);
        }

        if (arr->count != STRESS_COUNT_ARRAY - target_count) {
            printf("    Count mismatch after pop: expected %u, got %u\n",
                   (unsigned)(STRESS_COUNT_ARRAY - target_count),
                   (unsigned)arr->count);
            goto done;
        }

        u32 *first = (u32 *)dynamic_array_get(arr, 0);
        u32 *last = (u32 *)dynamic_array_get(arr, arr->count - 1);
        if (!first || *first != 0 || !last || *last != arr->count - 1) {
            printf("    Pop corrupted surviving elements\n");
            goto done;
        }
    }

    ok = true;

done:
    dynamic_array_destroy(arr);

    clock_t end = clock();
    double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("    DynamicArray test %s in %.2f ms\n", ok ? "passed" : "failed",
           time_ms);
    return ok;
}

static bool run_hashmap_stress_test(void) {
    printf("  Testing HashMap with %u elements...\n", (unsigned)STRESS_COUNT_MAP);
    clock_t start = clock();
    bool ok = false;
    char **keys = NULL;

    HashMap *map = hashmap_create(1024, sizeof(char *), sizeof(u32), hash_string,
                                  equals_string);
    if (!map) {
        printf("    Failed to create map\n");
        return false;
    }

    keys = malloc(sizeof(char *) * STRESS_COUNT_MAP);
    if (!keys) {
        printf("    Failed to allocate key table\n");
        hashmap_destroy(map);
        return false;
    }
    memset(keys, 0, sizeof(char *) * STRESS_COUNT_MAP);

    for (u32 i = 0; i < STRESS_COUNT_MAP; i++) {
        keys[i] = malloc(32);
        if (!keys[i]) {
            printf("    Failed to allocate key %u\n", (unsigned)i);
            goto done;
        }
        sprintf(keys[i], "key_%u", (unsigned)i);
        hashmap_insert_string(map, keys[i], &i);
    }

    if (hashmap_size(map) != STRESS_COUNT_MAP) {
        printf("    Map size mismatch: expected %u, got %u\n",
               (unsigned)STRESS_COUNT_MAP, (unsigned)hashmap_size(map));
        goto done;
    }

    for (u32 i = 0; i < STRESS_COUNT_MAP; i += 100) {
        u32 *val = (u32 *)hashmap_get_string(map, keys[i]);
        if (!val || *val != i) {
            printf("    Map lookup failed for %s\n", keys[i]);
            goto done;
        }
    }

    {
        u32 target_count = STRESS_COUNT_MAP / 2;
        for (u32 i = 0; i < target_count; i++) {
            char *k = keys[i];
            hashmap_remove(map, &k);
        }

        if (hashmap_size(map) != STRESS_COUNT_MAP - target_count) {
            printf("    Map size mismatch after remove: expected %u, got %u\n",
                   (unsigned)(STRESS_COUNT_MAP - target_count),
                   (unsigned)hashmap_size(map));
            goto done;
        }

        for (u32 i = target_count; i < STRESS_COUNT_MAP; i += 100) {
            u32 *val = (u32 *)hashmap_get_string(map, keys[i]);
            if (!val || *val != i) {
                printf("    Surviving key lookup failed for %s\n", keys[i]);
                goto done;
            }
        }
    }

    ok = true;

done:
    if (keys) {
        for (u32 i = 0; i < STRESS_COUNT_MAP; i++) {
            free(keys[i]);
        }
        free(keys);
    }
    hashmap_destroy(map);

    clock_t end = clock();
    double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("    HashMap test %s in %.2f ms\n", ok ? "passed" : "failed",
           time_ms);
    return ok;
}

int main(void) {
    printf("=== Data Structure Stress Tests ===\n");

    LoggerConfig config = {0};
    config.min_level = LOG_LEVEL_INFO;
    config.enabled_channels = LOG_CHANNEL_CONSOLE;
    config.show_timestamp = false;
    config.use_colors = false;
    for (int i = 0; i < LOG_CAT_COUNT; i++) {
        config.enabled_categories[i] = true;
    }

    if (!logger_init(&config)) {
        printf("Logger init failed\n");
        return 1;
    }

    bool passed = true;
    passed = run_dynamic_array_stress_test() && passed;
    passed = run_hashmap_stress_test() && passed;

    logger_shutdown();

    if (passed) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    printf("TESTS FAILED\n");
    return 1;
}
