#include "mat4.h"
#include <math.h>

mat4_t mat4_identity(void) {
    mat4_t res = {0};
    res.e[0][0] = 1.0f;
    res.e[1][1] = 1.0f;
    res.e[2][2] = 1.0f;
    res.e[3][3] = 1.0f;
    return res;
}

mat4_t mat4_zero(void) {
    mat4_t res = {0};
    return res;
}

// SIMD optimized matrix multiplication (Column-Major)
mat4_t mat4_mul(mat4_t a, mat4_t b) {
    mat4_t out;
    
    for (int i = 0; i < 4; i++) {
#if defined(RENDERING_SIMD_SSE)
        __m128 v_x = _mm_set1_ps(b.e[i][0]);
        __m128 v_y = _mm_set1_ps(b.e[i][1]);
        __m128 v_z = _mm_set1_ps(b.e[i][2]);
        __m128 v_w = _mm_set1_ps(b.e[i][3]);
        
        __m128 res = _mm_mul_ps(a.columns[0], v_x);
        res = _mm_add_ps(res, _mm_mul_ps(a.columns[1], v_y));
        res = _mm_add_ps(res, _mm_mul_ps(a.columns[2], v_z));
        res = _mm_add_ps(res, _mm_mul_ps(a.columns[3], v_w));
        out.columns[i] = res;
#elif defined(RENDERING_SIMD_NEON)
        float32x4_t v_x = vdupq_n_f32(b.e[i][0]);
        float32x4_t v_y = vdupq_n_f32(b.e[i][1]);
        float32x4_t v_z = vdupq_n_f32(b.e[i][2]);
        float32x4_t v_w = vdupq_n_f32(b.e[i][3]);
        
        float32x4_t res = vmulq_f32(a.columns[0], v_x);
        res = vaddq_f32(res, vmulq_f32(a.columns[1], v_y));
        res = vaddq_f32(res, vmulq_f32(a.columns[2], v_z));
        res = vaddq_f32(res, vmulq_f32(a.columns[3], v_w));
        out.columns[i] = res;
#else
        for (int r = 0; r < 4; r++) {
            out.e[i][r] = a.e[0][r] * b.e[i][0] +
                         a.e[1][r] * b.e[i][1] +
                         a.e[2][r] * b.e[i][2] +
                         a.e[3][r] * b.e[i][3];
        }
#endif
    }
    
    return out;
}

mat4_t mat4_translate(float x, float y, float z) {
    mat4_t res = mat4_identity();
    res.e[3][0] = x;
    res.e[3][1] = y;
    res.e[3][2] = z;
    return res;
}

mat4_t mat4_scale(float x, float y, float z) {
    mat4_t res = mat4_identity();
    res.e[0][0] = x;
    res.e[1][1] = y;
    res.e[2][2] = z;
    return res;
}

mat4_t mat4_perspective(float fov_y, float aspect, float near_z, float far_z) {
    mat4_t res = {0};
    float half_tan_fov = tanf(fov_y * 0.5f);
    
    res.e[0][0] = 1.0f / (aspect * half_tan_fov);
    res.e[1][1] = 1.0f / half_tan_fov;
    res.e[2][2] = far_z / (near_z - far_z);
    res.e[2][3] = -1.0f;
    res.e[3][2] = -(far_z * near_z) / (far_z - near_z);
    
    return res;
}

mat4_t mat4_look_at(vec3_t eye, vec3_t center, vec3_t up) {
    vec3_t f = vec3_normalize(vec3_sub(center, eye));
    vec3_t s = vec3_normalize(vec3_cross(f, up));
    vec3_t u = vec3_cross(s, f);
    
    mat4_t res = mat4_identity();
    res.e[0][0] = s.x;
    res.e[1][0] = s.y;
    res.e[2][0] = s.z;
    
    res.e[0][1] = u.x;
    res.e[1][1] = u.y;
    res.e[2][1] = u.z;
    
    res.e[0][2] = -f.x;
    res.e[1][2] = -f.y;
    res.e[2][2] = -f.z;
    
    res.e[3][0] = -vec3_dot(s, eye);
    res.e[3][1] = -vec3_dot(u, eye);
    res.e[3][2] = vec3_dot(f, eye);
    
    return res;
}

vec4_t mat4_mul_vec4(mat4_t m, vec4_t v) {
    vec4_t out;
#if defined(RENDERING_SIMD_SSE)
    __m128 v_x = _mm_set1_ps(v.x);
    __m128 v_y = _mm_set1_ps(v.y);
    __m128 v_z = _mm_set1_ps(v.z);
    __m128 v_w = _mm_set1_ps(v.w);
    
    out.simd = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(m.columns[0], v_x), _mm_mul_ps(m.columns[1], v_y)),
        _mm_add_ps(_mm_mul_ps(m.columns[2], v_z), _mm_mul_ps(m.columns[3], v_w))
    );
#elif defined(RENDERING_SIMD_NEON)
    float32x4_t v_x = vdupq_n_f32(v.x);
    float32x4_t v_y = vdupq_n_f32(v.y);
    float32x4_t v_z = vdupq_n_f32(v.z);
    float32x4_t v_w = vdupq_n_f32(v.w);
    
    out.simd = vaddq_f32(
        vaddq_f32(vmulq_f32(m.columns[0], v_x), vmulq_f32(m.columns[1], v_y)),
        vaddq_f32(vmulq_f32(m.columns[2], v_z), vmulq_f32(m.columns[3], v_w))
    );
#else
    out.x = m.e[0][0] * v.x + m.e[1][0] * v.y + m.e[2][0] * v.z + m.e[3][0] * v.w;
    out.y = m.e[0][1] * v.x + m.e[1][1] * v.y + m.e[2][1] * v.z + m.e[3][1] * v.w;
    out.z = m.e[0][2] * v.x + m.e[1][2] * v.y + m.e[2][2] * v.z + m.e[3][2] * v.w;
    out.w = m.e[0][3] * v.x + m.e[1][3] * v.y + m.e[2][3] * v.z + m.e[3][3] * v.w;
#endif
    return out;
}

mat4_t mat4_transpose(mat4_t m) {
    mat4_t res;
#if defined(RENDERING_SIMD_SSE)
    __m128 tmp0 = _mm_unpacklo_ps(m.columns[0], m.columns[1]);
    __m128 tmp1 = _mm_unpackhi_ps(m.columns[0], m.columns[1]);
    __m128 tmp2 = _mm_unpacklo_ps(m.columns[2], m.columns[3]);
    __m128 tmp3 = _mm_unpackhi_ps(m.columns[2], m.columns[3]);
    
    res.columns[0] = _mm_movelh_ps(tmp0, tmp2);
    res.columns[1] = _mm_movehl_ps(tmp2, tmp0);
    res.columns[2] = _mm_movelh_ps(tmp1, tmp3);
    res.columns[3] = _mm_movehl_ps(tmp3, tmp1);
#elif defined(RENDERING_SIMD_NEON)
    float32x4x2_t q01 = vtrnq_f32(m.columns[0], m.columns[1]);
    float32x4x2_t q23 = vtrnq_f32(m.columns[2], m.columns[3]);
    res.columns[0] = vcombine_f32(vget_low_f32(q01.val[0]), vget_low_f32(q23.val[0]));
    res.columns[1] = vcombine_f32(vget_low_f32(q01.val[1]), vget_low_f32(q23.val[1]));
    res.columns[2] = vcombine_f32(vget_high_f32(q01.val[0]), vget_high_f32(q23.val[0]));
    res.columns[3] = vcombine_f32(vget_high_f32(q01.val[1]), vget_high_f32(q23.val[1]));
#else
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            res.e[i][j] = m.e[j][i];
        }
    }
#endif
    return res;
}

mat4_t mat4_inverse(mat4_t m) {
    float inv[16];
    float det;
    float *e = m.m;

    inv[0] = e[5]  * e[10] * e[15] - 
             e[5]  * e[11] * e[14] - 
             e[9]  * e[6]  * e[15] + 
             e[9]  * e[7]  * e[14] +
             e[13] * e[6]  * e[11] - 
             e[13] * e[7]  * e[10];

    inv[4] = -e[4]  * e[10] * e[15] + 
              e[4]  * e[11] * e[14] + 
              e[8]  * e[6]  * e[15] - 
              e[8]  * e[7]  * e[14] - 
              e[12] * e[6]  * e[11] + 
              e[12] * e[7]  * e[10];

    inv[8] = e[4]  * e[9] * e[15] - 
             e[4]  * e[11] * e[13] - 
             e[8]  * e[5] * e[15] + 
             e[8]  * e[7] * e[13] + 
             e[12] * e[5] * e[11] - 
             e[12] * e[7] * e[9];

    inv[12] = -e[4]  * e[9] * e[14] + 
               e[4]  * e[10] * e[13] +
               e[8]  * e[5] * e[14] - 
               e[8]  * e[6] * e[13] - 
               e[12] * e[5] * e[10] + 
               e[12] * e[6] * e[9];

    inv[1] = -e[1]  * e[10] * e[15] + 
              e[1]  * e[11] * e[14] + 
              e[9]  * e[2] * e[15] - 
              e[9]  * e[3] * e[14] - 
              e[13] * e[2] * e[11] + 
              e[13] * e[3] * e[10];

    inv[5] = e[0]  * e[10] * e[15] - 
             e[0]  * e[11] * e[14] - 
             e[8]  * e[2] * e[15] + 
             e[8]  * e[3] * e[14] + 
             e[12] * e[2] * e[11] - 
             e[12] * e[3] * e[10];

    inv[9] = -e[0]  * e[9] * e[15] + 
              e[0]  * e[11] * e[13] + 
              e[8]  * e[1] * e[15] - 
              e[8]  * e[3] * e[13] - 
              e[12] * e[1] * e[11] + 
              e[12] * e[3] * e[9];

    inv[13] = e[0]  * e[9] * e[14] - 
              e[0]  * e[10] * e[13] - 
              e[8]  * e[1] * e[14] + 
              e[8]  * e[2] * e[13] + 
              e[12] * e[1] * e[10] - 
              e[12] * e[2] * e[9];

    inv[2] = e[1]  * e[6] * e[15] - 
             e[1]  * e[7] * e[14] - 
             e[5]  * e[2] * e[15] + 
             e[5]  * e[3] * e[14] + 
             e[13] * e[2] * e[7] - 
             e[13] * e[3] * e[6];

    inv[6] = -e[0]  * e[6] * e[15] + 
              e[0]  * e[7] * e[14] + 
              e[4]  * e[2] * e[15] - 
              e[4]  * e[3] * e[14] - 
              e[12] * e[2] * e[7] + 
              e[12] * e[3] * e[6];

    inv[10] = e[0]  * e[5] * e[15] - 
              e[0]  * e[7] * e[13] - 
              e[4]  * e[1] * e[15] + 
              e[4]  * e[3] * e[13] + 
              e[12] * e[1] * e[7] - 
              e[12] * e[3] * e[5];

    inv[14] = -e[0]  * e[5] * e[14] + 
               e[0]  * e[6] * e[13] + 
               e[4]  * e[1] * e[14] - 
               e[4]  * e[2] * e[13] - 
               e[12] * e[1] * e[6] + 
               e[12] * e[2] * e[5];

    inv[3] = -e[1] * e[6] * e[11] + 
              e[1] * e[7] * e[10] + 
              e[5] * e[2] * e[11] - 
              e[5] * e[3] * e[10] - 
              e[9] * e[2] * e[7] + 
              e[9] * e[3] * e[6];

    inv[7] = e[0] * e[6] * e[11] - 
             e[0] * e[7] * e[10] - 
             e[4] * e[2] * e[11] + 
             e[4] * e[3] * e[10] + 
             e[8] * e[2] * e[7] - 
             e[8] * e[3] * e[6];

    inv[11] = -e[0] * e[5] * e[11] + 
               e[0] * e[7] * e[9] + 
               e[4] * e[1] * e[11] - 
               e[4] * e[3] * e[9] - 
               e[8] * e[1] * e[7] + 
               e[8] * e[3] * e[5];

    inv[15] = e[0] * e[5] * e[10] - 
              e[0] * e[6] * e[9] - 
              e[4] * e[1] * e[10] + 
              e[4] * e[2] * e[9] + 
              e[8] * e[1] * e[6] - 
              e[8] * e[2] * e[5];

    det = e[0] * inv[0] + e[1] * inv[4] + e[2] * inv[8] + e[3] * inv[12];

    if (det == 0) return mat4_identity();

    det = 1.0f / det;

    mat4_t res;
    for (int i = 0; i < 16; i++) res.m[i] = inv[i] * det;
    return res;
}
