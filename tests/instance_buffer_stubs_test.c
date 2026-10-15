/*
 * instance_buffer_stubs_test.c
 * Create, update, validate, and pool accounting for the instance-buffer stubs.
 * Compiles instance_buffer_stubs.c directly and does not link Engine.
 *
 * The stub types are private to that file. These definitions match its layout
 * so the test can read the fields the functions actually update.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void *data;
    size_t size;
    size_t capacity;
    int count;
} instance_buffer_t;

typedef struct {
    size_t allocated;
    size_t used;
    int pool_count;
} instance_pool_t;

instance_buffer_t *instance_buffer_create(size_t capacity);
void instance_buffer_destroy(instance_buffer_t *buffer);
void instance_buffer_update(instance_buffer_t *buffer, const void *data, size_t size);
int instance_buffer_validate(const instance_buffer_t *buffer);
void instance_buffer_get_stats(const instance_buffer_t *buffer, size_t *allocated, size_t *used);

instance_pool_t *instance_pool_create(size_t capacity);
void instance_pool_destroy(instance_pool_t *pool);
void *instance_pool_alloc(instance_pool_t *pool, size_t size);
void instance_pool_free(instance_pool_t *pool, void *ptr);
int instance_pool_can_allocate(const instance_pool_t *pool, size_t size);
void instance_pool_reset(instance_pool_t *pool);

size_t instance_data_calculate_size(size_t count, size_t instance_size);
int instance_data_init(size_t capacity);
void instance_data_shutdown(void);
int instance_data_validate(const void *data, size_t size);

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_true(int cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static void expect_size(size_t got, size_t want, const char *msg) {
    if (got != want) {
        fprintf(stderr, "FAIL: %s (got %zu want %zu)\n", msg, got, want);
        exit(1);
    }
}

static int bytes_are(const unsigned char *data, size_t n, unsigned char value) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (data[i] != value) {
            return 0;
        }
    }
    return 1;
}

static void test_null_guards(void) {
    size_t allocated = 9;
    size_t used = 9;
    unsigned char byte = 1;

    instance_buffer_destroy(NULL);
    expect_true(!instance_buffer_validate(NULL), "null buffer is invalid");
    instance_buffer_update(NULL, &byte, 1);
    instance_buffer_get_stats(NULL, &allocated, &used);
    expect_size(allocated, 9, "null buffer leaves allocated");
    expect_size(used, 9, "null buffer leaves used");

    instance_pool_destroy(NULL);
    expect_true(instance_pool_alloc(NULL, 16) == NULL, "null pool alloc");
    instance_pool_free(NULL, NULL);
    expect_true(!instance_pool_can_allocate(NULL, 1), "null pool cannot allocate");
    instance_pool_reset(NULL);

    expect_true(!instance_data_validate(NULL, 8), "null instance data");
    expect_true(!instance_data_validate(&byte, 0), "zero-size instance data");
    expect_true(instance_data_init(0) == 1, "init accepts zero capacity");
    instance_data_shutdown();
    instance_data_shutdown();
}

static void test_instance_data(void) {
    unsigned char payload[4] = {1, 2, 3, 4};

    expect_size(instance_data_calculate_size(0, 64), 0, "zero count");
    expect_size(instance_data_calculate_size(4, 0), 0, "zero stride");
    expect_size(instance_data_calculate_size(3, 64), 192, "count times stride");
    expect_true(instance_data_init(32) == 1, "init");
    expect_true(instance_data_validate(payload, sizeof(payload)), "valid payload");
    instance_data_shutdown();
    expect_true(instance_data_validate(payload, 1), "validate does not need init");
}

static void test_buffer(void) {
    instance_buffer_t *buffer;
    unsigned char first[128];
    unsigned char second[64];
    unsigned char exact[256];
    unsigned char too_big[257];
    size_t allocated = 1;
    size_t used = 1;
    size_t untouched = 42;

    buffer = instance_buffer_create(4);
    expect_true(buffer != NULL, "create");
    expect_true(instance_buffer_validate(buffer), "created buffer is valid");
    expect_true(buffer->data != NULL, "backing store");
    expect_size(buffer->capacity, 4, "capacity");
    expect_size(buffer->size, 256, "byte size is capacity * 64");
    expect_true(buffer->count == 0, "count starts at zero");

    instance_buffer_get_stats(buffer, &allocated, &used);
    expect_size(allocated, 256, "stats allocated");
    expect_size(used, 0, "stats used starts at zero");
    instance_buffer_get_stats(buffer, NULL, &untouched);
    instance_buffer_get_stats(buffer, &untouched, NULL);
    expect_size(untouched, 42, "missing stat out-param is ignored");

    memset(first, 0xAB, sizeof(first));
    instance_buffer_update(buffer, NULL, sizeof(first));
    expect_true(buffer->count == 0, "null data does not update");

    memset(too_big, 0xCD, sizeof(too_big));
    instance_buffer_update(buffer, too_big, sizeof(too_big));
    expect_true(buffer->count == 0, "oversized update is rejected");

    instance_buffer_update(buffer, first, sizeof(first));
    expect_true(buffer->count == 2, "count is size / 64");
    expect_true(bytes_are(buffer->data, sizeof(first), 0xAB), "copied bytes");
    instance_buffer_get_stats(buffer, &allocated, &used);
    expect_size(allocated, 256, "allocated unchanged");
    expect_size(used, 128, "used is count * 64");

    memset(second, 0x11, sizeof(second));
    instance_buffer_update(buffer, second, 63);
    expect_true(buffer->count == 0, "partial instance floors the count");
    expect_true(bytes_are(buffer->data, 63, 0x11), "partial copy");
    expect_true(((unsigned char *)buffer->data)[63] == 0xAB, "bytes past the copy stay");
    instance_buffer_get_stats(buffer, &allocated, &used);
    expect_size(used, 0, "floored count reports no used bytes");

    memset(exact, 0x22, sizeof(exact));
    instance_buffer_update(buffer, exact, sizeof(exact));
    expect_true(buffer->count == 4, "exact capacity update");
    expect_true(bytes_are(buffer->data, sizeof(exact), 0x22), "full copy");
    instance_buffer_get_stats(buffer, &allocated, &used);
    expect_size(used, 256, "full buffer used");

    instance_buffer_update(buffer, exact, 0);
    expect_true(buffer->count == 0, "empty update clears the count");
    instance_buffer_get_stats(buffer, &allocated, &used);
    expect_size(used, 0, "empty update reports zero used");

    instance_buffer_destroy(buffer);
}

static void test_pool(void) {
    instance_pool_t *pool;
    void *first;
    void *second;
    void *third;
    unsigned char *bytes;

    pool = instance_pool_create(2);
    expect_true(pool != NULL, "pool create");
    expect_size(pool->allocated, 128, "pool bytes are capacity * 64");
    expect_size(pool->used, 0, "pool starts unused");
    expect_true(pool->pool_count == 0, "pool count starts at zero");
    expect_true(instance_pool_can_allocate(pool, 128), "exact pool fit");
    expect_true(!instance_pool_can_allocate(pool, 129), "over pool capacity");

    first = instance_pool_alloc(pool, 129);
    expect_true(first == NULL, "rejected alloc");
    expect_size(pool->used, 0, "rejected alloc does not consume");
    expect_true(pool->pool_count == 0, "rejected alloc does not count");

    first = instance_pool_alloc(pool, 40);
    expect_true(first != NULL, "first alloc");
    bytes = first;
    bytes[0] = 7;
    bytes[39] = 9;
    expect_size(pool->used, 40, "used after first alloc");
    expect_true(pool->pool_count == 1, "count after first alloc");
    expect_true(instance_pool_can_allocate(pool, 88), "remaining exact");
    expect_true(!instance_pool_can_allocate(pool, 89), "one byte over remaining");

    second = instance_pool_alloc(pool, 88);
    expect_true(second != NULL, "second alloc");
    expect_size(pool->used, 128, "pool is full");
    expect_true(pool->pool_count == 2, "two live allocs");
    expect_true(instance_pool_alloc(pool, 1) == NULL, "full pool rejects");
    expect_true(pool->pool_count == 2, "failed alloc keeps the count");

    instance_pool_free(pool, first);
    expect_true(pool->pool_count == 1, "free drops the count");
    expect_size(pool->used, 128, "free does not return bytes");
    expect_true(!instance_pool_can_allocate(pool, 1), "used bytes still block alloc");
    instance_pool_free(pool, NULL);
    expect_true(pool->pool_count == 1, "null free is a no-op");
    instance_pool_free(pool, second);
    expect_true(pool->pool_count == 0, "both frees");
    expect_size(pool->used, 128, "used remains until reset");

    instance_pool_reset(pool);
    expect_size(pool->used, 0, "reset clears used");
    expect_true(pool->pool_count == 0, "reset clears count");
    expect_true(instance_pool_can_allocate(pool, 128), "reset restores capacity");

    third = instance_pool_alloc(pool, 128);
    expect_true(third != NULL, "alloc after reset");
    expect_true(pool->pool_count == 1, "count after reset alloc");
    instance_pool_free(pool, third);
    instance_pool_destroy(pool);
}

int main(void) {
    test_null_guards();
    test_instance_data();
    test_buffer();
    test_pool();
    printf("ok\n");
    return 0;
}
