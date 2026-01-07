#include <metal_stdlib>
using namespace metal;

struct DecalInstance {
    float4x4 world_to_decal; // Inverse of decal model matrix
    float4x4 model_matrix;   // To transform the cube
    float4 color_tint;
    float normal_threshold;  // Cosine of angle
    uint albedo_texture_index;
    uint normal_texture_index;
    uint material_texture_index; // Optional extra data
    float roughness_override;
    float metallic_override;
};

struct SceneUniforms {
    float4x4 view_proj;
    float4x4 inv_view_proj;
    float3 camera_pos;
    float2 screen_size;
};

struct DecalVertexOut {
    float4 position [[position]];
    float4 screen_pos;
};

// Simple unit cube vertices
constant float3 cube_vertices[36] = {
    // Front face
    float3(-0.5, -0.5,  0.5), float3( 0.5, -0.5,  0.5), float3( 0.5,  0.5,  0.5),
    float3( 0.5,  0.5,  0.5), float3(-0.5,  0.5,  0.5), float3(-0.5, -0.5,  0.5),
    // Back face
    float3(-0.5, -0.5, -0.5), float3(-0.5,  0.5, -0.5), float3( 0.5,  0.5, -0.5),
    float3( 0.5,  0.5, -0.5), float3( 0.5, -0.5, -0.5), float3(-0.5, -0.5, -0.5),
    // Top face
    float3(-0.5,  0.5, -0.5), float3(-0.5,  0.5,  0.5), float3( 0.5,  0.5,  0.5),
    float3( 0.5,  0.5,  0.5), float3( 0.5,  0.5, -0.5), float3(-0.5,  0.5, -0.5),
    // Bottom face
    float3(-0.5, -0.5, -0.5), float3( 0.5, -0.5, -0.5), float3( 0.5, -0.5,  0.5),
    float3( 0.5, -0.5,  0.5), float3(-0.5, -0.5,  0.5), float3(-0.5, -0.5, -0.5),
    // Right face
    float3( 0.5, -0.5, -0.5), float3( 0.5,  0.5, -0.5), float3( 0.5,  0.5,  0.5),
    float3( 0.5,  0.5,  0.5), float3( 0.5, -0.5,  0.5), float3( 0.5, -0.5, -0.5),
    // Left face
    float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5,  0.5), float3(-0.5,  0.5,  0.5),
    float3(-0.5,  0.5,  0.5), float3(-0.5,  0.5, -0.5), float3(-0.5, -0.5, -0.5)
};

vertex DecalVertexOut decal_vertex(
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]],
    constant DecalInstance* instances [[buffer(0)]],
    constant SceneUniforms& uniforms [[buffer(1)]]
) {
    DecalVertexOut out;
    DecalInstance instance = instances[instanceID];
    
    float3 local_pos = cube_vertices[vertexID];
    float4 world_pos = instance.model_matrix * float4(local_pos, 1.0);
    out.position = uniforms.view_proj * world_pos;
    out.screen_pos = out.position;
    
    return out;
}

struct GBufferOut {
    float4 albedo [[color(0)]];
    float4 normal [[color(1)]];
    float4 material [[color(2)]];
};

// Octahedron encoding/decoding
static float3 decode_octahedron(float2 e) {
    float3 v = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0) {
        float2 s = sign(v.xy);
        v.xy = (1.0 - abs(v.yx)) * s;
    }
    return normalize(v);
}

static float2 encode_octahedron(float3 v) {
    v /= (abs(v.x) + abs(v.y) + abs(v.z));
    if (v.z < 0.0) {
        float2 s = sign(v.xy);
        v.xy = (1.0 - abs(v.yx)) * s;
    }
    return v.xy;
}

fragment GBufferOut decal_fragment(
    DecalVertexOut in [[stage_in]],
    uint instanceID [[instance_id]],
    constant DecalInstance* instances [[buffer(0)]],
    constant SceneUniforms& uniforms [[buffer(1)]],
    depth2d<float> gbuffer_depth [[texture(0)]],
    texture2d<float> albedo_textures [[texture(1)]], // Array of textures ideally, or bindless
    texture2d<float> normal_textures [[texture(2)]]  // Simplified for now
) {
    DecalInstance instance = instances[instanceID];
    
    // 1. Calculate screen UV
    float2 uv = (in.screen_pos.xy / in.screen_pos.w) * 0.5 + 0.5;
    uv.y = 1.0 - uv.y; // Flip Y for texture sampling if needed (Metal coords are top-down)
    
    // 2. Read depth
    int2 coord = int2(in.position.xy);
    float depth = gbuffer_depth.read(coord);
    
    // 3. Reconstruct World Position
    float4 clip_pos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clip_pos.y = -clip_pos.y; // Unflip Y to match standard clip space for inverse projection
    
    float4 world_pos = uniforms.inv_view_proj * clip_pos;
    world_pos /= world_pos.w;
    
    // 4. Transform to Decal Space (OS)
    float4 object_pos = instance.world_to_decal * world_pos;
    
    // 5. Clip
    // Decal box is defined as [-0.5, 0.5] in all axes
    if (abs(object_pos.x) > 0.5 || abs(object_pos.y) > 0.5 || abs(object_pos.z) > 0.5) {
        discard_fragment();
    }
    
    // 6. Calculate UVs for lookup
    float2 decal_uv = object_pos.xy + 0.5;
    
    // 7. Sample Decal Textures
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);
    
    // Ideally use texture arrays or heaps. For now, assuming single bound texture or similar.
    float4 decal_albedo = albedo_textures.sample(s, decal_uv);
    
    // 8. Output
    GBufferOut out;
    out.albedo = decal_albedo * instance.color_tint;
    // We are modifying the G-Buffer. Blending should be enabled in the pipeline state.
    // SRC_ALPHA, ONE_MINUS_SRC_ALPHA for Albedo
    
    // Note: Normal blending is tricky. 
    // Usually need to read current normal, blend, and write back. 
    // Or alpha blend 2 normals (imprecise but often okay).
    
    out.normal = float4(0,0,0,0); // Placeholder
    out.material = float4(0,0,0,0); // Placeholder
    
    return out;
}
