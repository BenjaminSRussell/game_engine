// include/core/utils.h
//
// Purpose: Provides a diverse collection of general-purpose utility functions
// for common operations across the game engine. This header includes
// mathematical helpers, bit manipulation functions, random number generation,
// timing utilities, array operations, and basic error handling mechanisms.
//
// Public APIs:
// - Math utilities: `clamp`, `lerp`, `smoothstep` for interpolation,
// `distance_squared`,
//   `distance` for 2D geometry, `next_power_of_two`, `round_up` for alignment,
//   and `is_power_of_two` for checking power-of-two values.
// - Bit utilities: `bit_is_set`, `bit_set`, `bit_clear` for individual bit
// manipulation,
//   and `bit_count` for counting set bits.
// - Random utilities: `RandomState` structure and functions (`random_init`,
// `random_next`,
//   `random_float`, `random_range`) for pseudo-random number generation.
// - Time utilities: `StopWatch` structure and functions (`get_time_ns`,
// `stopwatch_start`,
//   `stopwatch_stop`, `stopwatch_elapsed_ms`) for high-resolution timing.
// - Array utilities: `array_shuffle` for randomizing array elements and
// `array_bsearch`
//   for binary searching a sorted array.
// - Error handling: `ErrorCode` enumeration for standardized error reporting
// and
//   `error_code_to_string` for converting error codes to human-readable
//   strings.
//
// Ownership: Most utility functions operate on their input parameters and do
// not manage persistent data or dynamically allocated memory themselves, beyond
// temporary variables. `RandomState` and `StopWatch` are simple value types.
//
// Invariants:
// - Math functions assume standard floating-point arithmetic.
// - Bit manipulation functions operate on unsigned integers.
// - Random number generation requires `RandomState` to be initialized with a
// seed.
// - Array utility functions require valid array pointers, counts, element
// sizes, and comparison functions.
// - `ErrorCode` values provide a consistent way to report operation outcomes.
//
#ifndef UTILS_H
#define UTILS_H

#include "../common.h"
#include <math.h>

// Math utilities
static inline f32 clamp(f32 value, f32 min, f32 max) {
  return value < min ? min : (value > max ? max : value);
}

static inline f32 lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }

static inline f32 smoothstep(f32 a, f32 b, f32 t) {
  f32 x = clamp((t - a) / (b - a), 0.0f, 1.0f);
  return x * x * (3.0f - 2.0f * x);
}

static inline f32 distance_squared(f32 x1, f32 y1, f32 x2, f32 y2) {
  f32 dx = x2 - x1;
  f32 dy = y2 - y1;
  return dx * dx + dy * dy;
}

static inline f32 distance(f32 x1, f32 y1, f32 x2, f32 y2) {
  return sqrtf(distance_squared(x1, y1, x2, y2));
}

static inline u32 next_power_of_two(u32 v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return v + 1;
}

static inline u32 round_up(u32 v, u32 alignment) {
  return (v + alignment - 1) & ~(alignment - 1);
}

static inline bool is_power_of_two(u32 v) {
  return v > 0 && (v & (v - 1)) == 0;
}

// Bit utilities
static inline bool bit_is_set(u32 value, u32 bit) {
  return (value & (1 << bit)) != 0;
}

static inline void bit_set(u32 *value, u32 bit) { *value |= (1 << bit); }

static inline void bit_clear(u32 *value, u32 bit) { *value &= ~(1 << bit); }

static inline u32 bit_count(u32 value) {
  u32 count = 0;
  while (value) {
    count += value & 1;
    value >>= 1;
  }
  return count;
}

// Random utilities
typedef struct {
  u32 state;
} RandomState;

static inline RandomState random_init(u32 seed) {
  RandomState state = {seed};
  return state;
}

static inline u32 random_next(RandomState *state) {
  state->state = state->state * 1103515245 + 12345;
  return (state->state / 65536) % 32768;
}

static inline f32 random_float(RandomState *state) {
  return (f32)random_next(state) / 32768.0f;
}

static inline i32 random_range(RandomState *state, i32 min, i32 max) {
  return min + (i32)((f32)random_next(state) / 32768.0f * (max - min));
}

// Time utilities
typedef struct {
  u64 start_ns;
  u64 end_ns;
} StopWatch;

u64 get_time_ns(void);
f64 get_time(void);
StopWatch stopwatch_start(void);
u64 stopwatch_stop(StopWatch *watch);
f64 stopwatch_elapsed_ms(StopWatch *watch);

// Array utilities
void array_shuffle(void *array, u32 count, u32 element_size);
int array_bsearch(void *array, u32 count, u32 element_size, void *key,
                  int (*compare)(const void *, const void *));

// Error handling
typedef enum {
  ERROR_NONE = 0,
  ERROR_NULL_POINTER = 1,
  ERROR_OUT_OF_MEMORY = 2,
  ERROR_INVALID_ARGUMENT = 3,
  ERROR_NOT_FOUND = 4,
  ERROR_FILE_NOT_FOUND = 5,
  ERROR_PERMISSION_DENIED = 6,
  ERROR_UNKNOWN = 7
} ErrorCode;

const char *error_code_to_string(ErrorCode code);

#endif
