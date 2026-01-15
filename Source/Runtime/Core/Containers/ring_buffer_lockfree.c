#include <core/containers_advanced/ring_buffer_lockfree.h>

#include <stdalign.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CACHE_LINE_SIZE 64u

static uint32_t next_pow2(uint32_t value) {
  if (value == 0) {
    return 1;
  }
  value--;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  return value + 1;
}

struct RingBufferSPSC {
  void **buffer;
  uint32_t capacity;
  uint32_t mask;
  alignas(CACHE_LINE_SIZE) _Atomic uint32_t head;
  char pad0[CACHE_LINE_SIZE - sizeof(_Atomic uint32_t)];
  alignas(CACHE_LINE_SIZE) _Atomic uint32_t tail;
  char pad1[CACHE_LINE_SIZE - sizeof(_Atomic uint32_t)];
};

RingBufferSPSC *ring_init(uint32_t capacity) {
  if (capacity < 2) {
    capacity = 2;
  }
  capacity = next_pow2(capacity);

  RingBufferSPSC *ring = (RingBufferSPSC *)calloc(1, sizeof(RingBufferSPSC));
  if (!ring) {
    return NULL;
  }

  ring->buffer = (void **)calloc(capacity, sizeof(void *));
  if (!ring->buffer) {
    free(ring);
    return NULL;
  }

  ring->capacity = capacity;
  ring->mask = capacity - 1;
  atomic_store(&ring->head, 0);
  atomic_store(&ring->tail, 0);
  return ring;
}

void ring_spsc_destroy(RingBufferSPSC *ring) {
  if (!ring) {
    return;
  }
  free(ring->buffer);
  free(ring);
}

bool spsc_enqueue(RingBufferSPSC *ring, void *item) {
  if (!ring) {
    return false;
  }

  uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
  uint32_t next = (head + 1) & ring->mask;
  uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
  if (next == tail) {
    return false;
  }

  ring->buffer[head] = item;
  atomic_store_explicit(&ring->head, next, memory_order_release);
  return true;
}

void *spsc_dequeue(RingBufferSPSC *ring) {
  if (!ring) {
    return NULL;
  }

  uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
  uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
  if (tail == head) {
    return NULL;
  }

  void *item = ring->buffer[tail];
  atomic_store_explicit(&ring->tail, (tail + 1) & ring->mask,
                        memory_order_release);
  return item;
}

size_t spsc_enqueue_batch(RingBufferSPSC *ring, void **items, size_t count) {
  if (!ring || !items || count == 0) {
    return 0;
  }

  size_t pushed = 0;
  for (; pushed < count; ++pushed) {
    if (!spsc_enqueue(ring, items[pushed])) {
      break;
    }
  }
  return pushed;
}

size_t spsc_dequeue_batch(RingBufferSPSC *ring, void **items, size_t count) {
  if (!ring || !items || count == 0) {
    return 0;
  }

  size_t popped = 0;
  for (; popped < count; ++popped) {
    void *item = spsc_dequeue(ring);
    if (!item) {
      break;
    }
    items[popped] = item;
  }
  return popped;
}

typedef struct MPMCCell {
  _Atomic size_t seq;
  void *data;
} MPMCCell;

struct RingBufferMPMC {
  MPMCCell *cells;
  size_t capacity;
  size_t mask;
  alignas(CACHE_LINE_SIZE) _Atomic size_t enqueue_pos;
  char pad0[CACHE_LINE_SIZE - sizeof(_Atomic size_t)];
  alignas(CACHE_LINE_SIZE) _Atomic size_t dequeue_pos;
  char pad1[CACHE_LINE_SIZE - sizeof(_Atomic size_t)];
};

RingBufferMPMC *ring_mpmc_create(uint32_t capacity) {
  if (capacity < 2) {
    capacity = 2;
  }
  capacity = next_pow2(capacity);

  RingBufferMPMC *ring = (RingBufferMPMC *)calloc(1, sizeof(RingBufferMPMC));
  if (!ring) {
    return NULL;
  }

  ring->cells = (MPMCCell *)calloc(capacity, sizeof(MPMCCell));
  if (!ring->cells) {
    free(ring);
    return NULL;
  }

  for (size_t i = 0; i < capacity; ++i) {
    atomic_store(&ring->cells[i].seq, i);
  }

  ring->capacity = capacity;
  ring->mask = capacity - 1;
  atomic_store(&ring->enqueue_pos, 0);
  atomic_store(&ring->dequeue_pos, 0);
  return ring;
}

void ring_mpmc_destroy(RingBufferMPMC *ring) {
  if (!ring) {
    return;
  }
  free(ring->cells);
  free(ring);
}

bool mpmc_enqueue(RingBufferMPMC *ring, void *item) {
  if (!ring) {
    return false;
  }

  MPMCCell *cell;
  size_t pos = atomic_load_explicit(&ring->enqueue_pos, memory_order_relaxed);
  for (;;) {
    cell = &ring->cells[pos & ring->mask];
    size_t seq = atomic_load_explicit(&cell->seq, memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)pos;
    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(
              &ring->enqueue_pos, &pos, pos + 1, memory_order_relaxed,
              memory_order_relaxed)) {
        break;
      }
    } else if (diff < 0) {
      return false;
    } else {
      pos = atomic_load_explicit(&ring->enqueue_pos, memory_order_relaxed);
    }
  }

  cell->data = item;
  atomic_store_explicit(&cell->seq, pos + 1, memory_order_release);
  return true;
}

void *mpmc_dequeue(RingBufferMPMC *ring) {
  if (!ring) {
    return NULL;
  }

  MPMCCell *cell;
  size_t pos = atomic_load_explicit(&ring->dequeue_pos, memory_order_relaxed);
  for (;;) {
    cell = &ring->cells[pos & ring->mask];
    size_t seq = atomic_load_explicit(&cell->seq, memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);
    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(
              &ring->dequeue_pos, &pos, pos + 1, memory_order_relaxed,
              memory_order_relaxed)) {
        break;
      }
    } else if (diff < 0) {
      return NULL;
    } else {
      pos = atomic_load_explicit(&ring->dequeue_pos, memory_order_relaxed);
    }
  }

  void *data = cell->data;
  atomic_store_explicit(&cell->seq, pos + ring->capacity, memory_order_release);
  return data;
}

size_t mpmc_enqueue_batch(RingBufferMPMC *ring, void **items, size_t count) {
  if (!ring || !items || count == 0) {
    return 0;
  }

  size_t pushed = 0;
  for (; pushed < count; ++pushed) {
    if (!mpmc_enqueue(ring, items[pushed])) {
      break;
    }
  }
  return pushed;
}

size_t mpmc_dequeue_batch(RingBufferMPMC *ring, void **items, size_t count) {
  if (!ring || !items || count == 0) {
    return 0;
  }

  size_t popped = 0;
  for (; popped < count; ++popped) {
    void *item = mpmc_dequeue(ring);
    if (!item) {
      break;
    }
    items[popped] = item;
  }
  return popped;
}

void ring_lockfree_benchmark(uint32_t iterations) {
  if (iterations == 0) {
    return;
  }

  RingBufferSPSC *ring = ring_init(1024);
  if (!ring) {
    return;
  }

  clock_t start = clock();
  for (uint32_t i = 0; i < iterations; ++i) {
    spsc_enqueue(ring, (void *)(uintptr_t)i);
    spsc_dequeue(ring);
  }
  clock_t end = clock();

  (void)start;
  (void)end;

  ring_spsc_destroy(ring);
}
