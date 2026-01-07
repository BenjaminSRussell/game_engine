# Metal Mesh Shader Alternatives Guide

## Overview

Metal does not currently support mesh shaders (as of Metal 3.x). However, there are several techniques to achieve similar results using existing Metal features.

---

## Technique 1: Geometry Amplification in Vertex Shader

**Use Case:** Creating multiple primitives from a single vertex

```metal
// Vertex shader that generates multiple vertices
vertex VertexOut amplification_vertex(uint vertexID [[vertex_id]],
                                      constant float4* positions [[buffer(0)]]) {
    // Generate 6 vertices from 1 input (e.g., for particle to quad)
    uint particleID = vertexID / 6;
    uint localID = vertexID % 6;
    
    float4 center = positions[particleID];
    
    // Generate quad vertices
    const float2 offsets[6] = {
        float2(-1, -1), float2(1, -1), float2(1, 1),
        float2(-1, -1), float2(1, 1), float2(-1, 1)
    };
    
    VertexOut out;
    out.position = center + float4(offsets[localID] * 0.1, 0, 0);
    return out;
}
```

**Rendering:**
```c
// CPU side: calculate expanded vertex count
uint32_t particle_count = 1000;
uint32_t vertex_count = particle_count * 6; // 6 vertices per particle

mtl_draw_primitives_args_t args = {
    .primitiveType = MTL_PRIMITIVE_TYPE_TRIANGLE,
    .vertexStart = 0,
    .vertexCount = vertex_count,
    .instanceCount = 1
};
metal_render_encoder_draw_primitives(encoder, args);
```

---

## Technique 2: Instanced Rendering with Per-Instance Data

**Use Case:** Rendering many copies of the same mesh with variations

```metal
struct InstanceData {
    float4x4 transform;
    float4 color;
};

vertex VertexOut instanced_vertex(uint vertexID [[vertex_id]],
                                   uint instanceID [[instance_id]],
                                   constant float4* positions [[buffer(0)]],
                                   constant InstanceData* instances [[buffer(1)]]) {
    InstanceData instance = instances[instanceID];
    
    VertexOut out;
    out.position = instance.transform * positions[vertexID];
    out.color = instance.color;
    return out;
}
```

**Rendering:**
```c
// Draw 10,000 instances of a mesh
mtl_draw_indexed_primitives_args_t args = {
    .primitiveType = MTL_PRIMITIVE_TYPE_TRIANGLE,
    .indexCount = mesh_index_count,
    .indexType = MTL_INDEX_TYPE_UINT32,
    .indexBuffer = index_buffer,
    .indexBufferOffset = 0,
    .instanceCount = 10000,  // Many instances!
    .baseVertex = 0,
    .baseInstance = 0
};
metal_render_encoder_draw_indexed_primitives(encoder, args);
```

---

## Technique 3: Compute Shader + Indirect Drawing (GPU-Driven)

**Use Case:** GPU-side mesh generation and culling

**Step 1: Compute Shader Generates Geometry**
```metal
kernel void generate_meshlets(device VertexData* output [[buffer(0)]],
                              device IndirectDrawArgs* drawArgs [[buffer(1)]],
                              constant InputData* input [[buffer(2)]],
                              uint tid [[thread_position_in_grid]]) {
    // Generate vertices
    output[tid * 3 + 0] = compute_vertex(input, tid, 0);
    output[tid * 3 + 1] = compute_vertex(input, tid, 1);
    output[tid * 3 + 2] = compute_vertex(input, tid, 2);
    
    // Write draw command
    if (tid == 0) {
        drawArgs->vertexCount = input.meshletCount * 3;
        drawArgs->instanceCount = 1;
    }
}
```

**Step 2: Indirect Draw**
```c
// Compute pass: generate geometry
mtl_compute_command_encoder_t compute = metal_compute_command_encoder_create(cmd);
metal_compute_encoder_set_compute_pipeline_state(compute, generate_pipeline);
metal_compute_encoder_set_buffer(compute, output_buffer, 0, 0);
metal_compute_encoder_set_buffer(compute, draw_args_buffer, 0, 1);
metal_compute_encoder_set_buffer(compute, input_buffer, 0, 2);

mtl_dispatch_threadgroups_args_t dispatch = {
    .threadgroupsPerGrid = {meshlet_count, 1, 1},
    .threadsPerThreadgroup = {64, 1, 1}
};
metal_compute_encoder_dispatch_threadgroups(compute, dispatch);
metal_compute_encoder_end_encoding(compute);

// Render pass: draw generated geometry
mtl_render_command_encoder_t render = metal_render_command_encoder_create(cmd, pass_desc);
metal_render_encoder_set_render_pipeline_state(render, render_pipeline);
metal_render_encoder_set_vertex_buffer(render, output_buffer, 0, 0);

// Indirect draw
metal_render_encoder_draw_primitives_indirect(render, 
    MTL_PRIMITIVE_TYPE_TRIANGLE, 
    draw_args_buffer, 
    0);
```

---

## Technique 4: Tessellation (Limited)

**Use Case:** Subdivision surfaces, terrain LOD

```metal
// Vertex shader
[[patch(triangle, 4)]] // 4 control points
vertex TessellationVertexOut tess_vertex(uint patchID [[patch_id]],
                                         constant float4* controlPoints [[buffer(0)]]) {
    TessellationVertexOut out;
    out.position = controlPoints[patchID];
    return out;
}

// Post-tessellation vertex shader
[[stage_in]]
vertex RenderVertexOut post_tess_vertex(TessellationVertexOut in [[stage_in]],
                                         float3 barycentrics [[position_in_patch]]) {
    // Interpolate using barycentric coordinates
    RenderVertexOut out;
    out.position = interpolate(in, barycentrics);
    return out;
}
```

---

## Technique 5: Procedural Geometry in Fragment Shader

**Use Case:** Raymarched SDFs, procedural shapes

```metal
fragment float4 sdf_fragment(VertexOut in [[stage_in]]) {
    // Raymarch a sphere SDF
    float3 rayOrigin = in.worldPos;
    float3 rayDir = normalize(in.worldPos - cameraPos);
    
    float t = 0;
    for (int i = 0; i < 64; i++) {
        float3 p = rayOrigin + rayDir * t;
        float dist = length(p) - 1.0; // Sphere SDF
        
        if (dist < 0.001) {
            return float4(compute_normal(p), 1.0);
        }
        t += dist;
    }
    
    discard_fragment();
}
```

---

## Performance Comparison

| Technique | Throughput | Flexibility | Complexity |
|-----------|-----------|-------------|------------|
| Vertex Amplification | ⭐⭐⭐ | ⭐⭐ | ⭐ |
| Instancing | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐ |
| Compute + Indirect | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| Tessellation | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| Fragment Shader | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |

---

## Best Practices

1. **Use Instancing First**: For most use cases, instancing is the fastest and simplest
2. **Compute + Indirect for Complexity**: When you need GPU-side logic, use compute shaders
3. **Avoid Per-Fragment Work**: Fragment shader techniques are slow; use sparingly
4. **Pre-compute When Possible**: Generate geometry offline if it doesn't change
5. **LOD Systems**: Combine techniques at different distances

---

## Future: When Mesh Shaders Arrive

Once Metal supports mesh shaders, the migration path from these techniques will be:

1. **Vertex Amplification** → Direct mesh shader with amplification
2. **Compute + Indirect** → Mesh shader with same logic
3. **Instancing** → Mesh shader with per-meshlet instances

The compute shader approach is the closest conceptually to mesh shaders, making it the best choice for forward compatibility.

---

## Example: Full GPU-Driven Pipeline

```c
// This achieves mesh shader-like functionality today

// 1. Compute: Cull and generate draw commands
mtl_compute_command_encoder_t compute = metal_compute_command_encoder_create(cmd);
// ... cull meshlets, write to ICB
metal_compute_encoder_end_encoding(compute);

// 2. Render: Execute indirect commands
mtl_render_command_encoder_t render = metal_render_command_encoder_create(cmd, pass);
// ... execute ICB with generated commands
metal_render_encoder_end_encoding(render);

// This pipeline supports:
// - GPU culling
// - GPU LOD selection
// - Dynamic geometry generation
// - All without CPU intervention
```

---

## Conclusion

While Metal doesn't have mesh shaders yet, **compute shaders + indirect command buffers** provide equivalent functionality with only slightly more code. For simpler cases, **instancing remains the fastest option**.

The Metal backend now provides all the building blocks needed to implement mesh shader-style rendering today.
