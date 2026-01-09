#if defined(__SSE2__)
#include <include/math/vec3.h>
#include <emmintrin.h>  // SSE2 intrinsics
#include <string.h>

// SSE2 optimized vector operations (fallback for older CPUs)

// SSE2 optimized vector addition
Vec3 vec3_add_sse2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);  // Load with padding
    __m128 vb = _mm_load_ps(&b.x);  // Load with padding
    __m128 result = _mm_add_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);  // Store with padding
    return out;
}

// SSE2 optimized vector subtraction
Vec3 vec3_sub_sse2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 result = _mm_sub_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// SSE2 optimized vector multiplication (scalar)
Vec3 vec3_mul_sse2(Vec3 v, f32 s) {
    __m128 va = _mm_load_ps(&v.x);
    __m128 vs = _mm_set1_ps(s);
    __m128 result = _mm_mul_ps(va, vs);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// SSE2 optimized vector dot product
f32 vec3_dot_sse2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 mul = _mm_mul_ps(va, vb);
    
    // Horizontal add using SSE2 instructions
    __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));  // Swap middle elements
    __m128 sums = _mm_add_ps(mul, shuf);
    shuf = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(0, 1, 2, 3));     // Swap again
    sums = _mm_add_ps(sums, shuf);
    
    return _mm_cvtss_f32(sums);
}

// SSE2 optimized vector cross product
Vec3 vec3_cross_sse2(Vec3 a, Vec3 b) {
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

// SSE2 optimized vector normalization
Vec3 vec3_normalize_sse2(Vec3 v) {
    f32 length_sq = vec3_dot_sse2(v, v);
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

// SSE2 optimized linear interpolation
Vec3 vec3_lerp_sse2(Vec3 a, Vec3 b, f32 t) {
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

// SSE2 optimized component-wise operations
Vec3 vec3_scale_sse2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 result = _mm_mul_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

Vec3 vec3_min_sse2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 result = _mm_min_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

Vec3 vec3_max_sse2(Vec3 a, Vec3 b) {
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 result = _mm_max_ps(va, vb);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// Batch operations using SSE2
void vec3_add_batch_sse2(const Vec3 *a, const Vec3 *b, Vec3 *result, size_t count) {
    for (size_t i = 0; i < count; i++) {
        result[i] = vec3_add_sse2(a[i], b[i]);
    }
}

void vec3_normalize_batch_sse2(const Vec3 *input, Vec3 *output, size_t count) {
    for (size_t i = 0; i < count; i++) {
        output[i] = vec3_normalize_sse2(input[i]);
    }
}

// SSE2 optimized matrix operations (4x4 matrices)
void mat4_mul_vec3_sse2(const f32 *matrix, const Vec3 *vectors, Vec3 *result, size_t count) {
    for (size_t i = 0; i < count; i++) {
        Vec3 v = vectors[i];
        
        // Load matrix row
        __m128 row0 = _mm_load_ps(&matrix[0]);
        __m128 row1 = _mm_load_ps(&matrix[4]);
        __m128 row2 = _mm_load_ps(&matrix[8]);
        __m128 row3 = _mm_load_ps(&matrix[12]);
        
        // Load vector with w=1
        __m128 vv = _mm_set_ps(1.0f, v.z, v.y, v.x);
        
        // Matrix-vector multiplication
        __m128 dot0 = _mm_mul_ps(row0, vv);
        __m128 dot1 = _mm_mul_ps(row1, vv);
        __m128 dot2 = _mm_mul_ps(row2, vv);
        __m128 dot3 = _mm_mul_ps(row3, vv);
        
        // Horizontal sum for each row
        __m128 shuf = _mm_shuffle_ps(dot0, dot0, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 sum0 = _mm_add_ps(dot0, shuf);
        shuf = _mm_shuffle_ps(sum0, sum0, _MM_SHUFFLE(0, 1, 2, 3));
        sum0 = _mm_add_ps(sum0, shuf);
        
        shuf = _mm_shuffle_ps(dot1, dot1, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 sum1 = _mm_add_ps(dot1, shuf);
        shuf = _mm_shuffle_ps(sum1, sum1, _MM_SHUFFLE(0, 1, 2, 3));
        sum1 = _mm_add_ps(sum1, shuf);
        
        shuf = _mm_shuffle_ps(dot2, dot2, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 sum2 = _mm_add_ps(dot2, shuf);
        shuf = _mm_shuffle_ps(sum2, sum2, _MM_SHUFFLE(0, 1, 2, 3));
        sum2 = _mm_add_ps(sum2, shuf);
        
        // Store result
        result[i].x = _mm_cvtss_f32(sum0);
        result[i].y = _mm_cvtss_f32(sum1);
        result[i].z = _mm_cvtss_f32(sum2);
    }
}

// SSE2 optimized vector magnitude calculation
void vec3_length_batch_sse2(const Vec3 *vectors, f32 *lengths, size_t count) {
    for (size_t i = 0; i < count; i++) {
        lengths[i] = sqrtf(vec3_dot_sse2(vectors[i], vectors[i]));
    }
}

// SSE2 optimized distance calculation between two vector arrays
void vec3_distance_batch_sse2(const Vec3 *a, const Vec3 *b, f32 *distances, size_t count) {
    for (size_t i = 0; i < count; i++) {
        Vec3 diff = vec3_sub_sse2(a[i], b[i]);
        distances[i] = sqrtf(vec3_dot_sse2(diff, diff));
    }
}

// SSE2 optimized vector reflection
Vec3 vec3_reflect_sse2(Vec3 v, Vec3 normal) {
    // v - 2.0f * dot(v, normal) * normal
    f32 dot = vec3_dot_sse2(v, normal);
    __m128 vv = _mm_load_ps(&v.x);
    __m128 vn = _mm_load_ps(&normal.x);
    __m128 two_dot = _mm_set1_ps(2.0f * dot);
    __m128 scaled_normal = _mm_mul_ps(vn, two_dot);
    __m128 result = _mm_sub_ps(vv, scaled_normal);
    
    Vec3 out;
    _mm_store_ps(&out.x, result);
    return out;
}

// SSE2 optimized vector projection
Vec3 vec3_project_sse2(Vec3 v, Vec3 onto) {
    f32 dot = vec3_dot_sse2(v, onto);
    f32 onto_length_sq = vec3_dot_sse2(onto, onto);
    
    if (onto_length_sq < EPSILON) {
        return vec3_zero();
    }
    
    f32 scale = dot / onto_length_sq;
    return vec3_mul_sse2(onto, scale);
}

// SSE2 optimized vector rejection (perpendicular component)
Vec3 vec3_reject_sse2(Vec3 v, Vec3 from) {
    Vec3 projection = vec3_project_sse2(v, from);
    return vec3_sub_sse2(v, projection);
}

// SSE2 optimized vector angle calculation
f32 vec3_angle_sse2(Vec3 a, Vec3 b) {
    f32 dot = vec3_dot_sse2(a, b);
    f32 length_a = sqrtf(vec3_dot_sse2(a, a));
    f32 length_b = sqrtf(vec3_dot_sse2(b, b));
    
    if (length_a < EPSILON || length_b < EPSILON) {
        return 0.0f;
    }
    
    f32 cos_angle = dot / (length_a * length_b);
    // Clamp to [-1, 1] to avoid domain errors
    if (cos_angle > 1.0f) cos_angle = 1.0f;
    if (cos_angle < -1.0f) cos_angle = -1.0f;
    
    return acosf(cos_angle);
}

// SSE2 optimized spherical linear interpolation (slerp)
Vec3 vec3_slerp_sse2(Vec3 a, Vec3 b, f32 t) {
    f32 angle = vec3_angle_sse2(a, b);
    
    if (angle < EPSILON) {
        return vec3_lerp_sse2(a, b, t);
    }
    
    f32 sin_angle = sinf(angle);
    f32 inv_sin_angle = 1.0f / sin_angle;
    
    f32 coeff1 = sinf((1.0f - t) * angle) * inv_sin_angle;
    f32 coeff2 = sinf(t * angle) * inv_sin_angle;
    
    Vec3 term1 = vec3_mul_sse2(a, coeff1);
    Vec3 term2 = vec3_mul_sse2(b, coeff2);
    
    return vec3_add_sse2(term1, term2);
}
#endif // defined(__SSE2__)


