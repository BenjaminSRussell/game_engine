#include "vertex_compression.h"
#include <math.h>
#include <core/logger.h>

/* ===== OCTAHEDRAL NORMAL ENCODING ===== */

// Helper: Sign-preserving normalization
static inline f32 sign_not_zero(f32 v) {
    return (v >= 0.0f) ? 1.0f : -1.0f;
}

u16 compress_normal_oct16(Vec3 normal) {
    // Normalize
    f32 len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len < 0.0001f) {
        return 0; // Degenerate normal
    }
    normal.x /= len;
    normal.y /= len;
    normal.z /= len;
    
    // Project to octahedron
    f32 l1_norm = fabsf(normal.x) + fabsf(normal.y) + fabsf(normal.z);
    f32 px = normal.x / l1_norm;
    f32 py = normal.y / l1_norm;
    
    // Fold negative hemisphere
    if (normal.z < 0.0f) {
        f32 temp_x = (1.0f - fabsf(py)) * sign_not_zero(px);
        f32 temp_y = (1.0f - fabsf(px)) * sign_not_zero(py);
        px = temp_x;
        py = temp_y;
    }
    
    // Map [-1, 1] to [0, 255]
    u8 x = (u8)((px * 0.5f + 0.5f) * 255.0f);
    u8 y = (u8)((py * 0.5f + 0.5f) * 255.0f);
    
    return (u16)x | ((u16)y << 8);
}

Vec3 decompress_normal_oct16(u16 encoded) {
    // Extract components
    u8 x = (u8)(encoded & 0xFF);
    u8 y = (u8)((encoded >> 8) & 0xFF);
    
    // Map [0, 255] to [-1, 1]
    f32 px = ((f32)x / 255.0f) * 2.0f - 1.0f;
    f32 py = ((f32)y / 255.0f) * 2.0f - 1.0f;
    
    // Reconstruct z
    f32 pz = 1.0f - fabsf(px) - fabsf(py);
    
    // Unfold negative hemisphere
    if (pz < 0.0f) {
        f32 temp_x = (1.0f - fabsf(py)) * sign_not_zero(px);
        f32 temp_y = (1.0f - fabsf(px)) * sign_not_zero(py);
        px = temp_x;
        py = temp_y;
    }
    
    // Normalize
    Vec3 result = {px, py, pz};
    f32 len = sqrtf(px * px + py * py + pz * pz);
    if (len > 0.0001f) {
        result.x /= len;
        result.y /= len;
        result.z /= len;
    }
    
    return result;
}

u32 compress_normal_oct32(Vec3 normal) {
    // Similar to oct16 but with 16-bit per component
    f32 len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len < 0.0001f) {
        return 0;
    }
    normal.x /= len;
    normal.y /= len;
    normal.z /= len;
    
    f32 l1_norm = fabsf(normal.x) + fabsf(normal.y) + fabsf(normal.z);
    f32 px = normal.x / l1_norm;
    f32 py = normal.y / l1_norm;
    
    if (normal.z < 0.0f) {
        f32 temp_x = (1.0f - fabsf(py)) * sign_not_zero(px);
        f32 temp_y = (1.0f - fabsf(px)) * sign_not_zero(py);
        px = temp_x;
        py = temp_y;
    }
    
    u16 x = (u16)((px * 0.5f + 0.5f) * 65535.0f);
    u16 y = (u16)((py * 0.5f + 0.5f) * 65535.0f);
    
    return (u32)x | ((u32)y << 16);
}

Vec3 decompress_normal_oct32(u32 encoded) {
    u16 x = (u16)(encoded & 0xFFFF);
    u16 y = (u16)((encoded >> 16) & 0xFFFF);
    
    f32 px = ((f32)x / 65535.0f) * 2.0f - 1.0f;
    f32 py = ((f32)y / 65535.0f) * 2.0f - 1.0f;
    
    f32 pz = 1.0f - fabsf(px) - fabsf(py);
    
    if (pz < 0.0f) {
        f32 temp_x = (1.0f - fabsf(py)) * sign_not_zero(px);
        f32 temp_y = (1.0f - fabsf(px)) * sign_not_zero(py);
        px = temp_x;
        py = temp_y;
    }
    
    Vec3 result = {px, py, pz};
    f32 len = sqrtf(px * px + py * py + pz * pz);
    if (len > 0.0001f) {
        result.x /= len;
        result.y /= len;
        result.z /= len;
    }
    
    return result;
}

/* ===== TANGENT SPACE COMPRESSION ===== */

u32 compress_tangent(Vec3 tangent, f32 bitangent_sign) {
    // Use oct16 for tangent direction
    u16 encoded_tangent = compress_normal_oct16(tangent);
    
    // Pack sign in upper 16 bits (we only need 1 bit, so plenty of room for future use)
    u16 sign_bits = (bitangent_sign >= 0.0f) ? 0 : 1;
    
    return (u32)encoded_tangent | ((u32)sign_bits << 16);
}

Vec4 decompress_tangent(u32 encoded) {
    u16 encoded_tangent = (u16)(encoded & 0xFFFF);
    u16 sign_bits = (u16)((encoded >> 16) & 0xFFFF);
    
    Vec3 tangent = decompress_normal_oct16(encoded_tangent);
    f32 bitangent_sign = (sign_bits & 1) ? -1.0f : 1.0f;
    
    Vec4 result = {tangent.x, tangent.y, tangent.z, bitangent_sign};
    return result;
}

/* ===== POSITION QUANTIZATION ===== */

QuantizedPosition compress_position(Vec3 position, BoundingBox bounds) {
    // Normalize position to [0, 1] within bounding box
    f32 nx = (position.x - bounds.min.x) / (bounds.max.x - bounds.min.x);
    f32 ny = (position.y - bounds.min.y) / (bounds.max.y - bounds.min.y);
    f32 nz = (position.z - bounds.min.z) / (bounds.max.z - bounds.min.z);
    
    // Clamp and convert to u16
    QuantizedPosition result;
    result.x = (u16)(fminf(fmaxf(nx, 0.0f), 1.0f) * 65535.0f);
    result.y = (u16)(fminf(fmaxf(ny, 0.0f), 1.0f) * 65535.0f);
    result.z = (u16)(fminf(fmaxf(nz, 0.0f), 1.0f) * 65535.0f);
    
    return result;
}

Vec3 decompress_position(QuantizedPosition qpos, BoundingBox bounds) {
    // Convert back to [0, 1]
    f32 nx = (f32)qpos.x / 65535.0f;
    f32 ny = (f32)qpos.y / 65535.0f;
    f32 nz = (f32)qpos.z / 65535.0f;
    
    // Scale back to world space
    Vec3 result;
    result.x = bounds.min.x + nx * (bounds.max.x - bounds.min.x);
    result.y = bounds.min.y + ny * (bounds.max.y - bounds.min.y);
    result.z = bounds.min.z + nz * (bounds.max.z - bounds.min.z);
    
    return result;
}

/* ===== UV COMPRESSION ===== */

u32 compress_uv(f32 u, f32 v) {
    // Clamp to [0, 1] and convert to 16-bit fixed point
    u16 iu = (u16)(fminf(fmaxf(u, 0.0f), 1.0f) * 65535.0f);
    u16 iv = (u16)(fminf(fmaxf(v, 0.0f), 1.0f) * 65535.0f);
    
    return (u32)iu | ((u32)iv << 16);
}

void decompress_uv(u32 encoded, f32* out_u, f32* out_v) {
    u16 iu = (u16)(encoded & 0xFFFF);
    u16 iv = (u16)((encoded >> 16) & 0xFFFF);
    
    *out_u = (f32)iu / 65535.0f;
    *out_v = (f32)iv / 65535.0f;
}

/* ===== COLOR COMPRESSION ===== */

u32 compress_color_rgba8(Vec4 color) {
    u8 r = (u8)(fminf(fmaxf(color.x, 0.0f), 1.0f) * 255.0f);
    u8 g = (u8)(fminf(fmaxf(color.y, 0.0f), 1.0f) * 255.0f);
    u8 b = (u8)(fminf(fmaxf(color.z, 0.0f), 1.0f) * 255.0f);
    u8 a = (u8)(fminf(fmaxf(color.w, 0.0f), 1.0f) * 255.0f);
    
    return (u32)r | ((u32)g << 8) | ((u32)b << 16) | ((u32)a << 24);
}

Vec4 decompress_color_rgba8(u32 encoded) {
    Vec4 result;
    result.x = (f32)((encoded >> 0) & 0xFF) / 255.0f;
    result.y = (f32)((encoded >> 8) & 0xFF) / 255.0f;
    result.z = (f32)((encoded >> 16) & 0xFF) / 255.0f;
    result.w = (f32)((encoded >> 24) & 0xFF) / 255.0f;
    
    return result;
}
