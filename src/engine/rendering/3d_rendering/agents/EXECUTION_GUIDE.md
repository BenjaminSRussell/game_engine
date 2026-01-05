# Rendering Engine Implementation - Execution Guide

## Quick Reference

### Phase Overview
| Phase | Focus | Agents | Est. TODOs |
|-------|-------|--------|------------|
| 1 | Core Infrastructure | 6 | ~600 |
| 2 | Geometry & Mesh | 6 | ~600 |
| 3 | Lighting & Shadows | 6 | ~600 |
| 4 | Materials & Textures | 6 | ~600 |
| 5 | Effects & Post-Processing | 6 | ~600 |
| 6 | Advanced Systems | 6 | ~600 |
| **Total** | | **36** | **~3,600** |

---

## How to Run Agents

### Single Agent Execution
```
Launch agent with Task tool:
- subagent_type: "general-purpose"
- prompt: [Copy prompt from phase file]
- model: "sonnet" (or "opus" for complex tasks)
```

### Parallel Execution (6 agents per phase)
```
Launch all 6 agents in single message:
- All agents in same phase can run in parallel
- Each agent handles ~100 TODOs
- Monitor with TaskOutput tool
```

---

## Phase 1: Core Infrastructure

**Must complete before any other phase**

| Agent | Focus | Directories |
|-------|-------|-------------|
| 1.1 | Device & Memory | core/, memory/, platform/ |
| 1.2 | Command & Sync | core/command/, core/sync/ |
| 1.3 | Resources | resource_management/, bindless/ |
| 1.4 | Render Graph | rendering/render_graph/, framebuffer/ |
| 1.5 | Math & Debug | math/, profiling/, debugging/ |
| 1.6 | Shaders & PBR | shading/brdf/, materials/shaders/ |

**Prompt files:** `agents/phase1_prompts.md`

---

## Phase 2: Geometry & Mesh

**Depends on: Phase 1**

| Agent | Focus | Directories |
|-------|-------|-------------|
| 2.1 | Mesh Data | geometry/mesh/, asset_system/loading/ |
| 2.2 | LOD & Streaming | geometry/lod/, lod_streaming/ |
| 2.3 | Instancing | geometry/instancing/, scene_management/ |
| 2.4 | Meshlets & Nanite | geometry/meshlets/, nanite/cluster/ |
| 2.5 | Culling | culling/, gpu_culling/, occlusion/ |
| 2.6 | Mesh Rendering | static_mesh_rendering/, rendering/ |

**Prompt files:** `agents/phase2_prompts.md`

---

## Phase 3: Lighting & Shadows

**Depends on: Phase 1, 2**

| Agent | Focus | Directories |
|-------|-------|-------------|
| 3.1 | Light Sources | lighting/sources/, lighting/probes/ |
| 3.2 | Shadow Mapping | lighting/shadows/, lighting/cascades/ |
| 3.3 | Virtual Shadows | virtual_shadow_maps/, shadows_advanced/ |
| 3.4 | Basic GI | lighting/global_illumination/, lightmap_baking/ |
| 3.5 | Lumen GI | lumen/ (all subdirs) |
| 3.6 | Volumetric | lighting/volumetric/, atmosphere/ |

**Prompt files:** `agents/phase3_prompts.md`

---

## Phase 4: Materials & Textures

**Depends on: Phase 1, 2, 3**

| Agent | Focus | Directories |
|-------|-------|-------------|
| 4.1 | Material System | materials/material_system/, materials/layering/ |
| 4.2 | Advanced Shading | shading/subsurface/, shading/hair/, shading/cloth/ |
| 4.3 | Texture Streaming | texture/streaming/, texture/virtual_texturing/ |
| 4.4 | Character Materials | skin_rendering/, eye_rendering/, hair_rendering/ |
| 4.5 | Environment Materials | metal_rendering/, wood_rendering/, stone_rendering/ |
| 4.6 | Weathering | age_weathering/, rust_corrosion/, wetness_system/ |

**Prompt files:** `agents/phase4_prompts.md`

---

## Phase 5: Effects & Post-Processing

**Depends on: Phase 1, 2, 3, 4**

| Agent | Focus | Directories |
|-------|-------|-------------|
| 5.1 | Particles | effects/particles/, effects/gpu_particles/ |
| 5.2 | Environmental | effects/weather/, effects/decals/, puddles/ |
| 5.3 | Special Effects | fire_rendering/, smoke_rendering/, explosion_effects/ |
| 5.4 | Tonemapping | postprocessing/tonemapping/, postprocessing/bloom/ |
| 5.5 | Anti-Aliasing | postprocessing/anti_aliasing/, temporal_upscaling/ |
| 5.6 | Screen-Space | postprocessing/screen_space/, postprocessing/depth_of_field/ |

**Prompt files:** `agents/phase5_prompts.md`

---

## Phase 6: Advanced Systems

**Depends on: All previous phases**

| Agent | Focus | Directories |
|-------|-------|-------------|
| 6.1 | Landscape | landscape/, landscape_grass/, vegetation_rendering_advanced/ |
| 6.2 | Water & Ocean | water/, ocean_waves/, planar_reflections/ |
| 6.3 | Ray Tracing | raytracing/ (all subdirs) |
| 6.4 | Nanite & GPU-Driven | nanite/culling/, rendering/gpu_driven/, visibility_buffer/ |
| 6.5 | Animation & Physics | animation/, cloth_system/, hair_system/, destruction/ |
| 6.6 | Editor & Tools | editor/, tools/, ui_rendering/, asset_system/import/ |

**Prompt files:** `agents/phase6_prompts.md`

---

## Example: Launching Phase 1 Agents

```python
# Launch all 6 Phase 1 agents in parallel
agents = [
    Task(prompt=PHASE1_AGENT1_PROMPT, subagent_type="general-purpose"),
    Task(prompt=PHASE1_AGENT2_PROMPT, subagent_type="general-purpose"),
    Task(prompt=PHASE1_AGENT3_PROMPT, subagent_type="general-purpose"),
    Task(prompt=PHASE1_AGENT4_PROMPT, subagent_type="general-purpose"),
    Task(prompt=PHASE1_AGENT5_PROMPT, subagent_type="general-purpose"),
    Task(prompt=PHASE1_AGENT6_PROMPT, subagent_type="general-purpose"),
]

# Wait for all to complete before Phase 2
results = await asyncio.gather(*agents)
```

---

## Tracking Progress

Each agent should:
1. Read existing files in their directories
2. Implement TODOs one by one
3. Mark completed TODOs (change TODO to DONE or IMPLEMENTED)
4. Report progress: "Completed X/100 TODOs in Y files"

---

## Quality Checklist

Before considering a phase complete:
- [ ] All header files have proper guards
- [ ] All functions have proper signatures
- [ ] Memory allocation has matching deallocation
- [ ] Error handling returns proper error codes
- [ ] No compiler warnings
- [ ] Thread safety documented where applicable
- [ ] Public API is documented

---

## Estimated Timeline

With 6 parallel agents per phase:
- Phase 1: Foundation (must be solid)
- Phase 2: Can start after Phase 1 core is stable
- Phase 3-6: Can overlap once dependencies are met

Total: 36 agent runs across 6 phases
