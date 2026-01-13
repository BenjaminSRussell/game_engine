#include <metal_stdlib>
using namespace metal;

struct VoxelVertex {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
    uchar  ao       [[attribute(3)]];
    uchar  light    [[attribute(4)]];
    ushort block_id [[attribute(5)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float  ao;
    float  light;
    uint   block_id;
    float3 world_pos;
    float3 view_pos;
};

struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 model_view_projection;
    float3 camera_pos;
    float time;
    float fog_density;
    float fog_start;
    float fog_end;
};

struct LightingData {
    float3 sun_direction;
    float3 sun_color;
    float3 ambient_color;
    float sun_intensity;
    float ambient_intensity;
};

// Texture array for block types
constexpr sampler texture_sampler(address::repeat, mag_filter::linear, min_filter::linear);

vertex VertexOut voxel_vertex_main(VoxelVertex v [[stage_in]],
                                  constant Uniforms &uniforms [[buffer(1)]],
                                  constant LightingData &lighting [[buffer(2)]]) {
    VertexOut out;
    
    // Transform position
    float4 world_pos = uniforms.model * float4(v.position, 1.0);
    out.world_pos = world_pos.xyz;
    out.view_pos = (uniforms.view * world_pos).xyz;
    out.position = uniforms.projection * float4(out.view_pos, 1.0);
    
    // Pass through data
    out.normal = (uniforms.model * float4(v.normal, 0.0)).xyz;
    out.uv = v.uv;
    out.ao = float(v.ao) / 255.0;
    out.light = float(v.light) / 255.0;
    out.block_id = uint(v.block_id);
    
    return out;
}

fragment float4 voxel_fragment_main(VertexOut in [[stage_in]],
                                   constant Uniforms &uniforms [[buffer(0)]],
                                   constant LightingData &lighting [[buffer(1)]],
                                   texture2d_array<float> block_textures [[texture(0)]]) {
    // Get base color from texture atlas
    float2 atlas_uv = in.uv;
    uint texture_index = in.block_id;
    
    // Sample texture (fallback to solid color if texture not available)
    float3 albedo;
    if (texture_index < block_textures.get_array_size()) {
        albedo = block_textures.sample(texture_sampler, atlas_uv, texture_index).rgb;
    } else {
        // Fallback colors for different block types
        switch (in.block_id) {
            case 1: albedo = float3(0.5, 0.5, 0.5); break;  // Stone
            case 2: albedo = float3(0.1, 0.8, 0.2); break;  // Grass
            case 3: albedo = float3(0.5, 0.3, 0.1); break;  // Dirt
            case 4: albedo = float3(0.6, 0.4, 0.2); break;  // Wood
            case 5: albedo = float3(0.2, 0.6, 0.1); break;  // Leaves
            case 6: albedo = float3(0.2, 0.4, 0.8); break;  // Water
            case 7: albedo = float3(0.8, 0.8, 0.4); break;  // Sand
            case 8: albedo = float3(0.2, 0.2, 0.2); break;  // Coal
            case 9: albedo = float3(0.7, 0.7, 0.7); break;  // Iron
            case 10: albedo = float3(1.0, 0.8, 0.2); break; // Gold
            case 11: albedo = float3(0.8, 0.8, 1.0); break; // Diamond
            default: albedo = float3(0.5, 0.5, 0.5); break; // Default
        }
    }
    
    // Calculate lighting
    float3 normal = normalize(in.normal);
    
    // Sunlight (directional lighting)
    float NdotL = max(dot(normal, lighting.sun_direction), 0.0);
    float3 sunlight = lighting.sun_color * lighting.sun_intensity * NdotL;
    
    // Ambient lighting
    float3 ambient = lighting.ambient_color * lighting.ambient_intensity;
    
    // Block light (torch light)
    float3 block_light = albedo * in.light;
    
    // Ambient occlusion
    float ao_factor = mix(0.3, 1.0, in.ao);
    
    // Combine lighting
    float3 final_color = albedo * (sunlight + ambient) * ao_factor + block_light * 0.5;
    
    // Apply fog
    float distance = length(in.view_pos);
    float fog_factor = saturate((distance - uniforms.fog_start) / (uniforms.fog_end - uniforms.fog_start));
    fog_factor = pow(fog_factor, uniforms.fog_density);
    
    float3 fog_color = float3(0.7, 0.8, 0.9); // Sky blue fog
    final_color = mix(final_color, fog_color, fog_factor);
    
    // Add subtle animation for water blocks
    if (in.block_id == 6) { // Water
        float wave = sin(uniforms.time * 2.0 + in.world_pos.x * 0.5) * 0.05;
        final_color += wave;
    }
    
    return float4(final_color, 1.0);
}

// Compute shader for voxel mesh generation
kernel void voxel_mesh_generate(
    device VoxelVertex *vertices [[buffer(0)]],
    device uint *indices [[buffer(1)]],
    device uint *vertex_count [[buffer(2)]],
    device uint *index_count [[buffer(3)]],
    const device uint16 *blocks [[buffer(4)]],
    constant Uniforms &uniforms [[buffer(5)]],
    uint3 tid [[thread_position_in_grid]])
{
    uint3 chunk_size = uint3(16, 64, 16);
    if (tid.x >= chunk_size.x || tid.y >= chunk_size.y || tid.z >= chunk_size.z) {
        return;
    }
    
    uint block_index = tid.x + tid.y * chunk_size.x + tid.z * chunk_size.x * chunk_size.y;
    uint16 block_type = blocks[block_index];
    
    if (block_type == 0) return; // Skip air blocks
    
    // Generate faces for this block
    // This is a simplified version - full implementation would check neighbors
    uint local_vertex_offset = atomic_add(vertex_count, 24); // 6 faces * 4 vertices
    uint local_index_offset = atomic_add(index_count, 36);  // 6 faces * 6 indices
    
    if (local_vertex_offset + 24 >= 65536 || local_index_offset + 36 >= 98304) {
        return; // Buffer overflow protection
    }
    
    // Generate vertices for each face
    float3 block_pos = float3(tid.x, tid.y, tid.z);
    
    for (int face = 0; face < 6; face++) {
        float3 normal;
        float3 face_vertices[4];
        
        // Define face vertices based on face direction
        switch (face) {
            case 0: // -X
                normal = float3(-1, 0, 0);
                face_vertices[0] = block_pos + float3(-0.5, -0.5, -0.5);
                face_vertices[1] = block_pos + float3(-0.5,  0.5, -0.5);
                face_vertices[2] = block_pos + float3(-0.5,  0.5,  0.5);
                face_vertices[3] = block_pos + float3(-0.5, -0.5,  0.5);
                break;
            case 1: // +X
                normal = float3(1, 0, 0);
                face_vertices[0] = block_pos + float3(0.5, -0.5,  0.5);
                face_vertices[1] = block_pos + float3(0.5,  0.5,  0.5);
                face_vertices[2] = block_pos + float3(0.5,  0.5, -0.5);
                face_vertices[3] = block_pos + float3(0.5, -0.5, -0.5);
                break;
            case 2: // -Y
                normal = float3(0, -1, 0);
                face_vertices[0] = block_pos + float3(-0.5, -0.5,  0.5);
                face_vertices[1] = block_pos + float3( 0.5, -0.5,  0.5);
                face_vertices[2] = block_pos + float3( 0.5, -0.5, -0.5);
                face_vertices[3] = block_pos + float3(-0.5, -0.5, -0.5);
                break;
            case 3: // +Y
                normal = float3(0, 1, 0);
                face_vertices[0] = block_pos + float3(-0.5,  0.5, -0.5);
                face_vertices[1] = block_pos + float3( 0.5,  0.5, -0.5);
                face_vertices[2] = block_pos + float3( 0.5,  0.5,  0.5);
                face_vertices[3] = block_pos + float3(-0.5,  0.5,  0.5);
                break;
            case 4: // -Z
                normal = float3(0, 0, -1);
                face_vertices[0] = block_pos + float3( 0.5, -0.5, -0.5);
                face_vertices[1] = block_pos + float3( 0.5,  0.5, -0.5);
                face_vertices[2] = block_pos + float3(-0.5,  0.5, -0.5);
                face_vertices[3] = block_pos + float3(-0.5, -0.5, -0.5);
                break;
            case 5: // +Z
                normal = float3(0, 0, 1);
                face_vertices[0] = block_pos + float3(-0.5, -0.5,  0.5);
                face_vertices[1] = block_pos + float3(-0.5,  0.5,  0.5);
                face_vertices[2] = block_pos + float3( 0.5,  0.5,  0.5);
                face_vertices[3] = block_pos + float3( 0.5, -0.5,  0.5);
                break;
        }
        
        // Write vertices
        for (int i = 0; i < 4; i++) {
            uint vertex_index = local_vertex_offset + face * 4 + i;
            vertices[vertex_index].position = face_vertices[i];
            vertices[vertex_index].normal = normal;
            vertices[vertex_index].uv = float2((i & 1) ? 1.0 : 0.0, (i & 2) ? 1.0 : 0.0);
            vertices[vertex_index].ao = 255;
            vertices[vertex_index].light = 255;
            vertices[vertex_index].block_id = block_type;
        }
        
        // Write indices (2 triangles per quad)
        uint base_vertex = local_vertex_offset + face * 4;
        indices[local_index_offset + face * 6 + 0] = base_vertex + 0;
        indices[local_index_offset + face * 6 + 1] = base_vertex + 1;
        indices[local_index_offset + face * 6 + 2] = base_vertex + 2;
        indices[local_index_offset + face * 6 + 3] = base_vertex + 2;
        indices[local_index_offset + face * 6 + 4] = base_vertex + 3;
        indices[local_index_offset + face * 6 + 5] = base_vertex + 0;
    }
}
