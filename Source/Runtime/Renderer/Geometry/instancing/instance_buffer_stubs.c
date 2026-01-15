// Stub implementation for instance buffer functions to make tests compile
// These are disabled in the main engine but needed for test linking

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

instance_buffer_t* instance_buffer_create(size_t capacity) {
    instance_buffer_t *buffer = calloc(1, sizeof(instance_buffer_t));
    if (buffer) {
        buffer->data = malloc(capacity * 64); // Assume 64 bytes per instance
        buffer->capacity = capacity;
        buffer->size = capacity * 64;
    }
    return buffer;
}

void instance_buffer_destroy(instance_buffer_t *buffer) {
    if (buffer) {
        free(buffer->data);
        free(buffer);
    }
}

void instance_buffer_update(instance_buffer_t *buffer, const void *data, size_t size) {
    if (buffer && data && size <= buffer->size) {
        memcpy(buffer->data, data, size);
        buffer->count = size / 64; // Rough estimate
    }
}

int instance_buffer_validate(const instance_buffer_t *buffer) {
    return buffer && buffer->data ? 1 : 0;
}

void instance_buffer_get_stats(const instance_buffer_t *buffer, size_t *allocated, size_t *used) {
    if (buffer && allocated && used) {
        *allocated = buffer->size;
        *used = buffer->count * 64; // Rough estimate
    }
}

instance_pool_t* instance_pool_create(size_t capacity) {
    instance_pool_t *pool = calloc(1, sizeof(instance_pool_t));
    if (pool) {
        pool->allocated = capacity * 64; // Assume 64 bytes per instance
        pool->used = 0;
        pool->pool_count = 0;
    }
    return pool;
}

void instance_pool_destroy(instance_pool_t *pool) {
    free(pool);
}

void* instance_pool_alloc(instance_pool_t *pool, size_t size) {
    if (pool && pool->used + size <= pool->allocated) {
        pool->used += size;
        pool->pool_count++;
        return malloc(size);
    }
    return NULL;
}

void instance_pool_free(instance_pool_t *pool, void *ptr) {
    if (pool && ptr) {
        pool->pool_count--;
        free(ptr);
    }
}

int instance_pool_can_allocate(const instance_pool_t *pool, size_t size) {
    return pool && (pool->used + size <= pool->allocated);
}

void instance_pool_reset(instance_pool_t *pool) {
    if (pool) {
        pool->used = 0;
        pool->pool_count = 0;
    }
}

size_t instance_data_calculate_size(size_t count, size_t instance_size) {
    return count * instance_size;
}

int instance_data_init(size_t capacity) {
    // Stub: return 1 for success
    return 1;
}

void instance_data_shutdown(void) {
    // Stub: nothing to cleanup
}

int instance_data_validate(const void *data, size_t size) {
    // Stub: return 1 for valid
    return data && size > 0 ? 1 : 0;
}
