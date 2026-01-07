#include "core/containers_advanced/ring_buffer.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

struct RingBuffer {
  uint8_t *buffer;
  size_t capacity;
  size_t element_size;
  size_t head;
  size_t tail;
  size_t count;
  bool overwrite;
  bool thread_safe;
  atomic_flag lock;
};

static void ring_lock(RingBuffer *ring) {
  if (!ring || !ring->thread_safe) {
    return;
  }
  while (atomic_flag_test_and_set(&ring->lock)) {
  }
}

static void ring_unlock(RingBuffer *ring) {
  if (!ring || !ring->thread_safe) {
    return;
  }
  atomic_flag_clear(&ring->lock);
}

RingBuffer *ring_create(size_t capacity, size_t element_size) {
  if (capacity == 0 || element_size == 0) {
    return NULL;
  }

  RingBuffer *ring = (RingBuffer *)calloc(1, sizeof(RingBuffer));
  if (!ring) {
    return NULL;
  }

  ring->buffer = (uint8_t *)malloc(capacity * element_size);
  if (!ring->buffer) {
    free(ring);
    return NULL;
  }

  ring->capacity = capacity;
  ring->element_size = element_size;
  ring->head = 0;
  ring->tail = 0;
  ring->count = 0;
  ring->overwrite = false;
  ring->thread_safe = false;
  atomic_flag_clear(&ring->lock);
  return ring;
}

void ring_destroy(RingBuffer *ring) {
  if (!ring) {
    return;
  }
  free(ring->buffer);
  free(ring);
}

bool ring_push(RingBuffer *ring, const void *item) {
  if (!ring || !item) {
    return false;
  }

  ring_lock(ring);

  if (ring->count == ring->capacity) {
    if (!ring->overwrite) {
      ring_unlock(ring);
      return false;
    }
    ring->head = (ring->head + 1) % ring->capacity;
    ring->count--;
  }

  uint8_t *dst =
      ring->buffer + (ring->tail * ring->element_size);
  memcpy(dst, item, ring->element_size);
  ring->tail = (ring->tail + 1) % ring->capacity;
  ring->count++;

  ring_unlock(ring);
  return true;
}

bool ring_pop(RingBuffer *ring, void *out_item) {
  if (!ring || !out_item) {
    return false;
  }

  ring_lock(ring);

  if (ring->count == 0) {
    ring_unlock(ring);
    return false;
  }

  uint8_t *src =
      ring->buffer + (ring->head * ring->element_size);
  memcpy(out_item, src, ring->element_size);
  ring->head = (ring->head + 1) % ring->capacity;
  ring->count--;

  ring_unlock(ring);
  return true;
}

size_t ring_count(const RingBuffer *ring) {
  return ring ? ring->count : 0;
}

size_t ring_capacity(const RingBuffer *ring) {
  return ring ? ring->capacity : 0;
}

void ring_set_overwrite(RingBuffer *ring, bool enabled) {
  if (ring) {
    ring->overwrite = enabled;
  }
}

void ring_set_thread_safe(RingBuffer *ring, bool enabled) {
  if (ring) {
    ring->thread_safe = enabled;
    if (!enabled) {
      atomic_flag_clear(&ring->lock);
    }
  }
}

size_t ring_copy_out(const RingBuffer *ring, void *dest, size_t max_items) {
  if (!ring || !dest || max_items == 0) {
    return 0;
  }

  size_t count = ring->count < max_items ? ring->count : max_items;
  for (size_t i = 0; i < count; ++i) {
    size_t idx = (ring->head + i) % ring->capacity;
    const uint8_t *src = ring->buffer + idx * ring->element_size;
    memcpy((uint8_t *)dest + i * ring->element_size, src,
           ring->element_size);
  }
  return count;
}

bool ring_self_test(void) {
  RingBuffer *ring = ring_create(4, sizeof(int));
  if (!ring) {
    return false;
  }

  int value = 1;
  ring_push(ring, &value);
  value = 2;
  ring_push(ring, &value);

  int out = 0;
  ring_pop(ring, &out);
  bool ok = (out == 1);

  ring_destroy(ring);
  return ok;
}
