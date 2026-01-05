# Phase 6: Advanced Systems & Integration - Agent Prompts

## Agent 6.1: Landscape & Terrain

```
TASK: Implement Landscape & Terrain Rendering (Phase 6, Agent 1)

You are implementing landscape and terrain systems for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/landscape/
- src/engine/rendering/3d_rendering/landscape_grass/
- src/engine/rendering/3d_rendering/landscape_splines/
- src/engine/rendering/3d_rendering/vegetation_rendering_advanced/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. terrain_renderer.c - Terrain LOD and rendering
2. terrain_lod.c - CLOD / Geo-clipmap terrain
3. terrain_tessellation.c - GPU tessellation
4. heightmap_streaming.c - Stream heightmap tiles
5. splat_renderer.c - Multi-texture terrain splats
6. grass_renderer.c - GPU-instanced grass blades
7. grass_lod.c - Grass density by distance
8. foliage_culling.c - GPU foliage culling
9. foliage_wind.c - Wind animation
10. vegetation_interaction.c - Player foliage interaction

IMPLEMENTATION GUIDELINES:
- Terrain: clipmap or CDLOD for continuous LOD
- Heightmap: stream 16-bit heightmap tiles
- Splat: up to 16 layers with virtual texturing
- Grass: geometry shader or compute instancing
- Wind: world-space noise for coherent motion

KEY PATTERNS:
```c
typedef struct terrain_clipmap {
    texture_handle_t heightmap;
    texture_handle_t normalmap;
    texture_handle_t splatmap;
    uint32_t levels;
    float base_scale;
    vec2_t center;
} terrain_clipmap_t;

// Clipmap level rendering
void render_terrain_level(terrain_clipmap_t* terrain, int level, camera_t* camera) {
    float scale = terrain->base_scale * pow(2, level);

    // Snap to grid
    vec2_t snapped_center = snap_to_grid(camera->position.xz, scale);

    // Update ring geometry
    update_clipmap_ring(level, snapped_center);

    // Render with appropriate LOD
    bind_heightmap(terrain->heightmap, level);
    bind_splatmap(terrain->splatmap);
    set_uniform("level_scale", scale);
    draw_clipmap_ring(level);
}

// Grass rendering
typedef struct grass_instance {
    vec3_t position;
    float rotation;
    float scale;
    float wind_offset;
} grass_instance_t;

void render_grass(grass_system_t* grass, camera_t* camera) {
    // 1. GPU cull grass instances
    dispatch_grass_cull(grass->instance_buffer, grass->visible_buffer, camera);

    // 2. Draw indirect
    bind_buffer(grass->visible_buffer, 0);
    draw_indirect(grass->grass_mesh, grass->indirect_args);
}

// Wind animation (in vertex shader)
vec3_t apply_wind(vec3_t vertex, vec3_t world_pos, float time) {
    float wind_strength = sample_wind_texture(world_pos.xz + time * wind_speed);
    float height_factor = vertex.y;  // More sway at top
    vec2_t displacement = wind_direction * wind_strength * height_factor;
    return vertex + vec3(displacement.x, 0, displacement.y);
}
```

FOCUS: Terrain is the foundation of outdoor scenes. Grass density is key for believability.
```

---

## Agent 6.2: Water & Ocean

```
TASK: Implement Water & Ocean Rendering (Phase 6, Agent 2)

You are implementing water and ocean systems for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/water/
- src/engine/rendering/3d_rendering/ocean_waves/
- src/engine/rendering/3d_rendering/planar_reflections/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. ocean_renderer.c - Ocean surface rendering
2. fft_waves.c - FFT ocean wave simulation
3. gerstner_waves.c - Gerstner wave superposition
4. ocean_foam.c - Foam generation and rendering
5. ocean_lod.c - Ocean mesh LOD
6. water_reflection.c - Planar reflection capture
7. water_refraction.c - Refraction with depth
8. water_caustics.c - Underwater caustics projection
9. underwater_fog.c - Underwater rendering
10. river_rendering.c - River flow/splines

IMPLEMENTATION GUIDELINES:
- FFT: use GPU compute for wave spectrum
- Gerstner: sum 4-8 waves for simple water
- Foam: Jacobian-based folding detection
- Caustics: project caustic texture from sun
- Underwater: fog + god rays

KEY PATTERNS:
```c
// FFT Ocean
typedef struct ocean_fft {
    texture_handle_t spectrum_h0;     // Initial spectrum
    texture_handle_t spectrum_ht;     // Time-evolved spectrum
    texture_handle_t displacement;    // xyz displacement
    texture_handle_t derivatives;     // For normal calculation
    uint32_t resolution;              // e.g., 512
    float tile_size;
} ocean_fft_t;

void update_ocean_fft(ocean_fft_t* ocean, float time) {
    // 1. Update spectrum H(k,t) = H0(k) * exp(i * omega * t)
    dispatch_spectrum_update(ocean->spectrum_h0, ocean->spectrum_ht, time);

    // 2. IFFT to get displacement
    dispatch_ifft_2d(ocean->spectrum_ht, ocean->displacement);

    // 3. Calculate derivatives for normals
    dispatch_derivatives(ocean->displacement, ocean->derivatives);
}

// Foam from Jacobian
float calculate_foam(vec2_t derivatives_xx_yy, vec2_t derivatives_xy) {
    float jacobian = (1.0 + derivatives_xx_yy.x) * (1.0 + derivatives_xx_yy.y)
                   - derivatives_xy.x * derivatives_xy.y;
    return saturate(1.0 - jacobian);  // Foam where waves fold
}

// Underwater rendering
vec3_t render_underwater(vec3_t scene_color, float depth, vec3_t water_color) {
    // Exponential fog
    float fog_factor = 1.0 - exp(-depth * fog_density);
    vec3_t fogged = lerp(scene_color, water_color, fog_factor);

    // Caustics
    vec2_t caustic_uv = world_pos.xz * caustic_scale + time * caustic_speed;
    float caustic = sample_caustic_texture(caustic_uv);
    fogged += caustic * caustic_intensity;

    return fogged;
}

// Gerstner waves (simpler alternative)
vec3_t gerstner_wave(vec2_t pos, float time, wave_params_t* waves, int wave_count) {
    vec3_t displacement = vec3_zero();

    for (int i = 0; i < wave_count; i++) {
        float phase = dot(waves[i].direction, pos) * waves[i].frequency + time * waves[i].speed;
        float c = cos(phase);
        float s = sin(phase);

        displacement.x += waves[i].steepness * waves[i].amplitude * waves[i].direction.x * c;
        displacement.z += waves[i].steepness * waves[i].amplitude * waves[i].direction.y * c;
        displacement.y += waves[i].amplitude * s;
    }

    return displacement;
}
```

FOCUS: Water is visually prominent. FFT gives realistic ocean motion. Foam adds detail.
```

---

## Agent 6.3: Ray Tracing Integration

```
TASK: Implement Ray Tracing Integration (Phase 6, Agent 3)

You are implementing ray tracing systems for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/raytracing/acceleration/
- src/engine/rendering/3d_rendering/raytracing/shadows_rt/
- src/engine/rendering/3d_rendering/raytracing/reflections_rt/
- src/engine/rendering/3d_rendering/raytracing/gi_rt/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. tlas_builder.c - Build/update top-level AS
2. blas_builder.c - Build bottom-level AS per mesh
3. as_compaction.c - AS memory compaction
4. as_update.c - Dynamic object AS updates
5. rt_shadow_ray.c - Shadow ray generation/trace
6. rt_reflection_rays.c - Reflection ray generation
7. rt_glossy_reflections.c - Rough surface reflections
8. ddgi_probes.c - DDGI probe grid
9. ddgi_update.c - Update DDGI probes with RT
10. rt_ao.c - Ray-traced ambient occlusion

IMPLEMENTATION GUIDELINES:
- BLAS: one per unique mesh, reuse with transforms
- TLAS: rebuilt every frame for dynamic objects
- Shadows: 1 ray/pixel, denoise temporally
- Reflections: importance sample BRDF
- DDGI: sparse probe grid, update subset per frame

KEY PATTERNS:
```c
// Acceleration structure management
typedef struct rt_scene {
    blas_t* blas_array;
    uint32_t blas_count;
    tlas_t tlas;
    buffer_handle_t instance_buffer;
} rt_scene_t;

void build_blas(mesh_t* mesh, blas_t* out_blas) {
    vk_acceleration_structure_geometry_t geo = {
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES,
        .geometry.triangles = {
            .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
            .vertexData = mesh->vertex_buffer,
            .vertexStride = mesh->vertex_stride,
            .indexType = VK_INDEX_TYPE_UINT32,
            .indexData = mesh->index_buffer,
        }
    };

    build_acceleration_structure(AS_TYPE_BOTTOM, &geo, 1, out_blas);
}

void rebuild_tlas(rt_scene_t* scene, instance_t* instances, uint32_t count) {
    // Update instance buffer with transforms
    for (uint32_t i = 0; i < count; i++) {
        rt_instance_t rt_inst = {
            .transform = instances[i].transform,
            .instanceCustomIndex = instances[i].material_id,
            .accelerationStructureReference = scene->blas_array[instances[i].mesh_id].address,
        };
        buffer_update_region(scene->instance_buffer, i * sizeof(rt_instance_t), &rt_inst, sizeof(rt_instance_t));
    }

    build_acceleration_structure(AS_TYPE_TOP, instances, count, &scene->tlas);
}

// DDGI
typedef struct ddgi_volume {
    ivec3_t probe_counts;          // e.g., 32x8x32
    vec3_t probe_spacing;
    vec3_t origin;
    texture_handle_t irradiance;   // Octahedral encoded
    texture_handle_t depth;        // Probe visibility depth
} ddgi_volume_t;

void update_ddgi(ddgi_volume_t* ddgi, rt_scene_t* scene, int probes_per_frame) {
    // Select subset of probes to update
    int* probe_indices = select_probes_to_update(ddgi, probes_per_frame);

    for (int i = 0; i < probes_per_frame; i++) {
        vec3_t probe_pos = get_probe_position(ddgi, probe_indices[i]);

        // Trace rays in hemisphere
        for (int r = 0; r < RAYS_PER_PROBE; r++) {
            vec3_t dir = fibonacci_hemisphere(r, RAYS_PER_PROBE);

            ray_t ray = { probe_pos, dir, 0.0, MAX_DIST };
            hit_t hit;

            if (trace_ray(scene, ray, &hit)) {
                // Shade hit point
                vec3_t radiance = shade_hit(hit, scene);
                // Update probe octahedral map
                update_probe_texel(ddgi->irradiance, probe_indices[i], dir, radiance);
            }
        }
    }
}
```

FOCUS: RT provides ground truth lighting. DDGI is efficient real-time GI. Balance quality and performance.
```

---

## Agent 6.4: Nanite & GPU-Driven Rendering

```
TASK: Implement Nanite & GPU-Driven Rendering (Phase 6, Agent 4)

You are implementing Nanite-style rendering and GPU-driven pipelines for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/nanite/culling_nanite/
- src/engine/rendering/3d_rendering/nanite/rendering_nanite/
- src/engine/rendering/3d_rendering/rendering/gpu_driven/
- src/engine/rendering/3d_rendering/visibility_buffer/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. cluster_cull_gpu.c - GPU cluster culling compute
2. triangle_cull.c - Per-triangle backface/small cull
3. two_pass_cull.c - Two-phase occlusion culling
4. software_raster.c - Software rasterizer for small triangles
5. hardware_raster.c - Hardware path for large clusters
6. visibility_buffer.c - Visibility buffer rendering
7. material_classify.c - Classify pixels by material
8. gpu_scene.c - GPU scene representation
9. draw_command_gen.c - Generate draw commands on GPU
10. multi_draw_indirect.c - Multi-draw indirect dispatch

IMPLEMENTATION GUIDELINES:
- Clusters: 64-128 triangles each
- Two-phase: coarse with last frame, fine with current depth
- Software raster for < 32 pixel triangles
- Visibility buffer: store triangle ID, derivative for materials
- GPU-driven: CPU submits one draw, GPU decides what to render

KEY PATTERNS:
```c
// Nanite pipeline
typedef struct nanite_context {
    buffer_handle_t cluster_buffer;       // All clusters
    buffer_handle_t visible_clusters;     // After culling
    buffer_handle_t indirect_args;        // Draw indirect args
    texture_handle_t visibility_buffer;   // Triangle IDs
    texture_handle_t hzb;                 // Hierarchical Z
} nanite_context_t;

void render_nanite_frame(nanite_context_t* ctx, camera_t* camera) {
    // Phase 1: Cull with previous frame HZB
    dispatch_cluster_cull(ctx->cluster_buffer, ctx->hzb, camera, ctx->visible_clusters);

    // Phase 2: Rasterize visible clusters
    //   - Software rasterize small triangles
    //   - Hardware rasterize large triangles
    dispatch_software_raster(ctx->visible_clusters, ctx->visibility_buffer);

    // Phase 3: Build new HZB from result
    build_hzb(ctx->visibility_buffer, ctx->hzb);

    // Phase 4: Re-cull with new HZB (catches newly visible)
    dispatch_cluster_cull_phase2(ctx->cluster_buffer, ctx->hzb, camera, ctx->visible_clusters);

    // Phase 5: Rasterize newly visible
    dispatch_software_raster(ctx->visible_clusters, ctx->visibility_buffer);

    // Phase 6: Material resolve
    dispatch_material_resolve(ctx->visibility_buffer, gbuffer);
}

// Software rasterizer (compute shader support)
void software_rasterize_cluster(cluster_t cluster, texture_handle_t vis_buffer) {
    for (each triangle in cluster) {
        // Edge equations
        vec3_t e0 = edge_equation(v0, v1);
        vec3_t e1 = edge_equation(v1, v2);
        vec3_t e2 = edge_equation(v2, v0);

        // Bounding box
        ivec2_t bb_min = max(ivec2(0), floor(min3(v0.xy, v1.xy, v2.xy)));
        ivec2_t bb_max = min(screen_size, ceil(max3(v0.xy, v1.xy, v2.xy)));

        // Rasterize
        for (int y = bb_min.y; y < bb_max.y; y++) {
            for (int x = bb_min.x; x < bb_max.x; x++) {
                vec2_t p = vec2(x + 0.5, y + 0.5);

                float w0 = dot(e0, vec3(p, 1));
                float w1 = dot(e1, vec3(p, 1));
                float w2 = dot(e2, vec3(p, 1));

                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    float z = interpolate_depth(v0.z, v1.z, v2.z, w0, w1, w2);
                    atomic_min(depth_buffer[y * width + x], z);
                    if (passed_depth_test) {
                        vis_buffer[y * width + x] = triangle_id;
                    }
                }
            }
        }
    }
}

// Material resolve (deferred texturing)
void resolve_materials(texture_handle_t vis_buffer, gbuffer_t* gbuffer) {
    // For each pixel:
    //   - Read triangle ID from visibility buffer
    //   - Look up triangle vertices
    //   - Calculate barycentric coords from derivatives
    //   - Sample textures at interpolated UV
    //   - Write to G-buffer
}
```

FOCUS: Nanite enables unlimited geometry detail. The visibility buffer decouples geometry from shading.
```

---

## Agent 6.5: Animation & Physics Rendering

```
TASK: Implement Animation & Physics Rendering (Phase 6, Agent 5)

You are implementing animation and physics-based rendering for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/animation/
- src/engine/rendering/3d_rendering/cloth_system/
- src/engine/rendering/3d_rendering/hair_system/
- src/engine/rendering/3d_rendering/destruction/
- src/engine/rendering/3d_rendering/physics/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. skeleton_data.c - Skeleton hierarchy storage
2. animation_player.c - Animation playback/sampling
3. gpu_skinning.c - Compute shader skinning
4. blend_tree.c - Animation blend tree evaluation
5. cloth_simulation.c - Cloth physics solver
6. cloth_rendering.c - Cloth mesh rendering
7. hair_simulation.c - Hair strand simulation
8. hair_rendering_sys.c - Hair strand rendering
9. fracture_system.c - Destruction fracturing
10. debris_rendering.c - Physics debris rendering

IMPLEMENTATION GUIDELINES:
- GPU skinning: compute shader transforms vertices
- Blend trees: evaluate on CPU, upload final poses
- Cloth: position-based dynamics (PBD)
- Hair: mass-spring or PBD per strand
- Destruction: pre-fractured + runtime breaking

KEY PATTERNS:
```c
// GPU Skinning
typedef struct skinning_data {
    buffer_handle_t bone_matrices;       // mat4 per bone
    buffer_handle_t src_vertices;        // Bind pose
    buffer_handle_t dst_vertices;        // Skinned output
    buffer_handle_t bone_weights;        // 4 weights per vertex
    buffer_handle_t bone_indices;        // 4 indices per vertex
} skinning_data_t;

void dispatch_skinning(skinning_data_t* skin, uint32_t vertex_count) {
    bind_buffer(skin->bone_matrices, 0);
    bind_buffer(skin->src_vertices, 1);
    bind_buffer(skin->bone_weights, 2);
    bind_buffer(skin->bone_indices, 3);
    bind_buffer(skin->dst_vertices, 4);  // Output

    dispatch_compute(ceil(vertex_count / 64.0), 1, 1);
}

// Cloth PBD
typedef struct cloth_sim {
    vec3_t* positions;
    vec3_t* prev_positions;
    vec3_t* velocities;
    float* inv_masses;
    constraint_t* constraints;
    uint32_t particle_count;
    uint32_t constraint_count;
} cloth_sim_t;

void simulate_cloth(cloth_sim_t* cloth, float dt) {
    // 1. Apply forces (gravity, wind)
    for (int i = 0; i < cloth->particle_count; i++) {
        if (cloth->inv_masses[i] > 0) {
            cloth->velocities[i] += GRAVITY * dt;
            cloth->prev_positions[i] = cloth->positions[i];
            cloth->positions[i] += cloth->velocities[i] * dt;
        }
    }

    // 2. Solve constraints (multiple iterations)
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        for (int c = 0; c < cloth->constraint_count; c++) {
            solve_distance_constraint(&cloth->constraints[c], cloth->positions, cloth->inv_masses);
        }
    }

    // 3. Update velocities
    for (int i = 0; i < cloth->particle_count; i++) {
        cloth->velocities[i] = (cloth->positions[i] - cloth->prev_positions[i]) / dt;
    }
}

// Destruction system
typedef struct destructible {
    mesh_handle_t intact_mesh;
    mesh_handle_t* fracture_pieces;      // Pre-fractured
    uint32_t piece_count;
    float* piece_health;
    bool fractured;
} destructible_t;

void apply_damage(destructible_t* dest, vec3_t impact_point, float damage) {
    if (dest->fractured) return;

    // Find pieces near impact
    for (int i = 0; i < dest->piece_count; i++) {
        float dist = distance(get_piece_center(dest, i), impact_point);
        if (dist < damage_radius) {
            dest->piece_health[i] -= damage * (1.0 - dist / damage_radius);

            if (dest->piece_health[i] <= 0) {
                // Spawn as physics object
                spawn_debris(dest->fracture_pieces[i], impact_point, damage);
            }
        }
    }

    // Check if should switch to fractured
    if (should_fracture(dest)) {
        dest->fractured = true;
        // Hide intact, show remaining pieces
    }
}
```

FOCUS: Animation brings characters to life. Physics simulation adds believable motion.
```

---

## Agent 6.6: Editor & Tools

```
TASK: Implement Editor Rendering & Tools (Phase 6, Agent 6)

You are implementing editor rendering and tools for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/editor/
- src/engine/rendering/3d_rendering/tools/
- src/engine/rendering/3d_rendering/ui_rendering/
- src/engine/rendering/3d_rendering/asset_system/import/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. editor_camera.c - Editor camera controls (fly, orbit)
2. editor_grid.c - Infinite ground grid
3. editor_gizmos.c - Transform gizmos (translate, rotate, scale)
4. editor_selection.c - Selection outline rendering
5. editor_picking.c - Object/component picking
6. editor_icons.c - Billboard icons for lights, etc.
7. debug_lines.c - Debug line rendering
8. debug_shapes.c - Debug shape primitives
9. gltf_importer.c - glTF 2.0 import
10. fbx_importer.c - FBX import

IMPLEMENTATION GUIDELINES:
- Gizmos: raycast for picking, highlight on hover
- Selection: stencil-based outline or jump flood
- Grid: infinite grid with LOD fade
- Icons: screen-space size, depth-tested or always-on-top
- Importers: convert to engine-native format

KEY PATTERNS:
```c
// Transform gizmo
typedef struct gizmo_state {
    gizmo_mode_t mode;           // TRANSLATE, ROTATE, SCALE
    gizmo_space_t space;         // LOCAL, WORLD
    int hovered_axis;            // X, Y, Z, XY, XZ, YZ, XYZ
    int active_axis;
    vec3_t drag_start;
    mat4_t initial_transform;
} gizmo_state_t;

void render_gizmo(gizmo_state_t* gizmo, mat4_t object_transform, camera_t* camera) {
    vec3_t pos = extract_translation(object_transform);

    // Scale gizmo to constant screen size
    float dist = distance(camera->position, pos);
    float scale = dist * GIZMO_SCREEN_SIZE;

    switch (gizmo->mode) {
        case GIZMO_TRANSLATE:
            render_translate_gizmo(pos, scale, gizmo->hovered_axis);
            break;
        case GIZMO_ROTATE:
            render_rotate_gizmo(pos, scale, object_transform, gizmo->hovered_axis);
            break;
        case GIZMO_SCALE:
            render_scale_gizmo(pos, scale, object_transform, gizmo->hovered_axis);
            break;
    }
}

int gizmo_pick(gizmo_state_t* gizmo, ray_t mouse_ray, mat4_t object_transform) {
    vec3_t pos = extract_translation(object_transform);

    // Test each axis handle
    if (ray_intersect_cone(mouse_ray, x_axis_cone)) return AXIS_X;
    if (ray_intersect_cone(mouse_ray, y_axis_cone)) return AXIS_Y;
    if (ray_intersect_cone(mouse_ray, z_axis_cone)) return AXIS_Z;

    // Test planes
    if (ray_intersect_quad(mouse_ray, xy_plane)) return AXIS_XY;
    // ...

    return AXIS_NONE;
}

// Selection outline (jump flood)
void render_selection_outline(texture_handle_t selected_mask, texture_handle_t output) {
    // 1. Jump flood to create distance field
    texture_handle_t jfa = jump_flood(selected_mask);

    // 2. Render outline where distance is within threshold
    for (each pixel) {
        float dist = sample(jfa, uv);
        if (dist > 0 && dist < OUTLINE_WIDTH) {
            output = OUTLINE_COLOR;
        }
    }
}

// Infinite grid
void render_infinite_grid(camera_t* camera) {
    // Render screen-filling quad
    // In fragment shader:
    //   - Reconstruct world position from depth
    //   - Calculate grid lines based on world pos
    //   - Fade based on distance

    float grid_intensity = grid_pattern(world_pos.xz, grid_spacing);
    grid_intensity *= fade_by_distance(camera_pos, world_pos);
    grid_intensity *= fade_by_angle(camera_forward);  // Fade at grazing angles

    output = vec4(GRID_COLOR, grid_intensity);
}

// glTF import
mesh_t* import_gltf(const char* path) {
    gltf_data_t gltf = parse_gltf(path);

    mesh_t* mesh = alloc_mesh();

    // Convert vertex data
    for (each primitive) {
        extract_positions(gltf, primitive, mesh);
        extract_normals(gltf, primitive, mesh);
        extract_texcoords(gltf, primitive, mesh);
        extract_indices(gltf, primitive, mesh);
    }

    // Convert materials
    for (each material) {
        material_t* mat = convert_gltf_material(gltf, material);
        mesh->materials[material_index] = mat;
    }

    // Convert skeleton if present
    if (gltf.has_skeleton) {
        mesh->skeleton = convert_gltf_skeleton(gltf);
    }

    return mesh;
}
```

FOCUS: Editor tools enable content creation. Good gizmos and visualization are essential for productivity.
```
