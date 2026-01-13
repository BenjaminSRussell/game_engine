#pragma once

#include "engine/include/common.h"
#include <core/types.h>
#include <math/vec3.h>
#include <math/vec4.h>

/**
 * =================================================================================================
 *                          VERTEX ATTRIBUTE COMPRESSION
 * =================================================================================================
 *
 * Purpose: Compress vertex attributes to reduce memory bandwidth and storage.
 * - Octahedral encoding for normals (12 bytes -> 2 bytes)
 * - Tangent space packing (quaternion encoding)
 * - Position quantization with bounding box
 */

/* ===== NORMAL COMPRESSION (Octahedral Encoding) ===== */

/**
 * Encode a normalized Vec3 normal to 16-bit octahedral representation (2 bytes)
 * Returns packed u16 with x in low byte, y in high byte
 */
u16 compress_normal_oct16(Vec3 normal);

/**
 * Decode octahedral 16-bit normal back to Vec3
 */
Vec3 decompress_normal_oct16(u16 encoded);

/**
 * Encode normal to 32-bit octahedral (higher precision, 4 bytes)
 */
u32 compress_normal_oct32(Vec3 normal);
Vec3 decompress_normal_oct32(u32 encoded);

/* ===== TANGENT SPACE COMPRESSION ===== */

/**
 * Pack tangent + bitangent sign into 32-bit value
 * Uses octahedral encoding for tangent, 1 bit for bitangent sign
 */
u32 compress_tangent(Vec3 tangent, f32 bitangent_sign);

/**
 * Decompress tangent. Returns tangent vector and bitangent_sign in w component
 */
Vec4 decompress_tangent(u32 encoded);

/* ===== POSITION QUANTIZATION ===== */

/**
 * Quantize position to 16-bit per component relative to bounding box
 */
typedef struct {
    Vec3 min;
    Vec3 max;
} BoundingBox;

typedef struct {
    u16 x, y, z;
} QuantizedPosition;

QuantizedPosition compress_position(Vec3 position, BoundingBox bounds);
Vec3 decompress_position(QuantizedPosition qpos, BoundingBox bounds);

/* ===== UV COMPRESSION ===== */

/**
 * Pack UV coordinates to 16-bit fixed point (0..1 range)
 */
u32 compress_uv(f32 u, f32 v);
void decompress_uv(u32 encoded, f32* out_u, f32* out_v);

/* ===== COLOR COMPRESSION ===== */

/**
 * Pack RGBA color to 32-bit (8 bits per channel)
 */
u32 compress_color_rgba8(Vec4 color);
Vec4 decompress_color_rgba8(u32 encoded);
