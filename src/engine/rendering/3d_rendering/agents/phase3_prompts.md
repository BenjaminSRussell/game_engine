# Phase 3: Lighting & Shadows - Agent Prompts

## Agent 3.1: Light Sources & Management

```
TASK: Implement Light Sources & Light Management (Phase 3, Agent 1)

You are implementing the lighting system for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/lighting/sources/
- src/engine/rendering/3d_rendering/lighting/probes/
- src/engine/rendering/3d_rendering/reflection_captures/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. point_light.c - Point light with attenuation
2. spot_light.c - Spotlight with cone angle
3. directional_light.c - Sun/moon directional light
4. rect_light.c - Rectangular area lights (LTC)
5. sphere_light.c - Spherical area lights
6. light_culling.c - Tile/cluster light assignment
7. reflection_probe.c - Cubemap reflection probes
8. irradiance_probe.c - Diffuse irradiance probes
9. probe_blending.c - Blend multiple probes
10. probe_parallax.c - Parallax-corrected cubemaps

IMPLEMENTATION GUIDELINES:
- Use inverse square falloff with radius cutoff
- Cluster/tile-based light culling for forward+
- Area lights use LTC (Linearly Transformed Cosines)
- Reflection probes need parallax correction for accuracy
- Irradiance probes store SH coefficients

KEY PATTERNS:
```c
typedef struct point_light {
    vec3_t position;
    vec3_t color;
    float intensity;
    float radius;
    bool cast_shadows;
    shadow_handle_t shadow;
} point_light_t;

typedef struct light_cluster {
    uint32_t offset;
    uint32_t count;
} light_cluster_t;

// Clustered light culling
void assign_lights_to_clusters(
    light_t* lights, uint32_t light_count,
    frustum_t* cluster_frustums, uint32_t cluster_count,
    light_cluster_t* out_clusters, uint32_t* out_light_indices
);

// Area light (LTC)
vec3_t evaluate_rect_light(
    rect_light_t* light,
    vec3_t P, vec3_t N, vec3_t V,
    float roughness,
    texture_handle_t ltc_mat, texture_handle_t ltc_amp
);
```

FOCUS: Proper light falloff and culling are essential. Area lights add realism.
```

---

## Agent 3.2: Shadow Mapping

```
TASK: Implement Shadow Mapping Systems (Phase 3, Agent 2)

You are implementing shadow mapping for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/lighting/shadows/
- src/engine/rendering/3d_rendering/lighting/cascades/
- src/engine/rendering/3d_rendering/shadows_advanced/shadow_techniques/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. shadow_atlas.c - Shadow map atlas allocation
2. shadow_caster.c - Shadow pass rendering
3. pcf_filter.c - PCF shadow filtering
4. vsm_shadows.c - Variance shadow maps
5. evsm_shadows.c - Exponential variance shadows
6. cascade_splits.c - CSM cascade split calculation
7. cascade_selection.c - Runtime cascade lookup
8. cascade_blending.c - Blend between cascades
9. cascade_stabilization.c - Stabilize cascade movement
10. shadow_cache.c - Cache static shadow maps

IMPLEMENTATION GUIDELINES:
- Shadow atlas: pack multiple lights into one texture
- CSM: 4 cascades typical, logarithmic splits
- Stabilize cascade matrices to prevent shimmer
- VSM enables soft shadows via filtering
- Cache shadows for static lights

KEY PATTERNS:
```c
typedef struct shadow_atlas {
    texture_handle_t texture;
    uint32_t size;
    rect_allocator_t allocator;
} shadow_atlas_t;

typedef struct cascade_shadow {
    mat4_t view_proj[4];
    float split_depths[4];
    texture_handle_t cascade_maps[4];
} cascade_shadow_t;

// Cascade split calculation (practical split scheme)
void calculate_cascade_splits(float near, float far, float lambda, float* splits, int count) {
    for (int i = 0; i < count; i++) {
        float p = (float)(i + 1) / count;
        float log_split = near * powf(far / near, p);
        float uniform_split = near + (far - near) * p;
        splits[i] = lambda * log_split + (1 - lambda) * uniform_split;
    }
}

// PCF filtering
float sample_shadow_pcf(texture_handle_t shadow_map, vec3_t shadow_coord, float bias) {
    float shadow = 0.0;
    vec2_t texel_size = 1.0 / textureSize(shadow_map);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            shadow += sample_shadow(shadow_map, shadow_coord.xy + vec2(x,y) * texel_size, shadow_coord.z - bias);
        }
    }
    return shadow / 9.0;
}
```

FOCUS: Shadow quality defines scene believability. CSM is essential for outdoor scenes.
```

---

## Agent 3.3: Virtual Shadow Maps

```
TASK: Implement Virtual Shadow Maps (Phase 3, Agent 3)

You are implementing virtual shadow maps for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/virtual_shadow_maps/
- src/engine/rendering/3d_rendering/shadows_advanced/ray_traced_shadows/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. vsm_page_table.c - Virtual shadow map page table
2. vsm_page_pool.c - Physical page pool management
3. vsm_allocation.c - On-demand page allocation
4. vsm_invalidation.c - Invalidate dirty pages
5. vsm_culling.c - Cull geometry per page
6. vsm_rasterization.c - Render shadow into pages
7. vsm_sampling.c - Sample virtual shadow map
8. rt_shadow_ray.c - Ray traced shadow rays
9. rt_shadow_denoise.c - Denoise RT shadows
10. rt_penumbra.c - Soft shadow penumbra

IMPLEMENTATION GUIDELINES:
- VSM: 16K x 16K virtual texture, 128x128 physical pages
- Only allocate pages where shadows are sampled
- Clipmap for directional light (multiple resolutions)
- RT shadows for sharp contact shadows
- Denoise RT shadows temporally

KEY PATTERNS:
```c
typedef struct virtual_shadow_map {
    texture_handle_t page_table;      // R32UI: physical page index
    texture_handle_t physical_pages;  // Array texture of shadow pages
    uint32_t virtual_size;            // e.g., 16384
    uint32_t page_size;               // e.g., 128
    page_pool_t* page_pool;
} virtual_shadow_map_t;

// Page table lookup
uint32_t vsm_get_page(virtual_shadow_map_t* vsm, ivec2_t virtual_coord) {
    ivec2_t page_coord = virtual_coord / vsm->page_size;
    return sample_page_table(vsm->page_table, page_coord);
}

// Allocate page on demand
void vsm_request_page(virtual_shadow_map_t* vsm, ivec2_t page_coord) {
    if (!page_allocated(vsm, page_coord)) {
        uint32_t physical_page = page_pool_alloc(vsm->page_pool);
        set_page_table(vsm->page_table, page_coord, physical_page);
        mark_page_dirty(vsm, physical_page);
    }
}

// RT shadow (1 ray per pixel)
float trace_shadow_ray(vec3_t origin, vec3_t light_dir, float max_dist) {
    ray_t ray = { origin + normal * 0.001, light_dir, 0.0, max_dist };
    return trace_visibility(ray) ? 1.0 : 0.0;
}
```

FOCUS: VSM provides high-resolution shadows everywhere. RT shadows add physical accuracy.
```

---

## Agent 3.4: Global Illumination Basics

```
TASK: Implement Basic Global Illumination (Phase 3, Agent 4)

You are implementing basic GI systems for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/lighting/global_illumination/
- src/engine/rendering/3d_rendering/lighting/lightmaps/
- src/engine/rendering/3d_rendering/lightmap_baking/
- src/engine/rendering/3d_rendering/gi_baking/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. diffuse_gi.c - Diffuse indirect lighting
2. specular_gi.c - Specular GI (reflections)
3. gi_probe_grid.c - Irradiance probe grid
4. indirect_lighting.c - Combine direct + indirect
5. lightmap_baker.c - Bake lightmaps (path trace)
6. lightmap_uvs.c - Generate lightmap UVs
7. lightmap_packer.c - Pack lightmaps into atlas
8. lightmap_sampling.c - Sample lightmaps at runtime
9. directional_lightmaps.c - Store directional info
10. gi_debug_viz.c - Visualize GI for debugging

IMPLEMENTATION GUIDELINES:
- Probe grid: 3D grid of SH probes
- Lightmaps for static geometry
- Directional lightmaps for normal-dependent lighting
- Path trace baker for high quality
- Blend probes by distance

KEY PATTERNS:
```c
typedef struct irradiance_probe {
    vec3_t position;
    vec3_t sh_coefficients[9];  // L0, L1, L2 SH bands
} irradiance_probe_t;

typedef struct probe_grid {
    irradiance_probe_t* probes;
    ivec3_t resolution;
    vec3_t min_bounds;
    vec3_t max_bounds;
} probe_grid_t;

// Sample probe grid
vec3_t sample_irradiance(probe_grid_t* grid, vec3_t position, vec3_t normal) {
    // Find 8 surrounding probes
    ivec3_t base = world_to_grid(grid, position);
    vec3_t irradiance = vec3_zero();

    for (int i = 0; i < 8; i++) {
        ivec3_t offset = corner_offsets[i];
        irradiance_probe_t* probe = get_probe(grid, base + offset);
        float weight = trilinear_weight(position, probe->position, grid);
        irradiance += weight * evaluate_sh(probe->sh_coefficients, normal);
    }
    return irradiance;
}

// Lightmap baking
void bake_lightmap(mesh_t* mesh, lightmap_t* output) {
    for (each texel in output) {
        vec3_t world_pos = lightmap_to_world(mesh, texel_coord);
        vec3_t normal = get_normal_at(mesh, texel_coord);
        vec3_t irradiance = path_trace_irradiance(world_pos, normal, num_samples);
        output->data[texel] = irradiance;
    }
}
```

FOCUS: GI transforms flat lighting into believable environments. Lightmaps are free at runtime.
```

---

## Agent 3.5: Lumen GI System

```
TASK: Implement Lumen-style Real-time GI (Phase 3, Agent 5)

You are implementing Lumen-style real-time global illumination for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/lumen/surface_cache/
- src/engine/rendering/3d_rendering/lumen/radiance_cache/
- src/engine/rendering/3d_rendering/lumen/tracing_lumen/
- src/engine/rendering/3d_rendering/lumen/final_gather/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. surface_atlas.c - Surface cache texture atlas
2. surface_card.c - Generate surface cards from meshes
3. surface_capture.c - Capture surface radiance
4. surface_update.c - Incremental surface cache updates
5. probe_octahedron.c - Octahedral probe encoding
6. radiance_inject.c - Inject radiance into probe grid
7. screen_trace.c - Screen-space ray tracing
8. software_trace.c - Software ray trace against SDF/mesh
9. final_gather_rays.c - Final gather ray generation
10. denoise_lumen.c - Temporal denoising

IMPLEMENTATION GUIDELINES:
- Surface cache: rasterize scene from multiple views
- Radiance cache: probe grid with octahedral encoding
- Screen trace first (fast), fallback to software trace
- Final gather: trace rays from camera, sample caches
- Heavy temporal filtering for stability

KEY PATTERNS:
```c
typedef struct surface_cache {
    texture_handle_t atlas;           // Surface card atlas
    surface_card_t* cards;            // Card metadata
    uint32_t card_count;
    uint32_t atlas_size;
} surface_cache_t;

typedef struct radiance_cache {
    texture_handle_t probe_texture;   // 3D texture of octahedral probes
    ivec3_t resolution;
    vec3_t bounds_min, bounds_max;
} radiance_cache_t;

// Surface card capture
void capture_surface_card(surface_card_t* card, scene_t* scene) {
    // Render scene from card's viewpoint
    mat4_t view = look_at(card->position, card->position + card->normal, up);
    render_to_card(scene, view, card->atlas_region);
}

// Screen-space trace
bool screen_trace(vec3_t origin, vec3_t dir, texture_handle_t depth, texture_handle_t normal,
                  vec3_t* hit_pos, vec3_t* hit_normal) {
    // Ray march in screen space
    vec3_t ray_pos = origin;
    for (int i = 0; i < MAX_STEPS; i++) {
        ray_pos += dir * step_size;
        vec2_t screen_pos = world_to_screen(ray_pos);
        float scene_depth = sample_depth(depth, screen_pos);
        if (ray_pos.z > scene_depth) {
            *hit_pos = ray_pos;
            *hit_normal = sample_normal(normal, screen_pos);
            return true;
        }
    }
    return false;
}
```

FOCUS: Lumen provides real-time GI. The surface cache is the key innovation.
```

---

## Agent 3.6: Volumetric Lighting

```
TASK: Implement Volumetric Lighting & Atmosphere (Phase 3, Agent 6)

You are implementing volumetric lighting and atmosphere for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/lighting/volumetric/
- src/engine/rendering/3d_rendering/volumetric_fog_advanced/
- src/engine/rendering/3d_rendering/atmosphere/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. volumetric_fog.c - Volumetric fog rendering
2. volumetric_lighting.c - God rays, light shafts
3. froxel_grid.c - Froxel (frustum voxel) allocation
4. participating_media.c - Scattering/absorption
5. temporal_reprojection.c - Temporal filtering for volumetrics
6. atmospheric_scattering.c - Rayleigh/Mie scattering
7. sky_lut.c - Precompute sky lookup tables
8. aerial_perspective.c - Distance-based atmosphere
9. sun_disk.c - Sun rendering with bloom
10. volumetric_clouds.c - Cloud density and lighting

IMPLEMENTATION GUIDELINES:
- Froxel grid: 3D texture covering view frustum
- Ray march through froxels, accumulate scattering
- Use temporal reprojection for stability
- Sky LUT: precompute for sun angle
- Volumetric clouds use noise + ray marching

KEY PATTERNS:
```c
typedef struct froxel_grid {
    texture_handle_t scattering;      // RGBA16F: inscatter.rgb, transmittance.a
    texture_handle_t density;         // R16F: participating media density
    ivec3_t resolution;               // e.g., 160x90x64
    float near, far;
} froxel_grid_t;

// Froxel ray march
void compute_volumetric_fog(froxel_grid_t* grid, light_t* lights, uint32_t light_count) {
    for (each froxel) {
        vec3_t world_pos = froxel_to_world(grid, froxel_coord);
        float density = sample_density(grid, world_pos);

        vec3_t inscatter = vec3_zero();
        for (each light) {
            float shadow = sample_shadow(light, world_pos);
            inscatter += light->color * phase_function(view_dir, light_dir) * shadow * density;
        }

        grid->scattering[froxel_coord] = inscatter;
    }
}

// Apply volumetric fog to final image
vec3_t apply_volumetric(vec3_t color, vec2_t uv, float depth) {
    vec3_t inscatter = sample_froxel_scattering(froxel_grid, uv, depth);
    float transmittance = sample_froxel_transmittance(froxel_grid, uv, depth);
    return color * transmittance + inscatter;
}
```

FOCUS: Volumetric lighting adds depth and atmosphere. Essential for outdoor and moody scenes.
```
