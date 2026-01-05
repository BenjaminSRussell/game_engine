# Phase 5: Effects & Post-Processing - Agent Prompts

## Agent 5.1: Particle Systems

```
TASK: Implement Particle Systems (Phase 5, Agent 1)

You are implementing CPU and GPU particle systems for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/effects/particles/
- src/engine/rendering/3d_rendering/effects/gpu_particles/
- src/engine/rendering/3d_rendering/effects/vfx_graph/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. particle_emitter.c - Emitter configuration (rate, shape, velocity)
2. particle_simulation.c - CPU particle update
3. gpu_particles.c - GPU compute particle system
4. particle_buffer.c - GPU particle storage (SoA)
5. spawn_compute.c - GPU spawn shader
6. update_compute.c - GPU update shader
7. dead_list.c - Recycle dead particles
8. particle_rendering.c - Billboard rendering
9. particle_sorting.c - Back-to-front sorting
10. vfx_context.c - VFX graph runtime context

IMPLEMENTATION GUIDELINES:
- GPU particles: compute spawn + update, indirect draw
- Dead list: append-consume buffer for recycling
- Sort transparent particles back-to-front
- Support velocity stretching, rotation
- VFX graph: node-based particle authoring

KEY PATTERNS:
```c
typedef struct particle {
    vec3_t position;
    vec3_t velocity;
    vec4_t color;
    float size;
    float rotation;
    float lifetime;
    float age;
} particle_t;

typedef struct gpu_particle_system {
    buffer_handle_t particle_buffer;      // SoA particle data
    buffer_handle_t dead_list;            // Available indices
    buffer_handle_t alive_list;           // Active particle indices
    buffer_handle_t indirect_args;        // Draw indirect args
    buffer_handle_t counter;              // Atomic counters
    uint32_t max_particles;
} gpu_particle_system_t;

// GPU spawn (compute shader support)
void dispatch_spawn(gpu_particle_system_t* sys, uint32_t spawn_count, emitter_t* emitter) {
    bind_buffer(sys->particle_buffer, 0);
    bind_buffer(sys->dead_list, 1);
    bind_buffer(sys->counter, 2);
    set_uniform("spawn_count", spawn_count);
    set_uniform("emitter_pos", emitter->position);
    set_uniform("emitter_velocity", emitter->velocity);
    dispatch_compute(ceil(spawn_count / 64.0), 1, 1);
}

// GPU update
void dispatch_update(gpu_particle_system_t* sys, float dt) {
    bind_buffer(sys->particle_buffer, 0);
    bind_buffer(sys->alive_list, 1);
    bind_buffer(sys->dead_list, 2);
    set_uniform("dt", dt);
    set_uniform("gravity", vec3(0, -9.8, 0));
    dispatch_compute_indirect(sys->counter);  // Dispatch based on alive count
}
```

FOCUS: GPU particles enable millions of particles. The dead list pattern is key for recycling.
```

---

## Agent 5.2: Environmental Effects

```
TASK: Implement Environmental Effects (Phase 5, Agent 2)

You are implementing environmental effects for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/effects/weather/
- src/engine/rendering/3d_rendering/effects/decals/
- src/engine/rendering/3d_rendering/effects/trails/
- src/engine/rendering/3d_rendering/puddles/
- src/engine/rendering/3d_rendering/footprints/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. rain_system.c - Rain particles + streaks
2. snow_system.c - Snow accumulation
3. weather_volumes.c - Weather zone transitions
4. decal_projector.c - Deferred decal projection
5. decal_rendering.c - Decal blending to G-buffer
6. decal_sorting.c - Decal priority/order
7. trail_renderer.c - Trail/ribbon mesh generation
8. trail_points.c - Trail point sampling
9. puddle_rendering.c - Dynamic puddle reflections
10. footprint_system.c - Deformable snow/sand

IMPLEMENTATION GUIDELINES:
- Rain: GPU particles + screen-space streaks
- Decals: project box onto G-buffer, blend
- Trails: generate mesh from point history
- Puddles: planar reflections or SSR
- Footprints: displacement + normal modification

KEY PATTERNS:
```c
typedef struct decal {
    mat4_t projection;         // World -> decal space
    texture_handle_t albedo;
    texture_handle_t normal;
    float opacity;
    uint32_t sort_order;
} decal_t;

// Deferred decal rendering
void render_decals(gbuffer_t* gbuffer, decal_t* decals, uint32_t count) {
    bind_texture(gbuffer->depth, 0);

    for (uint32_t i = 0; i < count; i++) {
        decal_t* d = &decals[i];

        // Reconstruct world pos from depth
        // Project into decal space
        // Sample decal textures
        // Blend with G-buffer

        set_uniform("decal_matrix", d->projection);
        bind_texture(d->albedo, 1);
        bind_texture(d->normal, 2);
        draw_box();  // Decal volume
    }
}

// Trail mesh generation
void generate_trail_mesh(trail_t* trail, vertex_t* out_verts, uint32_t* out_count) {
    for (int i = 0; i < trail->point_count - 1; i++) {
        vec3_t p0 = trail->points[i].position;
        vec3_t p1 = trail->points[i+1].position;

        vec3_t dir = normalize(p1 - p0);
        vec3_t right = cross(dir, camera_up) * trail->points[i].width;

        // Quad vertices
        out_verts[*out_count++] = (vertex_t){ p0 - right, vec2(0, i / trail->point_count) };
        out_verts[*out_count++] = (vertex_t){ p0 + right, vec2(1, i / trail->point_count) };
        // ... etc
    }
}

// Footprint deformation
void apply_footprint(heightmap_t* ground, vec3_t position, texture_handle_t foot_shape) {
    // Stamp foot shape into heightmap
    // Update normals in affected region
}
```

FOCUS: Environmental effects make worlds feel alive. Decals add detail without geometry.
```

---

## Agent 5.3: Special Effects

```
TASK: Implement Special Effects (Phase 5, Agent 3)

You are implementing special visual effects for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/fire_rendering/
- src/engine/rendering/3d_rendering/smoke_rendering/
- src/engine/rendering/3d_rendering/explosion_effects/
- src/engine/rendering/3d_rendering/liquid_rendering/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. fire_simulation.c - Fire particle/volume simulation
2. fire_shading.c - Fire color and emission
3. fire_distortion.c - Heat distortion
4. smoke_density.c - Smoke density field
5. smoke_lighting.c - Smoke self-shadowing
6. smoke_rendering.c - Ray march smoke volumes
7. explosion_debris.c - Explosion debris particles
8. explosion_shockwave.c - Shockwave distortion
9. liquid_surface.c - Liquid surface mesh
10. liquid_splashes.c - Splash particle effects

IMPLEMENTATION GUIDELINES:
- Fire: blend particles with volumetric
- Smoke: 3D density texture, ray march
- Explosions: particles + decals + distortion
- Liquids: metaballs or mesh-based

KEY PATTERNS:
```c
// Fire rendering
typedef struct fire_effect {
    particle_system_t* flames;
    particle_system_t* embers;
    texture_handle_t noise_3d;
    vec3_t base_color;
    vec3_t tip_color;
    float intensity;
} fire_effect_t;

void render_fire(fire_effect_t* fire, camera_t* camera) {
    // 1. Render flame particles with additive blending
    render_particles(fire->flames, BLEND_ADDITIVE);

    // 2. Render embers
    render_particles(fire->embers, BLEND_ADDITIVE);

    // 3. Apply heat distortion (post-process)
    apply_heat_distortion(fire->position, fire->intensity);
}

// Smoke ray marching
vec4_t raymarch_smoke(texture_handle_t density, vec3_t ray_origin, vec3_t ray_dir, float max_dist) {
    vec3_t accumulated_color = vec3_zero();
    float accumulated_alpha = 0.0;

    float step_size = max_dist / NUM_STEPS;
    vec3_t pos = ray_origin;

    for (int i = 0; i < NUM_STEPS; i++) {
        float density = sample_density_field(density, pos);

        if (density > 0.01) {
            // Self-shadowing: trace toward light
            float shadow = trace_shadow(pos, light_dir);

            vec3_t color = SMOKE_COLOR * shadow * density;
            float alpha = density * step_size;

            accumulated_color += color * (1.0 - accumulated_alpha);
            accumulated_alpha += alpha * (1.0 - accumulated_alpha);

            if (accumulated_alpha > 0.99) break;
        }

        pos += ray_dir * step_size;
    }

    return vec4(accumulated_color, accumulated_alpha);
}

// Explosion shockwave
void render_shockwave(vec3_t center, float radius, float thickness) {
    // Distort UVs based on distance to shockwave sphere
    // Sample scene with distorted UVs
}
```

FOCUS: Special effects create memorable moments. Balance quality with performance.
```

---

## Agent 5.4: Tonemapping & Color

```
TASK: Implement Tonemapping & Color Processing (Phase 5, Agent 4)

You are implementing HDR tonemapping and color processing for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/postprocessing/tonemapping/
- src/engine/rendering/3d_rendering/postprocessing/bloom/
- src/engine/rendering/3d_rendering/postprocessing/ambient_occlusion/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. aces_tonemapper.c - ACES filmic tonemapping
2. reinhard_tonemapper.c - Reinhard extended
3. exposure_adaptation.c - Auto exposure
4. histogram_compute.c - Luminance histogram (compute)
5. bloom_downsample.c - Bloom threshold + downsample
6. bloom_upsample.c - Tent filter upsample
7. bloom_lens_dirt.c - Lens dirt overlay
8. ssao_pass.c - SSAO implementation
9. gtao_pass.c - Ground Truth AO
10. ao_blur.c - Bilateral AO blur

IMPLEMENTATION GUIDELINES:
- ACES: industry standard, preserve colors
- Auto-exposure: histogram-based, smooth adaptation
- Bloom: 13-tap tent filter, 5-6 mip levels
- GTAO: better quality than SSAO, similar cost

KEY PATTERNS:
```c
// ACES tonemapping
vec3_t aces_tonemap(vec3_t color) {
    // sRGB -> AP0
    mat3_t srgb_to_ap0 = /* ... */;
    vec3_t ap0 = mat3_mul_vec3(srgb_to_ap0, color);

    // RRT + ODT
    vec3_t a = ap0 * (ap0 + 0.0245786) - 0.000090537;
    vec3_t b = ap0 * (0.983729 * ap0 + 0.4329510) + 0.238081;
    return a / b;
}

// Auto exposure with histogram
void compute_exposure(texture_handle_t hdr, float* target_exposure) {
    // 1. Compute luminance histogram (compute shader)
    dispatch_histogram_compute(hdr, histogram_buffer);

    // 2. Find average luminance (ignoring outliers)
    float avg_lum = analyze_histogram(histogram_buffer, 0.1, 0.9);

    // 3. Calculate target exposure
    float target = 0.18 / avg_lum;  // Middle gray

    // 4. Smooth adaptation
    *target_exposure = lerp(*target_exposure, target, adaptation_speed * dt);
}

// Bloom (physically-based)
void render_bloom(texture_handle_t hdr, texture_handle_t* bloom_out) {
    // 1. Threshold + downsample
    texture_handle_t mip0 = bloom_threshold(hdr, threshold);

    // 2. Progressive downsample (13-tap)
    texture_handle_t mips[6];
    mips[0] = mip0;
    for (int i = 1; i < 6; i++) {
        mips[i] = bloom_downsample_13tap(mips[i-1]);
    }

    // 3. Upsample and accumulate
    texture_handle_t result = mips[5];
    for (int i = 4; i >= 0; i--) {
        result = bloom_upsample_tent(result, mips[i]);
    }

    *bloom_out = result;
}

// GTAO
float gtao(vec2_t uv, texture_handle_t depth, texture_handle_t normal) {
    vec3_t view_pos = reconstruct_position(depth, uv);
    vec3_t view_normal = sample_normal(normal, uv);

    float ao = 0.0;
    for (int slice = 0; slice < NUM_SLICES; slice++) {
        vec2_t dir = slice_direction(slice);

        // March along horizon in both directions
        float h1 = find_horizon(view_pos, dir, depth);
        float h2 = find_horizon(view_pos, -dir, depth);

        // Integrate AO for this slice
        ao += integrate_arc(view_normal, h1, h2, dir);
    }

    return ao / NUM_SLICES;
}
```

FOCUS: Tonemapping defines the look of your game. Bloom and AO add depth and polish.
```

---

## Agent 5.5: Anti-Aliasing & Temporal

```
TASK: Implement Anti-Aliasing & Temporal Effects (Phase 5, Agent 5)

You are implementing anti-aliasing and temporal processing for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/postprocessing/anti_aliasing/
- src/engine/rendering/3d_rendering/postprocessing/motion_blur/
- src/engine/rendering/3d_rendering/temporal_upscaling/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. taa_resolve.c - TAA temporal resolve
2. taa_velocity.c - Velocity buffer generation
3. taa_history.c - History buffer management
4. taa_sharpening.c - TAA sharpen pass
5. fxaa_pass.c - FXAA implementation
6. smaa_pass.c - SMAA edge detection + blend
7. velocity_buffer.c - Per-pixel motion vectors
8. motion_blur_gather.c - Motion blur gather
9. motion_blur_tile.c - Tile-based motion blur
10. dlss_wrapper.c - DLSS/FSR integration wrapper

IMPLEMENTATION GUIDELINES:
- TAA: jitter projection, reproject history, clamp
- Velocity: current - previous position in screen space
- Motion blur: variable-length based on velocity
- DLSS/FSR: wrapper for vendor implementations

KEY PATTERNS:
```c
// TAA resolve
vec3_t taa_resolve(vec2_t uv, texture_handle_t current, texture_handle_t history,
                   texture_handle_t velocity, texture_handle_t depth) {
    // 1. Sample current frame
    vec3_t current_color = sample(current, uv);

    // 2. Get velocity and reproject
    vec2_t vel = sample(velocity, uv).xy;
    vec2_t history_uv = uv - vel;

    // 3. Sample history
    vec3_t history_color = sample(history, history_uv);

    // 4. Neighborhood clamp (variance clip)
    vec3_t neighborhood_min, neighborhood_max;
    compute_neighborhood_aabb(current, uv, &neighborhood_min, &neighborhood_max);
    history_color = clip_aabb(history_color, neighborhood_min, neighborhood_max);

    // 5. Blend
    float blend_factor = 0.1;  // Favor history
    if (out_of_bounds(history_uv)) blend_factor = 1.0;

    return lerp(history_color, current_color, blend_factor);
}

// Jitter pattern (Halton 2,3)
vec2_t get_taa_jitter(int frame_index) {
    return vec2(
        halton(frame_index, 2) - 0.5,
        halton(frame_index, 3) - 0.5
    ) / screen_size;
}

// Motion blur
vec4_t motion_blur(vec2_t uv, texture_handle_t color, texture_handle_t velocity) {
    vec2_t vel = sample(velocity, uv).xy;
    float speed = length(vel);

    if (speed < 0.001) return sample(color, uv);

    vec2_t dir = vel / speed;
    int num_samples = clamp(int(speed * MAX_SAMPLES), 1, MAX_SAMPLES);

    vec4_t result = vec4_zero();
    for (int i = 0; i < num_samples; i++) {
        float t = (float(i) / num_samples) - 0.5;
        vec2_t sample_uv = uv + dir * t * speed;
        result += sample(color, sample_uv);
    }

    return result / num_samples;
}

// DLSS/FSR wrapper
typedef struct upscaler {
    upscaler_type_t type;  // DLSS, FSR, XeSS
    void* impl;            // Vendor implementation
} upscaler_t;

void upscale(upscaler_t* up, texture_handle_t input, texture_handle_t output,
             texture_handle_t depth, texture_handle_t velocity, float sharpness) {
    switch (up->type) {
        case UPSCALER_DLSS: dlss_evaluate(up->impl, ...); break;
        case UPSCALER_FSR:  fsr_dispatch(up->impl, ...); break;
        case UPSCALER_XESS: xess_execute(up->impl, ...); break;
    }
}
```

FOCUS: TAA is essential for modern rendering. Temporal stability reduces aliasing without blur.
```

---

## Agent 5.6: Screen-Space Effects

```
TASK: Implement Screen-Space Effects (Phase 5, Agent 6)

You are implementing screen-space effects for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/postprocessing/screen_space/
- src/engine/rendering/3d_rendering/postprocessing/depth_of_field/
- src/engine/rendering/3d_rendering/screen_space_gi/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. ssr_trace.c - Screen-space reflection ray march
2. ssr_resolve.c - SSR temporal resolve/filter
3. ssr_fallback.c - SSR to cubemap fallback
4. ss_gi.c - Screen-space global illumination
5. ss_shadows.c - Screen-space contact shadows
6. dof_coc.c - Circle of confusion calculation
7. dof_bokeh.c - Bokeh shape (hexagonal, circular)
8. dof_gather.c - DOF gather pass
9. dof_combine.c - Near/far field combination
10. lens_flare.c - Lens flare sprites

IMPLEMENTATION GUIDELINES:
- SSR: hierarchical ray march, binary search refine
- Fallback to reflection probes where SSR fails
- DOF: separate near/far fields, proper CoC dilation
- Contact shadows: short ray march toward light

KEY PATTERNS:
```c
// SSR hierarchical trace
bool ssr_trace(vec3_t origin, vec3_t dir, texture_handle_t depth,
               vec2_t* hit_uv, float* hit_depth) {
    // Start at high mip, step coarsely
    // Drop to lower mip when potential hit
    // Binary search at mip 0 for precise hit

    int mip = MAX_MIP;
    vec3_t ray = origin;
    float step = initial_step;

    for (int i = 0; i < MAX_STEPS; i++) {
        ray += dir * step;
        vec2_t uv = world_to_screen(ray);

        float scene_depth = sample_depth_mip(depth, uv, mip);

        if (ray.z > scene_depth) {
            if (mip == 0) {
                // Hit! Binary search for precision
                *hit_uv = binary_search(origin, dir, depth);
                return true;
            }
            // Step back and decrease mip
            ray -= dir * step;
            mip--;
            step *= 0.5;
        }
    }

    return false;  // No hit
}

// Depth of Field
typedef struct dof_params {
    float focus_distance;
    float aperture;        // f-stop
    float focal_length;    // mm
} dof_params_t;

float calculate_coc(float depth, dof_params_t* params) {
    // Circle of confusion from thin lens equation
    float coc = abs(params->aperture * params->focal_length *
                   (params->focus_distance - depth) /
                   (depth * (params->focus_distance - params->focal_length)));
    return coc * sensor_height;  // In pixels
}

void render_dof(texture_handle_t color, texture_handle_t depth, dof_params_t* params) {
    // 1. Calculate CoC for each pixel
    texture_handle_t coc = compute_coc(depth, params);

    // 2. Separate near and far fields
    texture_handle_t near_field = extract_near(color, coc);
    texture_handle_t far_field = extract_far(color, coc);

    // 3. Blur each field based on CoC
    texture_handle_t near_blurred = gather_dof(near_field, coc);
    texture_handle_t far_blurred = gather_dof(far_field, coc);

    // 4. Composite
    composite_dof(color, near_blurred, far_blurred, coc);
}

// Contact shadows
float contact_shadow(vec3_t pos, vec3_t light_dir, texture_handle_t depth) {
    vec3_t ray = pos;
    float step = 0.01;

    for (int i = 0; i < 16; i++) {
        ray += light_dir * step;
        vec2_t uv = world_to_screen(ray);
        float scene_depth = sample_depth(depth, uv);

        if (ray.z > scene_depth + bias) {
            return 0.0;  // In shadow
        }
        step *= 1.2;  // Increase step size
    }

    return 1.0;  // Lit
}
```

FOCUS: Screen-space effects are cheap but limited. Layer them with world-space for best results.
```
