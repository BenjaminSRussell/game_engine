# Phase 5: Effects & Environment Rendering

## Overview
This phase implements particle systems, environmental effects, water/ocean, and atmospheric rendering.

---

## Agent 5.1: GPU Particle System

```
TASK: Implement GPU Particle System (Phase 5, Agent 1)

You are implementing a GPU-driven particle system using Metal compute shaders.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/effects/

EXISTING FILES:
- particles/ (has partial implementations)
- gpu_particles/ (needs completion)

WHAT TO IMPLEMENT:
1. GPU particle buffer (structure of arrays)
2. Compute shader for particle spawn
3. Compute shader for particle update
4. Indirect draw for rendering
5. Dead list for particle recycling

METAL GPU PARTICLES:
```c
typedef struct particle_soa {
    id<MTLBuffer> positions;      // float4: xyz + padding
    id<MTLBuffer> velocities;     // float4: xyz + padding
    id<MTLBuffer> colors;         // float4: rgba
    id<MTLBuffer> sizes;          // float2: size + rotation
    id<MTLBuffer> lifetimes;      // float2: age, max_life
    id<MTLBuffer> alive_list;     // uint: indices of alive particles
    id<MTLBuffer> dead_list;      // uint: indices of available slots
    id<MTLBuffer> counters;       // uint4: alive_count, dead_count, emit_count, ...
    id<MTLBuffer> indirect_args;  // MTLDrawPrimitivesIndirectArguments
    uint32_t max_particles;
} particle_soa_t;

typedef struct particle_emitter {
    simd_float3 position;
    simd_float3 velocity_min;
    simd_float3 velocity_max;
    simd_float4 color_start;
    simd_float4 color_end;
    float size_start;
    float size_end;
    float lifetime_min;
    float lifetime_max;
    float emit_rate;
    float emit_accumulator;
} particle_emitter_t;

typedef struct gpu_particle_system {
    particle_soa_t particles;
    id<MTLComputePipelineState> spawn_pipeline;
    id<MTLComputePipelineState> update_pipeline;
    id<MTLComputePipelineState> compact_pipeline;
    id<MTLRenderPipelineState> render_pipeline;
} gpu_particle_system_t;

// Metal shader:
```metal
struct ParticleCounters {
    atomic_uint alive_count;
    atomic_uint dead_count;
    atomic_uint emit_count;
    uint draw_count;  // For indirect args
};

kernel void particle_spawn(
    device float4* positions [[buffer(0)]],
    device float4* velocities [[buffer(1)]],
    device float4* colors [[buffer(2)]],
    device float2* sizes [[buffer(3)]],
    device float2* lifetimes [[buffer(4)]],
    device uint* dead_list [[buffer(5)]],
    device ParticleCounters& counters [[buffer(6)]],
    constant ParticleEmitter& emitter [[buffer(7)]],
    uint tid [[thread_position_in_grid]]
) {
    if (tid >= emitter.emit_count) return;

    // Pop from dead list
    uint dead_idx = atomic_fetch_sub_explicit(&counters.dead_count, 1, memory_order_relaxed);
    if (dead_idx == 0) {
        atomic_fetch_add_explicit(&counters.dead_count, 1, memory_order_relaxed);
        return;  // No available slots
    }

    uint particle_idx = dead_list[dead_idx - 1];

    // Initialize particle
    float3 rand = hash_to_float3(uint3(tid, emitter.frame, particle_idx));
    positions[particle_idx] = float4(emitter.position + rand * 0.1, 1.0);
    velocities[particle_idx] = float4(mix(emitter.velocity_min, emitter.velocity_max, rand), 0.0);
    colors[particle_idx] = emitter.color_start;
    sizes[particle_idx] = float2(emitter.size_start, rand.x * 6.28);
    lifetimes[particle_idx] = float2(0.0, mix(emitter.lifetime_min, emitter.lifetime_max, rand.z));

    // Increment alive count
    atomic_fetch_add_explicit(&counters.alive_count, 1, memory_order_relaxed);
}

kernel void particle_update(
    device float4* positions [[buffer(0)]],
    device float4* velocities [[buffer(1)]],
    device float4* colors [[buffer(2)]],
    device float2* sizes [[buffer(3)]],
    device float2* lifetimes [[buffer(4)]],
    device uint* alive_list [[buffer(5)]],
    device uint* dead_list [[buffer(6)]],
    device ParticleCounters& counters [[buffer(7)]],
    constant UpdateUniforms& uniforms [[buffer(8)]],
    uint tid [[thread_position_in_grid]]
) {
    if (tid >= counters.alive_count) return;

    uint idx = alive_list[tid];

    // Update age
    lifetimes[idx].x += uniforms.delta_time;

    // Check if dead
    if (lifetimes[idx].x >= lifetimes[idx].y) {
        // Push to dead list
        uint dead_idx = atomic_fetch_add_explicit(&counters.dead_count, 1, memory_order_relaxed);
        dead_list[dead_idx] = idx;
        return;
    }

    // Physics update
    float3 vel = velocities[idx].xyz;
    vel += uniforms.gravity * uniforms.delta_time;

    float3 pos = positions[idx].xyz;
    pos += vel * uniforms.delta_time;

    positions[idx].xyz = pos;
    velocities[idx].xyz = vel;

    // Interpolate color and size
    float t = lifetimes[idx].x / lifetimes[idx].y;
    colors[idx] = mix(uniforms.color_start, uniforms.color_end, t);
    sizes[idx].x = mix(uniforms.size_start, uniforms.size_end, t);
}
```

C code:
```c
void gpu_particles_emit(gpu_particle_system_t* sys, particle_emitter_t* emitter,
                         id<MTLComputeCommandEncoder> encoder, float dt) {
    emitter->emit_accumulator += emitter->emit_rate * dt;
    uint32_t emit_count = (uint32_t)emitter->emit_accumulator;
    emitter->emit_accumulator -= emit_count;

    if (emit_count == 0) return;

    [encoder setComputePipelineState:sys->spawn_pipeline];
    [encoder setBuffer:sys->particles.positions offset:0 atIndex:0];
    [encoder setBuffer:sys->particles.velocities offset:0 atIndex:1];
    [encoder setBuffer:sys->particles.colors offset:0 atIndex:2];
    [encoder setBuffer:sys->particles.sizes offset:0 atIndex:3];
    [encoder setBuffer:sys->particles.lifetimes offset:0 atIndex:4];
    [encoder setBuffer:sys->particles.dead_list offset:0 atIndex:5];
    [encoder setBuffer:sys->particles.counters offset:0 atIndex:6];

    emitter->emit_count = emit_count;
    [encoder setBytes:emitter length:sizeof(particle_emitter_t) atIndex:7];

    MTLSize groups = MTLSizeMake((emit_count + 63) / 64, 1, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(64, 1, 1)];
}

void gpu_particles_render(gpu_particle_system_t* sys, id<MTLRenderCommandEncoder> encoder) {
    [encoder setRenderPipelineState:sys->render_pipeline];
    [encoder setVertexBuffer:sys->particles.positions offset:0 atIndex:0];
    [encoder setVertexBuffer:sys->particles.colors offset:0 atIndex:1];
    [encoder setVertexBuffer:sys->particles.sizes offset:0 atIndex:2];

    // Indirect draw based on alive count
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
             indirectBuffer:sys->particles.indirect_args
       indirectBufferOffset:0];
}
```

OUTPUT: GPU particle system with millions of particles.
```

---

## Agent 5.2: Ocean & Water Rendering

```
TASK: Implement Ocean/Water Rendering (Phase 5, Agent 2)

You are implementing FFT-based ocean simulation and rendering.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/effects/water/

EXISTING FILES: Some partial implementations in water/, ocean_waves/

WHAT TO IMPLEMENT:
1. FFT wave spectrum generation
2. GPU FFT compute shader
3. Ocean mesh displacement
4. Foam generation (Jacobian)
5. Underwater rendering

METAL OCEAN:
```c
typedef struct ocean_system {
    // Spectrum textures
    id<MTLTexture> spectrum_h0;       // Complex initial spectrum
    id<MTLTexture> spectrum_ht;       // Time-evolved spectrum
    id<MTLTexture> displacement_xy;   // XZ displacement
    id<MTLTexture> displacement_z;    // Y displacement
    id<MTLTexture> derivatives;       // For normals and foam

    // Compute pipelines
    id<MTLComputePipelineState> spectrum_update_pipeline;
    id<MTLComputePipelineState> fft_horizontal_pipeline;
    id<MTLComputePipelineState> fft_vertical_pipeline;
    id<MTLComputePipelineState> displacement_pipeline;

    // Render pipeline
    id<MTLRenderPipelineState> ocean_pipeline;

    // Ocean mesh (flat grid that gets displaced)
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
    uint32_t index_count;

    // Parameters
    uint32_t resolution;  // FFT size (512, 1024)
    float tile_size;
    float amplitude;
    float wind_speed;
    simd_float2 wind_direction;
} ocean_system_t;

// Spectrum generation (run once at init)
void ocean_generate_spectrum(ocean_system_t* ocean, metal_device_t* dev) {
    // Phillips spectrum
    float* h0_data = malloc(ocean->resolution * ocean->resolution * 4 * sizeof(float));

    for (uint32_t y = 0; y < ocean->resolution; y++) {
        for (uint32_t x = 0; x < ocean->resolution; x++) {
            simd_float2 k = simd_make_float2(
                (float)x - ocean->resolution / 2,
                (float)y - ocean->resolution / 2
            ) * (2.0f * M_PI / ocean->tile_size);

            float k_len = simd_length(k);
            if (k_len < 0.0001f) k_len = 0.0001f;

            simd_float2 k_norm = k / k_len;
            float k_dot_wind = simd_dot(k_norm, ocean->wind_direction);

            // Phillips spectrum
            float L = ocean->wind_speed * ocean->wind_speed / 9.81f;
            float phillips = ocean->amplitude * expf(-1.0f / (k_len * L * k_len * L)) /
                            (k_len * k_len * k_len * k_len) * k_dot_wind * k_dot_wind;

            // Gaussian random
            float xi_r = gaussian_random();
            float xi_i = gaussian_random();

            uint32_t idx = (y * ocean->resolution + x) * 4;
            h0_data[idx + 0] = sqrtf(phillips * 0.5f) * xi_r;
            h0_data[idx + 1] = sqrtf(phillips * 0.5f) * xi_i;
            h0_data[idx + 2] = sqrtf(phillips * 0.5f) * xi_r;  // Conjugate
            h0_data[idx + 3] = sqrtf(phillips * 0.5f) * xi_i;
        }
    }

    // Upload to texture
    MTLRegion region = MTLRegionMake2D(0, 0, ocean->resolution, ocean->resolution);
    [ocean->spectrum_h0 replaceRegion:region mipmapLevel:0
                            withBytes:h0_data bytesPerRow:ocean->resolution * 16];

    free(h0_data);
}

// Update spectrum with time
```metal
kernel void ocean_spectrum_update(
    texture2d<float, access::read> h0 [[texture(0)]],
    texture2d<float, access::write> ht [[texture(1)]],
    constant float& time [[buffer(0)]],
    constant OceanParams& params [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    float4 h0_data = h0.read(gid);
    float2 h0k = h0_data.xy;
    float2 h0k_conj = h0_data.zw;

    // Wave vector
    float2 k = float2(int2(gid) - int2(params.resolution / 2)) * (2.0 * M_PI / params.tile_size);
    float k_len = length(k);

    // Dispersion relation: omega = sqrt(g * k)
    float omega = sqrt(9.81 * k_len);

    // Time evolution
    float cos_t = cos(omega * time);
    float sin_t = sin(omega * time);

    // h(k,t) = h0(k) * exp(i*omega*t) + h0*(-k) * exp(-i*omega*t)
    float2 ht_k;
    ht_k.x = h0k.x * cos_t - h0k.y * sin_t + h0k_conj.x * cos_t + h0k_conj.y * sin_t;
    ht_k.y = h0k.x * sin_t + h0k.y * cos_t - h0k_conj.x * sin_t + h0k_conj.y * cos_t;

    ht.write(float4(ht_k, 0, 0), gid);
}
```

C code:
```c
void ocean_update(ocean_system_t* ocean, id<MTLCommandBuffer> cmd, float time) {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

    // Update spectrum
    [encoder setComputePipelineState:ocean->spectrum_update_pipeline];
    [encoder setTexture:ocean->spectrum_h0 atIndex:0];
    [encoder setTexture:ocean->spectrum_ht atIndex:1];
    [encoder setBytes:&time length:sizeof(float) atIndex:0];

    MTLSize groups = MTLSizeMake(ocean->resolution / 8, ocean->resolution / 8, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

    // 2D FFT (horizontal then vertical)
    [encoder setComputePipelineState:ocean->fft_horizontal_pipeline];
    [encoder setTexture:ocean->spectrum_ht atIndex:0];
    [encoder setTexture:ocean->displacement_z atIndex:1];
    [encoder dispatchThreadgroups:MTLSizeMake(ocean->resolution, 1, 1)
                threadsPerThreadgroup:MTLSizeMake(ocean->resolution, 1, 1)];

    [encoder setComputePipelineState:ocean->fft_vertical_pipeline];
    [encoder setTexture:ocean->displacement_z atIndex:0];
    [encoder setTexture:ocean->displacement_z atIndex:1];
    [encoder dispatchThreadgroups:MTLSizeMake(1, ocean->resolution, 1)
                threadsPerThreadgroup:MTLSizeMake(1, ocean->resolution, 1)];

    // Calculate derivatives for normals
    [encoder setComputePipelineState:ocean->displacement_pipeline];
    [encoder setTexture:ocean->displacement_z atIndex:0];
    [encoder setTexture:ocean->derivatives atIndex:1];
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

    [encoder endEncoding];
}

void ocean_render(ocean_system_t* ocean, id<MTLRenderCommandEncoder> encoder, camera_t* camera) {
    [encoder setRenderPipelineState:ocean->ocean_pipeline];
    [encoder setVertexBuffer:ocean->vertex_buffer offset:0 atIndex:0];
    [encoder setVertexTexture:ocean->displacement_z atIndex:0];
    [encoder setFragmentTexture:ocean->derivatives atIndex:0];

    // ... set uniforms

    [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                        indexCount:ocean->index_count
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:ocean->index_buffer
                 indexBufferOffset:0];
}
```

OUTPUT: FFT ocean simulation with displacement and foam.
```

---

## Agent 5.3: Atmospheric Scattering

```
TASK: Implement Atmospheric Scattering (Phase 5, Agent 3)

You are implementing physically-based sky and atmospheric rendering.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/environment/atmosphere/

EXISTING: Some files in atmosphere/, sky_atmosphere/

WHAT TO IMPLEMENT:
1. Rayleigh + Mie scattering LUTs
2. Transmittance LUT
3. Sky rendering
4. Aerial perspective
5. Sun disk

METAL ATMOSPHERE:
```c
typedef struct atmosphere_system {
    id<MTLTexture> transmittance_lut;    // 256x64
    id<MTLTexture> scattering_lut;       // 32x128x32 (3D)
    id<MTLTexture> irradiance_lut;       // 64x16

    id<MTLComputePipelineState> transmittance_pipeline;
    id<MTLComputePipelineState> scattering_pipeline;
    id<MTLRenderPipelineState> sky_pipeline;

    // Atmosphere parameters
    float planet_radius;           // 6360 km
    float atmosphere_height;       // 100 km
    simd_float3 rayleigh_coeff;    // Scattering coefficients
    float rayleigh_scale_height;   // 8 km
    float mie_coeff;
    float mie_scale_height;        // 1.2 km
    float mie_g;                   // Asymmetry factor
    simd_float3 sun_direction;
    simd_float3 sun_intensity;
} atmosphere_system_t;

// Precompute transmittance LUT:
```metal
kernel void compute_transmittance(
    texture2d<float, access::write> transmittance [[texture(0)]],
    constant AtmosphereParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    float2 uv = float2(gid) / float2(transmittance.get_width(), transmittance.get_height());

    // Map UV to height and cos(view_zenith)
    float h = mix(params.planet_radius, params.planet_radius + params.atmosphere_height, uv.y);
    float cos_theta = uv.x * 2.0 - 1.0;

    // Ray march through atmosphere
    float3 origin = float3(0, h, 0);
    float3 dir = float3(sqrt(1.0 - cos_theta * cos_theta), cos_theta, 0);

    float t_max = ray_sphere_intersect(origin, dir, params.planet_radius + params.atmosphere_height);

    float3 optical_depth = float3(0);
    int steps = 64;
    float dt = t_max / steps;

    for (int i = 0; i < steps; i++) {
        float3 pos = origin + dir * (i + 0.5) * dt;
        float altitude = length(pos) - params.planet_radius;

        float rho_rayleigh = exp(-altitude / params.rayleigh_scale_height);
        float rho_mie = exp(-altitude / params.mie_scale_height);

        optical_depth += (params.rayleigh_coeff * rho_rayleigh + params.mie_coeff * rho_mie) * dt;
    }

    float3 transmittance_value = exp(-optical_depth);
    transmittance.write(float4(transmittance_value, 1.0), gid);
}
```

Sky rendering:
```metal
fragment float4 sky_fragment(
    float4 position [[position]],
    constant SkyUniforms& uniforms [[buffer(0)]],
    texture2d<float> transmittance_lut [[texture(0)]],
    texture3d<float> scattering_lut [[texture(1)]]
) {
    // Reconstruct view direction from screen position
    float2 uv = position.xy / uniforms.screen_size;
    float4 clip = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    clip.y = -clip.y;
    float4 world = uniforms.inv_view_proj * clip;
    float3 view_dir = normalize(world.xyz / world.w - uniforms.camera_pos);

    // Sample scattering LUT
    float cos_view_sun = dot(view_dir, uniforms.sun_direction);
    float altitude = length(uniforms.camera_pos) - uniforms.planet_radius;

    float3 inscatter = sample_scattering_lut(scattering_lut, altitude, view_dir.y, cos_view_sun);
    float3 transmittance = sample_transmittance_lut(transmittance_lut, altitude, view_dir.y);

    // Sun disk
    float sun_angle = acos(cos_view_sun);
    float sun_size = 0.0093;  // Angular size of sun
    float sun_intensity = smoothstep(sun_size, sun_size * 0.9, sun_angle);

    float3 color = inscatter + transmittance * uniforms.sun_intensity * sun_intensity;

    return float4(color, 1.0);
}
```

C code:
```c
void atmosphere_precompute(atmosphere_system_t* atmo, metal_device_t* dev, id<MTLCommandBuffer> cmd) {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

    // Transmittance LUT
    [encoder setComputePipelineState:atmo->transmittance_pipeline];
    [encoder setTexture:atmo->transmittance_lut atIndex:0];

    struct AtmosphereParams params = {
        .planet_radius = atmo->planet_radius * 1000.0f,  // Convert km to m
        .atmosphere_height = atmo->atmosphere_height * 1000.0f,
        .rayleigh_coeff = atmo->rayleigh_coeff,
        .rayleigh_scale_height = atmo->rayleigh_scale_height * 1000.0f,
        .mie_coeff = atmo->mie_coeff,
        .mie_scale_height = atmo->mie_scale_height * 1000.0f,
        .mie_g = atmo->mie_g
    };
    [encoder setBytes:&params length:sizeof(params) atIndex:0];

    [encoder dispatchThreadgroups:MTLSizeMake(256/8, 64/8, 1)
                threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

    // Scattering LUT (similar, but 3D)
    // ...

    [encoder endEncoding];
}

void atmosphere_render_sky(atmosphere_system_t* atmo, id<MTLRenderCommandEncoder> encoder,
                           camera_t* camera) {
    [encoder setRenderPipelineState:atmo->sky_pipeline];
    [encoder setFragmentTexture:atmo->transmittance_lut atIndex:0];
    [encoder setFragmentTexture:atmo->scattering_lut atIndex:1];

    struct SkyUniforms uniforms = {
        .inv_view_proj = simd_inverse(camera->view_proj),
        .camera_pos = camera->position,
        .sun_direction = atmo->sun_direction,
        .sun_intensity = atmo->sun_intensity,
        .planet_radius = atmo->planet_radius * 1000.0f,
        .screen_size = simd_make_float2(camera->viewport_width, camera->viewport_height)
    };
    [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];

    // Draw fullscreen triangle
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}
```

OUTPUT: Physically-based atmosphere with LUTs.
```

---

## Agent 5.4: TAA Implementation

```
TASK: Implement Temporal Anti-Aliasing (Phase 5, Agent 4)

You are implementing TAA for high-quality anti-aliasing.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/postprocess/anti_aliasing/

FILES TO CREATE/COMPLETE:
- taa_resolve.c/h
- taa_velocity.c/h
- taa_history.c/h

METAL TAA:
```c
typedef struct taa_system {
    id<MTLTexture> history_texture[2];  // Ping-pong
    id<MTLTexture> velocity_texture;
    id<MTLComputePipelineState> resolve_pipeline;
    uint32_t current_history;
    uint32_t frame_index;
} taa_system_t;

// Halton sequence for jitter
simd_float2 taa_get_jitter(uint32_t frame_index, simd_float2 screen_size) {
    // Halton(2, 3) sequence
    float x = 0, y = 0;
    float fx = 1.0f / 2.0f, fy = 1.0f / 3.0f;

    uint32_t i = frame_index % 16;
    while (i > 0) {
        x += (i % 2) * fx;
        fx /= 2.0f;
        i /= 2;
    }

    i = frame_index % 16;
    while (i > 0) {
        y += (i % 3) * fy;
        fy /= 3.0f;
        i /= 3;
    }

    return simd_make_float2(x - 0.5f, y - 0.5f) / screen_size;
}

// TAA resolve shader:
```metal
kernel void taa_resolve(
    texture2d<float, access::read> current [[texture(0)]],
    texture2d<float, access::read> history [[texture(1)]],
    texture2d<float, access::read> velocity [[texture(2)]],
    texture2d<float, access::read> depth [[texture(3)]],
    texture2d<float, access::write> output [[texture(4)]],
    constant TAAUniforms& uniforms [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    float2 uv = (float2(gid) + 0.5) / float2(uniforms.screen_size);

    // Sample current frame
    float4 current_color = current.read(gid);

    // Get velocity
    float2 vel = velocity.read(gid).xy;

    // Reproject to previous frame
    float2 history_uv = uv - vel;

    // Check if reprojection is valid
    if (history_uv.x < 0 || history_uv.x > 1 || history_uv.y < 0 || history_uv.y > 1) {
        output.write(current_color, gid);
        return;
    }

    // Sample history with bilinear
    float4 history_color = history.sample(linear_sampler, history_uv);

    // Neighborhood clamping (variance clip)
    float4 neighbor_min = current_color;
    float4 neighbor_max = current_color;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float4 neighbor = current.read(gid + uint2(x, y));
            neighbor_min = min(neighbor_min, neighbor);
            neighbor_max = max(neighbor_max, neighbor);
        }
    }

    // Variance clip
    float4 mu = (neighbor_min + neighbor_max) * 0.5;
    float4 sigma = (neighbor_max - neighbor_min) * 0.5;
    float4 aabb_min = mu - sigma * 1.5;
    float4 aabb_max = mu + sigma * 1.5;

    history_color = clamp(history_color, aabb_min, aabb_max);

    // Blend
    float blend_factor = 0.1;  // Favor history for stability

    // Increase blend factor for fast motion or disoccluded areas
    float motion_weight = saturate(length(vel) * 10.0);
    blend_factor = mix(blend_factor, 0.5, motion_weight);

    float4 result = mix(history_color, current_color, blend_factor);
    output.write(result, gid);
}
```

C code:
```c
void taa_render(taa_system_t* taa, id<MTLCommandBuffer> cmd,
                id<MTLTexture> current_frame, id<MTLTexture> depth,
                id<MTLTexture> output) {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

    uint32_t prev_history = taa->current_history;
    taa->current_history = 1 - taa->current_history;

    [encoder setComputePipelineState:taa->resolve_pipeline];
    [encoder setTexture:current_frame atIndex:0];
    [encoder setTexture:taa->history_texture[prev_history] atIndex:1];
    [encoder setTexture:taa->velocity_texture atIndex:2];
    [encoder setTexture:depth atIndex:3];
    [encoder setTexture:taa->history_texture[taa->current_history] atIndex:4];  // Also output to history

    struct TAAUniforms uniforms = {
        .screen_size = simd_make_uint2(current_frame.width, current_frame.height)
    };
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:0];

    MTLSize groups = MTLSizeMake((current_frame.width + 7) / 8, (current_frame.height + 7) / 8, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

    // Copy result to output
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
    [blit copyFromTexture:taa->history_texture[taa->current_history] toTexture:output];
    [blit endEncoding];

    [encoder endEncoding];
    taa->frame_index++;
}
```

OUTPUT: TAA with variance clipping and motion handling.
```

---

## Agent 5.5: Decal System

```
TASK: Implement Deferred Decal System (Phase 5, Agent 5)

You are implementing a deferred decal system.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/effects/decals/

WHAT TO IMPLEMENT:
1. Decal volume rendering
2. Decal projection into G-buffer
3. Decal sorting by priority
4. Normal blending options

OUTPUT: Deferred decals that modify G-buffer.
```

---

## Agent 5.6: Debug Visualization

```
TASK: Implement Debug Visualization Tools (Phase 5, Agent 6)

You are implementing debug visualization for development.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/editor/debug/

WHAT TO IMPLEMENT:
1. Debug line rendering (immediate mode)
2. Debug shapes (sphere, box, cone, frustum)
3. Debug text overlay
4. Buffer visualization modes
5. Performance overlay

METAL DEBUG RENDERING:
```c
typedef struct debug_line {
    simd_float3 start;
    simd_float4 start_color;
    simd_float3 end;
    simd_float4 end_color;
} debug_line_t;

typedef struct debug_renderer {
    id<MTLBuffer> line_buffer;
    uint32_t line_count;
    uint32_t max_lines;
    id<MTLRenderPipelineState> line_pipeline;
    id<MTLDepthStencilState> depth_test_state;
    id<MTLDepthStencilState> no_depth_state;
} debug_renderer_t;

void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end, simd_float4 color) {
    if (dbg->line_count >= dbg->max_lines) return;

    debug_line_t* lines = (debug_line_t*)[dbg->line_buffer contents];
    lines[dbg->line_count++] = (debug_line_t){
        .start = start,
        .start_color = color,
        .end = end,
        .end_color = color
    };
}

void debug_draw_aabb(debug_renderer_t* dbg, simd_float3 min, simd_float3 max, simd_float4 color) {
    // 12 edges
    simd_float3 corners[8] = {
        simd_make_float3(min.x, min.y, min.z),
        simd_make_float3(max.x, min.y, min.z),
        simd_make_float3(max.x, max.y, min.z),
        simd_make_float3(min.x, max.y, min.z),
        simd_make_float3(min.x, min.y, max.z),
        simd_make_float3(max.x, min.y, max.z),
        simd_make_float3(max.x, max.y, max.z),
        simd_make_float3(min.x, max.y, max.z),
    };

    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    for (int i = 0; i < 12; i++) {
        debug_draw_line(dbg, corners[edges[i][0]], corners[edges[i][1]], color);
    }
}

void debug_render(debug_renderer_t* dbg, id<MTLRenderCommandEncoder> encoder,
                  simd_float4x4 view_proj, bool depth_test) {
    if (dbg->line_count == 0) return;

    [encoder setRenderPipelineState:dbg->line_pipeline];
    [encoder setDepthStencilState:depth_test ? dbg->depth_test_state : dbg->no_depth_state];
    [encoder setVertexBuffer:dbg->line_buffer offset:0 atIndex:0];
    [encoder setVertexBytes:&view_proj length:sizeof(simd_float4x4) atIndex:1];

    [encoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:dbg->line_count * 2];
}

void debug_clear(debug_renderer_t* dbg) {
    dbg->line_count = 0;
}
```

OUTPUT: Debug visualization tools for development.
```
