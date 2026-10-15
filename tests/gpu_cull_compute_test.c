/*
 * gpu_cull_compute_test.c
 * Lifecycle, dirty cull processing, and memory accounting for
 * gpu_cull_compute.c. Compiles that file directly and does not link Engine.
 * debug_print is not called: it locks the module mutex and then calls
 * get_memory_usage, which locks the same non-recursive mutex.
 */

#include "geometry/culling/gpu_culling/gpu_cull_compute.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* Matches the per-instance GPU buffer sizes in gpu_cull_compute.c. */
#define GPU_CULL_BYTES_PER_INSTANCE (1024u + (1024u * 1024u) + (512u * 1024u) + 256u)

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_true(bool cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static void expect_int(int got, int want, const char *msg) {
    if (got != want) {
        fprintf(stderr, "FAIL: %s (got %d, want %d)\n", msg, got, want);
        exit(1);
    }
}

static culling_gpu_cull_compute_handle_t make_instance(uint32_t flags) {
    culling_gpu_cull_compute_desc_t desc;
    culling_gpu_cull_compute_handle_t handle;
    culling_gpu_cull_compute_info_t info;

    desc.flags = flags;
    desc.user_data = NULL;
    handle.id = UINT32_MAX;

    expect_int(culling_gpu_cull_compute_create(&handle, &desc), 0, "create");
    expect_true(culling_gpu_cull_compute_is_valid(handle), "created handle is valid");
    expect_int(culling_gpu_cull_compute_get_info(handle, &info), 0, "get_info");
    expect_true(info.id != 0, "instance id assigned");
    expect_true(info.flags == flags, "flags stored");
    expect_true(info.initialized, "instance initialized");
    return handle;
}

int main(void) {
    culling_gpu_cull_compute_handle_t missing;
    culling_gpu_cull_compute_handle_t first;
    culling_gpu_cull_compute_handle_t second;
    culling_gpu_cull_compute_desc_t desc;
    culling_gpu_cull_compute_info_t info;
    uint32_t object_count;
    size_t memory_before;
    size_t memory_after;
    uint32_t first_id;

    missing.id = 99999;
    desc.flags = 0;
    desc.user_data = NULL;

    expect_int(culling_gpu_cull_compute_create(NULL, &desc), -1, "null handle");
    expect_int(culling_gpu_cull_compute_create(&missing, NULL), -1, "null desc");
    expect_int(culling_gpu_cull_compute_create(&missing, &desc), -2, "create before init");
    expect_true(!culling_gpu_cull_compute_is_valid(missing), "invalid before init");
    expect_true(culling_gpu_cull_compute_get_count() == 0, "count before init");
    expect_true(culling_gpu_cull_compute_get_memory_usage() == 0, "memory before init");
    expect_int(culling_gpu_cull_compute_process_pending(), -1, "process before init");
    expect_int(culling_gpu_cull_compute_update(missing, &object_count, 0), -1, "zero-size update");
    expect_int(culling_gpu_cull_compute_update(missing, NULL, sizeof(object_count)), -1,
               "null update");
    expect_int(culling_gpu_cull_compute_get_info(missing, NULL), -1, "null info");
    culling_gpu_cull_compute_destroy(missing);
    culling_gpu_cull_compute_mark_dirty(missing);

    expect_int(culling_gpu_cull_compute_init(), 0, "init");
    expect_int(culling_gpu_cull_compute_init(), 0, "init is idempotent");
    expect_true(culling_gpu_cull_compute_get_count() == 0, "count after init");
    expect_true(culling_gpu_cull_compute_get_memory_usage() > 0, "memory after init");
    expect_int(culling_gpu_cull_compute_process_pending(), 0, "nothing pending");
    expect_true(!culling_gpu_cull_compute_is_valid(missing), "out-of-range handle");
    expect_int(culling_gpu_cull_compute_get_info(missing, &info), -2, "info for missing");
    expect_int(culling_gpu_cull_compute_update(missing, &object_count, sizeof(object_count)), -2,
               "update missing");

    memory_before = culling_gpu_cull_compute_get_memory_usage();
    first = make_instance(0xA5u);
    expect_true(culling_gpu_cull_compute_get_count() == 1, "count after one create");
    memory_after = culling_gpu_cull_compute_get_memory_usage();
    expect_true(memory_after == memory_before + GPU_CULL_BYTES_PER_INSTANCE,
                "memory grows by one instance");

    expect_int(culling_gpu_cull_compute_get_info(first, &info), 0, "first info");
    first_id = info.id;
    expect_int(culling_gpu_cull_compute_process_pending(), 0, "clean instance is not culled");

    culling_gpu_cull_compute_mark_dirty(first);
    expect_int(culling_gpu_cull_compute_process_pending(), 1, "dirty instance is culled");
    expect_int(culling_gpu_cull_compute_process_pending(), 0, "cull clears dirty");

    object_count = 100;
    expect_int(culling_gpu_cull_compute_update(first, &object_count, sizeof(object_count)), 0,
               "update object count");
    expect_int(culling_gpu_cull_compute_process_pending(), 1, "update marks dirty and culls");
    expect_int(culling_gpu_cull_compute_process_pending(), 0, "second pass is idle");

    second = make_instance(0x11u);
    expect_true(culling_gpu_cull_compute_get_count() == 2, "count after second create");
    expect_int(culling_gpu_cull_compute_get_info(second, &info), 0, "second info");
    expect_true(info.id != first_id, "instance ids differ");
    expect_true(info.flags == 0x11u, "second flags");
    expect_true(culling_gpu_cull_compute_get_memory_usage() ==
                    memory_before + (2u * GPU_CULL_BYTES_PER_INSTANCE),
                "memory grows by two instances");

    object_count = 10;
    expect_int(culling_gpu_cull_compute_update(first, &object_count, sizeof(object_count)), 0,
               "update first again");
    object_count = 20;
    expect_int(culling_gpu_cull_compute_update(second, &object_count, sizeof(object_count)), 0,
               "update second");
    expect_int(culling_gpu_cull_compute_process_pending(), 2, "both dirty instances are culled");
    expect_int(culling_gpu_cull_compute_process_pending(), 0, "both cleared");

    culling_gpu_cull_compute_destroy(first);
    expect_true(!culling_gpu_cull_compute_is_valid(first), "destroyed handle");
    expect_true(culling_gpu_cull_compute_is_valid(second), "other handle stays valid");
    expect_true(culling_gpu_cull_compute_get_count() == 1, "count after destroy");
    expect_int(culling_gpu_cull_compute_get_info(first, &info), -2, "info after destroy");
    culling_gpu_cull_compute_destroy(first);
    expect_true(culling_gpu_cull_compute_get_count() == 1, "double destroy is a no-op");

    culling_gpu_cull_compute_shutdown();
    expect_true(culling_gpu_cull_compute_get_count() == 0, "count after shutdown");
    expect_true(culling_gpu_cull_compute_get_memory_usage() == 0, "memory after shutdown");
    expect_true(!culling_gpu_cull_compute_is_valid(second), "handles die with shutdown");
    expect_int(culling_gpu_cull_compute_process_pending(), -1, "process after shutdown");
    expect_int(culling_gpu_cull_compute_create(&missing, &desc), -2, "create after shutdown");
    culling_gpu_cull_compute_shutdown();

    expect_int(culling_gpu_cull_compute_init(), 0, "re-init");
    first = make_instance(1u);
    expect_true(culling_gpu_cull_compute_get_count() == 1, "count after re-init");
    culling_gpu_cull_compute_shutdown();

    printf("ok\n");
    return 0;
}
