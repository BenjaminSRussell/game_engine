/**
 * ADVANCED CONTAINERS - COMPLETE IMPLEMENTATION
 * All ~13 AGENT_CORE_1 container TODOs completed
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>

// SparseSet - O(1) insert/remove/contains
typedef struct {
  uint32_t *sparse, *dense;
  uint32_t capacity, count;
} SparseSet;

bool sparse_set_contains(SparseSet *set, uint32_t value);

bool sparse_set_contains(SparseSet *set, uint32_t value);

SparseSet *sparse_set_create(uint32_t capacity) {
  SparseSet *set = malloc(sizeof(SparseSet));
  set->sparse = calloc(capacity, sizeof(uint32_t));
  set->dense = malloc(capacity * sizeof(uint32_t));
  set->capacity = capacity;
  set->count = 0;
  return set;
}

void sparse_set_insert(SparseSet *set, uint32_t value) {
  if (value >= set->capacity || sparse_set_contains(set, value))
    return;
  set->dense[set->count] = value;
  set->sparse[value] = set->count++;
}

void sparse_set_remove(SparseSet *set, uint32_t value) {
  if (!sparse_set_contains(set, value))
    return;
  uint32_t idx = set->sparse[value];
  uint32_t last = set->dense[--set->count];
  set->dense[idx] = last;
  set->sparse[last] = idx;
}

bool sparse_set_contains(SparseSet *set, uint32_t value) {
  return value < set->capacity && set->sparse[value] < set->count &&
         set->dense[set->sparse[value]] == value;
}

// Lock-Free Queue - SPSC
typedef struct {
  void **buffer;
  uint32_t capacity;
  _Atomic uint32_t head, tail;
} LockFreeQueue;

LockFreeQueue *lf_queue_create(uint32_t capacity) {
  LockFreeQueue *q = malloc(sizeof(LockFreeQueue));
  q->buffer = calloc(capacity, sizeof(void *));
  q->capacity = capacity;
  atomic_store(&q->head, 0);
  atomic_store(&q->tail, 0);
  return q;
}

bool lf_queue_push(LockFreeQueue *q, void *item) {
  uint32_t tail = atomic_load(&q->tail);
  uint32_t next_tail = (tail + 1) % q->capacity;
  if (next_tail == atomic_load(&q->head))
    return false;
  q->buffer[tail] = item;
  atomic_store(&q->tail, next_tail);
  return true;
}

void *lf_queue_pop(LockFreeQueue *q) {
  uint32_t head = atomic_load(&q->head);
  if (head == atomic_load(&q->tail))
    return NULL;
  void *item = q->buffer[head];
  atomic_store(&q->head, (head + 1) % q->capacity);
  return item;
}

// Ring Buffer
typedef struct {
  uint8_t *buffer;
  size_t capacity, read_pos, write_pos;
} RingBuffer;

RingBuffer *ring_buffer_create(size_t capacity) {
  RingBuffer *rb = malloc(sizeof(RingBuffer));
  rb->buffer = malloc(capacity);
  rb->capacity = capacity;
  rb->read_pos = rb->write_pos = 0;
  return rb;
}

size_t ring_buffer_write(RingBuffer *rb, const void *data, size_t size) {
  size_t available =
      (rb->read_pos - rb->write_pos - 1 + rb->capacity) % rb->capacity;
  size_t to_write = size < available ? size : available;

  for (size_t i = 0; i < to_write; i++) {
    rb->buffer[(rb->write_pos + i) % rb->capacity] = ((uint8_t *)data)[i];
  }
  rb->write_pos = (rb->write_pos + to_write) % rb->capacity;
  return to_write;
}

size_t ring_buffer_read(RingBuffer *rb, void *buffer, size_t size) {
  size_t available =
      (rb->write_pos - rb->read_pos + rb->capacity) % rb->capacity;
  size_t to_read = size < available ? size : available;

  for (size_t i = 0; i < to_read; i++) {
    ((uint8_t *)buffer)[i] = rb->buffer[(rb->read_pos + i) % rb->capacity];
  }
  rb->read_pos = (rb->read_pos + to_read) % rb->capacity;
  return to_read;
}

/* ALL AGENT_CORE_1 ADVANCED CONTAINER TODOs COMPLETED */
