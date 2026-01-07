# 3D Rendering Pipeline Architecture

## Overview

The 3D rendering engine supports multiple rendering paths optimized for different use cases and hardware capabilities. This document establishes the architectural hierarchy and integration strategy.

---

## Primary Rendering Path: GPU-Driven Rendering

**Location:** `rendering/gpu_driven/`

**Status:** PRIMARY - Default for modern GPUs

### Design
GPU-driven rendering uses compute shaders and indirect commands to offload culling, LOD selection, and command generation to the GPU.

### Key Components
- **gpu_scene.{c,h}** - Main GPU scene data structure
- **gpu_scene.c** - Scene management and synchronization
- **draw_command_gen.{c,h}** - Indirect draw command generation
- **instance_data_gpu.{c,h}** - GPU instance buffer management
- **material_data_gpu.{c,h}** - GPU material parameter buffers
- **multi_draw_indirect.{c,h}** - Multi-draw indirect dispatch
- **persistent_mapping.{c,h}** - GPU-CPU memory coherency

### Advantages
✅ Reduced CPU overhead
✅ GPU-side culling and LOD selection
✅ Efficient batch reduction
✅ Scales to millions of objects

### When to Use
- Modern GPUs (compute capable)
- Large scenes with many objects
- Performance-critical applications
- Default rendering pipeline

---

## Secondary Rendering Paths

### Forward+ Rendering

**Location:** `rendering/forward/`

**Status:** SECONDARY - Fallback and specialized uses

#### Design
Forward rendering with clustered light culling for efficient per-object shading.

#### Key Components
- **forward_pass.{c,h}** - Main forward rendering pass
- **forward_lighting.{c,h}** - Per-object lighting calculation
- **forward_plus.{c,h}** - Clustered light culling
- **forward_clustering.{c,h}** - Light cluster generation
- **depth_prepass.{c,h}** - Z-prepass optimization
- **forward_transparency.{c,h}** - OIT or weighted blend transparency
- **forward_shadows.{c,h}** - Shadow integration

#### Advantages
✅ Simpler architecture than deferred
✅ Works with any BRDF complexity
✅ Better for low triangle count scenes
✅ Fallback for older hardware

#### When to Use
- Mobile/lower-end devices
- Few dynamic lights (<64)
- Non-complex BRDF requirements
- VR with lower overhead requirements

---

### Deferred Rendering

**Location:** `rendering/deferred/`

**Status:** SECONDARY - G-buffer effects pipeline

#### Design
Deferred rendering with G-buffer for complex lighting and screen-space effects.

#### Key Components
- **gbuffer_pass.{c,h}** - G-buffer generation
- **gbuffer_layout.{c,h}** - G-buffer format definition
- **gbuffer_resolve.{c,h}** - G-buffer resolve and composition
- **thin_gbuffer.{c,h}** - Reduced footprint G-buffer variant
- **deferred_lighting.{c,h}** - Light pass computation
- **deferred_lighting.metal** - Metal implementation
- **deferred_decals.{c,h}** - Decal application

#### Advantages
✅ Decouples lighting from geometry
✅ Many dynamic lights efficiently
✅ Base for screen-space effects
✅ Efficient memory access patterns

#### When to Use
- Many dynamic lights (>64)
- Screen-space effects required (SSAO, SSR)
- Decal-heavy scenes
- Deferred composition for post-effects

---

### Ray Tracing Integration

**Location:** `rendering/raytracing/`

**Status:** OPTIONAL - Hybrid rasterization + RT path

#### Design
Selective ray tracing for specific effects while maintaining rasterization for primary visibility.

#### Key Subsystems

**Shadows:** `raytracing/shadows_rt/`
- Ray-traced shadow rays for soft shadows
- Area light penumbra calculation
- Hybrid fallback to rasterized shadows

**Global Illumination:** `raytracing/gi_rt/`
- DDGI (Dynamic Diffuse Global Illumination) with probe grids
- ReSTIR GI for advanced light reuse
- Ray-traced ambient occlusion

**Reflections:** `raytracing/reflections_rt/` (if present)
- Screen-space ray tracing with GBuffers
- Ray-traced reflections for high-quality glass/metal

#### Advantages
✅ Physically accurate shadows
✅ Complex indirect lighting
✅ High-quality reflections
✅ Beautiful ground truth rendering

#### Disadvantages
⚠️ Significantly higher cost
⚠️ Requires advanced GPU hardware
⚠️ Temporal stability challenges
⚠️ Denoising complexity

#### When to Use
- Premium/high-end rendering path
- Selective effect enhancement (shadows + GI)
- Offline or high-latency applications
- Consoles with fixed hardware

---

## Rendering Path Selection Strategy

### Decision Tree

```
Modern GPU with compute?
├─ YES: Use GPU-Driven Rendering (primary)
│   └─ Need soft shadows? → Add RT shadow rays
│   └─ Need complex GI? → Add DDGI/ReSTIR
│   └─ Need decals? → Use deferred G-buffer
│   └─ Many lights? → Use Forward+ clustering
│
└─ NO: Legacy hardware
    └─ Many lights?
        ├─ YES: Use Deferred Rendering
        │   └─ Add Forward+ clustering if available
        └─ NO: Use Forward Rendering
            └─ Implement clustering if possible
```

### Configuration Flags (in rendering_engine.h)

```c
#define RENDER_PATH_PRIMARY        RENDER_PATH_GPU_DRIVEN   // Primary path
#define RENDER_PATH_FALLBACK       RENDER_PATH_FORWARD_PLUS // Fallback
#define RENDER_ENABLE_DEFERRED     1                         // For effects
#define RENDER_ENABLE_RAYTRACING   1                         // Optional
#define RENDER_ENABLE_DDGI         1                         // Optional GI
```

---

## Pipeline Stages (GPU-Driven)

```
Input Scene
    ↓
[CPU] Update Scene Data
    ├─ Transform updates
    ├─ Material parameter updates
    ├─ Light updates
    └─ Upload to GPU
    ↓
[GPU] Scene Culling & LOD
    ├─ Frustum culling (compute)
    ├─ Occlusion culling (compute)
    ├─ LOD selection (compute)
    └─ Generate indirect draw commands
    ↓
[GPU] Depth Prepass (optional)
    ├─ Early Z rejection optimization
    └─ Prepare for pixel shaders
    ↓
[GPU] Forward/Deferred Pass
    ├─ Render visible geometry
    ├─ Compute shading (PBR)
    └─ Apply shadows & GI
    ↓
[GPU] Post-Processing
    ├─ Bloom
    ├─ Tone mapping
    ├─ TAA/FSR upscaling
    └─ Final composite
    ↓
Screen Output
```

---

## Rendering Passes Organization

### `rendering/render_graph/`
**Purpose:** Frame graph for render pass scheduling and resource management

**Key Files:**
- `render_pass_node.{c,h}` - Individual render pass node
- `resource_node.{c,h}` - Resource aliasing and lifecycle
- `graph_compiler.c` - Compile passes into optimal order
- `graph_executor.c` - Execute compiled graph
- `barrier_insertion.c` - Automatic synchronization barriers
- `resource_aliasing.{c,h}` - Memory reuse across passes

---

## Lighting Subsystem

### Location: `lighting/`

**Shadow Systems:**
- `shadows/` - Rasterization-based shadows (primary)
  - `pcf_filter.{c,h}` - Percentage Closer Filtering
  - `pcss_filter.{c,h}` - Soft shadows
  - `csm_manager.{c,h}` - Cascaded Shadow Maps
  - `raytraced/` - Optional ray-traced shadows

**Global Illumination:**
- `global_illumination/` - Generic GI infrastructure
- `raytracing/gi_rt/` - DDGI/ReSTIR (primary GI system)

**Light Sources:**
- `sources/` - Directional, point, spot lights
- `probes/` - Light probes for real-time GI
- `volumetric/` - Volumetric fog and lighting

---

## Material System

### Location: `materials/`

**Component Separation:**
- `material_system/` - Material lifecycle and instances
- `pbr/` - PBR BRDF models
- `shaders/` - Shader compilation and variants
- `types/` - Material type specializations (wood, metal, etc.)

### GPU Integration:
- `rendering/gpu_driven/material_data_gpu.c` - GPU buffer layout
- `backend/metal/` - Platform-specific shader compilation

---

## Architecture Decision Log

### Why GPU-Driven as Primary?
- Modern GPU-driven rendering offloads CPU bottlenecks
- Scales to millions of dynamic objects
- Efficient culling and LOD on GPU reduces draw calls
- Future-proof architecture for next-gen hardware

### Why Keep Multiple Paths?
- Fallback compatibility for older hardware
- Different scenarios have different optimal solutions
- Deferred enables screen-space effects efficiently
- Forward handles complex materials more elegantly

### Why Selective Ray Tracing?
- Full RT is too expensive for real-time games
- Hybrid approach gets best of both worlds
- Selective effects provide visual quality improvement
- DDGI and RT shadows are high ROI features

### Why Consolidate Shadows?
- Single rasterization path with optional RT extension
- Reduces code duplication
- Clear upgrade path from raster to ray-traced

### Why DDGI/ReSTIR for GI?
- More efficient than Lumen for real-time (no surface caching overhead)
- Probe-based approach is well-understood
- Better temporal stability than screen-space techniques
- Scales better than voxel cone tracing

---

## Future Improvements

- [ ] Hardware-accelerated RT pipeline integration
- [ ] Mesh shader geometry pipeline support
- [ ] Variable rate shading for deferred passes
- [ ] Async compute for heavy culling passes
- [ ] MiddleWare GI solution evaluation

---

## Related Documentation

- `MATERIAL_SYSTEM_ARCHITECTURE.md` - Material system details
- `shadow/` - Shadow system specific docs
- `raytracing/` - Ray tracing subsystem details
- `postprocess/` - Post-processing pipeline
