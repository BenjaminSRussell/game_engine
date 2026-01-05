# Phase 4: Materials & Textures - Agent Prompts

## Agent 4.1: Material System Core

```
TASK: Implement Core Material System (Phase 4, Agent 1)

You are implementing the material system core for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/materials/material_system/
- src/engine/rendering/3d_rendering/materials/textures/
- src/engine/rendering/3d_rendering/materials/layering/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. material_instance.c - Create material instances from templates
2. material_parameters.c - Bind material parameters to shaders
3. material_overrides.c - Per-object material overrides
4. material_sorting.c - Sort by shader/texture for batching
5. material_lod.c - Simplify materials at distance
6. texture_binding.c - Bind textures to material slots
7. texture_defaults.c - Default textures (white, normal, black)
8. material_layers.c - Layer-based material system
9. layer_blending.c - Blend modes (multiply, add, overlay)
10. height_blending.c - Height-based layer blending

IMPLEMENTATION GUIDELINES:
- Materials are instances of shader templates
- Parameters stored in uniform buffer per material
- Support parameter inheritance (instance from master)
- Layers blend using masks and height maps
- Cache material parameter blocks

KEY PATTERNS:
```c
typedef struct material_template {
    shader_handle_t shader;
    parameter_layout_t params;
    texture_slot_t texture_slots[MAX_TEXTURES];
    uint32_t texture_count;
} material_template_t;

typedef struct material_instance {
    material_template_t* template;
    buffer_handle_t parameter_buffer;
    texture_handle_t textures[MAX_TEXTURES];
    uint32_t flags;
} material_instance_t;

// Create instance
material_instance_t* material_create_instance(material_template_t* template) {
    material_instance_t* inst = alloc_material();
    inst->template = template;
    inst->parameter_buffer = buffer_create(template->params.size);
    // Copy default parameters
    buffer_update(inst->parameter_buffer, template->params.defaults, template->params.size);
    return inst;
}

// Set parameter
void material_set_float(material_instance_t* mat, const char* name, float value) {
    uint32_t offset = param_layout_get_offset(&mat->template->params, name);
    buffer_update_region(mat->parameter_buffer, offset, &value, sizeof(float));
}
```

FOCUS: The material system connects art to rendering. Make it flexible and efficient.
```

---

## Agent 4.2: Advanced Shading Models

```
TASK: Implement Advanced Shading Models (Phase 4, Agent 2)

You are implementing advanced shading models for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/shading/subsurface/
- src/engine/rendering/3d_rendering/shading/cloth/
- src/engine/rendering/3d_rendering/shading/hair/
- src/engine/rendering/3d_rendering/shading/clearcoat/
- src/engine/rendering/3d_rendering/shading/special/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. sss_profile.c - Subsurface scattering profiles
2. sss_blur.c - Separable SSS blur pass
3. skin_shading.c - Skin-specific shading model
4. cloth_brdf.c - Cloth BRDF (Charlie/Ashikhmin)
5. sheen_layer.c - Fabric sheen
6. hair_strand_shading.c - Kajiya-Kay / Marschner hair
7. hair_scattering.c - Hair multiple scattering
8. clearcoat_layer.c - Clearcoat on top of base
9. anisotropy.c - Anisotropic highlights
10. iridescence.c - Thin-film interference

IMPLEMENTATION GUIDELINES:
- SSS: separable blur with diffusion profile
- Cloth: Use Charlie distribution, not GGX
- Hair: R, TT, TRT lobes (Marschner)
- Clearcoat: separate roughness, IOR ~1.5
- Anisotropy: tangent-space roughness

KEY PATTERNS:
```c
// Subsurface scattering profile
typedef struct sss_profile {
    vec3_t scatter_distance;  // RGB falloff distances
    vec3_t scatter_color;
    float scale;
} sss_profile_t;

// Separable SSS blur
void sss_blur_pass(texture_handle_t color, texture_handle_t depth,
                   sss_profile_t* profile, vec2_t direction) {
    // Gaussian blur weighted by depth difference
    // Sample along direction, weight by profile
}

// Cloth shading (Charlie distribution)
float D_Charlie(float roughness, float NdotH) {
    float inv_r = 1.0 / roughness;
    float cos2h = NdotH * NdotH;
    float sin2h = 1.0 - cos2h;
    return (2.0 + inv_r) * pow(sin2h, inv_r * 0.5) / (2.0 * PI);
}

// Marschner hair
vec3_t hair_shading(vec3_t T, vec3_t V, vec3_t L, hair_params_t* params) {
    // R: primary specular
    // TT: transmitted through fiber
    // TRT: internal reflection
    float sin_theta_i = dot(T, L);
    float sin_theta_o = dot(T, V);
    // ... compute R, TT, TRT contributions
}
```

FOCUS: These shading models handle materials that standard PBR can't. Essential for characters.
```

---

## Agent 4.3: Texture Streaming & Virtual Texturing

```
TASK: Implement Texture Streaming & Virtual Texturing (Phase 4, Agent 3)

You are implementing texture streaming and virtual texturing for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/texture/streaming/
- src/engine/rendering/3d_rendering/texture/virtual_texturing/
- src/engine/rendering/3d_rendering/texture/compression/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. texture_streamer.c - Stream texture mips on demand
2. stream_priority.c - Priority based on screen coverage
3. resident_mips.c - Track resident mip levels
4. virtual_texture.c - Virtual texture system
5. page_table.c - VT page table management
6. page_cache.c - Physical page cache (LRU)
7. feedback_buffer.c - GPU feedback for requested pages
8. feedback_analysis.c - CPU analysis of feedback
9. bc_compression.c - BC1-BC7 compression
10. astc_compression.c - ASTC compression

IMPLEMENTATION GUIDELINES:
- Stream based on screen-space derivatives (mip level)
- Virtual texture: 128x128 pages, 16K+ virtual size
- Feedback buffer: GPU writes requested page coords
- Use LRU eviction for page cache
- BC7 for quality, BC1 for speed/size

KEY PATTERNS:
```c
typedef struct texture_streamer {
    texture_entry_t* textures;
    uint32_t texture_count;
    priority_queue_t request_queue;
    uint32_t memory_budget;
    uint32_t memory_used;
} texture_streamer_t;

typedef struct virtual_texture {
    texture_handle_t page_table;       // Indirection table
    texture_handle_t physical_cache;   // Physical page storage
    uint32_t virtual_size;             // e.g., 16384
    uint32_t page_size;                // e.g., 128
    page_cache_t cache;
} virtual_texture_t;

// Streaming update
void streamer_update(texture_streamer_t* streamer, camera_t* camera) {
    // Calculate priority for each texture
    for (each texture) {
        float screen_size = calculate_screen_coverage(texture, camera);
        int target_mip = calculate_target_mip(screen_size);
        int current_mip = texture->resident_mip;

        if (target_mip < current_mip) {
            queue_stream_request(streamer, texture, target_mip, screen_size);
        }
    }

    // Process highest priority requests within budget
    while (budget_available && !queue_empty) {
        stream_request_t req = queue_pop(streamer);
        load_texture_mip(req.texture, req.mip);
    }
}

// Virtual texture sampling (shader)
vec4 sample_virtual_texture(virtual_texture_t vt, vec2 uv) {
    ivec2 page_coord = ivec2(uv * vt.virtual_size / vt.page_size);
    vec4 page_info = texelFetch(vt.page_table, page_coord);
    vec2 physical_uv = page_info.xy + fract(uv * vt.virtual_size / vt.page_size) * page_info.z;
    return texture(vt.physical_cache, physical_uv);
}
```

FOCUS: Texture streaming enables infinite texture detail. Virtual texturing eliminates texture limits.
```

---

## Agent 4.4: Specialized Material Rendering

```
TASK: Implement Character Material Rendering (Phase 4, Agent 4)

You are implementing specialized character material rendering for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/skin_rendering/
- src/engine/rendering/3d_rendering/eye_rendering/
- src/engine/rendering/3d_rendering/hair_rendering/
- src/engine/rendering/3d_rendering/fabric_rendering/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. skin_diffusion.c - Skin subsurface diffusion
2. skin_detail.c - Pore detail, wrinkles
3. skin_translucency.c - Ear/nose translucency
4. eye_refraction.c - Cornea refraction
5. eye_caustics.c - Eye caustics
6. iris_detail.c - Iris parallax, color
7. hair_transparency.c - Hair alpha sorting
8. hair_shadows_render.c - Deep opacity maps
9. fabric_detail.c - Weave patterns
10. fabric_sheen.c - Fabric edge sheen

IMPLEMENTATION GUIDELINES:
- Skin: pre-integrated SSS + separable blur
- Eyes: two-layer (cornea + iris) with refraction
- Hair: order-independent transparency or sorted
- Fabric: detail normal maps for weave

KEY PATTERNS:
```c
// Skin rendering
typedef struct skin_params {
    vec3_t scatter_color;
    float scatter_radius;
    float detail_normal_strength;
    float translucency;
    texture_handle_t diffusion_profile;
} skin_params_t;

void render_skin(mesh_t* mesh, skin_params_t* params) {
    // 1. Render to separate target with SSS mask
    render_gbuffer_skin(mesh, params);

    // 2. Separable SSS blur
    sss_blur_horizontal(skin_target, params->diffusion_profile);
    sss_blur_vertical(skin_target, params->diffusion_profile);

    // 3. Composite
    composite_skin(final_target, skin_target);
}

// Eye rendering
typedef struct eye_params {
    float iris_depth;
    float pupil_size;
    float cornea_ior;      // ~1.376
    vec3_t iris_color;
    texture_handle_t iris_texture;
} eye_params_t;

vec3_t shade_eye(vec3_t V, vec3_t N, eye_params_t* params) {
    // Refract view through cornea
    vec3_t refracted = refract(V, N, 1.0 / params->cornea_ior);

    // Intersect with iris plane
    vec3_t iris_hit = ray_plane_intersect(eye_pos, refracted, iris_plane);

    // Sample iris with parallax
    vec2_t iris_uv = calculate_iris_uv(iris_hit, params->pupil_size);
    vec3_t iris_color = sample_iris(params->iris_texture, iris_uv);

    return iris_color + specular;
}
```

FOCUS: Characters are the focus of attention. Their materials must be highest quality.
```

---

## Agent 4.5: Environment Materials

```
TASK: Implement Environment Material Rendering (Phase 4, Agent 5)

You are implementing environment material rendering for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/metal_rendering/
- src/engine/rendering/3d_rendering/wood_rendering/
- src/engine/rendering/3d_rendering/stone_rendering/
- src/engine/rendering/3d_rendering/concrete_rendering/
- src/engine/rendering/3d_rendering/glass_rendering/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. metal_anisotropy.c - Brushed metal anisotropy
2. metal_flakes.c - Car paint metallic flakes
3. wood_grain.c - Wood grain patterns
4. wood_subsurface.c - Wood SSS
5. stone_detail.c - Stone surface detail
6. stone_parallax.c - Parallax occlusion mapping
7. concrete_cracks.c - Concrete crack rendering
8. concrete_staining.c - Surface staining
9. glass_refraction.c - Glass refraction
10. glass_thickness.c - Thickness-based absorption

IMPLEMENTATION GUIDELINES:
- Metal: High metallic, low roughness, anisotropy
- Wood: Slight SSS, grain-aligned anisotropy
- Stone: Parallax for depth, detail normals
- Glass: Refraction, absorption based on thickness

KEY PATTERNS:
```c
// Brushed metal anisotropy
typedef struct brushed_metal {
    float roughness_along;      // Along brush direction
    float roughness_across;     // Perpendicular
    vec3_t brush_direction;     // Tangent space
} brushed_metal_t;

vec3_t shade_brushed_metal(vec3_t N, vec3_t V, vec3_t L, brushed_metal_t* params) {
    // Build anisotropic TBN
    vec3_t T = normalize(params->brush_direction);
    vec3_t B = cross(N, T);

    // Anisotropic GGX
    float roughness_t = params->roughness_along;
    float roughness_b = params->roughness_across;

    return anisotropic_ggx(N, V, L, T, B, roughness_t, roughness_b);
}

// Parallax occlusion mapping
vec2_t parallax_occlusion(texture_handle_t height_map, vec2_t uv, vec3_t view_ts) {
    float height_scale = 0.05;
    int num_layers = 32;

    float layer_depth = 1.0 / num_layers;
    float current_depth = 0.0;
    vec2_t delta_uv = -view_ts.xy / view_ts.z * height_scale / num_layers;

    vec2_t current_uv = uv;
    float current_height = sample_height(height_map, current_uv);

    while (current_depth < current_height) {
        current_uv += delta_uv;
        current_height = sample_height(height_map, current_uv);
        current_depth += layer_depth;
    }

    // Binary search for precision
    // ...

    return current_uv;
}

// Glass with thickness
vec3_t shade_glass(vec3_t V, vec3_t N, float thickness, vec3_t absorption) {
    vec3_t refracted = refract(V, N, 1.0 / 1.5);  // IOR 1.5
    vec3_t transmitted = exp(-absorption * thickness);
    return transmitted;
}
```

FOCUS: Environment materials define the world. Parallax and detail make them believable.
```

---

## Agent 4.6: Material Effects & Weathering

```
TASK: Implement Material Weathering & Effects (Phase 4, Agent 6)

You are implementing material weathering and procedural effects for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/age_weathering/
- src/engine/rendering/3d_rendering/rust_corrosion/
- src/engine/rendering/3d_rendering/dirt_grunge/
- src/engine/rendering/3d_rendering/wear_tear/
- src/engine/rendering/3d_rendering/wetness_system/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. weathering_system.c - Overall weathering controller
2. age_simulation.c - Simulate material aging
3. rust_growth.c - Procedural rust patterns
4. rust_color.c - Rust color variation
5. dirt_accumulation.c - Dirt in crevices
6. grunge_masks.c - Procedural grunge
7. wear_edges.c - Edge wear (curvature-based)
8. tear_damage.c - Damage patterns
9. wetness_mask.c - Dynamic wetness
10. puddle_rendering.c - Puddle reflections

IMPLEMENTATION GUIDELINES:
- Use world-space noise for consistency
- Cavity/curvature maps drive accumulation
- Rust grows from edges and scratches
- Wetness darkens albedo, increases roughness
- Blend weathering layers procedurally

KEY PATTERNS:
```c
typedef struct weathering_params {
    float age;                 // 0-1, how weathered
    float rust_amount;
    float dirt_amount;
    float wetness;
    texture_handle_t noise;
    texture_handle_t cavity;   // Baked cavity map
} weathering_params_t;

// Apply weathering to material
void apply_weathering(material_t* mat, weathering_params_t* weather, vec3_t world_pos) {
    float noise = sample_3d_noise(weather->noise, world_pos * 0.1);
    float cavity = sample_cavity(weather->cavity, mat->uv);

    // Rust in cavities and edges
    float rust_mask = saturate(weather->rust_amount * (noise + cavity * 0.5));
    mat->albedo = lerp(mat->albedo, RUST_COLOR, rust_mask);
    mat->roughness = lerp(mat->roughness, 0.8, rust_mask);
    mat->metallic = lerp(mat->metallic, 0.0, rust_mask);

    // Dirt accumulation in cavities
    float dirt_mask = saturate(weather->dirt_amount * cavity);
    mat->albedo = lerp(mat->albedo, DIRT_COLOR, dirt_mask);

    // Wetness
    if (weather->wetness > 0) {
        mat->albedo *= lerp(1.0, 0.6, weather->wetness);  // Darken
        mat->roughness *= lerp(1.0, 0.3, weather->wetness);  // Smoother
    }
}

// Edge wear using curvature
float calculate_edge_wear(texture_handle_t curvature_map, vec2_t uv, float wear_amount) {
    float curvature = sample_curvature(curvature_map, uv);
    return saturate(curvature * wear_amount);
}

// Dynamic puddles
float puddle_mask(vec3_t world_pos, float water_level, texture_handle_t height_map) {
    float ground_height = sample_height(height_map, world_pos.xz);
    return smoothstep(0.0, 0.1, water_level - ground_height);
}
```

FOCUS: Weathering adds history and life to materials. Makes environments feel lived-in.
```
