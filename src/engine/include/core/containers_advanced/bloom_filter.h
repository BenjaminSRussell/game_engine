#ifndef CORE_CONTAINERS_ADVANCED_BLOOM_FILTER_H
#define CORE_CONTAINERS_ADVANCED_BLOOM_FILTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BloomFilter BloomFilter;

BloomFilter *bloom_create(size_t bit_count, size_t hash_count);
BloomFilter *bloom_create_scaling(size_t bit_count, size_t hash_count,
                                  double max_load_factor);
void bloom_destroy(BloomFilter *filter);

void bloom_insert(BloomFilter *filter, const void *data, size_t length);
bool bloom_test(const BloomFilter *filter, const void *data, size_t length);

bool bloom_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
