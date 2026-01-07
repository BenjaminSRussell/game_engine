// Quaternion math implementation with full SIMD optimization.
// Features: AVX2/SSE2 support, SLERP/NLERP interpolation, validation, caching.
// Includes comprehensive quaternion operations, statistics tracking, and batch processing.
#include "math/quat.h"
#include "math/mat4.h"
#include <math/math.h>
#include <immintrin.h>  // AVX intrinsics
#include <emmintrin.h>  // SSE intrinsics
#include <string.h>

// SIMD capability detection
static bool g_has_avx2 = false;
static bool g_has_sse2 = false;
static bool g_simd_initialized = false;

// Quaternion validation system
typedef struct {
    bool has_nan;
    bool has_inf;
    bool is_valid;
    bool_is_normalized;
    f32 length;
} QuatValidation;

// Quaternion cache system
#define QUAT_CACHE_SIZE 32
typedef struct {
    Quat quaternions[QUAT_CACHE_SIZE];
    bool used[QUAT_CACHE_SIZE];
    u32 last_used;
} QuatCache;

// Quaternion statistics tracking
typedef struct {
    u64 operations_count;
    u64 simd_operations_count;
    u64 cache_hits;
    u64 cache_misses;
    u64 normalizations_count;
    f64 total_operation_time;
} QuatStatistics;

static QuatCache g_quat_cache = {0};
static QuatStatistics g_quat_stats = {0};

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

// AVX2 optimized quaternion multiplication
Quat quat_mul_avx2(Quat a, Quat b) {
    __m128 qa = _mm_load_ps(&a.w);
    __m128 qb = _mm_load_ps(&b.w);
    
    // Shuffle for quaternion multiplication
    __m128 qaxx = _mm_shuffle_ps(qa, qa, _MM_SHUFFLE(0, 0, 0, 0));  // a.w, a.w, a.w, a.w
    __m128 qayy = _mm_shuffle_ps(qa, qa, _MM_SHUFFLE(1, 1, 1, 1));  // a.x, a.x, a.x, a.x
    __m128 qazz = _mm_shuffle_ps(qa, qa, _MM_SHUFFLE(2, 2, 2, 2));  // a.y, a.y, a.y, a.y
    __m128 qaww = _mm_shuffle_ps(qa, qa, _MM_SHUFFLE(3, 3, 3, 3));  // a.z, a.z, a.z, a.z
    
    __m128 qbaa = _mm_shuffle_ps(qb, qb, _MM_SHUFFLE(2, 3, 0, 1));  // b.z, b.w, b.x, b.y
    __m128 qbbb = _mm_shuffle_ps(qb, qb, _MM_SHUFFLE(3, 2, 1, 0));  // b.w, b.z, b.y, b.x
    __m128 qbcc = _mm_shuffle_ps(qb, qb, _MM_SHUFFLE(0, 1, 2, 3));  // b.x, b.y, b.z, b.w
    __m128 qbdd = _mm_shuffle_ps(qb, qb, _MM_SHUFFLE(1, 0, 3, 2));  // b.y, b.x, b.w, b.z
    
    __m128 mul1 = _mm_mul_ps(qaxx, qb);
    __m128 mul2 = _mm_mul_ps(qayy, qbaa);
    __m128 mul3 = _mm_mul_ps(qazz, qbbb);
    __m128 mul4 = _mm_mul_ps(qaww, qbcc);
    
    __m128 add1 = _mm_add_ps(mul1, mul2);
    __m128 add2 = _mm_add_ps(mul3, mul4);
    __m128 sub1 = _mm_sub_ps(add1, mul4);
    __m128 result = _mm_add_ps(sub1, add2);
    
    Quat out;
    _mm_store_ps(&out.w, result);
    return out;
}

// AVX2 optimized quaternion normalization
Quat quat_normalize_avx2(Quat q) {
    __m128 qq = _mm_load_ps(&q.w);
    __m128 mul = _mm_mul_ps(qq, qq);
    
    // Horizontal add to get squared length
    __m128 shuf = _mm_movehdup_ps(mul);
    __m128 sums = _mm_add_ps(mul, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    
    f32 length_sq = _mm_cvtss_f32(sums);
    
    if (length_sq < EPSILON) {
        return quat_identity();
    }
    
    __m128 length_vec = _mm_set1_ps(sqrtf(length_sq));
    __m128 result = _mm_div_ps(qq, length_vec);
    
    Quat out;
    _mm_store_ps(&out.w, result);
    return out;
}

// AVX2 optimized quaternion-vector rotation
Vec3 quat_rotate_vec3_avx2(Quat q, Vec3 v) {
    // Create quaternion from vector (0, v)
    __m128 v_quat = _mm_set_ps(0.0f, v.z, v.y, v.x);
    
    // Load quaternion q
    __m128 qq = _mm_load_ps(&q.w);
    
    // Create conjugate of q
    __m128 q_conj = _mm_set_ps(q.w, -q.z, -q.y, -q.x);
    
    // Calculate q * v_quat
    __m128 mul1 = _mm_mul_ps(qq, v_quat);
    
    // Calculate (q * v_quat) * q_conjugate
    __m128 mul2 = _mm_mul_ps(mul1, q_conj);
    
    // Extract vector part
    f32 result[4];
    _mm_store_ps(result, mul2);
    
    return vec3(result[1], result[2], result[3]);
}

Quat quat_from_axis_angle(Vec3 axis, f32 angle) {
    f32 half_angle = angle * 0.5f;
    f32 s = sinf(half_angle);
    Vec3 n = vec3_normalize(axis);
    
    return quat(cosf(half_angle), n.x * s, n.y * s, n.z * s);
}

Quat quat_from_euler(f32 pitch, f32 yaw, f32 roll) {
    f32 half_pitch = pitch * 0.5f;
    f32 half_yaw = yaw * 0.5f;
    f32 half_roll = roll * 0.5f;
    
    f32 sp = sinf(half_pitch);
    f32 cp = cosf(half_pitch);
    f32 sy = sinf(half_yaw);
    f32 cy = cosf(half_yaw);
    f32 sr = sinf(half_roll);
    f32 cr = cosf(half_roll);
    
    return quat(
        cr * cp * cy + sr * sp * sy,
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy
    );
}

Quat quat_mul(Quat a, Quat b) {
    return quat(
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
    );
}

Quat quat_normalize(Quat q) {
    f32 len = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (len < EPSILON) return quat_identity();
    f32 inv_len = 1.0f / len;
    return quat(q.w * inv_len, q.x * inv_len, q.y * inv_len, q.z * inv_len);
}

Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
    Quat q_inv = quat(q.w, -q.x, -q.y, -q.z);
    Quat v_quat = quat(0.0f, v.x, v.y, v.z);
    Quat result = quat_mul(quat_mul(q, v_quat), q_inv);
    return vec3(result.x, result.y, result.z);
}

Mat4 quat_to_mat4(Quat q) {
    Quat n = quat_normalize(q);
    f32 x = n.x, y = n.y, z = n.z, w = n.w;
    
    Mat4 m = mat4_identity();
    m.m00 = 1.0f - 2.0f * (y * y + z * z);
    m.m01 = 2.0f * (x * y - z * w);
    m.m02 = 2.0f * (x * z + y * w);
    m.m10 = 2.0f * (x * y + z * w);
    m.m11 = 1.0f - 2.0f * (x * x + z * z);
    m.m12 = 2.0f * (y * z - x * w);
    m.m20 = 2.0f * (x * z - y * w);
    m.m21 = 2.0f * (y * z + x * w);
    m.m22 = 1.0f - 2.0f * (x * x + y * y);
    return m;
}

// Quaternion interpolation system
Quat quat_slerp(Quat a, Quat b, f32 t) {
    // Calculate dot product
    f32 dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    
    // If dot product is negative, negate one quaternion to take shorter path
    if (dot < 0.0f) {
        b.w = -b.w; b.x = -b.x; b.y = -b.y; b.z = -b.z;
        dot = -dot;
    }
    
    // Clamp dot to avoid numerical errors
    dot = MAX(-1.0f, MIN(1.0f, dot));
    
    // Calculate interpolation angle
    f32 angle = acosf(dot);
    
    if (fabsf(angle) < EPSILON) {
        // Quaternions are very close, use linear interpolation
        return quat_nlerp(a, b, t);
    }
    
    f32 sin_angle = sinf(angle);
    f32 inv_sin_angle = 1.0f / sin_angle;
    
    f32 coeff1 = sinf((1.0f - t) * angle) * inv_sin_angle;
    f32 coeff2 = sinf(t * angle) * inv_sin_angle;
    
    return quat(
        coeff1 * a.w + coeff2 * b.w,
        coeff1 * a.x + coeff2 * b.x,
        coeff1 * a.y + coeff2 * b.y,
        coeff1 * a.z + coeff2 * b.z
    );
}

Quat quat_nlerp(Quat a, Quat b, f32 t) {
    Quat result = quat(
        a.w + (b.w - a.w) * t,
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
    return quat_normalize(result);
}

// Additional quaternion operations
Quat quat_conjugate(Quat q) {
    return quat(q.w, -q.x, -q.y, -q.z);
}

Quat quat_inverse(Quat q) {
    f32 length_sq = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if (length_sq < EPSILON) {
        return quat_identity();
    }
    
    f32 inv_length_sq = 1.0f / length_sq;
    Quat conj = quat_conjugate(q);
    
    return quat(conj.w * inv_length_sq, conj.x * inv_length_sq, 
               conj.y * inv_length_sq, conj.z * inv_length_sq);
}

f32 quat_dot(Quat a, Quat b) {
    return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
}

f32 quat_length_sq(Quat q) {
    return quat_dot(q, q);
}

f32 quat_length(Quat q) {
    return sqrtf(quat_length_sq(q));
}

Quat quat_exp(Quat q) {
    f32 angle = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z);
    f32 sin_angle = sinf(angle);
    f32 cos_angle = cosf(angle);
    
    if (angle < EPSILON) {
        return quat(cos_angle, q.x, q.y, q.z);
    }
    
    f32 coeff = sin_angle / angle;
    return quat(cos_angle, q.x * coeff, q.y * coeff, q.z * coeff);
}

Quat quat_log(Quat q) {
    f32 length = quat_length(q);
    if (length < EPSILON) {
        return quat(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    q = quat_normalize(q);
    f32 angle = acosf(q.w);
    
    if (fabsf(angle) < EPSILON) {
        return quat(0.0f, q.x, q.y, q.z);
    }
    
    f32 coeff = angle / sqrtf(1.0f - q.w * q.w);
    return quat(0.0f, q.x * coeff, q.y * coeff, q.z * coeff);
}

// Quaternion validation system
QuatValidation quat_validate(Quat q) {
    QuatValidation result = {0};
    
    // Check for NaN and infinity
    result.has_nan = isnan(q.w) || isnan(q.x) || isnan(q.y) || isnan(q.z);
    result.has_inf = isinf(q.w) || isinf(q.x) || isinf(q.y) || isinf(q.z);
    
    result.is_valid = !result.has_nan && !result.has_inf;
    
    // Calculate length
    result.length = quat_length(q);
    
    // Check if normalized (with tolerance)
    result.is_normalized = fabsf(result.length - 1.0f) < 0.001f;
    
    return result;
}

bool quat_is_valid(Quat q) {
    QuatValidation validation = quat_validate(q);
    return validation.is_valid;
}

bool quat_has_nan(Quat q) {
    return isnan(q.w) || isnan(q.x) || isnan(q.y) || isnan(q.z);
}

bool quat_has_inf(Quat q) {
    return isinf(q.w) || isinf(q.x) || isinf(q.y) || isinf(q.z);
}

bool quat_is_normalized(Quat q, f32 tolerance) {
    f32 length = quat_length(q);
    return fabsf(length - 1.0f) <= tolerance;
}

bool quat_is_equal(Quat a, Quat b, f32 tolerance) {
    return fabsf(a.w - b.w) <= tolerance &&
           fabsf(a.x - b.x) <= tolerance &&
           fabsf(a.y - b.y) <= tolerance &&
           fabsf(a.z - b.z) <= tolerance;
}

Quat quat_sanitize(Quat q) {
    Quat result = q;
    
    // Replace NaN with 0
    if (isnan(result.w)) result.w = 0.0f;
    if (isnan(result.x)) result.x = 0.0f;
    if (isnan(result.y)) result.y = 0.0f;
    if (isnan(result.z)) result.z = 0.0f;
    
    // Clamp infinity to large values
    if (isinf(result.w)) result.w = result.w > 0 ? FLT_MAX : -FLT_MAX;
    if (isinf(result.x)) result.x = result.x > 0 ? FLT_MAX : -FLT_MAX;
    if (isinf(result.y)) result.y = result.y > 0 ? FLT_MAX : -FLT_MAX;
    if (isinf(result.z)) result.z = result.z > 0 ? FLT_MAX : -FLT_MAX;
    
    return result;
}

// Quaternion cache system
u32 quat_cache_hash(Quat q) {
    u32 hash = 0;
    hash ^= *(u32*)&q.w;
    hash ^= *(u32*)&q.x << 1;
    hash ^= *(u32*)&q.y << 2;
    hash ^= *(u32*)&q.z << 3;
    return hash % QUAT_CACHE_SIZE;
}

bool quat_cache_get(Quat q, Quat *out) {
    u32 index = quat_cache_hash(q);
    
    if (g_quat_cache.used[index] && 
        quat_is_equal(g_quat_cache.quaternions[index], q, EPSILON)) {
        *out = g_quat_cache.quaternions[index];
        g_quat_stats.cache_hits++;
        return true;
    }
    
    g_quat_stats.cache_misses++;
    return false;
}

void quat_cache_put(Quat q) {
    u32 index = quat_cache_hash(q);
    g_quat_cache.quaternions[index] = q;
    g_quat_cache.used[index] = true;
    g_quat_cache.last_used = index;
}

void quat_cache_clear() {
    memset(&g_quat_cache, 0, sizeof(g_quat_cache));
}

// Quaternion statistics system
void quat_stats_reset() {
    memset(&g_quat_stats, 0, sizeof(g_quat_stats));
}

QuatStatistics quat_stats_get() {
    return g_quat_stats;
}

void quat_stats_increment_operation(bool used_simd) {
    g_quat_stats.operations_count++;
    if (used_simd) {
        g_quat_stats.simd_operations_count++;
    }
}

void quat_stats_increment_normalization() {
    g_quat_stats.normalizations_count++;
}

// SIMD support functions
bool quat_has_avx2_support() {
    init_simd_capabilities();
    return g_has_avx2;
}

bool quat_has_sse2_support() {
    init_simd_capabilities();
    return g_has_sse2;
}

// Optimized versions that automatically use SIMD when available
Quat quat_mul_optimized(Quat a, Quat b) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return quat_mul_avx2(a, b);
    }
    return quat_mul(a, b);
}

Quat quat_normalize_optimized(Quat q) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return quat_normalize_avx2(q);
    }
    return quat_normalize(q);
}

Vec3 quat_rotate_vec3_optimized(Quat q, Vec3 v) {
    init_simd_capabilities();
    if (g_has_avx2) {
        return quat_rotate_vec3_avx2(q, v);
    }
    return quat_rotate_vec3(q, v);
}

// Debug visualization helpers
const char* quat_to_string(Quat q, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "Quat(%.3f, %.3f, %.3f, %.3f)", q.w, q.x, q.y, q.z);
    return buffer;
}

void quat_print(Quat q) {
    char buffer[64];
    printf("%s\n", quat_to_string(q, buffer, sizeof(buffer)));
}

// Batch operations
void quat_mul_batch_avx2(const Quat *a, const Quat *b, Quat *result, size_t count) {
    init_simd_capabilities();
    if (!g_has_avx2) {
        // Fallback to scalar implementation
        for (size_t i = 0; i < count; i++) {
            result[i] = quat_mul(a[i], b[i]);
        }
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        result[i] = quat_mul_avx2(a[i], b[i]);
    }
}

void quat_normalize_batch_avx2(const Quat *input, Quat *output, size_t count) {
    init_simd_capabilities();
    if (!g_has_avx2) {
        // Fallback to scalar implementation
        for (size_t i = 0; i < count; i++) {
            output[i] = quat_normalize(input[i]);
        }
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        output[i] = quat_normalize_avx2(input[i]);
    }
}

void quat_rotate_vec3_batch(const Quat *quaternions, const Vec3 *vectors, Vec3 *result, size_t count) {
    for (size_t i = 0; i < count; i++) {
        result[i] = quat_rotate_vec3_optimized(quaternions[i], vectors[i]);
    }
}

