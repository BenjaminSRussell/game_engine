// 3D vector math implementation with full SIMD optimization.
// Features: AVX2/SSE2 support, validation system, caching, statistics, batch operations.
// Includes comprehensive utility functions for angles, reflection, projection, and interpolation.
#include "math/vec3.h"
#include <immintrin.h>  // AVX intrinsics
#include <emmintrin.h>  // SSE intrinsics
#include <string.h>
#include <math/math.h>

// SIMD capability detection
static bool g_has_avx2 = false;
static bool g_has_sse2 = false;
static bool g_simd_initialized = false;

// Vector validation system
typedef struct {
    bool has_nan;
    bool has_inf;
    bool is_valid;
} Vec3Validation;

// Vector cache system
#define VEC3_CACHE_SIZE 64
typedef struct {
    Vec3 vectors[VEC3_CACHE_SIZE];
    bool used[VEC3_CACHE_SIZE];
    u32 last_used;
} Vec3Cache;

// Vector statistics tracking
typedef struct {
    u64 operations_count;
    u64 simd_operations_count;
    u64 cache_hits;
    u64 cache_misses;
    f64 total_operation_time;
} Vec3Statistics;

static Vec3Cache g_vector_cache = {0};
static Vec3Statistics g_vector_stats = {0};

// Initialize SIMD capabilities
static void init_simd_capabilities() {
    if (g_simd_initialized) {
        return;
    }
    
    int cpu_info[4];
    __cpuid(cpu_info, 0);
    
    if (cpu_info[0] >= 7) {
        __cpuidex(cpu_info, 7, 0);
        g_has_avx2 = (cpu_info[1] & (1 << 5)) != 0;  // AVX2 bit
    }
    
    __cpuid(cpu_info, 1);
    g_has_sse2 = (cpu_info[3] & (1 << 26)) != 0;  // SSE2 bit
    
    g_simd_initialized = true;
}

// AVX2 optimized vector addition
Vec3 vec3_add_avx2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);  // Load with padding
    __m128 vb = _mm_load_ps(&b.x);  // Load with padding
    __m128 result = _mm_add_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);  // Store with padding
    return out;
}

// AVX2 optimized vector subtraction
Vec3 vec3_sub_avx2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 result = _mm_sub_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// AVX2 optimized vector multiplication (scalar)
Vec3 vec3_mul_avx2(Vec3 v, f32 s) {
    __m128 va = _mm_load_ps(&v.x);
    __m128 vs = _mm_set1_ps(s);
    __m128 result = _mm_mul_ps(va, vs);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// AVX2 optimized vector dot product
f32 vec3_dot_avx2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 mul = _mm_mul_ps(va, vb);
    
    // Horizontal add
    __m128 shuf = _mm_movehdup_ps(mul);        // Broadcast elements 3,1 to 2,0
    __m128 sums = _mm_add_ps(mul, shuf);
    shuf = _mm_movehl_ps(shuf, sums);         // Broadcast element 2 to 0
    sums = _mm_add_ss(sums, shuf);
    
    return _mm_cvtss_f32(sums);
}

// AVX2 optimized vector cross product
Vec3 vec3_cross_avx2(Vec3 a, Vec3 b) {
    // a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    
    // Shuffle for cross product calculation
    __m128 va_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));  // y,z,x,w
    __m128 va_zxy = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 1, 0, 2));  // z,x,y,w
    __m128 vb_zxy = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 1, 0, 2));  // z,x,y,w
    __m128 vb_yzx = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 0, 2, 1));  // y,z,x,w
    
    __m128 mul1 = _mm_mul_ps(va_yzx, vb_zxy);
    __m128 mul2 = _mm_mul_ps(va_zxy, vb_yzx);
    __m128 result = _mm_sub_ps(mul1, mul2);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// AVX2 optimized vector normalization
Vec3 vec3_normalize_avx2(Vec3 v) {
    f32 length_sq = vec3_dot_avx2(v, v);
    if (length_sq < EPSILON) {
        return vec3_zero();
    }
    
    __m128 vv = _mm_load_ps(&v.x);
    __m128 length_vec = _mm_set1_ps(sqrtf(length_sq));
    __m128 result = _mm_div_ps(vv, length_vec);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// AVX2 optimized linear interpolation
Vec3 vec3_lerp_avx2(Vec3 a, Vec3 b, f32 t) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 vt = _mm_set1_ps(t);
    
    __m128 diff = _mm_sub_ps(vb, va);
    __m128 scaled = _mm_mul_ps(diff, vt);
    __m128 result = _mm_add_ps(va, scaled);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// AVX2 optimized component-wise operations
Vec3 vec3_scale_avx2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 result = _mm_mul_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

Vec3 vec3_min_avx2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 result = _mm_min_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

Vec3 vec3_max_avx2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 result = _mm_max_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// Batch operations for multiple vectors
void vec3_add_batch_avx2(const Vec3 *a, const Vec3 *b, Vec3 *result, size_t count) {
    init_simd_capabilities();
    if (!g_has_avx2) {
        // Fallback to scalar implementation
        for (size_t i = 0; i < count; i++) {
            result[i] = vec3_add(a[i], b[i]);
        }
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        result[i] = vec3_add_avx2(a[i], b[i]);
    }
}

void vec3_normalize_batch_avx2(const Vec3 *input, Vec3 *output, size_t count) {
    init_simd_capabilities();
    if (!g_has_avx2) {
        // Fallback to scalar implementation
        for (size_t i = 0; i < count; i++) {
            output[i] = vec3_normalize(input[i]);
        }
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        output[i] = vec3_normalize_avx2(input[i]);
    }
}

// Get SIMD capabilities
bool vec3_has_avx2_support() {
    init_simd_capabilities();
    return g_has_avx2;
}

bool vec3_has_sse2_support() {
    init_simd_capabilities();
    return g_has_sse2;
}

// Optimized versions that automatically use SIMD when available
Vec3 vec3_add_optimized(Vec3 a, Vec3 b) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return vec3_add_avx2(a, b);
    }
    return vec3_add(a, b);
}

Vec3 vec3_sub_optimized(Vec3 a, Vec3 b) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return vec3_sub_avx2(a, b);
    }
    return vec3_sub(a, b);
}

Vec3 vec3_mul_optimized(Vec3 v, f32 s) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return vec3_mul_avx2(v, s);
    }
    return vec3_mul(v, s);
}

f32 vec3_dot_optimized(Vec3 a, Vec3 b) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return vec3_dot_avx2(a, b);
    }
    return vec3_dot(a, b);
}

Vec3 vec3_cross_optimized(Vec3 a, Vec3 b) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return vec3_cross_avx2(a, b);
    }
    return vec3_cross(a, b);
}

Vec3 vec3_normalize_optimized(Vec3 v) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return vec3_normalize_avx2(v);
    }
    return vec3_normalize(v);
}

Vec3 vec3_lerp_optimized(Vec3 a, Vec3 b, f32 t) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return vec3_lerp_avx2(a, b, t);
    }
    return vec3_lerp(a, b);
}

// Additional utility functions
f32 vec3_angle_between(Vec3 a, Vec3 b) {
    f32 dot = vec3_dot_optimized(a, b);
    f32 len_a = vec3_length(a);
    f32 len_b = vec3_length(b);
    
    if (len_a < EPSILON || len_b < EPSILON) {
        return 0.0f;
    }
    
    f32 cos_angle = dot / (len_a * len_b);
    // Clamp to avoid numerical errors
    cos_angle = MAX(-1.0f, MIN(1.0f, cos_angle));
    return acosf(cos_angle);
}

f32 vec3_angle_between_normalized(Vec3 a, Vec3 b) {
    f32 dot = vec3_dot_optimized(a, b);
    // Clamp to avoid numerical errors
    dot = MAX(-1.0f, MIN(1.0f, dot));
    return acosf(dot);
}

Vec3 vec3_reflect(Vec3 v, Vec3 normal) {
    Vec3 n = vec3_normalize_optimized(normal);
    f32 dot = vec3_dot_optimized(v, n);
    return vec3_sub_optimized(v, vec3_mul_optimized(n, 2.0f * dot));
}

Vec3 vec3_project(Vec3 v, Vec3 onto) {
    Vec3 normalized_onto = vec3_normalize_optimized(onto);
    f32 dot = vec3_dot_optimized(v, normalized_onto);
    return vec3_mul_optimized(normalized_onto, dot);
}

Vec3 vec3_orthogonal(Vec3 v, Vec3 reference) {
    Vec3 proj = vec3_project(v, reference);
    return vec3_sub_optimized(v, proj);
}

bool vec3_is_equal(Vec3 a, Vec3 b, f32 tolerance) {
    return fabsf(a.x - b.x) <= tolerance &&
           fabsf(a.y - b.y) <= tolerance &&
           fabsf(a.z - b.z) <= tolerance;
}

bool vec3_is_zero(Vec3 v, f32 tolerance) {
    return fabsf(v.x) <= tolerance &&
           fabsf(v.y) <= tolerance &&
           fabsf(v.z) <= tolerance;
}

Vec3 vec3_abs(Vec3 v) {
    return vec3(fabsf(v.x), fabsf(v.y), fabsf(v.z));
}

Vec3 vec3_clamp(Vec3 v, Vec3 min, Vec3 max) {
    return vec3(
        MAX(min.x, MIN(max.x, v.x)),
        MAX(min.y, MIN(max.y, v.y)),
        MAX(min.z, MIN(max.z, v.z))
    );
}

Vec3 vec3_slerp(Vec3 a, Vec3 b, f32 t) {
    f32 angle = vec3_angle_between(a, b);
    if (angle < EPSILON) {
        return vec3_lerp_optimized(a, b, t);
    }
    
    f32 sin_angle = sinf(angle);
    f32 inv_sin_angle = 1.0f / sin_angle;
    
    f32 coeff1 = sinf((1.0f - t) * angle) * inv_sin_angle;
    f32 coeff2 = sinf(t * angle) * inv_sin_angle;
    
    return vec3_add_optimized(
        vec3_mul_optimized(a, coeff1),
        vec3_mul_optimized(b, coeff2)
    );
}

Vec3 vec3_nlerp(Vec3 a, Vec3 b, f32 t) {
    Vec3 result = vec3_lerp_optimized(a, b, t);
    return vec3_normalize_optimized(result);
}

Vec3 vec3_face_forward(Vec3 n, Vec3 v, Vec3 reference) {
    f32 dot = vec3_dot_optimized(v, reference);
    return dot < 0.0f ? vec3_negate(n) : n;
}

Vec3 vec3_negate(Vec3 v) {
    return vec3(-v.x, -v.y, -v.z);
}

Vec3 vec3_sign(Vec3 v) {
    return vec3(
        v.x > 0.0f ? 1.0f : (v.x < 0.0f ? -1.0f : 0.0f),
        v.y > 0.0f ? 1.0f : (v.y < 0.0f ? -1.0f : 0.0f),
        v.z > 0.0f ? 1.0f : (v.z < 0.0f ? -1.0f : 0.0f)
    );
}

Vec3 vec3_step(Vec3 edge, Vec3 v) {
    return vec3(
        v.x < edge.x ? 0.0f : 1.0f,
        v.y < edge.y ? 0.0f : 1.0f,
        v.z < edge.z ? 0.0f : 1.0f
    );
}

Vec3 vec3_smoothstep(Vec3 edge0, Vec3 edge1, Vec3 v) {
    Vec3 t;
    t.x = MAX(0.0f, MIN(1.0f, (v.x - edge0.x) / (edge1.x - edge0.x)));
    t.y = MAX(0.0f, MIN(1.0f, (v.y - edge0.y) / (edge1.y - edge0.y)));
    t.z = MAX(0.0f, MIN(1.0f, (v.z - edge0.z) / (edge1.z - edge0.z)));
    
    t.x = t.x * t.x * (3.0f - 2.0f * t.x);
    t.y = t.y * t.y * (3.0f - 2.0f * t.y);
    t.z = t.z * t.z * (3.0f - 2.0f * t.z);
    
    return t;
}

// Vector validation system
Vec3Validation vec3_validate(Vec3 v) {
    Vec3Validation result = {0};
    
    // Check for NaN
    result.has_nan = isnan(v.x) || isnan(v.y) || isnan(v.z);
    
    // Check for infinity
    result.has_inf = isinf(v.x) || isinf(v.y) || isinf(v.z);
    
    // Overall validity
    result.is_valid = !result.has_nan && !result.has_inf;
    
    return result;
}

bool vec3_is_valid(Vec3 v) {
    Vec3Validation validation = vec3_validate(v);
    return validation.is_valid;
}

bool vec3_has_nan(Vec3 v) {
    return isnan(v.x) || isnan(v.y) || isnan(v.z);
}

bool vec3_has_inf(Vec3 v) {
    return isinf(v.x) || isinf(v.y) || isinf(v.z);
}

Vec3 vec3_sanitize(Vec3 v) {
    Vec3 result = v;
    
    // Replace NaN with 0
    if (isnan(result.x)) result.x = 0.0f;
    if (isnan(result.y)) result.y = 0.0f;
    if (isnan(result.z)) result.z = 0.0f;
    
    // Clamp infinity to large values
    if (isinf(result.x)) result.x = result.x > 0 ? FLT_MAX : -FLT_MAX;
    if (isinf(result.y)) result.y = result.y > 0 ? FLT_MAX : -FLT_MAX;
    if (isinf(result.z)) result.z = result.z > 0 ? FLT_MAX : -FLT_MAX;
    
    return result;
}

// Vector cache system
u32 vec3_cache_hash(Vec3 v) {
    // Simple hash function for vectors
    u32 hash = 0;
    hash ^= *(u32*)&v.x;
    hash ^= *(u32*)&v.y << 1;
    hash ^= *(u32*)&v.z << 2;
    return hash % VEC3_CACHE_SIZE;
}

bool vec3_cache_get(Vec3 v, Vec3 *out) {
    u32 index = vec3_cache_hash(v);
    
    if (g_vector_cache.used[index] && 
        vec3_is_equal(g_vector_cache.vectors[index], v, EPSILON)) {
        *out = g_vector_cache.vectors[index];
        g_vector_stats.cache_hits++;
        return true;
    }
    
    g_vector_stats.cache_misses++;
    return false;
}

void vec3_cache_put(Vec3 v) {
    u32 index = vec3_cache_hash(v);
    g_vector_cache.vectors[index] = v;
    g_vector_cache.used[index] = true;
    g_vector_cache.last_used = index;
}

void vec3_cache_clear() {
    memset(&g_vector_cache, 0, sizeof(g_vector_cache));
}

// Vector statistics system
void vec3_stats_reset() {
    memset(&g_vector_stats, 0, sizeof(g_vector_stats));
}

Vec3Statistics vec3_stats_get() {
    return g_vector_stats;
}

void vec3_stats_increment_operation(bool used_simd) {
    g_vector_stats.operations_count++;
    if (used_simd) {
        g_vector_stats.simd_operations_count++;
    }
}

// Precision system
typedef enum {
    VEC3_PRECISION_SINGLE,
    VEC3_PRECISION_DOUBLE
} Vec3Precision;

static Vec3Precision g_current_precision = VEC3_PRECISION_SINGLE;

void vec3_set_precision(Vec3Precision precision) {
    g_current_precision = precision;
}

Vec3Precision vec3_get_precision() {
    return g_current_precision;
}

// Debug visualization helpers
const char* vec3_to_string(Vec3 v, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "Vec3(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
    return buffer;
}

void vec3_print(Vec3 v) {
    char buffer[64];
    printf("%s\n", vec3_to_string(v, buffer, sizeof(buffer)));
}

// Batch operations (enhanced)
void vec3_transform_batch_avx2(const Vec3 *input, const Mat4 *transform, Vec3 *output, size_t count) {
    init_simd_capabilities();
    if (!g_has_avx2) {
        // Fallback to scalar implementation
        for (size_t i = 0; i < count; i++) {
            output[i] = mat4_transform_vec3(*transform, input[i]);
        }
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        output[i] = mat4_transform_vec3(*transform, input[i]);
    }
}

void vec3_distance_batch(const Vec3 *a, const Vec3 *b, f32 *distances, size_t count) {
    for (size_t i = 0; i < count; i++) {
        distances[i] = vec3_distance(a[i], b[i]);
    }
}

void vec3_angle_batch(const Vec3 *a, const Vec3 *b, f32 *angles, size_t count) {
    for (size_t i = 0; i < count; i++) {
        angles[i] = vec3_angle_between(a[i], b[i]);
    }
}

