#ifndef CORE_CONTAINERS_ADVANCED_RING_BUFFER_H
#define CORE_CONTAINERS_ADVANCED_RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RingBuffer RingBuffer;

RingBuffer *ring_create(size_t capacity, size_t element_size);
void ring_destroy(RingBuffer *ring);

bool ring_push(RingBuffer *ring, const void *item);
bool ring_pop(RingBuffer *ring, void *out_item);

size_t ring_count(const RingBuffer *ring);
size_t ring_capacity(const RingBuffer *ring);

void ring_set_overwrite(RingBuffer *ring, bool enabled);
void ring_set_thread_safe(RingBuffer *ring, bool enabled);

size_t ring_copy_out(const RingBuffer *ring, void *dest, size_t max_items);

bool ring_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
