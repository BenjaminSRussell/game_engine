# Rendering Engine Implementation Phases

## Overview
- **6 Phases** of implementation
- **6 Agents per phase** working in parallel
- **~100 TODOs per agent** per round
- **Total: 36 agent tasks** covering the entire rendering pipeline

---

## PHASE 1: Core Infrastructure & Foundation
*Must be completed first - all other systems depend on this*

### Agent 1.1: Core Device & Memory
**Directories:** `core/`, `memory/`, `platform/vulkan/`, `platform/metal/`, `platform/d3d12/`
**Focus:** GPU device initialization, memory allocation, platform backends

### Agent 1.2: Command & Synchronization
**Directories:** `core/command/`, `core/sync/`, `synchronization/`, `core/pipeline/`
**Focus:** Command buffers, fences, semaphores, pipeline state

### Agent 1.3: Resource Management
**Directories:** `resource_management/`, `core/descriptor/`, `bindless_rendering/`
**Focus:** Resource handles, descriptor sets, bindless resources

### Agent 1.4: Render Graph & Frame Management
**Directories:** `rendering/render_graph/`, `framebuffer/`, `rendering/output/`
**Focus:** Render pass scheduling, render targets, swapchain

### Agent 1.5: Math & Utilities
**Directories:** `math/`, `statistics/`, `profiling/`, `debugging/`
**Focus:** Math library, profiling, debug visualization

### Agent 1.6: Basic Shading Infrastructure
**Directories:** `shading/brdf/`, `materials/shaders/`, `materials/pbr/`
**Focus:** BRDF implementation, shader compilation, PBR basics

---

## PHASE 2: Geometry & Mesh Systems
*Depends on Phase 1*

### Agent 2.1: Mesh Data & Loading
**Directories:** `geometry/mesh/`, `geometry/vertex/`, `asset_system/loading/`, `asset_system/formats/`
**Focus:** Mesh data structures, vertex formats, asset loading

### Agent 2.2: LOD & Streaming
**Directories:** `geometry/lod/`, `geometry/streaming/`, `lod_generation/`, `lod_streaming/`
**Focus:** LOD generation, mesh streaming, LOD selection

### Agent 2.3: Instancing & Batching
**Directories:** `geometry/instancing/`, `geometry/bvh/`, `instanced_static_mesh/`, `scene_management/batching/`
**Focus:** GPU instancing, BVH construction, draw batching

### Agent 2.4: Meshlets & Nanite Foundation
**Directories:** `geometry/meshlets/`, `nanite/cluster/`, `nanite/streaming/`
**Focus:** Meshlet generation, cluster DAG, Nanite streaming

### Agent 2.5: Culling Systems
**Directories:** `culling/`, `gpu_culling/`, `occlusion/`, `gpu_occlusion/`
**Focus:** Frustum culling, occlusion culling, HZB

### Agent 2.6: Static & Skeletal Mesh Rendering
**Directories:** `static_mesh_rendering/`, `skeletal_mesh_rendering/`, `rendering/forward/`, `rendering/deferred/`
**Focus:** Basic mesh rendering passes

---

## PHASE 3: Lighting & Shadows
*Depends on Phase 1 & 2*

### Agent 3.1: Light Sources & Management
**Directories:** `lighting/sources/`, `lighting/probes/`, `reflection_captures/`
**Focus:** Point/spot/directional lights, light culling, probes

### Agent 3.2: Shadow Mapping
**Directories:** `lighting/shadows/`, `lighting/cascades/`, `shadows_advanced/shadow_techniques/`
**Focus:** Shadow maps, CSM, VSM, ESM

### Agent 3.3: Virtual Shadow Maps
**Directories:** `virtual_shadow_maps/`, `shadows_advanced/ray_traced_shadows/`
**Focus:** VSM page management, RT shadows

### Agent 3.4: Global Illumination Basics
**Directories:** `lighting/global_illumination/`, `lighting/lightmaps/`, `lightmap_baking/`, `gi_baking/`
**Focus:** Lightmaps, irradiance, basic GI

### Agent 3.5: Lumen GI System
**Directories:** `lumen/surface_cache/`, `lumen/radiance_cache/`, `lumen/tracing_lumen/`, `lumen/final_gather/`
**Focus:** Lumen surface cache, radiance cache, ray tracing GI

### Agent 3.6: Volumetric Lighting
**Directories:** `lighting/volumetric/`, `volumetric_fog_advanced/`, `atmosphere/`
**Focus:** Volumetric fog, god rays, atmospheric scattering

---

## PHASE 4: Materials & Textures
*Depends on Phase 1, 2, 3*

### Agent 4.1: Material System Core
**Directories:** `materials/material_system/`, `materials/textures/`, `materials/layering/`
**Focus:** Material instances, parameter binding, layering

### Agent 4.2: Advanced Shading Models
**Directories:** `shading/subsurface/`, `shading/cloth/`, `shading/hair/`, `shading/clearcoat/`, `shading/special/`
**Focus:** SSS, cloth BRDF, hair shading, clearcoat

### Agent 4.3: Texture Streaming & Virtual Texturing
**Directories:** `texture/streaming/`, `texture/virtual_texturing/`, `texture/compression/`
**Focus:** Texture streaming, VT, compression

### Agent 4.4: Specialized Material Rendering
**Directories:** `skin_rendering/`, `eye_rendering/`, `hair_rendering/`, `fabric_rendering/`
**Focus:** Character rendering materials

### Agent 4.5: Environment Materials
**Directories:** `metal_rendering/`, `wood_rendering/`, `stone_rendering/`, `concrete_rendering/`, `glass_rendering/`
**Focus:** PBR materials for environment

### Agent 4.6: Material Effects & Weathering
**Directories:** `age_weathering/`, `rust_corrosion/`, `dirt_grunge/`, `wear_tear/`, `wetness_system/`
**Focus:** Procedural weathering, damage

---

## PHASE 5: Effects & Post-Processing
*Depends on Phase 1, 2, 3, 4*

### Agent 5.1: Particle Systems
**Directories:** `effects/particles/`, `effects/gpu_particles/`, `effects/vfx_graph/`
**Focus:** CPU/GPU particles, VFX graph

### Agent 5.2: Environmental Effects
**Directories:** `effects/weather/`, `effects/decals/`, `effects/trails/`, `puddles/`, `footprints/`
**Focus:** Weather, decals, environmental interaction

### Agent 5.3: Special Effects
**Directories:** `fire_rendering/`, `smoke_rendering/`, `explosion_effects/`, `liquid_rendering/`
**Focus:** Fire, smoke, explosions, liquids

### Agent 5.4: Tonemapping & Color
**Directories:** `postprocessing/tonemapping/`, `postprocessing/bloom/`, `postprocessing/ambient_occlusion/`
**Focus:** HDR, bloom, SSAO/GTAO

### Agent 5.5: Anti-Aliasing & Temporal
**Directories:** `postprocessing/anti_aliasing/`, `postprocessing/motion_blur/`, `temporal_upscaling/`
**Focus:** TAA, FXAA, motion blur, DLSS/FSR

### Agent 5.6: Screen-Space Effects
**Directories:** `postprocessing/screen_space/`, `postprocessing/depth_of_field/`, `screen_space_gi/`
**Focus:** SSR, DOF, SSGI

---

## PHASE 6: Advanced Systems & Integration
*Depends on all previous phases*

### Agent 6.1: Landscape & Terrain
**Directories:** `landscape/`, `landscape_grass/`, `landscape_splines/`, `vegetation_rendering_advanced/`
**Focus:** Terrain rendering, grass, vegetation

### Agent 6.2: Water & Ocean
**Directories:** `water/`, `ocean_waves/`, `planar_reflections/`
**Focus:** Ocean FFT, rivers, underwater

### Agent 6.3: Ray Tracing Integration
**Directories:** `raytracing/acceleration/`, `raytracing/shadows_rt/`, `raytracing/reflections_rt/`, `raytracing/gi_rt/`
**Focus:** RT acceleration structures, RT effects

### Agent 6.4: Nanite & GPU-Driven Rendering
**Directories:** `nanite/culling_nanite/`, `nanite/rendering_nanite/`, `rendering/gpu_driven/`, `visibility_buffer/`
**Focus:** Nanite rendering, visibility buffer

### Agent 6.5: Animation & Physics Rendering
**Directories:** `animation/`, `cloth_system/`, `hair_system/`, `destruction/`, `physics/`
**Focus:** Skeletal animation, cloth, hair, destruction

### Agent 6.6: Editor & Tools
**Directories:** `editor/`, `tools/`, `ui_rendering/`, `asset_system/import/`
**Focus:** Editor viewport, gizmos, asset import

---

## Execution Order

```
Phase 1 (Foundation)     ████████████████████████
                              |
Phase 2 (Geometry)            ████████████████████████
                                   |
Phase 3 (Lighting)                 ████████████████████████
                                        |
Phase 4 (Materials)                     ████████████████████████
                                             |
Phase 5 (Effects)                            ████████████████████████
                                                  |
Phase 6 (Advanced)                                ████████████████████████
```

Each phase's 6 agents run **in parallel** within their phase.
Phases run **sequentially** to ensure dependencies are met.
