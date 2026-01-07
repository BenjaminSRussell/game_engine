# Metal Rendering Engine - Complete Implementation Guide (6 Phases)

**Status:** Production-Grade 3D Rendering for macOS
**Target:** Better than Unreal Engine & Unity
**Platform:** macOS (Metal API only)
**Total Scope:** 36 agents × ~100 TODOs = ~3,600 implementation tasks

---

## Quick Phase Overview

| Phase | Title | Agents | Focus | Deliverables |
|-------|-------|--------|-------|--------------|
| **1** | Metal Core | 6 | Device, Commands, Buffers, Pipelines, Sync, Display | Complete GPU API wrapper |
| **2** | Geometry & Reorganization | 6 | Directory structure, Mesh, Vertex, LOD, BVH, Instancing | Organized codebase + geometry foundation |
| **3** | Rendering Pipeline | 6 | Render Graph, G-Buffer, Deferred, Forward, Shadows, Shaders | Complete rendering path |
| **4** | Materials & Lighting | 6 | PBR, Lights, GI Probes, Volumetrics, SSR, Bloom | Full shading system |
| **5** | Effects & Environment | 6 | Particles, Ocean, Sky, TAA, Decals, Profiling | Environment complete |
| **6** | Advanced Integration | 6 | GPU-Driven, Animation, Cloth/Hair, Assets, Editor, Testing | Complete engine |

---

## Phase 1: Metal Core Infrastructure

**6 Agents, ~600 TODOs**

### 1.1 Metal Device & Capabilities
- Device creation/lifecycle
- Command queue management
- Capability detection (25+ features)
- Memory heap management
- Device state tracking
- [See: phase1_metal_core.md](phase1_metal_core.md)

### 1.2 Command Buffers & Encoding
- Command buffer lifecycle
- Render encoding (30+ tasks)
- Compute encoding
- Blit operations
- Submission & completion
- [See: phase1_metal_core.md](phase1_metal_core.md)

### 1.3 Buffers, Textures & Resources
- Buffer creation (all storage modes)
- Texture types (2D, 3D, Cube, Array, etc.)
- Data transfers & DMA
- Resource pooling
- Memory management
- [See: phase1_metal_core.md](phase1_metal_core.md)

### 1.4 Pipeline State Objects
- Render pipeline creation
- Compute pipeline creation
- Depth/stencil state
- Shader library management
- Pipeline caching
- [See: phase1_metal_core.md](phase1_metal_core.md)

### 1.5 Synchronization & Frame Management
- Frame synchronization
- GPU-CPU sync (shared events)
- Memory barriers
- GPU timestamps
- Frame pacing
- [See: phase1_metal_core.md](phase1_metal_core.md)

### 1.6 Swapchain & Display Output
- CAMetalLayer integration
- Drawable management
- HDR output (EDR, P3)
- Variable refresh rate
- Window management
- [See: phase1_metal_core.md](phase1_metal_core.md)

**Dependencies:** None (foundation)
**Next:** Phase 2

---

## Phase 2: Geometry & Directory Reorganization

**6 Agents, ~600 TODOs**

### 2.1 Directory Reorganization
- Reorganize 154 flat dirs → ~20 logical subsystems
- Create core, backend, geometry, lighting, materials, rendering hierarchies
- Update includes & CMake
- Verify build integrity
- [See: phase2_geometry_reorganization.md](phase2_geometry_reorganization.md)

### 2.2 Mesh System Core
- Mesh data structures
- Mesh creation/destruction
- Bounds calculations
- Mesh deformation setup
- Material binding
- Mesh optimization
- [See: phase2_geometry_reorganization.md](phase2_geometry_reorganization.md)

### 2.3 Vertex Formats & Attributes
- Flexible vertex format system
- Standard formats (P3N3, P3N3U2, full PBR)
- GPU vertex binding
- Attribute compression
- Format reflection
- [See: phase2_geometry_reorganization.md](phase2_geometry_reorganization.md)

### 2.4 LOD System
- LOD generation (0-4 variants)
- Screen-space error selection
- Crossfade transitions
- Memory budgeting
- GPU LOD selection
- [See: phase2_geometry_reorganization.md](phase2_geometry_reorganization.md)

### 2.5 Spatial Acceleration & BVH
- BVH construction
- BVH traversal
- GPU-based frustum culling
- Incremental updates
- [See: phase2_geometry_reorganization.md](phase2_geometry_reorganization.md)

### 2.6 Instancing & GPU-Driven Rendering
- Instance data management
- Indirect rendering
- GPU-driven batching
- Performance optimization
- [See: phase2_geometry_reorganization.md](phase2_geometry_reorganization.md)

**Dependencies:** Phase 1
**Next:** Phase 3

---

## Phase 3: Metal Rendering Pipeline

**6 Agents, ~600 TODOs**

### 3.1 Render Graph & Frame Graph
- Render pass graph
- Resource aliasing
- Barrier insertion
- Compilation & execution
- Debugging tools
- [See: phase3_rendering_pipeline.md](phase3_rendering_pipeline.md)

### 3.2 G-Buffer & Deferred Setup
- Multi-render target setup (Albedo, Normal, Material, Depth, Velocity, Emissive)
- Normal encoding/decoding
- Format optimization
- Resize handling
- [See: phase3_rendering_pipeline.md](phase3_rendering_pipeline.md)

### 3.3 Deferred Lighting Pass
- Clustered light culling (3D grid)
- PBR BRDF lighting
- Light types (directional, point, spot, area)
- Shadow integration
- [See: phase3_rendering_pipeline.md](phase3_rendering_pipeline.md)

### 3.4 Forward Rendering & Transparency
- Forward opaque pass
- Transparency/alpha blending
- Refraction & distortion
- Special materials
- Performance optimization
- [See: phase3_rendering_pipeline.md](phase3_rendering_pipeline.md)

### 3.5 Cascaded Shadow Mapping
- Shadow atlas setup
- Cascade configuration
- Depth rendering
- PCF filtering
- Optimization
- [See: phase3_rendering_pipeline.md](phase3_rendering_pipeline.md)

### 3.6 Shader Compilation & Hot Reload
- .metallib loading
- Function extraction & reflection
- Pipeline creation
- Hot reload detection
- Shader variants
- [See: phase3_rendering_pipeline.md](phase3_rendering_pipeline.md)

**Dependencies:** Phase 1, 2
**Next:** Phase 4

---

## Phase 4: Materials, Lighting & Advanced GI

**6 Agents, ~600 TODOs**

### 4.1 PBR Material System
- Material definition (albedo, metallic, roughness, AO)
- Material instances
- Shader variant generation
- Material library
- Runtime updates
- Specialized materials
- [See: phase4_lights_materials.md](phase4_lights_materials.md)

### 4.2 Clustered Light Management
- Light creation (directional, point, spot)
- Culling system (3D grid)
- Light parameters
- Dynamic lighting updates
- Performance optimization
- [See: phase4_lights_materials.md](phase4_lights_materials.md)

### 4.3 Irradiance Probes & Global Illumination
- Probe grid setup
- Probe baking
- Runtime sampling
- Indirect lighting
- Optimization
- [See: phase4_lights_materials.md](phase4_lights_materials.md)

### 4.4 Volumetric Fog & Froxels
- Froxel grid setup
- Volume definition
- Volumetric rendering
- Lighting integration
- Optimization
- [See: phase4_lights_materials.md](phase4_lights_materials.md)

### 4.5 Screen-Space Reflections
- SSR ray tracing
- Reflection sampling
- Temporal reprojection
- Denoising
- Integration
- [See: phase4_lights_materials.md](phase4_lights_materials.md)

### 4.6 Post-Processing: Bloom & Tonemapping
- Bloom effect (13-tap filtering)
- ACES tone mapping
- Auto exposure
- Color grading LUTs
- Optimization
- [See: phase4_lights_materials.md](phase4_lights_materials.md)

**Dependencies:** Phase 1, 2, 3
**Next:** Phase 5

---

## Phase 5: Effects, Environment & Post-Processing

**6 Agents, ~600 TODOs**

### 5.1 GPU Particle System
- Particle pool management
- Emission
- GPU simulation (compute)
- Rendering (billboards, instanced)
- Optimization
- [See: phase5_effects.md](phase5_effects.md)

### 5.2 FFT Ocean Simulation
- Phillips spectrum
- FFT computation
- Wave displacement
- Ocean rendering
- Optimization
- [See: phase5_effects.md](phase5_effects.md)

### 5.3 Atmospheric Scattering & Sky
- Rayleigh/Mie scattering
- LUT precomputation
- Sky rendering (sun, clouds, stars)
- Aerial perspective
- Optimization
- [See: phase5_effects.md](phase5_effects.md)

### 5.4 Temporal Anti-Aliasing
- Halton jitter
- Velocity buffers
- Reprojection
- Temporal filtering
- Optimization
- [See: phase5_effects.md](phase5_effects.md)

### 5.5 Decal System
- Decal structures
- Projection
- Blending (add, multiply, normal blending)
- Optimization
- Visualization
- [See: phase5_effects.md](phase5_effects.md)

### 5.6 Debug Visualization & Profiling
- Debug line/shape rendering
- Visualization systems
- GPU profiling
- CPU-GPU timing
- Performance analysis
- [See: phase5_effects.md](phase5_effects.md)

**Dependencies:** Phase 1, 2, 3, 4
**Next:** Phase 6

---

## Phase 6: Advanced Systems & Integration

**File:** phase6_advanced_integration.md (from context)

**6 Agents, ~600 TODOs**

### 6.1 GPU-Driven Rendering (Nanite-Style)
- Cluster-based LOD
- Software rasterization
- Visibility buffer
- Persistent culling

### 6.2 Skeletal Animation System
- Skeleton data
- Animation clips
- GPU skinning
- Animation blending

### 6.3 Cloth & Hair Simulation
- Cloth physics
- Constraints
- Hair strands
- Rendering

### 6.4 Asset Import Pipeline
- glTF 2.0 import
- OBJ import
- Texture import
- Mesh optimization
- Asset caching

### 6.5 Editor Tools & Debug
- Transform gizmos
- Object picking
- Debug visualization
- Grid rendering
- Profiler overlay

### 6.6 Integration & Testing
- Full frame rendering
- System integration
- Unit tests
- Visual tests
- Benchmarks

**Dependencies:** Phase 1, 2, 3, 4, 5
**Completion:** Full engine ready

---

## Agent Execution Pattern

Each phase has 6 agents, each with ~100 TODOs:

```
Phase X, Agent X.1 (100 TODOs)
Phase X, Agent X.2 (100 TODOs)
Phase X, Agent X.3 (100 TODOs)
Phase X, Agent X.4 (100 TODOs)
Phase X, Agent X.5 (100 TODOs)
Phase X, Agent X.6 (100 TODOs)
───────────────────────
Total: 600 TODOs per phase
```

Agents **within a phase can run in parallel** (different subsystems).
Agents **across phases must be sequential** (dependencies).

---

## Success Metrics

**Per Agent:**
- ✓ ~100 TODOs implemented
- ✓ Code compiles without warnings
- ✓ All functions match headers
- ✓ No memory leaks
- ✓ Thread-safe where needed

**Per Phase:**
- ✓ All 6 agents complete
- ✓ Systems integrate
- ✓ No circular dependencies
- ✓ Build succeeds
- ✓ Basic tests pass

**Final Engine:**
- ✓ 3,600+ TODOs implemented
- ✓ Production-quality code
- ✓ Rivals Unreal/Unity features
- ✓ macOS Metal native
- ✓ High performance
- ✓ Fully documented

---

## File Reference

```
agents/
├── IMPLEMENTATION_PHASES.md ← You are here
├── EXECUTION_GUIDE.md
├── phase1_metal_core.md          (6 agents, 600 TODOs)
├── phase2_geometry_reorganization.md (6 agents, 600 TODOs)
├── phase3_rendering_pipeline.md  (6 agents, 600 TODOs)
├── phase4_lights_materials.md    (6 agents, 600 TODOs)
├── phase5_effects.md             (6 agents, 600 TODOs)
└── phase6_advanced_integration.md (exists from context, 6 agents, 600 TODOs)

Total: 36 agents, ~3,600 TODOs
```

---

## Next Steps

1. Review this plan
2. Start Phase 1 (6 agents can run in parallel)
3. Verify Phase 1 complete before Phase 2
4. Continue through phases
5. Final integration testing
6. Performance optimization

**Estimated Completion:** ~36 agent runs (can overlap phases)

