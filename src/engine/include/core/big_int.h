// include/core/big_int.h
//
// Purpose: Big integer system for handling very large numbers (inspired by GregTech)
// This system is essential for mod support where quantities can exceed 32/64-bit limits
// Supports arithmetic operations, comparisons, and conversions
//
#ifndef BIG_INT_H
#define BIG_INT_H

#include "../common.h"

// Big integer structure - using a 128-bit or arbitrary precision approach
// For GregTech-style mods, we need to handle numbers like 2^64 and beyond
typedef struct {
    u64 high;   // High 64 bits
    u64 low;    // Low 64 bits
    bool sign;  // true = negative, false = positive
} BigInt;

// Initialize big integers
BigInt big_int_from_u64(u64 value);
BigInt big_int_from_i64(i64 value);
BigInt big_int_from_double(f64 value);
BigInt big_int_zero(void);
BigInt big_int_one(void);

// Conversion back
u64 big_int_to_u64(const BigInt *bi);
i64 big_int_to_i64(const BigInt *bi);
f64 big_int_to_double(const BigInt *bi);
bool big_int_fits_u64(const BigInt *bi);
bool big_int_fits_i64(const BigInt *bi);

// Arithmetic operations
BigInt big_int_add(const BigInt *a, const BigInt *b);
BigInt big_int_sub(const BigInt *a, const BigInt *b);
BigInt big_int_mul(const BigInt *a, const BigInt *b);
BigInt big_int_div(const BigInt *a, const BigInt *b);
BigInt big_int_mod(const BigInt *a, const BigInt *b);
BigInt big_int_pow(const BigInt *base, u32 exponent);

// Comparison operations
bool big_int_eq(const BigInt *a, const BigInt *b);
bool big_int_lt(const BigInt *a, const BigInt *b);
bool big_int_le(const BigInt *a, const BigInt *b);
bool big_int_gt(const BigInt *a, const BigInt *b);
bool big_int_ge(const BigInt *a, const BigInt *b);

// Utility operations
BigInt big_int_abs(const BigInt *bi);
BigInt big_int_negate(const BigInt *bi);
BigInt big_int_min(const BigInt *a, const BigInt *b);
BigInt big_int_max(const BigInt *a, const BigInt *b);

// String conversion (for serialization and display)
char *big_int_to_string(const BigInt *bi, char *buffer, u32 buffer_size);
BigInt big_int_from_string(const char *str);

// Constants for common large values
extern const BigInt BIG_INT_MAX_U64;
extern const BigInt BIG_INT_MIN_I64;
extern const BigInt BIG_INT_MAX_I64;

#endif // BIG_INT_H

