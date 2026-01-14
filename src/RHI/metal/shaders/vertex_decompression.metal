#include <metal_stdlib>
using namespace metal;

/**
 * =================================================================================================
 *                      VERTEX ATTRIBUTE DECOMPRESSION (Metal Shaders)
 * =================================================================================================
 *
 * Shader functions for decompressing vertex attributes on the GPU.
 * These match the compression functions in vertex_compression.c
 */

/* ===== OCTAHEDRAL NORMAL DECODING ===== */

inline float3 decompress_normal_oct16(ushort encoded) {
    // Extract components
    float px = (float(encoded & 0xFF) / 255.0) * 2.0 - 1.0;
    float py = (float((encoded >> 8) & 0xFF) / 255.0) * 2.0 - 1.0;
    
    // Reconstruct z
    float pz = 1.0 - abs(px) - abs(py);
    
    // Unfold negative hemisphere
    if (pz < 0.0) {
        float temp_x = (1.0 - abs(py)) * (px >= 0.0 ? 1.0 : -1.0);
        float temp_y = (1.0 - abs(px)) * (py >= 0.0 ? 1.0 : -1.0);
        px = temp_x;
        py = temp_y;
    }
    
    return normalize(float3(px, py, pz));
}

inline float3 decompress_normal_oct32(uint encoded) {
    float px = (float(encoded & 0xFFFF) / 65535.0) * 2.0 - 1.0;
    float py = (float((encoded >> 16) & 0xFFFF) / 65535.0) * 2.0 - 1.0;
    
    float pz = 1.0 - abs(px) - abs(py);
    
    if (pz < 0.0) {
        float temp_x = (1.0 - abs(py)) * (px >= 0.0 ? 1.0 : -1.0);
        float temp_y = (1.0 - abs(px)) * (py >= 0.0 ? 1.0 : -1.0);
        px = temp_x;
        py = temp_y;
    }
    
    return normalize(float3(px, py, pz));
}

/* ===== TANGENT SPACE DECODING ===== */

inline float4 decompress_tangent(uint encoded) {
    ushort encoded_tangent = ushort(encoded & 0xFFFF);
    ushort sign_bits = ushort((encoded >> 16) & 0xFFFF);
    
    float3 tangent = decompress_normal_oct16(encoded_tangent);
    float bitangent_sign = (sign_bits & 1) ? -1.0 : 1.0;
    
    return float4(tangent, bitangent_sign);
}

/* ===== POSITION QUANTIZATION ===== */

struct BoundingBox {
    float3 min;
    float3 max;
};

inline float3 decompress_position(ushort3 qpos, BoundingBox bounds) {
    float3 normalized = float3(qpos) / 65535.0;
    return bounds.min + normalized * (bounds.max - bounds.min);
}

/* ===== UV DECOMPRESSION ===== */

inline float2 decompress_uv(uint encoded) {
    float u = float(encoded & 0xFFFF) / 65535.0;
    float v = float((encoded >> 16) & 0xFFFF) / 65535.0;
    return float2(u, v);
}

/* ===== COLOR DECOMPRESSION ===== */

inline float4 decompress_color_rgba8(uint encoded) {
    return float4(
        float((encoded >> 0) & 0xFF) / 255.0,
        float((encoded >> 8) & 0xFF) / 255.0,
        float((encoded >> 16) & 0xFF) / 255.0,
        float((encoded >> 24) & 0xFF) / 255.0
    );
}

/* ===== EXAMPLE VERTEX SHADER USING COMPRESSED ATTRIBUTES ===== */

struct CompressedVertexInput {
    ushort3 position [[attribute(0)]];  // Quantized position
    ushort normal [[attribute(1)]];      // Oct16 normal
    uint tangent [[attribute(2)]];       // Oct16 tangent + bitangent sign
    uint uv [[attribute(3)]];           // 16-bit UV
};

struct VertexOutput {
    float4 position [[position]];
    float3 normal;
    float3 tangent;
    float bitangent_sign;
    float2 uv;
};

vertex VertexOutput compressed_vertex_shader(
    CompressedVertexInput in [[stage_in]],
    constant BoundingBox& bounds [[buffer(1)]],
    constant float4x4& mvp [[buffer(2)]]
) {
    VertexOutput out;
    
    // Decompress position
    float3 world_pos = decompress_position(in.position, bounds);
    out.position = mvp * float4(world_pos, 1.0);
    
    // Decompress normal
    out.normal = decompress_normal_oct16(in.normal);
    
    // Decompress tangent
    float4 tangent_data = decompress_tangent(in.tangent);
    out.tangent = tangent_data.xyz;
    out.bitangent_sign = tangent_data.w;
    
    // Decompress UV
    out.uv = decompress_uv(in.uv);
    
    return out;
}
