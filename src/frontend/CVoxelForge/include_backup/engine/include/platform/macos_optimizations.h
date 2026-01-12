#ifndef MACOS_OPTIMIZATIONS_H
#define MACOS_OPTIMIZATIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// =================================================================================================
//                                   MACOS-SPECIFIC OPTIMIZATIONS API
// =================================================================================================

// Forward declarations
typedef float f32;
typedef double f64;
typedef uint64_t u64;
typedef size_t size_t;
typedef uint32_t u32;

// MARK: - ProMotion Display API

bool macos_promotion_init(void (*frame_callback)(f64, void*), void* user_data);
void macos_promotion_shutdown(void);
f64 macos_promotion_get_refresh_rate(void);
bool macos_promotion_is_variable_refresh_enabled(void);
void macos_promotion_set_target_refresh_rate(f64 target_rate);

// MARK: - HDR Display API

bool macos_hdr_init(void);
void macos_hdr_shutdown(void);
bool macos_hdr_is_supported(void);
void macos_hdr_set_enabled(bool enabled);
bool macos_hdr_is_enabled(void);
void macos_hdr_get_luminance_range(f32* min_luminance, f32* max_luminance);

// MARK: - Unified Memory API

bool macos_unified_memory_init(void);
void macos_unified_memory_shutdown(void);
void* macos_unified_memory_alloc_shared(size_t size);
void macos_unified_memory_free_shared(void* ptr, size_t size);
bool macos_unified_memory_is_available(void);

// MARK: - GCD Integration API

bool macos_gcd_init(void);
void macos_gcd_shutdown(void);
void macos_gcd_async_user_interactive(void (^block)(void));
void macos_gcd_async_background(void (^block)(void));
void macos_gcd_async_utility(void (^block)(void));
void macos_gcd_async_rendering(void (^block)(void));
void macos_gcd_apply(size_t count, void (^block)(size_t));

// MARK: - Activity Tracking API

void macos_activity_begin(const char* activity_name);
void macos_activity_end(void);

// MARK: - Performance Monitoring API

void macos_update_memory_usage(void);
u64 macos_get_available_memory(void);
u64 macos_get_total_memory(void);

// MARK: - Accelerate Framework API

void macos_vector_add_simd(const f32* a, const f32* b, f32* result, size_t count);
void macos_vector_multiply_simd(const f32* a, const f32* b, f32* result, size_t count);
void macos_vector_sin_simd(f32* angles, f32* results, size_t count);
void macos_vector_cos_simd(f32* angles, f32* results, size_t count);

#endif // MACOS_OPTIMIZATIONS_H
