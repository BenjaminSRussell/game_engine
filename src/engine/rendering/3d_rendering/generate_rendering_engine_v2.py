#!/usr/bin/env python3
"""
Rendering Engine Infrastructure Generator v2
Creates files with PROPER DESCRIPTIVE NAMES - no more processor_04.c garbage
"""

import os
from pathlib import Path
from typing import List, Dict, Tuple

BASE_PATH = Path("/Users/benjaminrussell/Desktop/Minecraft v2/src/engine/rendering/3d_rendering")

# Each subsystem has NAMED files with specific purposes
SUBSYSTEMS = {
    "core": {
        "command": [
            ("command_buffer", "Command buffer recording and management"),
            ("command_pool", "Command pool allocation and recycling"),
            ("command_queue", "Queue submission and synchronization"),
            ("command_encoder", "High-level command encoding"),
            ("command_list", "Secondary command list management"),
            ("indirect_commands", "Indirect draw/dispatch command building"),
        ],
        "device": [
            ("device_context", "GPU device context management"),
            ("device_capabilities", "Hardware capability detection"),
            ("device_memory", "Device memory allocation"),
            ("device_queue_families", "Queue family enumeration"),
            ("device_extensions", "Extension loading and validation"),
            ("physical_device", "Physical device selection"),
        ],
        "sync": [
            ("fence_pool", "Fence allocation and tracking"),
            ("semaphore_manager", "Timeline semaphore management"),
            ("barrier_batch", "Resource barrier batching"),
            ("event_system", "GPU event signaling"),
            ("frame_sync", "Frame-to-frame synchronization"),
            ("async_compute_sync", "Async compute synchronization"),
        ],
        "memory": [
            ("gpu_allocator", "GPU memory allocation strategies"),
            ("staging_buffer", "CPU-GPU transfer staging"),
            ("memory_pool", "Pooled memory allocation"),
            ("defragmenter", "Memory defragmentation"),
            ("budget_tracker", "Memory budget monitoring"),
            ("heap_manager", "Memory heap management"),
        ],
        "pipeline": [
            ("pipeline_cache", "Pipeline state object caching"),
            ("pipeline_layout", "Pipeline layout management"),
            ("shader_stages", "Shader stage configuration"),
            ("vertex_input", "Vertex input state"),
            ("rasterizer_state", "Rasterization configuration"),
            ("blend_state", "Color blending configuration"),
        ],
        "descriptor": [
            ("descriptor_set_layout", "Descriptor set layout creation"),
            ("descriptor_pool", "Descriptor pool management"),
            ("descriptor_writer", "Descriptor update batching"),
            ("bindless_manager", "Bindless resource indexing"),
            ("push_constants", "Push constant management"),
            ("dynamic_descriptors", "Dynamic uniform/storage buffers"),
        ],
    },
    "geometry": {
        "mesh": [
            ("mesh_data", "Mesh vertex/index data storage"),
            ("mesh_loader", "Mesh file format loading"),
            ("mesh_builder", "Procedural mesh construction"),
            ("mesh_optimizer", "Vertex cache optimization"),
            ("mesh_bounds", "AABB/bounding sphere calculation"),
            ("submesh_manager", "Sub-mesh and material slots"),
        ],
        "vertex": [
            ("vertex_format", "Vertex attribute layout definition"),
            ("vertex_buffer_pool", "Vertex buffer allocation"),
            ("vertex_compression", "Quantized vertex compression"),
            ("vertex_stream", "Multi-stream vertex data"),
            ("index_buffer_pool", "Index buffer allocation"),
            ("vertex_skinning_data", "Bone weights and indices"),
        ],
        "lod": [
            ("lod_generator", "Automatic LOD mesh generation"),
            ("lod_selector", "Runtime LOD selection"),
            ("lod_crossfade", "LOD transition blending"),
            ("screen_size_lod", "Screen-size based LOD"),
            ("distance_lod", "Distance-based LOD"),
            ("lod_bias", "LOD bias and forcing"),
        ],
        "streaming": [
            ("mesh_streamer", "Async mesh data streaming"),
            ("stream_priority", "Mesh streaming priority"),
            ("resident_set", "Resident mesh management"),
            ("stream_cache", "Streamed mesh caching"),
            ("prefetch_system", "Predictive mesh loading"),
            ("unload_policy", "Mesh unloading strategy"),
        ],
        "meshlets": [
            ("meshlet_builder", "Meshlet generation from mesh"),
            ("meshlet_bounds", "Per-meshlet bounding data"),
            ("meshlet_culling", "GPU meshlet visibility"),
            ("meshlet_lod", "Meshlet-level LOD"),
            ("cluster_dag", "Cluster DAG construction"),
            ("meshlet_renderer", "Mesh shader meshlet rendering"),
        ],
        "instancing": [
            ("instance_buffer", "Per-instance data buffers"),
            ("instance_culler", "Instance frustum/occlusion cull"),
            ("instance_lod", "Per-instance LOD selection"),
            ("instance_batching", "Instance draw batching"),
            ("gpu_instance_data", "GPU-side instance storage"),
            ("indirect_instancing", "Indirect draw instancing"),
        ],
        "bvh": [
            ("bvh_builder", "BVH tree construction"),
            ("bvh_traversal", "CPU BVH traversal"),
            ("bvh_update", "Dynamic BVH updates"),
            ("sah_builder", "Surface area heuristic BVH"),
            ("lbvh_builder", "Linear BVH construction"),
            ("bvh_compaction", "BVH memory optimization"),
        ],
    },
    "lighting": {
        "sources": [
            ("point_light", "Point light rendering"),
            ("spot_light", "Spotlight rendering"),
            ("directional_light", "Directional/sun light"),
            ("rect_light", "Rectangular area lights"),
            ("sphere_light", "Spherical area lights"),
            ("light_culling", "Light visibility culling"),
        ],
        "shadows": [
            ("shadow_atlas", "Shadow map atlas management"),
            ("shadow_caster", "Shadow casting setup"),
            ("pcf_filter", "PCF shadow filtering"),
            ("vsm_shadows", "Variance shadow maps"),
            ("evsm_shadows", "Exponential variance shadows"),
            ("shadow_cache", "Cached shadow maps"),
        ],
        "cascades": [
            ("cascade_splits", "Cascade split calculation"),
            ("cascade_selection", "Runtime cascade selection"),
            ("cascade_blending", "Inter-cascade blending"),
            ("cascade_stabilization", "Temporal stabilization"),
            ("cascade_culling", "Per-cascade object culling"),
            ("cascade_resolution", "Adaptive cascade resolution"),
        ],
        "global_illumination": [
            ("diffuse_gi", "Diffuse global illumination"),
            ("specular_gi", "Specular GI and reflections"),
            ("gi_probe_grid", "Irradiance probe grid"),
            ("gi_update_system", "GI update scheduling"),
            ("indirect_lighting", "Indirect light accumulation"),
            ("gi_debug_viz", "GI visualization/debugging"),
        ],
        "probes": [
            ("reflection_probe", "Reflection probe capture"),
            ("irradiance_probe", "Irradiance probe sampling"),
            ("probe_blending", "Multi-probe blending"),
            ("probe_parallax", "Parallax-corrected probes"),
            ("probe_streaming", "Probe data streaming"),
            ("probe_placement", "Automatic probe placement"),
        ],
        "volumetric": [
            ("volumetric_fog", "Volumetric fog rendering"),
            ("volumetric_lighting", "Volumetric light scattering"),
            ("froxel_grid", "Froxel volume allocation"),
            ("participating_media", "Participating media simulation"),
            ("temporal_reprojection", "Volumetric temporal AA"),
            ("volumetric_shadows", "Volumetric shadow rays"),
        ],
        "lightmaps": [
            ("lightmap_baker", "Lightmap baking system"),
            ("lightmap_uvs", "Lightmap UV generation"),
            ("lightmap_packer", "Lightmap atlas packing"),
            ("lightmap_sampling", "Lightmap texture sampling"),
            ("directional_lightmaps", "Directional lightmap data"),
            ("lightmap_streaming", "Lightmap streaming"),
        ],
    },
    "materials": {
        "shaders": [
            ("shader_compiler", "HLSL/GLSL compilation"),
            ("shader_reflection", "Shader parameter reflection"),
            ("shader_cache", "Compiled shader caching"),
            ("shader_variants", "Shader permutation system"),
            ("shader_includes", "Shader include resolution"),
            ("shader_hot_reload", "Runtime shader reloading"),
        ],
        "material_system": [
            ("material_instance", "Material instance creation"),
            ("material_parameters", "Material parameter binding"),
            ("material_overrides", "Per-object overrides"),
            ("material_sorting", "Material render ordering"),
            ("material_lod", "Material LOD/simplification"),
            ("dynamic_materials", "Runtime material creation"),
        ],
        "pbr": [
            ("pbr_parameters", "PBR material parameters"),
            ("metallic_roughness", "Metallic-roughness workflow"),
            ("specular_glossiness", "Specular-glossiness workflow"),
            ("pbr_validation", "PBR parameter validation"),
            ("pbr_presets", "Common PBR presets"),
            ("pbr_conversion", "Workflow conversion"),
        ],
        "textures": [
            ("texture_binding", "Material texture binding"),
            ("texture_defaults", "Default texture fallbacks"),
            ("texture_transform", "UV transform parameters"),
            ("detail_textures", "Detail texture tiling"),
            ("texture_blending", "Multi-texture blending"),
            ("procedural_textures", "Procedural texture generation"),
        ],
        "layering": [
            ("material_layers", "Layered material system"),
            ("layer_blending", "Layer blend modes"),
            ("layer_masks", "Layer mask textures"),
            ("layer_weights", "Layer weight computation"),
            ("height_blending", "Height-based blending"),
            ("triplanar_projection", "Triplanar texture projection"),
        ],
        "special_materials": [
            ("glass_material", "Glass/transparent materials"),
            ("water_material", "Water surface material"),
            ("foliage_material", "Foliage two-sided material"),
            ("skin_material", "Subsurface skin material"),
            ("hair_material", "Hair/fur material"),
            ("emissive_material", "Emissive/light materials"),
        ],
    },
    "animation": {
        "skeletal": [
            ("skeleton_data", "Skeleton bone hierarchy"),
            ("bone_transforms", "Bone transform computation"),
            ("animation_clip", "Animation clip data"),
            ("animation_player", "Animation playback"),
            ("animation_sampling", "Keyframe sampling"),
            ("gpu_skinning", "GPU bone matrix upload"),
        ],
        "blending": [
            ("blend_tree", "Animation blend tree"),
            ("blend_space_1d", "1D blend space"),
            ("blend_space_2d", "2D blend space"),
            ("additive_blending", "Additive animation layers"),
            ("pose_matching", "Animation pose matching"),
            ("blend_masks", "Per-bone blend masks"),
        ],
        "state_machine": [
            ("state_graph", "Animation state graph"),
            ("state_transitions", "State transition logic"),
            ("transition_conditions", "Transition parameters"),
            ("state_events", "Animation state events"),
            ("sub_state_machine", "Nested state machines"),
            ("state_debugging", "State machine debugging"),
        ],
        "ik": [
            ("fabrik_solver", "FABRIK IK solver"),
            ("ccd_solver", "CCD IK solver"),
            ("two_bone_ik", "Two-bone IK"),
            ("look_at_ik", "Look-at constraint"),
            ("foot_ik", "Foot placement IK"),
            ("hand_ik", "Hand IK for grabbing"),
        ],
        "morph_targets": [
            ("morph_data", "Morph target vertex data"),
            ("morph_weights", "Morph weight blending"),
            ("facial_morphs", "Facial expression morphs"),
            ("gpu_morphing", "GPU morph target blending"),
            ("morph_compression", "Morph data compression"),
            ("morph_streaming", "Morph target streaming"),
        ],
        "physics_animation": [
            ("ragdoll_setup", "Ragdoll skeleton setup"),
            ("ragdoll_blend", "Ragdoll-animation blending"),
            ("cloth_bones", "Cloth bone simulation"),
            ("jiggle_bones", "Secondary motion bones"),
            ("spring_bones", "Spring constraint bones"),
            ("physics_constraints", "Animation physics constraints"),
        ],
    },
    "effects": {
        "particles": [
            ("particle_emitter", "Particle emitter configuration"),
            ("particle_simulation", "CPU particle simulation"),
            ("gpu_particles", "GPU particle compute"),
            ("particle_rendering", "Particle billboard rendering"),
            ("particle_collision", "Particle-scene collision"),
            ("particle_sorting", "Depth-sorted particles"),
        ],
        "gpu_particles": [
            ("particle_buffer", "GPU particle buffer"),
            ("spawn_compute", "Particle spawn compute"),
            ("update_compute", "Particle update compute"),
            ("dead_list", "Dead particle recycling"),
            ("indirect_args", "Indirect draw arguments"),
            ("particle_forces", "Force field application"),
        ],
        "vfx_graph": [
            ("vfx_context", "VFX graph context"),
            ("vfx_nodes", "VFX graph node types"),
            ("vfx_compiler", "VFX graph compilation"),
            ("vfx_execution", "VFX graph runtime"),
            ("vfx_events", "VFX event triggers"),
            ("vfx_properties", "Exposed VFX properties"),
        ],
        "trails": [
            ("trail_renderer", "Trail/ribbon rendering"),
            ("trail_points", "Trail point generation"),
            ("trail_tessellation", "Trail mesh generation"),
            ("trail_uvs", "Trail UV coordinates"),
            ("trail_width", "Trail width curves"),
            ("trail_fading", "Trail fade-out"),
        ],
        "decals": [
            ("decal_projector", "Decal projection volumes"),
            ("decal_rendering", "Deferred decal rendering"),
            ("decal_sorting", "Decal render order"),
            ("decal_blending", "Decal blend modes"),
            ("decal_clustering", "Decal clustering"),
            ("decal_streaming", "Decal texture streaming"),
        ],
        "weather": [
            ("rain_system", "Rain particle system"),
            ("snow_system", "Snow particle system"),
            ("weather_volumes", "Weather zone volumes"),
            ("precipitation_occlusion", "Indoor/outdoor detection"),
            ("puddles", "Dynamic puddle rendering"),
            ("wetness_mask", "Surface wetness mask"),
        ],
    },
    "postprocessing": {
        "tonemapping": [
            ("aces_tonemapper", "ACES filmic tonemapping"),
            ("reinhard_tonemapper", "Reinhard tonemapping"),
            ("exposure_adaptation", "Auto exposure"),
            ("histogram_compute", "Luminance histogram"),
            ("hdr_encoding", "HDR output encoding"),
            ("color_transform", "Color space transforms"),
        ],
        "bloom": [
            ("bloom_downsample", "Bloom downsample chain"),
            ("bloom_upsample", "Bloom upsample blend"),
            ("bloom_threshold", "Bloom threshold filter"),
            ("bloom_lens_dirt", "Lens dirt overlay"),
            ("bloom_flares", "Lens flare generation"),
            ("bloom_anamorphic", "Anamorphic bloom"),
        ],
        "anti_aliasing": [
            ("taa_resolve", "TAA temporal resolve"),
            ("taa_velocity", "TAA velocity buffer"),
            ("taa_history", "TAA history management"),
            ("taa_sharpening", "TAA sharpen pass"),
            ("fxaa_pass", "FXAA post-process"),
            ("smaa_pass", "SMAA anti-aliasing"),
        ],
        "depth_of_field": [
            ("dof_bokeh", "Bokeh shape sampling"),
            ("dof_coc", "Circle of confusion"),
            ("dof_gather", "DOF gather pass"),
            ("dof_combine", "DOF combine pass"),
            ("dof_parameters", "DOF camera settings"),
            ("tiled_dof", "Tiled DOF optimization"),
        ],
        "motion_blur": [
            ("velocity_buffer", "Per-pixel velocity buffer"),
            ("motion_blur_tile", "Motion blur tile max"),
            ("motion_blur_gather", "Motion blur gather"),
            ("object_motion_blur", "Per-object motion blur"),
            ("camera_motion_blur", "Camera motion blur"),
            ("motion_blur_quality", "Motion blur quality levels"),
        ],
        "ambient_occlusion": [
            ("ssao_pass", "Screen-space AO"),
            ("gtao_pass", "Ground truth AO"),
            ("hbao_pass", "Horizon-based AO"),
            ("ao_blur", "AO bilateral blur"),
            ("ao_temporal", "AO temporal filter"),
            ("bent_normals", "Bent normal generation"),
        ],
        "screen_space": [
            ("ssr_trace", "Screen-space reflection trace"),
            ("ssr_resolve", "SSR resolve/filter"),
            ("ssr_fallback", "SSR probe fallback"),
            ("ss_gi", "Screen-space GI"),
            ("ss_shadows", "Screen-space shadows"),
            ("ss_refraction", "Screen-space refraction"),
        ],
    },
    "raytracing": {
        "acceleration": [
            ("tlas_builder", "Top-level AS construction"),
            ("blas_builder", "Bottom-level AS construction"),
            ("as_compaction", "AS memory compaction"),
            ("as_update", "Dynamic AS updates"),
            ("as_manager", "AS lifetime management"),
            ("instance_buffer_rt", "RT instance data"),
        ],
        "shadows_rt": [
            ("rt_shadow_rays", "Ray-traced shadow rays"),
            ("rt_soft_shadows", "RT soft shadow penumbra"),
            ("rt_shadow_denoise", "Shadow denoising"),
            ("rt_contact_shadows", "RT contact shadows"),
            ("rt_shadow_cache", "Shadow ray caching"),
            ("rt_shadow_lod", "Shadow ray LOD"),
        ],
        "reflections_rt": [
            ("rt_reflection_rays", "RT reflection rays"),
            ("rt_reflection_denoise", "Reflection denoising"),
            ("rt_glossy_reflections", "Glossy RT reflections"),
            ("rt_reflection_lod", "Reflection ray LOD"),
            ("rt_reflection_fallback", "RT-SSR hybrid"),
            ("rt_multi_bounce", "Multi-bounce reflections"),
        ],
        "gi_rt": [
            ("ddgi_probes", "DDGI probe grid"),
            ("ddgi_update", "DDGI probe update"),
            ("ddgi_sampling", "DDGI irradiance sampling"),
            ("rt_diffuse_gi", "RT diffuse GI rays"),
            ("gi_denoiser", "GI temporal denoiser"),
            ("restir_gi", "ReSTIR GI"),
        ],
        "path_tracing": [
            ("path_integrator", "Path tracing integrator"),
            ("path_sampler", "Path sample generation"),
            ("mis_weights", "Multiple importance sampling"),
            ("russian_roulette", "Russian roulette termination"),
            ("progressive_render", "Progressive refinement"),
            ("reference_renderer", "Reference path tracer"),
        ],
        "denoising": [
            ("svgf_denoiser", "SVGF denoiser"),
            ("relax_denoiser", "ReLAX denoiser"),
            ("nrd_integration", "NRD library integration"),
            ("temporal_accumulation", "Temporal sample accumulation"),
            ("variance_estimation", "Noise variance estimation"),
            ("edge_stopping", "Edge-aware filtering"),
        ],
    },
    "culling": {
        "frustum": [
            ("frustum_planes", "Frustum plane extraction"),
            ("frustum_aabb_test", "AABB frustum test"),
            ("frustum_sphere_test", "Sphere frustum test"),
            ("frustum_obb_test", "OBB frustum test"),
            ("simd_frustum_cull", "SIMD batch frustum cull"),
            ("frustum_debug", "Frustum visualization"),
        ],
        "occlusion": [
            ("hzb_builder", "Hierarchical Z-buffer build"),
            ("hzb_test", "HZB occlusion test"),
            ("software_rasterizer", "Software depth rasterizer"),
            ("occlusion_queries", "GPU occlusion queries"),
            ("temporal_occlusion", "Temporal reprojection cull"),
            ("occlusion_feedback", "GPU cull feedback"),
        ],
        "gpu_culling": [
            ("gpu_cull_compute", "GPU culling compute shader"),
            ("draw_indirect_gen", "Indirect draw generation"),
            ("meshlet_cull_gpu", "GPU meshlet culling"),
            ("two_phase_occlusion", "Two-phase occlusion"),
            ("cull_output_buffer", "Cull result buffers"),
            ("cull_statistics", "Culling statistics"),
        ],
        "spatial": [
            ("octree_culling", "Octree spatial culling"),
            ("bvh_scene_cull", "BVH scene traversal"),
            ("grid_culling", "Uniform grid culling"),
            ("portal_culling", "Portal visibility"),
            ("sector_culling", "Sector-based visibility"),
            ("pvs_lookup", "Precomputed visibility sets"),
        ],
        "lod_selection": [
            ("lod_screen_size", "Screen-size LOD selection"),
            ("lod_distance", "Distance-based LOD"),
            ("lod_error_metric", "Geometric error LOD"),
            ("lod_hysteresis", "LOD switch hysteresis"),
            ("lod_forcing", "LOD forcing/bias"),
            ("lod_streaming_priority", "LOD streaming priority"),
        ],
    },
    "rendering": {
        "forward": [
            ("forward_pass", "Forward rendering pass"),
            ("forward_plus", "Forward+ light culling"),
            ("forward_transparency", "Forward transparent pass"),
            ("forward_lighting", "Forward lighting loop"),
            ("forward_shadows", "Forward shadow sampling"),
            ("forward_clustering", "Forward light clusters"),
        ],
        "deferred": [
            ("gbuffer_pass", "G-buffer geometry pass"),
            ("gbuffer_layout", "G-buffer format layout"),
            ("deferred_lighting", "Deferred lighting pass"),
            ("deferred_decals", "Deferred decal pass"),
            ("gbuffer_resolve", "G-buffer MSAA resolve"),
            ("thin_gbuffer", "Thin G-buffer mode"),
        ],
        "visibility": [
            ("visibility_buffer", "Visibility buffer pass"),
            ("material_classify", "Material classification"),
            ("visibility_resolve", "Visibility buffer resolve"),
            ("deferred_texturing", "Deferred texture sampling"),
            ("visibility_debug", "Visibility visualization"),
            ("primitive_id_buffer", "Primitive ID storage"),
        ],
        "gpu_driven": [
            ("gpu_scene", "GPU scene representation"),
            ("instance_data_gpu", "GPU instance buffer"),
            ("material_data_gpu", "GPU material buffer"),
            ("draw_command_gen", "Draw command generation"),
            ("multi_draw_indirect", "Multi-draw indirect"),
            ("persistent_mapping", "Persistent buffer mapping"),
        ],
        "render_graph": [
            ("render_pass_node", "Render graph pass nodes"),
            ("resource_node", "Render graph resources"),
            ("graph_compiler", "Render graph compilation"),
            ("graph_executor", "Render graph execution"),
            ("barrier_insertion", "Automatic barriers"),
            ("resource_aliasing", "Transient resource aliasing"),
        ],
        "output": [
            ("swapchain", "Swapchain management"),
            ("present", "Frame presentation"),
            ("hdr_output", "HDR display output"),
            ("frame_pacing", "Frame timing/pacing"),
            ("vsync_modes", "VSync configuration"),
            ("resolution_scaling", "Dynamic resolution"),
        ],
    },
    "shading": {
        "brdf": [
            ("ggx_brdf", "GGX microfacet BRDF"),
            ("diffuse_brdf", "Diffuse BRDF models"),
            ("fresnel", "Fresnel equations"),
            ("visibility_term", "Visibility/geometry term"),
            ("energy_conservation", "Energy conservation"),
            ("multiscatter_ggx", "Multi-scatter GGX"),
        ],
        "subsurface": [
            ("sss_profile", "Subsurface scatter profile"),
            ("sss_blur", "Separable SSS blur"),
            ("sss_transmission", "SSS transmission"),
            ("skin_shading", "Skin shading model"),
            ("pre_integrated_sss", "Pre-integrated skin"),
            ("sss_shadow_maps", "SSS shadow maps"),
        ],
        "cloth": [
            ("cloth_brdf", "Cloth shading BRDF"),
            ("sheen_layer", "Sheen layer"),
            ("fuzz_lighting", "Fuzz lighting"),
            ("anisotropic_cloth", "Anisotropic cloth"),
            ("fabric_patterns", "Fabric pattern detail"),
            ("cloth_ao", "Cloth ambient occlusion"),
        ],
        "hair": [
            ("hair_strand_shading", "Hair strand shading"),
            ("marschner_model", "Marschner hair model"),
            ("hair_scattering", "Hair multi-scatter"),
            ("hair_shadows", "Deep opacity maps"),
            ("hair_ao", "Hair ambient occlusion"),
            ("hair_antialiasing", "Hair edge AA"),
        ],
        "clearcoat": [
            ("clearcoat_layer", "Clearcoat layer"),
            ("clearcoat_normal", "Clearcoat normal map"),
            ("car_paint", "Car paint model"),
            ("multi_layer_material", "Multi-layer materials"),
            ("coat_thickness", "Coat thickness variation"),
            ("coat_fresnel", "Coat Fresnel"),
        ],
        "special": [
            ("iridescence", "Thin-film iridescence"),
            ("anisotropy", "Anisotropic highlights"),
            ("transmission", "Transmission/refraction"),
            ("absorption", "Volume absorption"),
            ("eye_shading", "Eye shading model"),
            ("gemstone", "Gemstone material"),
        ],
    },
    "texture": {
        "streaming": [
            ("texture_streamer", "Texture streaming system"),
            ("stream_priority", "Streaming priority"),
            ("mip_bias", "Streaming mip bias"),
            ("resident_mips", "Resident mip tracking"),
            ("streaming_budget", "Texture memory budget"),
            ("prefetch_hints", "Texture prefetch hints"),
        ],
        "virtual_texturing": [
            ("virtual_texture", "Virtual texture system"),
            ("page_table", "Virtual texture page table"),
            ("page_cache", "Page cache management"),
            ("feedback_buffer", "VT feedback buffer"),
            ("feedback_analysis", "Feedback analysis"),
            ("page_provider", "Page data provider"),
        ],
        "compression": [
            ("bc_compression", "BC format compression"),
            ("astc_compression", "ASTC compression"),
            ("basis_transcoder", "Basis Universal transcoding"),
            ("texture_encoder", "Runtime texture encoding"),
            ("compression_quality", "Compression quality settings"),
            ("format_conversion", "Format conversion"),
        ],
        "sampling": [
            ("sampler_cache", "Sampler state caching"),
            ("anisotropic_filter", "Anisotropic filtering"),
            ("mipmap_generation", "Mipmap generation compute"),
            ("texture_arrays", "Texture array management"),
            ("cubemap_filtering", "Cubemap filtering"),
            ("texture_lod", "Texture LOD calculation"),
        ],
    },
    "landscape": {
        "terrain": [
            ("terrain_renderer", "Terrain rendering system"),
            ("terrain_lod", "Terrain LOD/CLOD"),
            ("terrain_tessellation", "GPU tessellation"),
            ("terrain_culling", "Terrain chunk culling"),
            ("terrain_normals", "Terrain normal maps"),
            ("terrain_holes", "Terrain hole support"),
        ],
        "heightmaps": [
            ("heightmap_loader", "Heightmap loading"),
            ("heightmap_streaming", "Heightmap streaming"),
            ("heightmap_compression", "Heightmap compression"),
            ("virtual_heightmap", "Virtual heightmap"),
            ("heightmap_blending", "Multi-heightmap blend"),
            ("heightmap_modification", "Runtime height modification"),
        ],
        "splatmaps": [
            ("splat_renderer", "Terrain splat rendering"),
            ("splat_blending", "Splat layer blending"),
            ("splat_streaming", "Splat map streaming"),
            ("virtual_splatmap", "Virtual splat maps"),
            ("splat_painting", "Runtime splat painting"),
            ("splat_materials", "Per-layer materials"),
        ],
        "vegetation": [
            ("foliage_system", "Foliage instance system"),
            ("grass_renderer", "Grass blade rendering"),
            ("tree_renderer", "Tree impostor/billboard"),
            ("foliage_culling", "Foliage GPU culling"),
            ("foliage_wind", "Wind animation"),
            ("foliage_interaction", "Player interaction"),
        ],
        "procedural": [
            ("terrain_generation", "Procedural terrain"),
            ("erosion_simulation", "Erosion simulation"),
            ("biome_generation", "Biome generation"),
            ("vegetation_placement", "Procedural vegetation"),
            ("road_generation", "Procedural roads"),
            ("river_generation", "Procedural rivers"),
        ],
    },
    "water": {
        "ocean": [
            ("ocean_renderer", "Ocean surface rendering"),
            ("fft_waves", "FFT wave simulation"),
            ("gerstner_waves", "Gerstner wave superposition"),
            ("ocean_lod", "Ocean mesh LOD"),
            ("ocean_foam", "Ocean foam generation"),
            ("ocean_displacement", "Vertex displacement"),
        ],
        "simulation": [
            ("wave_simulation", "Wave physics simulation"),
            ("wave_spectrum", "Wave spectrum generation"),
            ("wave_propagation", "Wave propagation"),
            ("buoyancy", "Object buoyancy"),
            ("wake_simulation", "Boat wake simulation"),
            ("splash_effects", "Splash particle effects"),
        ],
        "rendering_water": [
            ("water_reflection", "Planar reflection"),
            ("water_refraction", "Water refraction"),
            ("water_caustics", "Underwater caustics"),
            ("underwater_fog", "Underwater fog/scattering"),
            ("water_depth", "Water depth rendering"),
            ("water_normals", "Water normal maps"),
        ],
        "rivers": [
            ("river_spline", "River spline system"),
            ("river_flow", "River flow simulation"),
            ("river_rendering", "River surface rendering"),
            ("waterfall", "Waterfall effects"),
            ("river_foam", "River foam/rapids"),
            ("river_interaction", "River interaction"),
        ],
    },
    "atmosphere": {
        "sky": [
            ("sky_renderer", "Sky dome rendering"),
            ("atmospheric_scattering", "Atmospheric scattering"),
            ("rayleigh_mie", "Rayleigh/Mie scattering"),
            ("sky_lut", "Sky lookup tables"),
            ("aerial_perspective", "Aerial perspective"),
            ("sun_disk", "Sun disk rendering"),
        ],
        "clouds": [
            ("volumetric_clouds", "Volumetric cloud rendering"),
            ("cloud_noise", "Cloud noise generation"),
            ("cloud_lighting", "Cloud lighting/shadows"),
            ("cloud_density", "Cloud density sampling"),
            ("cloud_lod", "Cloud detail LOD"),
            ("cloud_animation", "Cloud movement/animation"),
        ],
        "time_of_day": [
            ("sun_moon_position", "Sun/moon positioning"),
            ("lighting_gradient", "Time-of-day lighting"),
            ("color_grading_tod", "TOD color grading"),
            ("star_field", "Night sky stars"),
            ("moon_phases", "Moon phase rendering"),
            ("twilight", "Dawn/dusk transitions"),
        ],
    },
    "ui_rendering": {
        "canvas": [
            ("ui_canvas", "UI canvas system"),
            ("ui_batch", "UI draw batching"),
            ("ui_atlas", "UI texture atlas"),
            ("ui_font", "Font rendering"),
            ("ui_sdf_text", "SDF text rendering"),
            ("ui_masking", "UI stencil masking"),
        ],
        "widgets": [
            ("ui_rect", "Rectangle rendering"),
            ("ui_image", "Image rendering"),
            ("ui_ninepatch", "9-patch sprite"),
            ("ui_rounded_rect", "Rounded rectangles"),
            ("ui_gradients", "Gradient fills"),
            ("ui_effects", "UI visual effects"),
        ],
    },
    "editor": {
        "viewport": [
            ("editor_camera", "Editor camera control"),
            ("editor_grid", "Infinite grid"),
            ("editor_gizmos", "Transform gizmos"),
            ("editor_selection", "Selection outline"),
            ("editor_picking", "Object picking"),
            ("editor_icons", "Billboard icons"),
        ],
        "debug_viz": [
            ("debug_lines", "Debug line rendering"),
            ("debug_shapes", "Debug shape primitives"),
            ("debug_text", "Debug text overlay"),
            ("debug_bounds", "Bounds visualization"),
            ("debug_skeleton", "Skeleton visualization"),
            ("debug_physics", "Physics debug viz"),
        ],
        "tools": [
            ("brush_preview", "Terrain brush preview"),
            ("spline_editor", "Spline visualization"),
            ("volume_editor", "Volume editing"),
            ("placement_preview", "Object placement preview"),
            ("measurement_tool", "Distance measurement"),
            ("camera_path", "Camera path visualization"),
        ],
    },
    "profiling": {
        "gpu": [
            ("gpu_timer", "GPU timestamp queries"),
            ("gpu_stats", "GPU statistics"),
            ("pipeline_stats", "Pipeline statistics"),
            ("bandwidth_counter", "Bandwidth measurement"),
            ("occupancy", "Shader occupancy"),
            ("gpu_markers", "GPU debug markers"),
        ],
        "cpu": [
            ("frame_timer", "Frame time measurement"),
            ("task_profiler", "Task/job profiling"),
            ("memory_profiler", "Memory profiling"),
            ("cache_profiler", "Cache hit profiling"),
            ("thread_profiler", "Thread timeline"),
            ("allocation_tracker", "Allocation tracking"),
        ],
        "visualization": [
            ("perf_overlay", "Performance overlay"),
            ("frame_graph_viz", "Frame graph visualization"),
            ("heat_map", "Performance heat map"),
            ("histogram_viz", "Histogram display"),
            ("timeline_view", "Timeline visualization"),
            ("resource_viewer", "Resource viewer"),
        ],
    },
}

# TODO templates per category
TODO_TEMPLATES = {
    "core": [
        "TODO: Implement Vulkan backend",
        "TODO: Implement Metal backend",
        "TODO: Implement D3D12 backend",
        "TODO: Add thread-safe access patterns",
        "TODO: Implement proper error handling with error codes",
        "TODO: Add memory tracking and leak detection",
        "TODO: Implement hot-reload support",
        "TODO: Add validation layer integration",
        "TODO: Implement resource state tracking",
        "TODO: Add GPU debugging markers",
    ],
    "geometry": [
        "TODO: Implement mesh optimization (vertex cache)",
        "TODO: Add meshlet generation for mesh shaders",
        "TODO: Implement progressive mesh streaming",
        "TODO: Add mesh simplification (QEM)",
        "TODO: Implement vertex compression",
        "TODO: Add LOD generation",
        "TODO: Implement BVH construction",
        "TODO: Add instanced rendering support",
        "TODO: Implement GPU-driven culling",
        "TODO: Add mesh bounds computation",
    ],
    "lighting": [
        "TODO: Implement clustered light culling",
        "TODO: Add ray-traced shadows",
        "TODO: Implement cascaded shadow maps",
        "TODO: Add area light support",
        "TODO: Implement global illumination",
        "TODO: Add volumetric lighting",
        "TODO: Implement light probes",
        "TODO: Add IES profile support",
        "TODO: Implement lightmap baking",
        "TODO: Add real-time GI",
    ],
    "materials": [
        "TODO: Implement PBR material model",
        "TODO: Add material instancing",
        "TODO: Implement shader permutation system",
        "TODO: Add material hot-reload",
        "TODO: Implement texture binding",
        "TODO: Add material LOD",
        "TODO: Implement layered materials",
        "TODO: Add procedural materials",
        "TODO: Implement material graph compilation",
        "TODO: Add material parameter animation",
    ],
    "animation": [
        "TODO: Implement skeletal animation",
        "TODO: Add animation blending",
        "TODO: Implement IK solvers",
        "TODO: Add morph target support",
        "TODO: Implement GPU skinning",
        "TODO: Add animation compression",
        "TODO: Implement state machine",
        "TODO: Add procedural animation",
        "TODO: Implement ragdoll physics",
        "TODO: Add animation retargeting",
    ],
    "effects": [
        "TODO: Implement GPU particle system",
        "TODO: Add particle collision",
        "TODO: Implement ribbon/trail rendering",
        "TODO: Add VFX graph system",
        "TODO: Implement decal rendering",
        "TODO: Add weather effects",
        "TODO: Implement particle sorting",
        "TODO: Add particle LOD",
        "TODO: Implement force fields",
        "TODO: Add particle events",
    ],
    "postprocessing": [
        "TODO: Implement ACES tonemapping",
        "TODO: Add physically-based bloom",
        "TODO: Implement TAA",
        "TODO: Add depth of field",
        "TODO: Implement motion blur",
        "TODO: Add GTAO",
        "TODO: Implement SSR",
        "TODO: Add color grading",
        "TODO: Implement lens effects",
        "TODO: Add film grain",
    ],
    "raytracing": [
        "TODO: Implement BVH construction",
        "TODO: Add TLAS/BLAS management",
        "TODO: Implement ray-traced shadows",
        "TODO: Add ray-traced reflections",
        "TODO: Implement DDGI",
        "TODO: Add denoising (SVGF/ReLAX)",
        "TODO: Implement path tracing",
        "TODO: Add hybrid rendering",
        "TODO: Implement ReSTIR",
        "TODO: Add ray-traced AO",
    ],
    "culling": [
        "TODO: Implement frustum culling (SIMD)",
        "TODO: Add HZB occlusion culling",
        "TODO: Implement GPU culling",
        "TODO: Add temporal reprojection culling",
        "TODO: Implement meshlet culling",
        "TODO: Add two-phase occlusion",
        "TODO: Implement software rasterizer",
        "TODO: Add portal culling",
        "TODO: Implement LOD selection",
        "TODO: Add streaming priority",
    ],
    "rendering": [
        "TODO: Implement forward+ rendering",
        "TODO: Add deferred rendering",
        "TODO: Implement visibility buffer",
        "TODO: Add GPU-driven pipeline",
        "TODO: Implement render graph",
        "TODO: Add multi-draw indirect",
        "TODO: Implement mesh shaders",
        "TODO: Add variable rate shading",
        "TODO: Implement async compute",
        "TODO: Add dynamic resolution",
    ],
    "shading": [
        "TODO: Implement GGX BRDF",
        "TODO: Add multi-scatter GGX",
        "TODO: Implement subsurface scattering",
        "TODO: Add cloth shading",
        "TODO: Implement hair shading",
        "TODO: Add clearcoat layer",
        "TODO: Implement anisotropy",
        "TODO: Add transmission",
        "TODO: Implement iridescence",
        "TODO: Add eye shading",
    ],
    "texture": [
        "TODO: Implement texture streaming",
        "TODO: Add virtual texturing",
        "TODO: Implement BC/ASTC compression",
        "TODO: Add mipmap generation",
        "TODO: Implement bindless textures",
        "TODO: Add texture arrays",
        "TODO: Implement feedback analysis",
        "TODO: Add residency management",
        "TODO: Implement format conversion",
        "TODO: Add anisotropic filtering",
    ],
    "landscape": [
        "TODO: Implement terrain LOD",
        "TODO: Add terrain tessellation",
        "TODO: Implement heightmap streaming",
        "TODO: Add splat map rendering",
        "TODO: Implement vegetation instancing",
        "TODO: Add grass rendering",
        "TODO: Implement procedural terrain",
        "TODO: Add erosion simulation",
        "TODO: Implement virtual heightmaps",
        "TODO: Add terrain holes",
    ],
    "water": [
        "TODO: Implement FFT ocean simulation",
        "TODO: Add Gerstner waves",
        "TODO: Implement foam rendering",
        "TODO: Add caustics",
        "TODO: Implement underwater rendering",
        "TODO: Add planar reflections",
        "TODO: Implement river rendering",
        "TODO: Add buoyancy physics",
        "TODO: Implement wake simulation",
        "TODO: Add shore waves",
    ],
    "atmosphere": [
        "TODO: Implement atmospheric scattering",
        "TODO: Add volumetric clouds",
        "TODO: Implement sky LUT",
        "TODO: Add aerial perspective",
        "TODO: Implement sun/moon rendering",
        "TODO: Add star field",
        "TODO: Implement time-of-day",
        "TODO: Add weather transitions",
        "TODO: Implement cloud shadows",
        "TODO: Add god rays",
    ],
    "ui_rendering": [
        "TODO: Implement UI batching",
        "TODO: Add SDF text rendering",
        "TODO: Implement UI atlas",
        "TODO: Add UI masking",
        "TODO: Implement UI effects",
        "TODO: Add 9-patch sprites",
        "TODO: Implement UI gradients",
        "TODO: Add UI animations",
        "TODO: Implement UI clipping",
        "TODO: Add UI render targets",
    ],
    "editor": [
        "TODO: Implement transform gizmos",
        "TODO: Add object picking",
        "TODO: Implement selection outline",
        "TODO: Add debug visualization",
        "TODO: Implement grid rendering",
        "TODO: Add camera controls",
        "TODO: Implement brush preview",
        "TODO: Add measurement tools",
        "TODO: Implement wireframe mode",
        "TODO: Add debug overlays",
    ],
    "profiling": [
        "TODO: Implement GPU timestamps",
        "TODO: Add frame profiler",
        "TODO: Implement memory tracking",
        "TODO: Add bandwidth measurement",
        "TODO: Implement performance overlay",
        "TODO: Add timeline visualization",
        "TODO: Implement heat maps",
        "TODO: Add regression detection",
        "TODO: Implement capture/export",
        "TODO: Add remote profiling",
    ],
}


def get_todos(subsystem: str, file_name: str, count: int = 30) -> List[str]:
    """Get TODO comments for a file."""
    base_todos = TODO_TEMPLATES.get(subsystem, TODO_TEMPLATES["core"])

    # Add file-specific TODOs
    file_specific = [
        f"TODO: Implement {file_name.replace('_', ' ')} initialization",
        f"TODO: Add {file_name.replace('_', ' ')} cleanup/shutdown",
        f"TODO: Implement {file_name.replace('_', ' ')} validation",
        f"TODO: Add {file_name.replace('_', ' ')} error handling",
        f"TODO: Implement {file_name.replace('_', ' ')} serialization",
        f"TODO: Add {file_name.replace('_', ' ')} debug output",
        f"TODO: Implement {file_name.replace('_', ' ')} unit tests",
        f"TODO: Add {file_name.replace('_', ' ')} performance counters",
        f"TODO: Implement {file_name.replace('_', ' ')} hot-reload",
        f"TODO: Add {file_name.replace('_', ' ')} thread safety",
        f"TODO: Implement {file_name.replace('_', ' ')} memory pooling",
        f"TODO: Add {file_name.replace('_', ' ')} caching layer",
        f"TODO: Implement {file_name.replace('_', ' ')} async operations",
        f"TODO: Add {file_name.replace('_', ' ')} GPU integration",
        f"TODO: Implement {file_name.replace('_', ' ')} SIMD optimization",
        f"TODO: Add {file_name.replace('_', ' ')} batch processing",
        f"TODO: Implement {file_name.replace('_', ' ')} streaming support",
        f"TODO: Add {file_name.replace('_', ' ')} LOD support",
        f"TODO: Implement {file_name.replace('_', ' ')} culling integration",
        f"TODO: Add {file_name.replace('_', ' ')} render graph node",
    ]

    all_todos = base_todos + file_specific
    return all_todos[:count]


def generate_c_file(subsystem: str, subdir: str, file_name: str, description: str) -> Tuple[str, int]:
    """Generate a .c file with proper naming and content."""
    todos = get_todos(subsystem, file_name, 40)
    struct_name = f"{subsystem}_{file_name}"

    todo_block = "\n".join(f" * {todo}" for todo in todos)

    content = f'''/*
 * {file_name}.c
 * {description}
 *
 * Part of the {subsystem.replace("_", " ").title()} subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
{todo_block}
 */

#include "{file_name}.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define {struct_name.upper()}_MAX_COUNT 4096
#define {struct_name.upper()}_DEFAULT_CAPACITY 256
#define {struct_name.upper()}_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {struct_name}_internal {{
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
}} {struct_name}_internal_t;

typedef struct {struct_name}_context {{
    {struct_name}_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
}} {struct_name}_context_t;

static {struct_name}_context_t g_{file_name}_ctx = {{0}};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool {struct_name}_validate(const {struct_name}_internal_t* item) {{
    // {todos[0]}
    // {todos[1]}
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}}

static void {struct_name}_cleanup_internal({struct_name}_internal_t* item) {{
    // {todos[2]}
    // {todos[3]}
    if (!item) return;
    if (item->data) {{
        free(item->data);
        item->data = NULL;
    }}
    item->initialized = false;
}}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int {struct_name}_init(void) {{
    // {todos[4]}
    // {todos[5]}
    // {todos[6]}
    // {todos[7]}

    if (g_{file_name}_ctx.initialized) {{
        return 0; // Already initialized
    }}

    g_{file_name}_ctx.capacity = {struct_name.upper()}_DEFAULT_CAPACITY;
    g_{file_name}_ctx.items = calloc(g_{file_name}_ctx.capacity, sizeof({struct_name}_internal_t));
    if (!g_{file_name}_ctx.items) {{
        return -1;
    }}

    g_{file_name}_ctx.count = 0;
    g_{file_name}_ctx.initialized = true;

    return 0;
}}

void {struct_name}_shutdown(void) {{
    // {todos[8]}
    // {todos[9]}
    // {todos[10]}
    // {todos[11]}

    if (!g_{file_name}_ctx.initialized) {{
        return;
    }}

    for (uint32_t i = 0; i < g_{file_name}_ctx.count; i++) {{
        {struct_name}_cleanup_internal(&g_{file_name}_ctx.items[i]);
    }}

    free(g_{file_name}_ctx.items);
    g_{file_name}_ctx.items = NULL;
    g_{file_name}_ctx.count = 0;
    g_{file_name}_ctx.capacity = 0;
    g_{file_name}_ctx.initialized = false;
}}

int {struct_name}_create({struct_name}_handle_t* out_handle, const {struct_name}_desc_t* desc) {{
    // {todos[12]}
    // {todos[13]}
    // {todos[14]}
    // {todos[15]}

    if (!out_handle || !desc) {{
        return -1;
    }}

    if (!g_{file_name}_ctx.initialized) {{
        return -2;
    }}

    if (g_{file_name}_ctx.count >= g_{file_name}_ctx.capacity) {{
        // {todos[16]}
        return -3;
    }}

    uint32_t index = g_{file_name}_ctx.count++;
    {struct_name}_internal_t* item = &g_{file_name}_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}}

void {struct_name}_destroy({struct_name}_handle_t handle) {{
    // {todos[17]}
    // {todos[18]}

    if (handle.id >= g_{file_name}_ctx.count) {{
        return;
    }}

    {struct_name}_cleanup_internal(&g_{file_name}_ctx.items[handle.id]);
}}

int {struct_name}_update({struct_name}_handle_t handle, const void* data, size_t size) {{
    // {todos[19]}
    // {todos[20]}
    // {todos[21]}
    // {todos[22]}

    if (handle.id >= g_{file_name}_ctx.count) {{
        return -1;
    }}

    {struct_name}_internal_t* item = &g_{file_name}_ctx.items[handle.id];
    if (!item->initialized) {{
        return -2;
    }}

    // {todos[23]}
    // {todos[24]}

    item->dirty = true;
    return 0;
}}

bool {struct_name}_is_valid({struct_name}_handle_t handle) {{
    // {todos[25]}
    if (handle.id >= g_{file_name}_ctx.count) {{
        return false;
    }}
    return g_{file_name}_ctx.items[handle.id].initialized;
}}

int {struct_name}_get_info({struct_name}_handle_t handle, {struct_name}_info_t* out_info) {{
    // {todos[26]}
    // {todos[27]}

    if (!out_info) {{
        return -1;
    }}

    if (handle.id >= g_{file_name}_ctx.count) {{
        return -2;
    }}

    const {struct_name}_internal_t* item = &g_{file_name}_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}}

void {struct_name}_mark_dirty({struct_name}_handle_t handle) {{
    // {todos[28]}
    if (handle.id < g_{file_name}_ctx.count) {{
        g_{file_name}_ctx.items[handle.id].dirty = true;
    }}
}}

int {struct_name}_process_pending(void) {{
    // {todos[29]}
    // {todos[30] if len(todos) > 30 else "TODO: Implement batch processing"}

    int processed = 0;
    for (uint32_t i = 0; i < g_{file_name}_ctx.count; i++) {{
        {struct_name}_internal_t* item = &g_{file_name}_ctx.items[i];
        if (item->initialized && item->dirty) {{
            // Process item
            item->dirty = false;
            processed++;
        }}
    }}

    return processed;
}}

uint32_t {struct_name}_get_count(void) {{
    return g_{file_name}_ctx.count;
}}

size_t {struct_name}_get_memory_usage(void) {{
    // {todos[31] if len(todos) > 31 else "TODO: Implement memory tracking"}
    size_t total = sizeof(g_{file_name}_ctx);
    total += g_{file_name}_ctx.capacity * sizeof({struct_name}_internal_t);

    for (uint32_t i = 0; i < g_{file_name}_ctx.count; i++) {{
        total += g_{file_name}_ctx.items[i].data_size;
    }}

    return total;
}}

void {struct_name}_debug_print(void) {{
    // {todos[32] if len(todos) > 32 else "TODO: Implement debug output"}
    // Debug printing implementation
}}

/* End of {file_name}.c */
'''

    return content, len(todos)


def generate_h_file(subsystem: str, subdir: str, file_name: str, description: str) -> str:
    """Generate a .h header file."""
    struct_name = f"{subsystem}_{file_name}"
    guard = f"{subsystem.upper()}_{file_name.upper()}_H"

    content = f'''/*
 * {file_name}.h
 * {description}
 *
 * Part of the {subsystem.replace("_", " ").title()} subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef {guard}
#define {guard}

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {{
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {struct_name}_handle {{
    uint32_t id;
}} {struct_name}_handle_t;

typedef struct {struct_name}_desc {{
    uint32_t flags;
    void* user_data;
}} {struct_name}_desc_t;

typedef struct {struct_name}_info {{
    uint32_t id;
    uint32_t flags;
    bool initialized;
}} {struct_name}_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int {struct_name}_init(void);
void {struct_name}_shutdown(void);

/* Lifecycle */
int {struct_name}_create({struct_name}_handle_t* out_handle, const {struct_name}_desc_t* desc);
void {struct_name}_destroy({struct_name}_handle_t handle);

/* Operations */
int {struct_name}_update({struct_name}_handle_t handle, const void* data, size_t size);
bool {struct_name}_is_valid({struct_name}_handle_t handle);
int {struct_name}_get_info({struct_name}_handle_t handle, {struct_name}_info_t* out_info);
void {struct_name}_mark_dirty({struct_name}_handle_t handle);
int {struct_name}_process_pending(void);

/* Statistics */
uint32_t {struct_name}_get_count(void);
size_t {struct_name}_get_memory_usage(void);
void {struct_name}_debug_print(void);

#ifdef __cplusplus
}}
#endif

#endif /* {guard} */
'''

    return content


def main():
    """Main generation function."""
    print("=" * 80)
    print("RENDERING ENGINE GENERATOR V2 - PROPER FILE NAMES")
    print("=" * 80)
    print()

    # First, remove the old generated files
    print("Removing old generated files...")
    import shutil

    for subsystem in SUBSYSTEMS.keys():
        subsystem_path = BASE_PATH / subsystem
        if subsystem_path.exists():
            shutil.rmtree(subsystem_path)

    # Remove extra folder if exists
    extra_path = BASE_PATH / "extra"
    if extra_path.exists():
        shutil.rmtree(extra_path)

    total_c_files = 0
    total_h_files = 0
    total_todos = 0

    for subsystem, subdirs in SUBSYSTEMS.items():
        print(f"\n[{subsystem.upper()}] Generating...")
        subsystem_path = BASE_PATH / subsystem
        subsystem_path.mkdir(parents=True, exist_ok=True)

        for subdir, files in subdirs.items():
            subdir_path = subsystem_path / subdir
            subdir_path.mkdir(parents=True, exist_ok=True)

            for file_name, description in files:
                # Generate .c file
                c_content, todo_count = generate_c_file(subsystem, subdir, file_name, description)
                c_path = subdir_path / f"{file_name}.c"
                c_path.write_text(c_content)
                total_c_files += 1
                total_todos += todo_count

                # Generate .h file
                h_content = generate_h_file(subsystem, subdir, file_name, description)
                h_path = subdir_path / f"{file_name}.h"
                h_path.write_text(h_content)
                total_h_files += 1

        file_count = sum(len(files) for files in subdirs.values()) * 2
        print(f"    Generated {file_count} files in {len(subdirs)} subdirectories")

    print()
    print("=" * 80)
    print("GENERATION COMPLETE")
    print("=" * 80)
    print(f"Total .c files: {total_c_files}")
    print(f"Total .h files: {total_h_files}")
    print(f"Total files: {total_c_files + total_h_files}")
    print(f"Total TODOs: {total_todos}")
    print()

    # Count for verification
    c_files = list(BASE_PATH.rglob("*.c"))
    h_files = list(BASE_PATH.rglob("*.h"))

    print(f"Verification:")
    print(f"  Actual .c files: {len(c_files)}")
    print(f"  Actual .h files: {len(h_files)}")


if __name__ == "__main__":
    main()
