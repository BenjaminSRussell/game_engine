#include "core/core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

// ✅ COMPLETED: Bloom Filter Implementation - AGENT_CORE_2
// Probabilistic data structure for fast set membership testing
// False positives possible, false negatives impossible

// Bit array operations for bloom filter
static inline void bloom_set_bit(u64* bits, size_t index) {
    bits[index / 64] |= (1ULL << (index % 64));
}

static inline bool bloom_get_bit(const u64* bits, size_t index) {
    return (bits[index / 64] & (1ULL << (index % 64))) != 0;
}

typedef struct {
    u64* bits;              // Bit array for filter
    size_t size_in_bits;    // Total number of bits
    size_t num_hashes;      // Number of hash functions
    size_t item_count;      // Number of items added
    u32 seed1;              // Seed for first hash function
    u32 seed2;              // Seed for second hash function
} BloomFilter;

// ✅ COMPLETED: FNV-1a Hash Implementation
static u64 bloom_fnv1a_hash(const void* data, size_t len, u64 seed) {
    const u8* bytes = (const u8*)data;
    u64 hash = seed;
    
    // FNV-1a constants
    const u64 fnv_offset_basis = 14695981039346656037ULL;
    const u64 fnv_prime = 1099511628211ULL;
    
    hash ^= fnv_offset_basis;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= fnv_prime;
    }
    
    return hash;
}

// ✅ COMPLETED: Double Hashing Optimization
// Generate k independent hashes from 2 base hashes
static u64 bloom_get_hash(const BloomFilter* filter, const void* data, size_t len, u32 hash_index) {
    u64 h1 = bloom_fnv1a_hash(data, len, filter->seed1);
    u64 h2 = bloom_fnv1a_hash(data, len, filter->seed2);
    
    // Double hashing: hash(i) = (h1 + i * h2) % m
    return (h1 + hash_index * h2) % filter->size_in_bits;
}

// ✅ COMPLETED: Bloom Filter Creation
BloomFilter* bloom_filter_create(size_t expected_items, f32 false_positive_rate) {
    if (expected_items == 0 || false_positive_rate <= 0.0f || false_positive_rate >= 1.0f) {
        return NULL;
    }
    
    // Calculate optimal size and hash count
    // m = -n * ln(p) / ln(2)^2
    // k = (m/n) * ln(2)
    f64 ln_p = log(false_positive_rate);
    f64 ln_2 = log(2.0);
    
    size_t size_in_bits = (size_t)ceil(-expected_items * ln_p / (ln_2 * ln_2));
    size_t num_hashes = (size_t)ceil(((f64)size_in_bits / expected_items) * ln_2);
    
    // Ensure minimum values
    if (size_in_bits < 64) size_in_bits = 64;
    if (num_hashes < 1) num_hashes = 1;
    if (num_hashes > 20) num_hashes = 20;  // Reasonable upper limit
    
    BloomFilter* filter = malloc(sizeof(BloomFilter));
    if (!filter) return NULL;
    
    // Allocate bit array (64-bit words)
    size_t array_size = (size_in_bits + 63) / 64;
    filter->bits = calloc(array_size, sizeof(u64));
    if (!filter->bits) {
        free(filter);
        return NULL;
    }
    
    filter->size_in_bits = size_in_bits;
    filter->num_hashes = num_hashes;
    filter->item_count = 0;
    filter->seed1 = 0x811c9dc5;  // FNV-1a offset basis
    filter->seed2 = 0x01000193;   // FNV-1a prime
    
    return filter;
}

// ✅ COMPLETED: Bloom Filter Add Operation
bool bloom_filter_add(BloomFilter* filter, const void* data, size_t len) {
    if (!filter || !data || len == 0) return false;
    
    // Set bits for all hash functions
    for (size_t i = 0; i < filter->num_hashes; i++) {
        u64 hash = bloom_get_hash(filter, data, len, (u32)i);
        bloom_set_bit(filter->bits, hash);
    }
    
    filter->item_count++;
    return true;
}

// ✅ COMPLETED: Bloom Filter Contains Operation
bool bloom_filter_contains(const BloomFilter* filter, const void* data, size_t len) {
    if (!filter || !data || len == 0) return false;
    
    // Check all hash functions
    for (size_t i = 0; i < filter->num_hashes; i++) {
        u64 hash = bloom_get_hash(filter, data, len, (u32)i);
        if (!bloom_get_bit(filter->bits, hash)) {
            return false;  // Definitely not present
        }
    }
    
    return true;  // Possibly present (may be false positive)
}

// ✅ COMPLETED: Bloom Filter String Helpers
bool bloom_filter_add_string(BloomFilter* filter, const char* str) {
    if (!str) return false;
    return bloom_filter_add(filter, str, strlen(str));
}

bool bloom_filter_contains_string(const BloomFilter* filter, const char* str) {
    if (!str) return false;
    return bloom_filter_contains(filter, str, strlen(str));
}

// ✅ COMPLETED: Bloom Filter Utility Functions
size_t bloom_filter_size(const BloomFilter* filter) {
    return filter ? filter->size_in_bits : 0;
}

size_t bloom_filter_num_hashes(const BloomFilter* filter) {
    return filter ? filter->num_hashes : 0;
}

size_t bloom_filter_item_count(const BloomFilter* filter) {
    return filter ? filter->item_count : 0;
}

// ✅ COMPLETED: Bloom Filter Statistics
f32 bloom_filter_current_false_positive_rate(const BloomFilter* filter) {
    if (!filter || filter->size_in_bits == 0) return 1.0f;
    
    // Current false positive rate based on actual usage
    // p = (1 - e^(-k*n/m))^k
    f64 k = (f64)filter->num_hashes;
    f64 n = (f64)filter->item_count;
    f64 m = (f64)filter->size_in_bits;
    
    f64 exponent = -k * n / m;
    f64 term = 1.0 - exp(exponent);
    f64 rate = pow(term, k);
    
    return (f32)rate;
}

void bloom_filter_print_stats(const BloomFilter* filter) {
    if (!filter) {
        printf("Bloom Filter: NULL\n");
        return;
    }
    
    printf("Bloom Filter Statistics:\n");
    printf("  Size: %zu bits (%.2f KB)\n", filter->size_in_bits, 
           filter->size_in_bits / 8.0 / 1024.0);
    printf("  Hash Functions: %zu\n", filter->num_hashes);
    printf("  Items Added: %zu\n", filter->item_count);
    printf("  Current False Positive Rate: %.6f\n", 
           bloom_filter_current_false_positive_rate(filter));
    
    // Calculate bit density
    size_t array_size = (filter->size_in_bits + 63) / 64;
    size_t set_bits = 0;
    for (size_t i = 0; i < array_size; i++) {
        u64 word = filter->bits[i];
        while (word) {
            set_bits += word & 1;
            word >>= 1;
        }
    }
    
    f64 density = (f64)set_bits / filter->size_in_bits * 100.0;
    printf("  Bit Density: %.2f%%\n", density);
}

// ✅ COMPLETED: Bloom Filter Destruction
void bloom_filter_destroy(BloomFilter* filter) {
    if (!filter) return;
    
    free(filter->bits);
    free(filter);
}

// ✅ COMPLETED: Bloom Filter Serialization
size_t bloom_filter_serialize_size(const BloomFilter* filter) {
    if (!filter) return 0;
    size_t array_size = (filter->size_in_bits + 63) / 64;
    return sizeof(BloomFilter) + array_size * sizeof(u64);
}

bool bloom_filter_serialize(const BloomFilter* filter, void* buffer, size_t buffer_size) {
    if (!filter || !buffer) return false;
    
    size_t required_size = bloom_filter_serialize_size(filter);
    if (buffer_size < required_size) return false;
    
    u8* ptr = (u8*)buffer;
    
    // Copy header
    memcpy(ptr, filter, sizeof(BloomFilter));
    ptr += sizeof(BloomFilter);
    
    // Copy bit array
    size_t array_size = (filter->size_in_bits + 63) / 64;
    memcpy(ptr, filter->bits, array_size * sizeof(u64));
    
    return true;
}

BloomFilter* bloom_filter_deserialize(const void* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < sizeof(BloomFilter)) return NULL;
    
    const u8* ptr = (const u8*)buffer;
    
    // Create copy of header
    BloomFilter* filter = malloc(sizeof(BloomFilter));
    if (!filter) return NULL;
    
    memcpy(filter, ptr, sizeof(BloomFilter));
    ptr += sizeof(BloomFilter);
    
    // Allocate and copy bit array
    size_t array_size = (filter->size_in_bits + 63) / 64;
    filter->bits = malloc(array_size * sizeof(u64));
    if (!filter->bits) {
        free(filter);
        return NULL;
    }
    
    memcpy(filter->bits, ptr, array_size * sizeof(u64));
    
    return filter;
}

// ✅ COMPLETED: Bloom Filter Union and Intersection
bool bloom_filter_union(BloomFilter* result, const BloomFilter* filter1, const BloomFilter* filter2) {
    if (!result || !filter1 || !filter2) return false;
    if (filter1->size_in_bits != filter2->size_in_bits) return false;
    if (filter1->num_hashes != filter2->num_hashes) return false;
    
    size_t array_size = (filter1->size_in_bits + 63) / 64;
    for (size_t i = 0; i < array_size; i++) {
        result->bits[i] = filter1->bits[i] | filter2->bits[i];
    }
    
    result->item_count = filter1->item_count + filter2->item_count;  // Approximation
    return true;
}

bool bloom_filter_intersection(BloomFilter* result, const BloomFilter* filter1, const BloomFilter* filter2) {
    if (!result || !filter1 || !filter2) return false;
    if (filter1->size_in_bits != filter2->size_in_bits) return false;
    if (filter1->num_hashes != filter2->num_hashes) return false;
    
    size_t array_size = (filter1->size_in_bits + 63) / 64;
    for (size_t i = 0; i < array_size; i++) {
        result->bits[i] = filter1->bits[i] & filter2->bits[i];
    }
    
    result->item_count = filter1->item_count < filter2->item_count ? 
                        filter1->item_count : filter2->item_count;  // Approximation
    return true;
}
