#include "animation_blender.h"

static void decompose_matrix(simd_float4x4 matrix, simd_float3* pos, simd_quatf* rot, simd_float3* scale) {
    // Extract translation
    *pos = matrix.columns[3].xyz;
    
    // Extract scale (length of columns)
    scale->x = simd_length(matrix.columns[0].xyz);
    scale->y = simd_length(matrix.columns[1].xyz);
    scale->z = simd_length(matrix.columns[2].xyz);
    
    // Extract rotation (columns divided by scale)
    simd_float3x3 rot_mat;
    rot_mat.columns[0] = matrix.columns[0].xyz / scale->x;
    rot_mat.columns[1] = matrix.columns[1].xyz / scale->y;
    rot_mat.columns[2] = matrix.columns[2].xyz / scale->z;
    
    *rot = simd_quaternion(rot_mat);
}

static simd_float4x4 compose_matrix(simd_float3 pos, simd_quatf rot, simd_float3 scale) {
    simd_float4x4 rot_matrix = simd_matrix4x4(rot);
    
    simd_float4x4 scale_matrix = (simd_float4x4){ .columns = {
        simd_make_float4(scale.x, 0, 0, 0),
        simd_make_float4(0, scale.y, 0, 0),
        simd_make_float4(0, 0, scale.z, 0),
        simd_make_float4(0, 0, 0, 1)
    }};
    
    simd_float4x4 trans_matrix = matrix_identity_float4x4;
    trans_matrix.columns[3] = simd_make_float4(pos.x, pos.y, pos.z, 1.0f);
    
    return simd_mul(trans_matrix, simd_mul(rot_matrix, scale_matrix));
}

void animation_blend(const simd_float4x4* source_a,
                     const simd_float4x4* source_b,
                     float factor,
                     simd_float4x4* result,
                     uint32_t count) {
    if (!source_a || !source_b || !result || count == 0) return;
    
    float t = factor;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    for (uint32_t i = 0; i < count; ++i) {
        simd_float3 pos_a, scale_a;
        simd_quatf rot_a;
        decompose_matrix(source_a[i], &pos_a, &rot_a, &scale_a);
        
        simd_float3 pos_b, scale_b;
        simd_quatf rot_b;
        decompose_matrix(source_b[i], &pos_b, &rot_b, &scale_b);
        
        simd_float3 pos_res = simd_mix(pos_a, pos_b, t);
        simd_quatf rot_res = simd_slerp(rot_a, rot_b, t);
        simd_float3 scale_res = simd_mix(scale_a, scale_b, t);
        
        result[i] = compose_matrix(pos_res, rot_res, scale_res);
    }
}
