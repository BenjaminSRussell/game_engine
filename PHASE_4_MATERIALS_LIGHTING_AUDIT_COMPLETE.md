# Phase 4 Materials, Lighting & Advanced GI - Comprehensive Audit

**Date:** January 7, 2026
**Status:** Complete Implementation Gap Analysis for Phase 4
**Overall Phase 4 Completion:** ~30% (Framework: 85%, Core: 25%, GPU: 10%)
**Action:** Add 6-8 remedial agents to Phase 4

---

## Executive Summary

Phase 4 (Materials, Lighting & Advanced GI) is **30% complete** with excellent architectural frameworks but severe GPU integration and core algorithm gaps preventing Phase 5 progression.

### Current State
- ✅ **~3,000+ lines of code** across materials, lighting, and GI systems
- ✅ **Strong architectural design** (PBR material system, light clustering, probe grids)
- ✅ **Type safety and data structures** well-designed
- ❌ **GPU integration severely lacking** (10-15%)
- ❌ **Core algorithms missing** (PBR shader, light culling, probe baking)
- ❌ **Shader integration absent** (no material sampling in shaders)
- ❌ **Post-processing incomplete** (bloom, tonemapping, TAA missing)

### The Problem
Phase 4 has solid data structures and type definitions but **critical execution gaps**:
- Material instances created but never synced to GPU
- Light clusters allocated but never populated with lights
- Probe grids created but probe baker doesn't exist (0 lines of baking code)
- PBR shader only 50 lines (40+ more needed)
- Volumetric fog structure defined but compute pass missing
- Post-processing pipelines defined but shaders missing

### The Solution
Add **6-8 remedial agents to Phase 4** addressing critical gaps:
- Agent 4.7: PBR Shader & Material GPU Sync (~120 TODOs)
- Agent 4.8: Light Culling & Clustering (~150 TODOs)
- Agent 4.9: Irradiance Probe Baking Engine (~200 TODOs)
- Agent 4.10: Volumetric Lighting & Froxels (~130 TODOs)
- Agent 4.11: Screen-Space Reflections (~100 TODOs)
- Agent 4.12: Post-Processing Pipelines (~140 TODOs)

**Total Phase 4 expansion: 600 → 1,340 TODOs**

---

## Phase 4 Detailed Status by Agent

### Agent 4.1: PBR Material System

**Planned (100 TODOs):** Complete PBR material system with GPU integration

**Actual Status:** 35-40% complete

**What's Done:**
- Material instance creation/destruction (321 lines)
- Parameter management framework (193 lines)
- Dynamic materials system (290 lines)
- PBR parameter validation
- Shader compilation framework
- Special material types (glass, water, skin, hair) - structure only
- Material layering system
- Texture binding framework

**What's Missing:**
- PBR.metal shader incomplete (50 lines only) - Missing full lighting model
- Material constant buffer sync not implemented (buffer_update() commented out)
- No shader parameter binding to pipelines
- No texture descriptor sets created
- No sampler binding to pipelines
- Material variants not generated
- Hot reload not integrated

**Critical Issues:**
- `material_instance.c` line 283: `// TODO: buffer_update(mat->gpu_buffer);` - DISABLED
- pbr.metal only contains vertex shader and 50 lines of fragment shader
- No normal mapping implementation
- No roughness/metallic evaluation in shader
- No light integration in material shader

**Impact:** BLOCKS ALL MATERIAL RENDERING - No way to shade pixels with material properties

**TODOs Remaining:** ~200 (shader completion, GPU sync, binding, variants)

---

### Agent 4.2: Clustered Light Management

**Planned (100 TODOs):** Complete light culling and clustering system

**Actual Status:** 30-35% complete

**What's Done:**
- Light creation functions for all types (point, directional, spot, rect, sphere)
- Cluster grid definition (16x9x24 = 3,456 clusters)
- AABB-sphere intersection testing
- Cluster frustum computation
- Cascade shadow framework
- CSM optimization system

**What's Missing:**
- Light iteration loop completely stubbed - `light_culling_assign_lights()` is empty (~line 104)
- GPU light grid never populated with light data
- No shader light sampling loop
- No per-cluster light lists generated
- Cluster bounds not computed for spatial queries
- No light assignment algorithm

**Critical Issues:**
- `light_culling_assign_lights()` function returns 0 without doing anything
- Light grid buffers allocated but never filled
- Forward shader cannot access clustered lights
- Deferred shader has no light loop

**Impact:** BLOCKS ALL DYNAMIC LIGHTING - Lights are defined but don't affect rendering

**TODOs Remaining:** ~220 (light iteration, GPU population, shader integration)

---

### Agent 4.3: Irradiance Probes & Global Illumination

**Planned (100 TODOs):** Complete irradiance probe system with baking

**Actual Status:** 15-20% complete

**What's Done:**
- Probe data structure with 9 SH (spherical harmonics) coefficients
- Probe grid allocation framework
- Probe blending infrastructure
- Metal buffer setup for probes
- Probe placement visualization framework

**What's Missing:**
- **ZERO probe baking engine implementation** (0 lines of code)
- No ray tracing for probe baking
- No SH projection from ray results
- No temporal filtering for probe updates
- Probes never sampled in shaders
- No probe grid population
- No cascaded probe updates
- No probe validity tracking

**Critical Files:**
- `irradiance_probe.c` - Structure definitions only (~200 lines, mostly declarations)
- **NO FILE:** `probe_baker.c` - Completely missing (should be 1,500+ lines)
- No shader integration in pbr.metal

**Critical Issues:**
- Probe grid allocated but 100% empty (all zeros = black GI)
- No probe baker exists at all
- pbr.metal has no probe sampling code
- Render graph has no probe baking pass

**Impact:** BLOCKS GI ENTIRELY - Global illumination always returns black

**TODOs Remaining:** ~280 (probe baker ~200, shader integration ~80)

---

### Agent 4.4: Volumetric Fog & Froxels

**Planned (100 TODOs):** Complete volumetric fog with froxel lighting

**Actual Status:** 10-15% complete

**What's Done:**
- Froxel grid structure defined (16x9x24 matching cluster grid)
- Fog volume container structures
- Temporal reprojection framework
- Fog parameter validation

**What's Missing:**
- No froxel traversal code
- No compute shader for light accumulation per froxel
- No phase function evaluation (Henyey-Greenstein)
- No transmittance calculation
- No inscattering computation
- No scattering coefficient sampling
- No temporal reprojection implementation
- No noise texture generation

**Critical Issues:**
- `volumetric_fog.c` has 15+ unimplemented functions
- No compute pipeline created for froxel lighting
- No shader code for fog accumulation
- No transmittance buffer management

**Impact:** BLOCKS VOLUMETRIC EFFECTS - No fog rendering possible

**TODOs Remaining:** ~180 (compute pass, shaders, reprojection, noise)

---

### Agent 4.5: Screen-Space Reflections

**Planned (100 TODOs):** Complete screen-space reflection rendering

**Actual Status:** 10-15% complete

**What's Done:**
- Ray generation framework sketched
- HZB (Hierarchical Z-Buffer) building logic outlined
- Basic reflection data structure

**What's Missing:**
- No ray-plane intersection code
- No hierarchical traversal implementation
- No Z-buffer pyramid construction
- No denoising/edge preservation
- No temporal reprojection
- No edge detection
- No bilateral filtering
- No ray tracing shader

**Critical Issues:**
- No compute shader for SSR
- HZB generation not scheduled in render graph
- No depth mipmap chain creation
- No roughness-based falloff

**Impact:** BLOCKS SCREEN-SPACE REFLECTIONS - No SSR possible

**TODOs Remaining:** ~140 (ray tracing, HZB, filtering, integration)

---

### Agent 4.6: Post-Processing (Bloom & Tonemapping)

**Planned (100 TODOs):** Complete post-processing pipeline

**Actual Status:** 20-30% complete

**What's Done:**
- Bloom pipeline structure (threshold, downsample, upsample framework)
- ACES tonemapping curves (math implemented, ~100 lines)
- Exposure adaptation state management (~100 lines)
- TAA framework defined
- LUT system created

**What's Missing:**
- **NO bloom render passes** (threshold, downsample, upsample shaders missing)
- **NO tonemapping compute pass** (ACES curves exist in C but no GPU dispatch)
- **NO exposure metering** (histogram not computed)
- **NO TAA implementation** (velocity resolve shader missing)
- No bloom upsampling filters
- No tone curve LUT generation
- No exposure feedback loop
- No render graph integration for all passes

**Critical Issues:**
- 5+ pass structures defined but no shaders
- Bloom upsampling weights calculated but not used
- Exposure adaptation has no feedback
- TAA shader exists but not dispatched

**Impact:** BLOCKS POST-PROCESSING - No effects rendering

**TODOs Remaining:** ~240 (shaders ~150, exposure ~40, integration ~50)

---

## Critical Blockers Summary - Phase 4

### P0 - MUST FIX (Blocks Phase 5)

1. **PBR Shader Implementation** (Agent 4.7)
   - pbr.metal only 50 lines
   - Missing: full lighting model, normal mapping, light sampling
   - Impact: Nothing renders with correct materials
   - Effort: 200 TODOs, ~1,500-2,000 lines

2. **Material GPU Synchronization** (Agent 4.7)
   - buffer_update() disabled
   - Impact: Material parameters don't reach GPU
   - Effort: 50 TODOs, ~300-400 lines

3. **Light Culling Implementation** (Agent 4.8)
   - light_culling_assign_lights() completely stubbed
   - Impact: Lights defined but don't affect pixels
   - Effort: 150 TODOs, ~1,200-1,500 lines

4. **Probe Baking Engine** (Agent 4.9)
   - 0 lines of baking code exist
   - Impact: GI always black
   - Effort: 200 TODOs, ~2,000-2,500 lines (HIGHEST COMPLEXITY)

5. **Post-Processing Shaders** (Agent 4.12)
   - 5+ passes with no GPU code
   - Impact: No bloom, no tone mapping, no TAA
   - Effort: 140 TODOs, ~1,000-1,200 lines

### P1 - CRITICAL (Blocks full functionality)

1. **Volumetric Fog Compute** (Agent 4.10)
   - No froxel lighting pass
   - Impact: No volumetric effects
   - Effort: 130 TODOs, ~1,000-1,200 lines

2. **Screen-Space Reflections** (Agent 4.11)
   - No ray tracing code
   - Impact: No SSR
   - Effort: 100 TODOs, ~800-1,000 lines

---

## Phase 4 Remedial Solution

### Proposed New Agents (4.7-4.12)

**Total New TODOs:** ~940 (expanding Phase 4 from 600 → 1,540)
**Estimated Timeline:** 12-16 weeks
**Target:** 85%+ Phase 4 completion

#### Agent 4.7: PBR Shader & Material GPU Sync (~120 TODOs)
- Complete pbr.metal fragment shader (1,200+ lines)
- Implement material GPU buffer synchronization
- Material parameter binding to pipelines
- Texture descriptor set creation
- Sampler binding implementation

#### Agent 4.8: Light Culling & Clustering (~150 TODOs)
- Light iteration and assignment algorithm
- GPU light grid population
- Cluster light list generation
- Forward shader light sampling
- Deferred shader light accumulation

#### Agent 4.9: Irradiance Probe Baking Engine (~200 TODOs)
- Complete probe baker implementation (2,000+ lines)
- Ray tracing for probe contribution
- SH projection from rays
- Temporal probe filtering
- Probe grid GPU population
- Shader probe sampling

#### Agent 4.10: Volumetric Lighting & Froxels (~130 TODOs)
- Froxel lighting compute pass
- Light accumulation per froxel
- Phase function evaluation
- Transmittance calculation
- Temporal reprojection
- Integration with forward/deferred passes

#### Agent 4.11: Screen-Space Reflections (~100 TODOs)
- HZB pyramid generation
- Ray tracing shader
- Denoising filters
- Temporal reprojection
- Render graph integration

#### Agent 4.12: Post-Processing Pipelines (~140 TODOs)
- Bloom shaders (threshold, downsample, upsample)
- Tonemapping compute pass
- Exposure metering histogram
- TAA reprojection shader
- Render graph passes for all effects

---

## Phase 4 Implementation Strategy

### Critical Path (12-16 weeks)

```
Week 1-2:  Agent 4.7 - PBR Shader (120 TODOs)
           └─ Unblocks: Basic material rendering

Week 2-3:  Agent 4.8 - Light Culling (150 TODOs) [parallel to 4.7]
           └─ Unblocks: Dynamic lighting

Week 3-5:  Agent 4.9 - Probe Baking (200 TODOs)
           └─ Unblocks: Global illumination

Week 5-6:  Agent 4.10 - Volumetric Fog (130 TODOs)
           └─ Unblocks: Atmospheric effects

Week 6-7:  Agent 4.11 - SSR (100 TODOs)
           └─ Unblocks: Screen-space reflections

Week 7-8:  Agent 4.12 - Post-Processing (140 TODOs)
           └─ Unblocks: Bloom, tonemapping, TAA

TOTAL: 12-16 weeks, ~940 additional TODOs
```

---

## Success Metrics for Phase 4

After completion of all 12 agents (original 6 + new 6):

**Functional Requirements:**
- ✅ PBR materials render with correct lighting
- ✅ Dynamic lights illuminate geometry
- ✅ Light clustering works efficiently
- ✅ Global illumination contributes to shading
- ✅ Irradiance probes bake and update
- ✅ Volumetric fog renders correctly
- ✅ Screen-space reflections work
- ✅ Bloom, tonemapping, TAA all functional
- ✅ All post-processing renders

**Code Quality Requirements:**
- ✅ All 1,540 Phase 4 TODOs implemented
- ✅ Zero stub functions remaining
- ✅ All shaders compiled and working
- ✅ Proper GPU memory management
- ✅ All code compiles without warnings

**Performance Requirements:**
- ✅ PBR shading < 2ms
- ✅ Light culling < 1ms
- ✅ Probe baking < 50ms (interactive)
- ✅ Volumetric fog < 3ms for 256³ froxel grid
- ✅ SSR < 2ms
- ✅ Post-processing < 5ms total

---

## Files to Create/Modify

### Agent 4.7 (PBR Shader)
- pbr.metal (~1,200 lines completion)
- material_instance.c (~300 lines completion)
- material_gpu_buffer.c (~200 lines new)

### Agent 4.8 (Light Culling)
- light_clustering.c (~600 lines)
- light_culling_compute.c (~400 lines new)
- forward_lighting.c light loop (~200 lines)

### Agent 4.9 (Probe Baking)
- probe_baker.c (~2,000 lines NEW)
- irradiance_probe.c (~300 lines completion)
- probe_sampling.metal (~300 lines)

### Agent 4.10 (Volumetric Fog)
- froxel_accumulation.compute (~400 lines)
- volumetric_fog_compute.c (~400 lines)
- volumetric_integration.c (~300 lines)

### Agent 4.11 (SSR)
- ssr_ray_trace.compute (~400 lines)
- hzb_builder.c (~300 lines)
- ssr_denoise.compute (~200 lines)

### Agent 4.12 (Post-Processing)
- bloom_passes.compute (~400 lines)
- tonemapping_pass.compute (~200 lines)
- taa_resolve.compute (~150 lines)

---

## Conclusion

Phase 4 has strong foundational architecture (30% complete) but **critical GPU implementation gaps** prevent Phase 5-6 progression. The 6 remedial agents (4.7-4.12) directly address these gaps with:

- Clear, measurable TODOs
- Well-defined success criteria
- Realistic implementation estimates
- Critical path for efficient development

With focused effort over 12-16 weeks, Phase 4 can reach 85%+ completion and unblock Phase 5 (Effects & Environment).

---

**Audit Date:** January 7, 2026
**Status:** Ready for Review and Implementation
**Next Step:** Create phase4_lights_materials_updated.md with all 12 agents
