#ifndef CORE_CONTAINERS_BLOOM_FILTER_H
#define CORE_CONTAINERS_BLOOM_FILTER_H

#include "common.h"
#include <stdbool.h>

typedef struct BloomFilter BloomFilter;

BloomFilter* bloom_filter_create(size_t expected_items, f32 false_positive_rate);
void bloom_filter_destroy(BloomFilter* filter);

bool bloom_filter_add(BloomFilter* filter, const void* data, size_t len);
bool bloom_filter_contains(const BloomFilter* filter, const void* data, size_t len);

bool bloom_filter_add_string(BloomFilter* filter, const char* str);
bool bloom_filter_contains_string(const BloomFilter* filter, const char* str);

size_t bloom_filter_size(const BloomFilter* filter);
size_t bloom_filter_num_hashes(const BloomFilter* filter);
size_t bloom_filter_item_count(const BloomFilter* filter);
f32 bloom_filter_current_false_positive_rate(const BloomFilter* filter);
void bloom_filter_print_stats(const BloomFilter* filter);

size_t bloom_filter_serialize_size(const BloomFilter* filter);
bool bloom_filter_serialize(const BloomFilter* filter, void* buffer, size_t buffer_size);
BloomFilter* bloom_filter_deserialize(const void* buffer, size_t buffer_size);

bool bloom_filter_union(BloomFilter* result, const BloomFilter* filter1, const BloomFilter* filter2);
bool bloom_filter_intersection(BloomFilter* result, const BloomFilter* filter1, const BloomFilter* filter2);

#endif // CORE_CONTAINERS_BLOOM_FILTER_H
