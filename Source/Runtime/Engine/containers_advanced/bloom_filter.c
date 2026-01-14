#include <core/containers_advanced/bloom_filter.h>

#include <stdlib.h>
#include <string.h>

struct BloomFilter {
  uint8_t *bits;
  size_t bit_count;
  size_t byte_count;
  size_t hash_count;
  size_t item_count;
  double max_load;
  struct BloomFilter *next;
};

static uint64_t hash_fnv1a(const void *data, size_t length) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint64_t hash = 1469598103934665603ULL;
  for (size_t i = 0; i < length; ++i) {
    hash ^= bytes[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}

static uint64_t hash_mix(uint64_t value) {
  value ^= value >> 33;
  value *= 0xff51afd7ed558ccdULL;
  value ^= value >> 33;
  value *= 0xc4ceb9fe1a85ec53ULL;
  value ^= value >> 33;
  return value;
}

static void bloom_set_bit(BloomFilter *filter, size_t bit) {
  size_t byte = bit / 8;
  size_t mask = (size_t)1 << (bit % 8);
  filter->bits[byte] |= (uint8_t)mask;
}

static bool bloom_get_bit(const BloomFilter *filter, size_t bit) {
  size_t byte = bit / 8;
  size_t mask = (size_t)1 << (bit % 8);
  return (filter->bits[byte] & (uint8_t)mask) != 0;
}

static BloomFilter *bloom_create_internal(size_t bit_count, size_t hash_count,
                                          double max_load) {
  if (bit_count == 0 || hash_count == 0) {
    return NULL;
  }

  BloomFilter *filter = (BloomFilter *)calloc(1, sizeof(BloomFilter));
  if (!filter) {
    return NULL;
  }

  filter->bit_count = bit_count;
  filter->byte_count = (bit_count + 7) / 8;
  filter->hash_count = hash_count;
  filter->max_load = max_load;
  filter->bits = (uint8_t *)calloc(filter->byte_count, 1);
  if (!filter->bits) {
    free(filter);
    return NULL;
  }

  return filter;
}

BloomFilter *bloom_create(size_t bit_count, size_t hash_count) {
  return bloom_create_internal(bit_count, hash_count, 0.0);
}

BloomFilter *bloom_create_scaling(size_t bit_count, size_t hash_count,
                                  double max_load_factor) {
  if (max_load_factor < 0.01) {
    max_load_factor = 0.01;
  }
  return bloom_create_internal(bit_count, hash_count, max_load_factor);
}

void bloom_destroy(BloomFilter *filter) {
  while (filter) {
    BloomFilter *next = filter->next;
    free(filter->bits);
    free(filter);
    filter = next;
  }
}

static BloomFilter *bloom_select_target(BloomFilter *filter) {
  if (!filter || filter->max_load <= 0.0) {
    return filter;
  }

  double load = (double)filter->item_count / (double)filter->bit_count;
  if (load <= filter->max_load) {
    return filter;
  }

  if (!filter->next) {
    filter->next = bloom_create_internal(filter->bit_count * 2,
                                         filter->hash_count, filter->max_load);
  }

  return bloom_select_target(filter->next);
}

void bloom_insert(BloomFilter *filter, const void *data, size_t length) {
  if (!filter || !data || length == 0) {
    return;
  }

  BloomFilter *target = bloom_select_target(filter);
  if (!target) {
    return;
  }

  uint64_t h1 = hash_fnv1a(data, length);
  uint64_t h2 = hash_mix(h1);
  for (size_t i = 0; i < target->hash_count; ++i) {
    size_t bit = (h1 + i * h2) % target->bit_count;
    bloom_set_bit(target, bit);
  }

  target->item_count++;
}

bool bloom_test(const BloomFilter *filter, const void *data, size_t length) {
  if (!filter || !data || length == 0) {
    return false;
  }

  uint64_t h1 = hash_fnv1a(data, length);
  uint64_t h2 = hash_mix(h1);

  for (const BloomFilter *current = filter; current; current = current->next) {
    bool present = true;
    for (size_t i = 0; i < current->hash_count; ++i) {
      size_t bit = (h1 + i * h2) % current->bit_count;
      if (!bloom_get_bit(current, bit)) {
        present = false;
        break;
      }
    }
    if (present) {
      return true;
    }
  }

  return false;
}

bool bloom_self_test(void) {
  BloomFilter *filter = bloom_create_scaling(128, 3, 0.2);
  if (!filter) {
    return false;
  }

  const char *value = "test-key";
  bloom_insert(filter, value, strlen(value));
  bool result = bloom_test(filter, value, strlen(value));

  bloom_destroy(filter);
  return result;
}
