# Rendering Engine Implementation Phases (Metal)

## Overview

This rendering engine is designed exclusively for **macOS using Metal API**. All implementations use Metal's native APIs - no Vulkan, D3D12, or OpenGL code paths.

## Phase Summary

| Phase | Focus | Agent Prompts | Key Systems |
|-------|-------|---------------|-------------|
| **1** | Metal Core | `phase1_metal_core.md` | Device, Commands, Buffers, Pipeline, Sync, Swapchain |
| **2** | Reorganization & Geometry | `phase2_reorganize_and_geometry.md` | Directory structure, Mesh, Vertex, LOD, BVH, Instancing |
| **3** | Metal Rendering | `phase3_metal_rendering.md` | Render Graph, G-buffer, Deferred, Forward, Shadows, Shaders |
| **4** | Materials & Lighting | `phase4_materials_lighting.md` | PBR, Lights, Probes, Volumetric Fog, SSR, Bloom |
| **5** | Effects & Environment | `phase5_effects_environment.md` | Particles, Ocean, Atmosphere, TAA, Decals, Debug |
| **6** | Advanced Integration | `phase6_advanced_integration.md` | Nanite, Animation, Cloth/Hair, Assets, Editor, Testing |

---

## Phase Dependencies

```
Phase 1 (Metal Core)
    ↓
Phase 2 (Reorganize & Geometry)
    ↓
Phase 3 (Metal Rendering)
    ↓
Phase 4 (Materials & Lighting)
    ↓
Phase 5 (Effects & Environment)
    ↓
Phase 6 (Advanced Integration)
```

Phases must be completed in order. Each phase builds on the previous.

---

## Technology Stack

### Metal API Components Used
- `MTLDevice` - GPU device interface
- `MTLCommandQueue` - Command submission
- `MTLCommandBuffer` - Recorded commands
- `MTLRenderCommandEncoder` - Graphics encoding
- `MTLComputeCommandEncoder` - Compute encoding
- `MTLBuffer` - GPU memory buffers
- `MTLTexture` - Texture resources
- `MTLRenderPipelineState` - Graphics pipeline
- `MTLComputePipelineState` - Compute pipeline
- `MTLDepthStencilState` - Depth/stencil config
- `MTLLibrary` - Compiled shaders (.metallib)
- `CAMetalLayer` - Display presentation

### Math Library
- Apple SIMD: `simd_float3`, `simd_float4`, `simd_float4x4`, `simd_quatf`

### Language
- C with Objective-C bridging for Metal calls
- Metal Shading Language for GPU code

---

## Directory Structure (After Phase 2)

```
src/engine/rendering/3d_rendering/
├── backend/
│   └── metal/              # All Metal API code
├── core/
│   ├── command/            # Command buffers, encoding
│   ├── sync/               # Fences, semaphores
│   └── resource/           # Resource management
├── geometry/
│   ├── mesh/               # Mesh data structures
│   ├── vertex/             # Vertex formats
│   ├── lod/                # LOD system
│   ├── bvh/                # Spatial acceleration
│   ├── cluster/            # Mesh clustering
│   └── nanite/             # GPU-driven geometry
├── lighting/
│   ├── direct/             # Lights, shadows
│   ├── gi/                 # Global illumination
│   └── volumetric/         # Fog, volumes
├── materials/
│   ├── pbr/                # PBR system
│   └── instances/          # Material instances
├── shading/
│   ├── brdf/               # BRDF models
│   └── shader_compiler/    # Metal shader compilation
├── rendering/
│   ├── render_graph/       # Frame graph
│   ├── gbuffer/            # Deferred rendering
│   ├── forward/            # Forward path
│   └── shadows/            # Shadow mapping
├── effects/
│   ├── particles/          # GPU particles
│   ├── decals/             # Deferred decals
│   └── weather/            # Weather effects
├── postprocess/
│   ├── taa/                # Temporal AA
│   ├── bloom/              # Bloom effect
│   └── tonemapping/        # HDR tonemapping
├── environment/
│   ├── ocean/              # FFT ocean
│   ├── atmosphere/         # Sky rendering
│   └── terrain/            # Terrain system
├── character/
│   ├── animation/          # Skeletal animation
│   ├── skeleton/           # Bone hierarchy
│   ├── cloth/              # Cloth simulation
│   └── hair/               # Hair rendering
├── assets/
│   ├── import/             # glTF, OBJ importers
│   └── processing/         # Mesh optimization
├── editor/
│   ├── gizmos/             # Transform tools
│   ├── debug/              # Debug rendering
│   └── profiling/          # GPU profiler
├── integration/            # System integration
└── testing/                # Test framework
```

---

## Agent Structure Per Phase

Each phase contains **6 agents**, each handling a specific subsystem:

| Agent | Pattern |
|-------|---------|
| X.1 | Foundation/Core for the phase |
| X.2-X.4 | Main subsystems |
| X.5 | Supporting systems |
| X.6 | Integration/Polish |

Each agent handles approximately **60-100 TODOs** to keep scope manageable.

---

## Running Agents

### Sequential (Recommended)
1. Run all 6 agents in Phase 1
2. Verify Phase 1 complete
3. Run all 6 agents in Phase 2
4. Continue through phases

### Parallel Within Phase
All 6 agents within a single phase can run in parallel since they work on different subsystems.

### Agent Execution
```
Task tool:
- subagent_type: "general-purpose"
- prompt: [Copy from phase markdown file]
- model: "sonnet" (or "opus" for complex systems)
```

---

## Success Criteria

### Per Agent
- All assigned TODOs implemented
- Code compiles without errors
- Functions match header declarations
- Memory properly managed (alloc/free pairs)

### Per Phase
- All 6 agents complete
- Systems integrate correctly
- No circular dependencies
- Phase documentation updated

### Final
- Complete frame renders correctly
- Performance targets met
- All files organized in proper directories
- No Vulkan/D3D12/OpenGL code
- Clean compile on macOS
