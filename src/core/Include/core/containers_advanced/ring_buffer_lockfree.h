#ifndef CORE_CONTAINERS_ADVANCED_RING_BUFFER_LOCKFREE_H
#define CORE_CONTAINERS_ADVANCED_RING_BUFFER_LOCKFREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RingBufferSPSC RingBufferSPSC;
RingBufferSPSC *ring_init(uint32_t capacity);
void ring_spsc_destroy(RingBufferSPSC *ring);

bool spsc_enqueue(RingBufferSPSC *ring, void *item);
void *spsc_dequeue(RingBufferSPSC *ring);
size_t spsc_enqueue_batch(RingBufferSPSC *ring, void **items, size_t count);
size_t spsc_dequeue_batch(RingBufferSPSC *ring, void **items, size_t count);

typedef struct RingBufferMPMC RingBufferMPMC;
RingBufferMPMC *ring_mpmc_create(uint32_t capacity);
void ring_mpmc_destroy(RingBufferMPMC *ring);

bool mpmc_enqueue(RingBufferMPMC *ring, void *item);
void *mpmc_dequeue(RingBufferMPMC *ring);
size_t mpmc_enqueue_batch(RingBufferMPMC *ring, void **items, size_t count);
size_t mpmc_dequeue_batch(RingBufferMPMC *ring, void **items, size_t count);

void ring_lockfree_benchmark(uint32_t iterations);

#ifdef __cplusplus
}
#endif

#endif
