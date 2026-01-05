# Missing TODOs (Phases 1-6)

Audit source: `src/engine/rendering/3d_rendering/agents/phase*_prompts.md` file lists, checked for file existence in the listed directories and scanned for `TODO` markers.

## Summary
- Missing files (not found in listed directories): 10
- Files with TODO markers: 196
- Files found outside listed directories: 65

## Phase Breakdown
- phase1: missing files 0, TODO files 23, found elsewhere 6
- phase2: missing files 1, TODO files 39, found elsewhere 10
- phase3: missing files 1, TODO files 28, found elsewhere 13
- phase4: missing files 0, TODO files 15, found elsewhere 0
- phase5: missing files 2, TODO files 48, found elsewhere 0
- phase6: missing files 6, TODO files 43, found elsewhere 36

## Details

### phase1

#### Agent 1.1: Core Device & Memory
- Files with TODOs:
  - src/engine/rendering/3d_rendering/core/device/device_capabilities.c (TODOs: 32)
  - src/engine/rendering/3d_rendering/platform/d3d12/dx_device.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/platform/metal/mtl_device.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/platform/vulkan/vk_device.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/platform/vulkan/vk_instance.c (TODOs: 31)

#### Agent 1.2: Command & Synchronization
- Files with TODOs:
  - src/engine/rendering/3d_rendering/core/command/command_buffer.c (TODOs: 1)
  - src/engine/rendering/3d_rendering/core/command/command_pool.c (TODOs: 1)

#### Agent 1.3: Resource Management
- Files with TODOs:
  - src/engine/rendering/3d_rendering/bindless_rendering/bindless_manager.c (TODOs: 6)
  - src/engine/rendering/3d_rendering/core/descriptor/descriptor_set_layout.c (TODOs: 2)
  - src/engine/rendering/3d_rendering/core/descriptor/dynamic_descriptors.c (TODOs: 2)
  - src/engine/rendering/3d_rendering/core/descriptor/descriptor_pool.c (TODOs: 1)

#### Agent 1.4: Render Graph & Frame Management
- Files found outside listed directories:
  - dynamic_resolution.c -> src/engine/renderer/optimization/dynamic_resolution.c
  - render_target.c -> src/engine/rendering/3d_rendering/framebuffer/render_targets/render_target.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/rendering/render_graph/resource_node.c (TODOs: 61)
  - src/engine/rendering/3d_rendering/rendering/render_graph/render_pass_node.c (TODOs: 58)
  - src/engine/rendering/3d_rendering/rendering/output/frame_pacing.c (TODOs: 56)
  - src/engine/rendering/3d_rendering/rendering/output/hdr_output.c (TODOs: 56)
  - src/engine/rendering/3d_rendering/rendering/output/swapchain.c (TODOs: 56)
  - src/engine/rendering/3d_rendering/rendering/render_graph/graph_executor.c (TODOs: 35)
  - src/engine/rendering/3d_rendering/rendering/render_graph/graph_compiler.c (TODOs: 32)

#### Agent 1.5: Math & Utilities
- Files found outside listed directories:
  - debug_lines.c -> src/engine/rendering/3d_rendering/editor/debug_viz/debug_lines.c
  - debug_shapes.c -> src/engine/rendering/3d_rendering/editor/debug_viz/debug_shapes.c
  - frame_timer.c -> src/engine/rendering/3d_rendering/profiling/cpu/frame_timer.c
  - gpu_timer.c -> src/engine/rendering/3d_rendering/profiling/gpu/gpu_timer.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/editor/debug_viz/debug_lines.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/debug_viz/debug_shapes.c (TODOs: 64)

#### Agent 1.6: Basic Shading Infrastructure
- Files with TODOs:
  - src/engine/rendering/3d_rendering/shading/brdf/diffuse_brdf.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/shading/brdf/energy_conservation.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/materials/shaders/shader_variants.c (TODOs: 1)

### phase2

#### Agent 2.1: Mesh Data & Loading
- Files with TODOs:
  - src/engine/rendering/3d_rendering/asset_system/formats/mesh_format.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/asset_system/loading/asset_loader.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/asset_system/loading/async_loading.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/mesh/mesh_builder.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/mesh/mesh_loader.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/vertex/index_buffer_pool.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/vertex/vertex_buffer_pool.c (TODOs: 64)

#### Agent 2.2: LOD & Streaming
- Missing files:
  - continuous_lod.c (expected in src/engine/rendering/3d_rendering/geometry/lod/, src/engine/rendering/3d_rendering/geometry/streaming/, src/engine/rendering/3d_rendering/lod_generation/, src/engine/rendering/3d_rendering/lod_streaming/)
- Files with TODOs:
  - src/engine/rendering/3d_rendering/geometry/lod/lod_bias.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/lod/lod_crossfade.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/lod/lod_generator.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/lod/lod_selector.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/lod/screen_size_lod.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/streaming/mesh_streamer.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/streaming/prefetch_system.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/streaming/resident_set.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/streaming/stream_priority.c (TODOs: 64)

#### Agent 2.3: Instancing & Batching
- Files with TODOs:
  - src/engine/rendering/3d_rendering/geometry/bvh/bvh_builder.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/bvh/bvh_traversal.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/instancing/gpu_instance_data.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/instancing/indirect_instancing.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/instancing/instance_batching.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/instancing/instance_buffer.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/instancing/instance_culler.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/scene_management/batching/batch_statistics.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/scene_management/batching/dynamic_batching.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/scene_management/batching/static_batching.c (TODOs: 64)

#### Agent 2.4: Meshlets & Nanite Foundation
- Files with TODOs:
  - src/engine/rendering/3d_rendering/geometry/meshlets/cluster_dag.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/geometry/meshlets/meshlet_builder.c (TODOs: 1)
  - src/engine/rendering/3d_rendering/nanite/cluster/cluster_builder.c (TODOs: 1)

#### Agent 2.5: Culling Systems
- Files found outside listed directories:
  - frustum_aabb_test.c -> src/engine/rendering/3d_rendering/culling/frustum/frustum_aabb_test.c
  - frustum_planes.c -> src/engine/rendering/3d_rendering/culling/frustum/frustum_planes.c
  - simd_frustum_cull.c -> src/engine/rendering/3d_rendering/culling/frustum/simd_frustum_cull.c
  - gpu_cull_compute.c -> src/engine/rendering/3d_rendering/culling/gpu_culling/gpu_cull_compute.c
  - hzb_builder.c -> src/engine/rendering/3d_rendering/culling/occlusion/hzb_builder.c
  - temporal_occlusion.c -> src/engine/rendering/3d_rendering/culling/occlusion/temporal_occlusion.c
  - two_pass_cull.c -> src/engine/rendering/3d_rendering/nanite/culling_nanite/two_pass_cull.c
  - hzb_test.c -> src/engine/rendering/3d_rendering/occlusion/hzb/hzb_test.c
  - occlusion_query.c -> src/engine/rendering/3d_rendering/occlusion/software_occlusion/occlusion_query.c
  - sw_rasterizer.c -> src/engine/rendering/3d_rendering/occlusion/software_occlusion/sw_rasterizer.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/culling/frustum/frustum_aabb_test.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/culling/frustum/frustum_planes.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/culling/frustum/simd_frustum_cull.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/culling/gpu_culling/gpu_cull_compute.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/culling/occlusion/hzb_builder.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/culling/occlusion/temporal_occlusion.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/nanite/culling_nanite/two_pass_cull.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/occlusion/hzb/hzb_test.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/occlusion/software_occlusion/occlusion_query.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/occlusion/software_occlusion/sw_rasterizer.c (TODOs: 64)

### phase3

#### Agent 3.1: Light Sources & Management
- Missing files:
  - sphere_light.c (expected in src/engine/rendering/3d_rendering/lighting/sources/, src/engine/rendering/3d_rendering/lighting/probes/, src/engine/rendering/3d_rendering/reflection_captures/)
- Files found outside listed directories:
  - light_culling.c -> src/engine/rendering/3d_rendering/lighting/culling/light_culling.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/lighting/probes/irradiance_probe.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/lighting/probes/probe_blending.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/lighting/probes/probe_parallax.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/lighting/probes/reflection_probe.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/lighting/sources/directional_light.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/lighting/culling/light_culling.c (TODOs: 1)

#### Agent 3.3: Virtual Shadow Maps
- Files found outside listed directories:
  - vsm_allocation.c -> src/engine/rendering/3d_rendering/virtual_shadow_maps/page_management/vsm_allocation.c
  - vsm_invalidation.c -> src/engine/rendering/3d_rendering/virtual_shadow_maps/page_management/vsm_invalidation.c
  - vsm_page_pool.c -> src/engine/rendering/3d_rendering/virtual_shadow_maps/page_management/vsm_page_pool.c
  - vsm_page_table.c -> src/engine/rendering/3d_rendering/virtual_shadow_maps/page_management/vsm_page_table.c
  - vsm_culling.c -> src/engine/rendering/3d_rendering/virtual_shadow_maps/rendering_vsm/vsm_culling.c
  - vsm_rasterization.c -> src/engine/rendering/3d_rendering/virtual_shadow_maps/rendering_vsm/vsm_rasterization.c
  - vsm_sampling.c -> src/engine/rendering/3d_rendering/virtual_shadow_maps/rendering_vsm/vsm_sampling.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/virtual_shadow_maps/rendering_vsm/vsm_rasterization.c (TODOs: 3)
  - src/engine/rendering/3d_rendering/shadows_advanced/ray_traced_shadows/rt_shadow_denoise.c (TODOs: 2)
  - src/engine/rendering/3d_rendering/shadows_advanced/ray_traced_shadows/rt_shadow_ray.c (TODOs: 1)
  - src/engine/rendering/3d_rendering/virtual_shadow_maps/rendering_vsm/vsm_culling.c (TODOs: 1)
  - src/engine/rendering/3d_rendering/virtual_shadow_maps/rendering_vsm/vsm_sampling.c (TODOs: 1)

#### Agent 3.4: Global Illumination Basics
- Files with TODOs:
  - src/engine/rendering/3d_rendering/lighting/global_illumination/gi_debug_viz.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/lighting/lightmaps/directional_lightmaps.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/lighting/lightmaps/lightmap_baker.c (TODOs: 60)
  - src/engine/rendering/3d_rendering/lighting/lightmaps/lightmap_sampling.c (TODOs: 58)
  - src/engine/rendering/3d_rendering/lighting/lightmaps/lightmap_uvs.c (TODOs: 58)
  - src/engine/rendering/3d_rendering/lighting/global_illumination/gi_probe_grid.c (TODOs: 54)
  - src/engine/rendering/3d_rendering/lighting/lightmaps/lightmap_packer.c (TODOs: 54)

#### Agent 3.5: Lumen GI System
- Files with TODOs:
  - src/engine/rendering/3d_rendering/lumen/tracing_lumen/software_trace.c (TODOs: 56)
  - src/engine/rendering/3d_rendering/lumen/surface_cache/surface_update.c (TODOs: 54)
  - src/engine/rendering/3d_rendering/lumen/final_gather/denoise_lumen.c (TODOs: 51)
  - src/engine/rendering/3d_rendering/lumen/final_gather/final_gather_rays.c (TODOs: 51)
  - src/engine/rendering/3d_rendering/lumen/radiance_cache/radiance_inject.c (TODOs: 51)
  - src/engine/rendering/3d_rendering/lumen/tracing_lumen/screen_trace.c (TODOs: 51)
  - src/engine/rendering/3d_rendering/lumen/surface_cache/surface_capture.c (TODOs: 50)
  - src/engine/rendering/3d_rendering/lumen/surface_cache/surface_card.c (TODOs: 49)
  - src/engine/rendering/3d_rendering/lumen/radiance_cache/probe_octahedron.c (TODOs: 48)
  - src/engine/rendering/3d_rendering/lumen/surface_cache/surface_atlas.c (TODOs: 48)

#### Agent 3.6: Volumetric Lighting
- Files found outside listed directories:
  - volumetric_clouds.c -> src/engine/renderer/sky/volumetric_clouds.c
  - aerial_perspective.c -> src/engine/rendering/3d_rendering/atmosphere/sky/aerial_perspective.c
  - atmospheric_scattering.c -> src/engine/rendering/3d_rendering/atmosphere/sky/atmospheric_scattering.c
  - sky_lut.c -> src/engine/rendering/3d_rendering/atmosphere/sky/sky_lut.c
  - sun_disk.c -> src/engine/rendering/3d_rendering/atmosphere/sky/sun_disk.c

### phase4

#### Agent 4.1: Material System Core
- Files with TODOs:
  - src/engine/rendering/3d_rendering/materials/material_system/material_instance.c (TODOs: 1)

#### Agent 4.2: Advanced Shading Models
- Files with TODOs:
  - src/engine/rendering/3d_rendering/shading/subsurface/sss_blur.c (TODOs: 1)

#### Agent 4.3: Texture Streaming & Virtual Texturing
- Files with TODOs:
  - src/engine/rendering/3d_rendering/texture/virtual_texturing/virtual_texture.c (TODOs: 1)

#### Agent 4.4: Specialized Material Rendering
- Files with TODOs:
  - src/engine/rendering/3d_rendering/skin_rendering/skin_diffusion.c (TODOs: 18)
  - src/engine/rendering/3d_rendering/hair_rendering/hair_transparency.c (TODOs: 11)
  - src/engine/rendering/3d_rendering/skin_rendering/skin_detail.c (TODOs: 11)
  - src/engine/rendering/3d_rendering/hair_rendering/hair_shadows_render.c (TODOs: 9)
  - src/engine/rendering/3d_rendering/eye_rendering/iris_detail.c (TODOs: 8)
  - src/engine/rendering/3d_rendering/fabric_rendering/fabric_detail.c (TODOs: 7)
  - src/engine/rendering/3d_rendering/skin_rendering/skin_translucency.c (TODOs: 6)
  - src/engine/rendering/3d_rendering/eye_rendering/eye_caustics.c (TODOs: 5)
  - src/engine/rendering/3d_rendering/eye_rendering/eye_refraction.c (TODOs: 5)
  - src/engine/rendering/3d_rendering/fabric_rendering/fabric_sheen.c (TODOs: 4)

#### Agent 4.5: Environment Materials
- Files with TODOs:
  - src/engine/rendering/3d_rendering/metal_rendering/metal_anisotropy.c (TODOs: 1)
  - src/engine/rendering/3d_rendering/metal_rendering/metal_flakes.c (TODOs: 1)

### phase5

#### Agent 5.1: Particle Systems
- Files with TODOs:
  - src/engine/rendering/3d_rendering/effects/gpu_particles/spawn_compute.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/effects/gpu_particles/update_compute.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/effects/vfx_graph/vfx_context.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/effects/particles/particle_emitter.c (TODOs: 34)
  - src/engine/rendering/3d_rendering/effects/gpu_particles/dead_list.c (TODOs: 32)
  - src/engine/rendering/3d_rendering/effects/gpu_particles/particle_buffer.c (TODOs: 32)
  - src/engine/rendering/3d_rendering/effects/particles/gpu_particles.c (TODOs: 32)
  - src/engine/rendering/3d_rendering/effects/particles/particle_rendering.c (TODOs: 32)
  - src/engine/rendering/3d_rendering/effects/particles/particle_simulation.c (TODOs: 32)
  - src/engine/rendering/3d_rendering/effects/particles/particle_sorting.c (TODOs: 32)

#### Agent 5.2: Environmental Effects
- Files with TODOs:
  - src/engine/rendering/3d_rendering/effects/decals/decal_projector.c (TODOs: 50)
  - src/engine/rendering/3d_rendering/effects/decals/decal_rendering.c (TODOs: 50)
  - src/engine/rendering/3d_rendering/effects/decals/decal_sorting.c (TODOs: 50)
  - src/engine/rendering/3d_rendering/effects/trails/trail_points.c (TODOs: 50)
  - src/engine/rendering/3d_rendering/effects/trails/trail_renderer.c (TODOs: 50)
  - src/engine/rendering/3d_rendering/effects/weather/rain_system.c (TODOs: 50)
  - src/engine/rendering/3d_rendering/effects/weather/snow_system.c (TODOs: 50)
  - src/engine/rendering/3d_rendering/effects/weather/weather_volumes.c (TODOs: 50)

#### Agent 5.3: Special Effects
- Files with TODOs:
  - src/engine/rendering/3d_rendering/fire_rendering/fire_distortion.c (TODOs: 1)
  - src/engine/rendering/3d_rendering/fire_rendering/fire_simulation.c (TODOs: 1)
  - src/engine/rendering/3d_rendering/smoke_rendering/smoke_density.c (TODOs: 1)

#### Agent 5.4: Tonemapping & Color
- Files with TODOs:
  - src/engine/rendering/3d_rendering/postprocessing/ambient_occlusion/ao_blur.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/ambient_occlusion/gtao_pass.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/ambient_occlusion/ssao_pass.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/bloom/bloom_downsample.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/bloom/bloom_lens_dirt.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/bloom/bloom_upsample.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/tonemapping/aces_tonemapper.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/tonemapping/exposure_adaptation.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/tonemapping/histogram_compute.c (TODOs: 31)
  - src/engine/rendering/3d_rendering/postprocessing/tonemapping/reinhard_tonemapper.c (TODOs: 31)

#### Agent 5.5: Anti-Aliasing & Temporal
- Missing files:
  - dlss_wrapper.c (expected in src/engine/rendering/3d_rendering/postprocessing/anti_aliasing/, src/engine/rendering/3d_rendering/postprocessing/motion_blur/, src/engine/rendering/3d_rendering/temporal_upscaling/)
- Files with TODOs:
  - src/engine/rendering/3d_rendering/postprocessing/anti_aliasing/fxaa_pass.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/anti_aliasing/smaa_pass.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/anti_aliasing/taa_resolve.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/anti_aliasing/taa_sharpening.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/anti_aliasing/taa_velocity.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/motion_blur/motion_blur_gather.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/motion_blur/motion_blur_tile.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/motion_blur/velocity_buffer.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/anti_aliasing/taa_history.c (TODOs: 60)

#### Agent 5.6: Screen-Space Effects
- Missing files:
  - lens_flare.c (expected in src/engine/rendering/3d_rendering/postprocessing/screen_space/, src/engine/rendering/3d_rendering/postprocessing/depth_of_field/, src/engine/rendering/3d_rendering/screen_space_gi/)
- Files with TODOs:
  - src/engine/rendering/3d_rendering/postprocessing/depth_of_field/dof_bokeh.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/depth_of_field/dof_coc.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/depth_of_field/dof_combine.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/depth_of_field/dof_gather.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/screen_space/ss_gi.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/screen_space/ss_shadows.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/screen_space/ssr_fallback.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/postprocessing/screen_space/ssr_resolve.c (TODOs: 64)

### phase6

#### Agent 6.1: Landscape & Terrain
- Missing files:
  - grass_lod.c (expected in src/engine/rendering/3d_rendering/landscape/, src/engine/rendering/3d_rendering/landscape_grass/, src/engine/rendering/3d_rendering/landscape_splines/, src/engine/rendering/3d_rendering/vegetation_rendering_advanced/)
  - vegetation_interaction.c (expected in src/engine/rendering/3d_rendering/landscape/, src/engine/rendering/3d_rendering/landscape_grass/, src/engine/rendering/3d_rendering/landscape_splines/, src/engine/rendering/3d_rendering/vegetation_rendering_advanced/)
- Files found outside listed directories:
  - terrain_lod.c -> src/engine/renderer/terrain/terrain_lod.c
  - heightmap_streaming.c -> src/engine/rendering/3d_rendering/landscape/heightmaps/heightmap_streaming.c
  - splat_renderer.c -> src/engine/rendering/3d_rendering/landscape/splatmaps/splat_renderer.c
  - terrain_renderer.c -> src/engine/rendering/3d_rendering/landscape/terrain/terrain_renderer.c
  - terrain_tessellation.c -> src/engine/rendering/3d_rendering/landscape/terrain/terrain_tessellation.c
  - foliage_culling.c -> src/engine/rendering/3d_rendering/landscape/vegetation/foliage_culling.c
  - grass_renderer.c -> src/engine/rendering/3d_rendering/landscape/vegetation/grass_renderer.c
  - foliage_wind.c -> src/engine/shader_library/environment/foliage_wind.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/landscape/heightmaps/heightmap_streaming.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/landscape/splatmaps/splat_renderer.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/landscape/terrain/terrain_tessellation.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/landscape/vegetation/foliage_culling.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/landscape/vegetation/grass_renderer.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/landscape/terrain/terrain_renderer.c (TODOs: 31)

#### Agent 6.2: Water & Ocean
- Files found outside listed directories:
  - fft_waves.c -> src/engine/rendering/3d_rendering/water/ocean/fft_waves.c
  - gerstner_waves.c -> src/engine/rendering/3d_rendering/water/ocean/gerstner_waves.c
  - ocean_foam.c -> src/engine/rendering/3d_rendering/water/ocean/ocean_foam.c
  - ocean_lod.c -> src/engine/rendering/3d_rendering/water/ocean/ocean_lod.c
  - ocean_renderer.c -> src/engine/rendering/3d_rendering/water/ocean/ocean_renderer.c
  - underwater_fog.c -> src/engine/rendering/3d_rendering/water/rendering_water/underwater_fog.c
  - water_caustics.c -> src/engine/rendering/3d_rendering/water/rendering_water/water_caustics.c
  - water_reflection.c -> src/engine/rendering/3d_rendering/water/rendering_water/water_reflection.c
  - water_refraction.c -> src/engine/rendering/3d_rendering/water/rendering_water/water_refraction.c
  - river_rendering.c -> src/engine/rendering/3d_rendering/water/rivers/river_rendering.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/water/ocean/ocean_foam.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/water/ocean/ocean_lod.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/water/ocean/ocean_renderer.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/water/rendering_water/underwater_fog.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/water/rendering_water/water_caustics.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/water/rendering_water/water_reflection.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/water/rendering_water/water_refraction.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/water/rivers/river_rendering.c (TODOs: 64)

#### Agent 6.3: Ray Tracing Integration
- Missing files:
  - rt_ao.c (expected in src/engine/rendering/3d_rendering/raytracing/acceleration/, src/engine/rendering/3d_rendering/raytracing/shadows_rt/, src/engine/rendering/3d_rendering/raytracing/reflections_rt/, src/engine/rendering/3d_rendering/raytracing/gi_rt/)
- Files found outside listed directories:
  - rt_shadow_ray.c -> src/engine/rendering/3d_rendering/shadows_advanced/ray_traced_shadows/rt_shadow_ray.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/raytracing/gi_rt/ddgi_probes.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/raytracing/gi_rt/ddgi_update.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/raytracing/reflections_rt/rt_glossy_reflections.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/raytracing/reflections_rt/rt_reflection_rays.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/shadows_advanced/ray_traced_shadows/rt_shadow_ray.c (TODOs: 1)

#### Agent 6.4: Nanite & GPU-Driven Rendering
- Files found outside listed directories:
  - material_classify.c -> src/engine/rendering/3d_rendering/rendering/visibility/material_classify.c
  - visibility_buffer.c -> src/engine/rendering/3d_rendering/rendering/visibility/visibility_buffer.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/nanite/culling_nanite/two_pass_cull.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/nanite/rendering_nanite/hardware_raster.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/nanite/rendering_nanite/software_raster.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/rendering/gpu_driven/draw_command_gen.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/rendering/gpu_driven/gpu_scene.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/rendering/gpu_driven/multi_draw_indirect.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/rendering/visibility/material_classify.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/rendering/visibility/visibility_buffer.c (TODOs: 64)

#### Agent 6.5: Animation & Physics Rendering
- Missing files:
  - cloth_rendering.c (expected in src/engine/rendering/3d_rendering/animation/, src/engine/rendering/3d_rendering/cloth_system/, src/engine/rendering/3d_rendering/hair_system/, src/engine/rendering/3d_rendering/destruction/, src/engine/rendering/3d_rendering/physics/)
  - fracture_system.c (expected in src/engine/rendering/3d_rendering/animation/, src/engine/rendering/3d_rendering/cloth_system/, src/engine/rendering/3d_rendering/hair_system/, src/engine/rendering/3d_rendering/destruction/, src/engine/rendering/3d_rendering/physics/)
  - hair_rendering_sys.c (expected in src/engine/rendering/3d_rendering/animation/, src/engine/rendering/3d_rendering/cloth_system/, src/engine/rendering/3d_rendering/hair_system/, src/engine/rendering/3d_rendering/destruction/, src/engine/rendering/3d_rendering/physics/)
- Files found outside listed directories:
  - cloth_simulation.c -> src/engine/physics/softbody/cloth_simulation.c
  - blend_tree.c -> src/engine/rendering/3d_rendering/animation/blending/blend_tree.c
  - animation_player.c -> src/engine/rendering/3d_rendering/animation/skeletal/animation_player.c
  - gpu_skinning.c -> src/engine/rendering/3d_rendering/animation/skeletal/gpu_skinning.c
  - skeleton_data.c -> src/engine/rendering/3d_rendering/animation/skeletal/skeleton_data.c
  - debris_rendering.c -> src/engine/rendering/3d_rendering/destruction/rendering_destruction/debris_rendering.c
  - hair_simulation.c -> src/engine/rendering/3d_rendering/hair_rendering/hair_simulation.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/hair_rendering/hair_simulation.c (TODOs: 145)
  - src/engine/rendering/3d_rendering/animation/blending/blend_tree.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/animation/skeletal/animation_player.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/animation/skeletal/gpu_skinning.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/destruction/rendering_destruction/debris_rendering.c (TODOs: 64)
  - src/engine/physics/softbody/cloth_simulation.c (TODOs: 2)

#### Agent 6.6: Editor & Tools
- Files found outside listed directories:
  - debug_lines.c -> src/engine/rendering/3d_rendering/editor/debug_viz/debug_lines.c
  - debug_shapes.c -> src/engine/rendering/3d_rendering/editor/debug_viz/debug_shapes.c
  - editor_camera.c -> src/engine/rendering/3d_rendering/editor/viewport/editor_camera.c
  - editor_gizmos.c -> src/engine/rendering/3d_rendering/editor/viewport/editor_gizmos.c
  - editor_grid.c -> src/engine/rendering/3d_rendering/editor/viewport/editor_grid.c
  - editor_icons.c -> src/engine/rendering/3d_rendering/editor/viewport/editor_icons.c
  - editor_picking.c -> src/engine/rendering/3d_rendering/editor/viewport/editor_picking.c
  - editor_selection.c -> src/engine/rendering/3d_rendering/editor/viewport/editor_selection.c
- Files with TODOs:
  - src/engine/rendering/3d_rendering/asset_system/import/fbx_importer.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/asset_system/import/gltf_importer.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/debug_viz/debug_lines.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/debug_viz/debug_shapes.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/viewport/editor_camera.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/viewport/editor_gizmos.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/viewport/editor_grid.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/viewport/editor_icons.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/viewport/editor_picking.c (TODOs: 64)
  - src/engine/rendering/3d_rendering/editor/viewport/editor_selection.c (TODOs: 64)
