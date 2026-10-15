/*
 * Issue #155. Exercises src/engine/assets/system/asset_system/loading/asset_loader.c,
 * the file cmake/sources.cmake compiles. Compiled directly, not through
 * add_engine_test: that macro links Engine, which does not build on origin/main.
 * The logger and the thread pool are stubbed. The checks below never submit a
 * load task; the thread pool stub counts submissions to prove it.
 */

#include "assets/system/asset_system/loading/asset_loader.h"
#include "core/logging/unified_logger.h"
#include "core/sync/thread_pool.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Logger stub: the loader's LOG_* macros expand to this call. */
void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

/* Thread pool stub. */
static int g_submit_count = 0;
static char g_pool_placeholder;

ThreadPool *thread_pool_create(u32 num_threads) {
    (void)num_threads;
    return (ThreadPool *)&g_pool_placeholder;
}

void thread_pool_destroy(ThreadPool *pool) {
    (void)pool;
}

void thread_pool_wait(ThreadPool *pool) {
    (void)pool;
}

void thread_pool_submit(ThreadPool *pool, ThreadWork work, void *arg) {
    (void)pool;
    (void)work;
    (void)arg;
    g_submit_count++;
}

/* The core loader API has no header, so declare the parts these checks use. */
bool asset_loader_init(u32 max_concurrent_loads, u32 cache_size_mb);
void asset_loader_shutdown(void);
u32 asset_loader_load_sync(const char *path);
u32 asset_loader_load_async(const char *path,
                            void (*callback)(u32 asset_id, bool success, void *data),
                            void *userdata);
void *asset_loader_get_data(u32 asset_id);
void asset_loader_get_stats(u32 *loaded_count, u32 *loading_count, u32 *total_memory);

static int g_asset_test_failures = 0;

#define ASSET_CHECK(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            g_asset_test_failures++;                                           \
        }                                                                      \
    } while (0)

static void test_create_requires_init(void) {
    asset_system_asset_loader_desc_t desc = {.flags = 1, .user_data = NULL};
    asset_system_asset_loader_handle_t handle = {.id = 0};

    ASSET_CHECK(asset_system_asset_loader_create(&handle, &desc) == -2);
    ASSET_CHECK(asset_system_asset_loader_get_count() == 0);
    ASSET_CHECK(!asset_system_asset_loader_is_valid(handle));
    ASSET_CHECK(asset_system_asset_loader_process_pending() == 0);
}

static void test_lifecycle(void) {
    asset_system_asset_loader_desc_t desc = {.flags = 0x5, .user_data = NULL};
    asset_system_asset_loader_handle_t a = {.id = 0xFFFFFFFFu};
    asset_system_asset_loader_handle_t b = {.id = 0xFFFFFFFFu};
    asset_system_asset_loader_handle_t missing = {.id = 2};
    asset_system_asset_loader_info_t info = {0};

    ASSET_CHECK(asset_system_asset_loader_init() == 0);
    ASSET_CHECK(asset_system_asset_loader_init() == 0);
    ASSET_CHECK(asset_system_asset_loader_get_count() == 0);

    ASSET_CHECK(asset_system_asset_loader_create(&a, &desc) == 0);
    ASSET_CHECK(asset_system_asset_loader_create(&b, &desc) == 0);
    ASSET_CHECK(a.id == 0);
    ASSET_CHECK(b.id == 1);
    ASSET_CHECK(asset_system_asset_loader_get_count() == 2);
    ASSET_CHECK(asset_system_asset_loader_is_valid(a));
    ASSET_CHECK(asset_system_asset_loader_is_valid(b));
    ASSET_CHECK(!asset_system_asset_loader_is_valid(missing));

    ASSET_CHECK(asset_system_asset_loader_get_info(b, &info) == 0);
    ASSET_CHECK(info.id == 1);
    ASSET_CHECK(info.flags == 0x5);
    ASSET_CHECK(info.initialized);
    ASSET_CHECK(asset_system_asset_loader_get_info(b, NULL) == -1);
    ASSET_CHECK(asset_system_asset_loader_get_info(missing, &info) == -2);

    /* create() marks each item dirty, so both are pending exactly once. */
    ASSET_CHECK(asset_system_asset_loader_process_pending() == 2);
    ASSET_CHECK(asset_system_asset_loader_process_pending() == 0);

    ASSET_CHECK(asset_system_asset_loader_update(a, NULL, 0) == 0);
    ASSET_CHECK(asset_system_asset_loader_process_pending() == 1);
    asset_system_asset_loader_mark_dirty(b);
    ASSET_CHECK(asset_system_asset_loader_process_pending() == 1);
    ASSET_CHECK(asset_system_asset_loader_update(missing, NULL, 0) == -1);
    ASSET_CHECK(asset_system_asset_loader_get_memory_usage() > 0);
    asset_system_asset_loader_debug_print();

    asset_system_asset_loader_destroy(a);
    ASSET_CHECK(!asset_system_asset_loader_is_valid(a));
    ASSET_CHECK(asset_system_asset_loader_update(a, NULL, 0) == -2);
    ASSET_CHECK(asset_system_asset_loader_get_info(a, &info) == 0);
    ASSET_CHECK(!info.initialized);
    ASSET_CHECK(asset_system_asset_loader_process_pending() == 0);
    ASSET_CHECK(asset_system_asset_loader_is_valid(b));

    asset_system_asset_loader_shutdown();
    ASSET_CHECK(asset_system_asset_loader_get_count() == 0);
    ASSET_CHECK(!asset_system_asset_loader_is_valid(b));
}

static void test_reinit_after_shutdown(void) {
    asset_system_asset_loader_desc_t desc = {.flags = 0, .user_data = NULL};
    asset_system_asset_loader_handle_t handle = {.id = 0xFFFFFFFFu};

    ASSET_CHECK(asset_system_asset_loader_init() == 0);
    ASSET_CHECK(asset_system_asset_loader_create(&handle, &desc) == 0);
    asset_system_asset_loader_shutdown();

    ASSET_CHECK(asset_system_asset_loader_create(&handle, &desc) == -2);

    ASSET_CHECK(asset_system_asset_loader_init() == 0);
    ASSET_CHECK(asset_system_asset_loader_get_count() == 0);
    handle.id = 0xFFFFFFFFu;
    ASSET_CHECK(asset_system_asset_loader_create(&handle, &desc) == 0);
    ASSET_CHECK(handle.id == 0);
    asset_system_asset_loader_shutdown();
}

static void test_capacity_limit(void) {
    asset_system_asset_loader_desc_t desc = {.flags = 0, .user_data = NULL};
    asset_system_asset_loader_handle_t handle = {.id = 0};
    uint32_t created = 0;
    int rc = 0;

    ASSET_CHECK(asset_system_asset_loader_init() == 0);
    while (created < 100000u && (rc = asset_system_asset_loader_create(&handle, &desc)) == 0) {
        created++;
    }
    ASSET_CHECK(rc == -3);
    ASSET_CHECK(created > 0);
    ASSET_CHECK(asset_system_asset_loader_get_count() == created);
    asset_system_asset_loader_shutdown();
}

static void test_loader_core_without_loaders(void) {
    u32 loaded = 99;
    u32 loading = 99;
    u32 memory = 99;

    ASSET_CHECK(asset_loader_init(4, 1));
    ASSET_CHECK(asset_loader_init(4, 1));

    /* No loader is registered, so every extension is unknown and nothing is submitted. */
    ASSET_CHECK(asset_loader_load_sync(NULL) == 0);
    ASSET_CHECK(asset_loader_load_sync("models/crate.unknown") == 0);
    ASSET_CHECK(asset_loader_load_async("models/crate.unknown", NULL, NULL) == 0);
    ASSET_CHECK(g_submit_count == 0);
    ASSET_CHECK(asset_loader_get_data(0) == NULL);

    asset_loader_get_stats(&loaded, &loading, &memory);
    ASSET_CHECK(loaded == 0);
    ASSET_CHECK(loading == 0);
    ASSET_CHECK(memory == 0);

    asset_loader_shutdown();
    loaded = 99;
    loading = 99;
    memory = 99;
    asset_loader_get_stats(&loaded, &loading, &memory);
    ASSET_CHECK(loaded == 0);
    ASSET_CHECK(loading == 0);
    ASSET_CHECK(memory == 0);
}

int main(void) {
    test_create_requires_init();
    test_lifecycle();
    test_reinit_after_shutdown();
    test_capacity_limit();
    test_loader_core_without_loaders();

    if (g_asset_test_failures != 0) {
        fprintf(stderr, "test_asset_loader: %d check(s) failed\n", g_asset_test_failures);
        return EXIT_FAILURE;
    }
    printf("test_asset_loader: all checks passed\n");
    return EXIT_SUCCESS;
}
