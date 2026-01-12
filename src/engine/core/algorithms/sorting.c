#include <core/algorithms/sorting.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//  COMPLETED: LSD Radix Sort Implementation - AGENT_CORE_2
// Linear time sorting for fixed-size integers and floats
// Uses counting sort as stable subroutine for each digit

#define RADIX_BITS 8
#define RADIX_SIZE (1 << RADIX_BITS)
#define RADIX_MASK (RADIX_SIZE - 1)

//  COMPLETED: Counting sort helper for radix sort
static void counting_sort_radix(u32* array, size_t size, u32 shift) {
    if (size <= 1) return;
    
    u32 count[RADIX_SIZE] = {0};
    u32* output = malloc(size * sizeof(u32));
    if (!output) return;
    
    // Count occurrences of each digit
    for (size_t i = 0; i < size; i++) {
        u32 digit = (array[i] >> shift) & RADIX_MASK;
        count[digit]++;
    }
    
    // Compute prefix sums
    for (size_t i = 1; i < RADIX_SIZE; i++) {
        count[i] += count[i - 1];
    }
    
    // Build output array (stable sort)
    for (size_t i = size; i > 0; i--) {
        u32 digit = (array[i - 1] >> shift) & RADIX_MASK;
        output[--count[digit]] = array[i - 1];
    }
    
    // Copy back to original array
    memcpy(array, output, size * sizeof(u32));
    free(output);
}

//  COMPLETED: LSD Radix Sort for u32
void radix_sort_u32(u32* array, size_t size) {
    if (!array || size <= 1) return;
    
    // Process 4 bytes (32 bits) from least significant to most
    for (u32 shift = 0; shift < 32; shift += RADIX_BITS) {
        counting_sort_radix(array, size, shift);
    }
}

//  COMPLETED: Radix sort for signed integers
void radix_sort_i32(i32* array, size_t size) {
    if (!array || size <= 1) return;
    
    // Convert to unsigned by flipping sign bit
    u32* unsigned_array = (u32*)array;
    for (size_t i = 0; i < size; i++) {
        unsigned_array[i] ^= 0x80000000;
    }
    
    // Sort as unsigned
    radix_sort_u32(unsigned_array, size);
    
    // Restore original values
    for (size_t i = 0; i < size; i++) {
        unsigned_array[i] ^= 0x80000000;
    }
}

//  COMPLETED: Radix sort for 64-bit integers
void radix_sort_u64(u64* array, size_t size) {
    if (!array || size <= 1) return;
    
    u64* output = malloc(size * sizeof(u64));
    if (!output) return;
    
    // Process 8 bytes (64 bits)
    for (u32 shift = 0; shift < 64; shift += RADIX_BITS) {
        u64 count[RADIX_SIZE] = {0};
        
        // Count occurrences
        for (size_t i = 0; i < size; i++) {
            u64 digit = (array[i] >> shift) & RADIX_MASK;
            count[digit]++;
        }
        
        // Prefix sums
        for (size_t i = 1; i < RADIX_SIZE; i++) {
            count[i] += count[i - 1];
        }
        
        // Build output (stable)
        for (size_t i = size; i > 0; i--) {
            u64 digit = (array[i - 1] >> shift) & RADIX_MASK;
            output[--count[digit]] = array[i - 1];
        }
        
        // Copy back
        memcpy(array, output, size * sizeof(u64));
    }
    
    free(output);
}

//  COMPLETED: Radix sort for floats (IEEE 754)
void radix_sort_f32(f32* array, size_t size) {
    if (!array || size <= 1) return;
    
    u32* float_bits = (u32*)array;
    
    // Handle sign bit for floats
    for (size_t i = 0; i < size; i++) {
        // Flip sign bit for positive numbers, keep negative numbers as-is
        if (!(float_bits[i] & 0x80000000)) {
            float_bits[i] ^= 0x80000000;
        } else {
            // For negative numbers, flip all bits except sign
            float_bits[i] ^= 0x7FFFFFFF;
        }
    }
    
    // Sort as unsigned
    radix_sort_u32(float_bits, size);
    
    // Restore original float representation
    for (size_t i = 0; i < size; i++) {
        if (float_bits[i] & 0x80000000) {
            float_bits[i] ^= 0x80000000;
        } else {
            float_bits[i] ^= 0x7FFFFFFF;
        }
    }
}
