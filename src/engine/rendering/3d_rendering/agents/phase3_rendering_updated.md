# Phase 3: Metal Rendering Pipeline (100+ Tasks Per Agent)

This phase establishes complete rendering system with render graph, G-buffer, deferred/forward lighting, and shadows. Each agent handles ~100-200 TODOs ensuring production-quality implementation.

---

## Agent 3.1: Render Graph & Frame Graph (~100 TODOs)

### Objective
Complete render graph framework with proper compilation and execution.

### Current Status: 35-40% Complete

**What's Done:**
- Frame graph core structure (1,248 lines)
- Render pass node system (1,974 lines)
- Dependency tracking
- Resource allocation API

**What's Missing:**
- Graph compilation implementation
- Execution scheduling
- Barrier insertion logic
- GPU command hookup

### Core Implementation Tasks

**Graph Compilation (40 TODOs)**
- [ ] Implement rg_compile() function
- [ ] Dependency analysis
- [ ] Topological sort algorithm
- [ ] Resource lifetime calculation
- [ ] Aliasing analysis
- [ ] Memory budget check
- [ ] Optimization pass
- [ ] Validation

**Execution Scheduling (30 TODOs)**
- [ ] Pass ordering algorithm
- [ ] Resource synchronization points
- [ ] Barrier insertion
- [ ] GPU command scheduling
- [ ] Dependency resolution
- [ ] Latency optimization

**GPU Integration (20 TODOs)**
- [ ] Metal command generation
- [ ] Encoder creation per pass
- [ ] Resource binding setup
- [ ] Error handling

**Remaining Work (10 TODOs)**
- [ ] Statistics collection
- [ ] Debugging visualization
- [ ] Performance profiling

### Success Criteria
- [ ] Graph compilation produces correct execution order
- [ ] Resources properly synchronized
- [ ] Metal commands generated correctly
- [ ] No resource hazards

---

## Agent 3.2: G-Buffer & Deferred Rendering Setup (~100 TODOs)

### Objective
Complete G-buffer rendering with material binding and deferred setup.

### Current Status: 45-50% Complete

**What's Done:**
- G-buffer texture layout (240 lines)
- Render pass descriptor
- Texture creation
- Initialization code

**What's Missing:**
- Rendering loop implementation
- Material parameter binding
- Normal encoding
- Velocity calculation
- Resolve pass
- Decal integration

### Core Implementation Tasks

**G-Buffer Rendering (40 TODOs)**
- [ ] Implement gbuffer_pass rendering loop
- [ ] Material parameter binding (albedo, normal, etc.)
- [ ] Vertex/fragment shader integration
- [ ] Render target setup
- [ ] Clearing and initialization
- [ ] Frame integration
- [ ] Command encoding

**Normal Encoding (20 TODOs)**
- [ ] Oct-tree encoding implementation
- [ ] Normal packing to RG16F
- [ ] Sphere map encoding
- [ ] Decoding in deferred shader
- [ ] Validation and testing

**Velocity Buffer (15 TODOs)**
- [ ] Velocity vector calculation
- [ ] Motion vector storage
- [ ] Frame reprojection support
- [ ] Temporal filtering setup

**Resolve Pass (15 TODOs)**
- [ ] Implement gbuffer_resolve
- [ ] Tile-based optimization
- [ ] Cache optimization
- [ ] Performance tuning

**Decal Integration (10 TODOs)**
- [ ] Decal blending in G-buffer
- [ ] Material override
- [ ] Deferred decal rendering

### Success Criteria
- [ ] Geometry renders to G-buffer correctly
- [ ] All textures contain valid data
- [ ] Normals properly encoded/decoded
- [ ] Ready for deferred lighting pass

---

## Agent 3.3: Deferred Lighting Pass (~100 TODOs)

### Objective
Complete deferred lighting with PBR BRDF and light integration.

### Current Status: 40-45% Complete

**What's Done:**
- Deferred pipeline structure (236 lines)
- Light buffer setup
- Shader framework (146 lines)
- BRDF partial implementation

**What's Missing:**
- Complete shader implementation (60% missing)
- Light culling compute
- Shadow sampling
- Light accumulation

### Core Implementation Tasks

**Shader Completion (50 TODOs)**
- [ ] G-buffer texture sampling
- [ ] World position reconstruction
- [ ] Normal reconstruction from encoding
- [ ] Complete GGX BRDF implementation
- [ ] Fresnel calculation
- [ ] Visibility function
- [ ] Light accumulation loop
- [ ] Dynamic range handling
- [ ] Performance optimization

**Light Integration (25 TODOs)**
- [ ] Light data binding
- [ ] Per-light contribution
- [ ] Attenuation calculation
- [ ] Distance falloff

**Shadow Integration (15 TODOs)**
- [ ] Shadow map sampling
- [ ] Cascade selection
- [ ] PCF filtering
- [ ] Bias calculation

**Clustering/Culling (10 TODOs)**
- [ ] Light cluster setup
- [ ] GPU culling compute
- [ ] Light list generation
- [ ] Optimization

### Success Criteria
- [ ] Deferred shader fully functional
- [ ] Correct PBR lighting
- [ ] Efficient light sampling
- [ ] Proper shadowing

---

## Agent 3.4: Forward Rendering & Transparency (~100 TODOs)

### Objective
Complete forward rendering with transparency support.

### Current Status: 40% Complete

**What's Done:**
- Forward lighting framework (1,272 lines)
- Pipeline creation
- Shader binding
- Blend mode setup

**What's Missing:**
- Rendering execution loop
- Transparent sorting
- Alpha blending
- OIT implementation

### Core Implementation Tasks

**Rendering Loop (40 TODOs)**
- [ ] Implement forward_pass_execute()
- [ ] Material binding
- [ ] Light setup
- [ ] Command encoding
- [ ] State caching
- [ ] Batch optimization

**Transparency Handling (30 TODOs)**
- [ ] Transparent geometry sorting
- [ ] Depth sorting
- [ ] Alpha blending
- [ ] Back-to-front ordering
- [ ] OIT framework (weighted blended)
- [ ] Transparency accumulation

**Advanced Materials (20 TODOs)**
- [ ] Glass shader completion
- [ ] Refraction implementation
- [ ] Water shader completion
- [ ] Parallax mapping
- [ ] Parallax occlusion mapping

**Optimization (10 TODOs)**
- [ ] Draw call batching
- [ ] State change reduction
- [ ] Material sorting
- [ ] Cache utilization

### Success Criteria
- [ ] Opaque geometry renders correctly
- [ ] Transparent geometry sorted properly
- [ ] Alpha blending works
- [ ] Advanced materials functional

---

## Agent 3.5: Cascaded Shadow Mapping (~100 TODOs)

### Objective
Complete shadow mapping with filtering and optimization.

### Current Status: 55-60% Complete (BEST COMPLETION)

**What's Done:**
- CSM manager (70% complete)
- Cascade configuration
- Shadow atlas
- PCF filtering (85% complete)
- Cascade blending (80% complete)
- Stabilization (85% complete)

**What's Missing:**
- Shadow pass execution
- Cascade update logic
- Temporal reprojection
- Advanced techniques

### Core Implementation Tasks

**Shadow Pass Execution (25 TODOs)**
- [ ] Implement shadow_pass rendering
- [ ] Depth-only rendering
- [ ] Back-face rendering
- [ ] Polygon offset setup
- [ ] Light space transformation

**Cascade Management (20 TODOs)**
- [ ] Dynamic cascade updates
- [ ] Partial update system
- [ ] Cascade refinement
- [ ] Scene-adaptive cascades

**Temporal Techniques (20 TODOs)**
- [ ] Temporal reprojection
- [ ] History buffering
- [ ] Artifact prevention
- [ ] Stability improvement

**Advanced Filtering (20 TODOs)**
- [ ] Contact hardening (PCSS)
- [ ] Percentage closer soft shadows
- [ ] Variable penumbra sizing
- [ ] Performance optimization

**Integration (15 TODOs)**
- [ ] Cascade selection in lighting
- [ ] Shadow sampling callbacks
- [ ] Multiple light support
- [ ] Error handling

### Success Criteria
- [ ] Shadows render and filter correctly
- [ ] Cascades transition smoothly
- [ ] No shadow artifacts
- [ ] Performance optimized

---

## Agent 3.6: Shader Compilation & Hot Reload (~100 TODOs)

### Objective
Complete shader system with variants, hot reload, and asset support.

### Current Status: 50-55% Complete

**What's Done:**
- Shader compiler framework
- Library loading
- Function extraction
- Hot reload detection

**What's Missing:**
- Variant permutation generation
- Hot reload execution
- Function reflection
- Asset conversion

### Core Implementation Tasks

**Shader Variants (35 TODOs)**
- [ ] Permutation generation
- [ ] Specialization constant handling
- [ ] Variant caching
- [ ] Cache management
- [ ] Memory optimization

**Hot Reload (25 TODOs)**
- [ ] File watching implementation
- [ ] Compilation triggering
- [ ] Pipeline invalidation
- [ ] Cache clearing
- [ ] Error handling

**Function Reflection (20 TODOs)**
- [ ] Buffer binding introspection
- [ ] Texture binding introspection
- [ ] Sampler binding introspection
- [ ] Parameter validation
- [ ] Mismatch detection

**Asset Shaders (20 TODOs)**
- [ ] GLSL to Metal conversion tools
- [ ] Shader asset pipeline
- [ ] Format compatibility
- [ ] Error reporting

### Success Criteria
- [ ] Shader variants generate automatically
- [ ] Hot reload works for development
- [ ] Function reflection complete
- [ ] Assets convert properly

---

# REMEDIAL AGENTS (Phase 3 Continued)

These agents address critical GPU integration gaps.

---

## Agent 3.7: Render Graph Compilation & Execution (~200 TODOs)

### Objective
Implement complete graph compilation and execution system.

### Critical Implementation Tasks

**Compilation Algorithm (80 TODOs)**
- [ ] rg_compile() main function
- [ ] Dependency graph analysis
- [ ] Topological sort implementation (Kahn's algorithm)
- [ ] Resource lifetime analysis
- [ ] Memory aliasing detection
- [ ] Cycle detection and error handling
- [ ] Optimization passes
- [ ] Validation against Metal constraints

**Execution Scheduling (60 TODOs)**
- [ ] Pass ordering algorithm
- [ ] Dependency resolution
- [ ] Barrier insertion at hazard points
- [ ] GPU synchronization points
- [ ] Resource transition scheduling
- [ ] Latency optimization

**GPU Command Generation (40 TODOs)**
- [ ] Metal command buffer setup
- [ ] Encoder creation per pass
- [ ] Resource binding
- [ ] Descriptor set management
- [ ] Error recovery

**Profiling & Validation (20 TODOs)**
- [ ] Execution statistics
- [ ] Performance metrics
- [ ] Correctness validation
- [ ] Debugging aids

### Success Criteria
- [ ] Graph compilation correct and fast
- [ ] Proper resource synchronization
- [ ] Metal commands generated correctly
- [ ] No resource hazards

---

## Agent 3.8: G-Buffer & Deferred Rendering Complete (~200 TODOs)

### Objective
Complete G-buffer rendering and deferred lighting implementation.

### Critical Implementation Tasks

**G-Buffer Rendering (70 TODOs)**
- [ ] gbuffer_pass_execute() full implementation
- [ ] Material parameter binding (albedo, normal, roughness, metallic)
- [ ] Vertex/fragment shader integration
- [ ] Render target configuration
- [ ] Clear and initialization
- [ ] Command encoding

**Deferred Lighting (100 TODOs)**
- [ ] Complete fragment shader (currently 40% done)
- [ ] World position reconstruction
- [ ] Normal decoding from oct-encoding
- [ ] GGX BRDF implementation (cook-torrance)
- [ ] Fresnel-Schlick approximation
- [ ] Visibility function (GGX smith)
- [ ] Light sampling and accumulation
- [ ] Shadow map integration
- [ ] Cascade selection in shader
- [ ] PCF shadow filtering

**Optimization (30 TODOs)**
- [ ] Cache efficiency
- [ ] Bandwidth reduction
- [ ] Tile-based deferred improvements
- [ ] Memory pooling

### Success Criteria
- [ ] Geometry renders to G-buffer with all required data
- [ ] Deferred lighting produces correct PBR illumination
- [ ] Shadows properly integrated
- [ ] Performance meets targets

---

## Agent 3.9: Forward Rendering & Transparency (~180 TODOs)

### Objective
Complete forward rendering pipeline with transparency.

### Critical Implementation Tasks

**Forward Pass Execution (60 TODOs)**
- [ ] forward_pass_execute() implementation
- [ ] Opaque geometry rendering
- [ ] Material binding
- [ ] Light binding and sampling
- [ ] Shadow sampling integration
- [ ] Command encoding

**Transparency System (70 TODOs)**
- [ ] Transparent geometry sorting
- [ ] Back-to-front ordering
- [ ] Depth sorting algorithm
- [ ] Alpha blending implementation
- [ ] Order-independent transparency setup
- [ ] Weighted blended OIT implementation
- [ ] Accumulation buffers

**Advanced Materials (35 TODOs)**
- [ ] Glass material shader completion
- [ ] Refraction implementation
- [ ] Normal mapping in refraction
- [ ] Water shader with displacement
- [ ] Parallax mapping
- [ ] Parallax occlusion mapping

**Optimization (15 TODOs)**
- [ ] Draw call batching
- [ ] State change minimization
- [ ] Material atlas support

### Success Criteria
- [ ] Opaque geometry renders with correct lighting
- [ ] Transparent geometry sorts and renders correctly
- [ ] Advanced materials functional
- [ ] No transparency artifacts

---

## Agent 3.10: Shadow Sampling & Integration (~160 TODOs)

### Objective
Complete shadow system integration with deferred and forward rendering.

### Critical Implementation Tasks

**Shadow Pass (40 TODOs)**
- [ ] shadow_pass rendering implementation
- [ ] Depth-only rendering
- [ ] Light space transformation
- [ ] Polygon offset for bias
- [ ] Back-face rendering support
- [ ] Partial update optimization

**Deferred Integration (45 TODOs)**
- [ ] Shadow sampling in deferred lighting shader
- [ ] Cascade selection algorithm
- [ ] PCF filtering in shader
- [ ] Contact shadows
- [ ] Shadow comparison
- [ ] Bias adjustment

**Forward Integration (35 TODOs)**
- [ ] Forward shadow sampling
- [ ] Cascade blending in forward
- [ ] Light space calculations
- [ ] Performance optimization

**Advanced Techniques (25 TODOs)**
- [ ] Temporal reprojection for cascades
- [ ] Contact hardening (PCSS)
- [ ] Variable penumbra
- [ ] Artifact prevention
- [ ] Quality tiers

**Optimization (15 TODOs)**
- [ ] Shadow atlas memory optimization
- [ ] Cascade update efficiency
- [ ] Cache optimization

### Success Criteria
- [ ] Shadows render correctly from all lights
- [ ] Cascade transitions smooth
- [ ] Advanced shadow techniques working
- [ ] No shadow artifacts

---

## Agent 3.11: Shader Compilation, Variants & Hot Reload (~180 TODOs)

### Objective
Complete shader system with variants, hot reload, and Metal integration.

### Critical Implementation Tasks

**Shader Variants (60 TODOs)**
- [ ] Variant permutation generation algorithm
- [ ] Specialization constant handling
- [ ] Permutation caching
- [ ] Variant compilation
- [ ] Fallback handling
- [ ] Memory optimization

**Hot Reload (50 TODOs)**
- [ ] File watching implementation
- [ ] Change detection
- [ ] Compilation triggering
- [ ] Pipeline invalidation
- [ ] Cache updating
- [ ] GPU sync before reload
- [ ] Error recovery
- [ ] Fallback to previous version

**Function Reflection (40 TODOs)**
- [ ] Buffer binding introspection
- [ ] Texture binding queries
- [ ] Sampler queries
- [ ] Parameter validation
- [ ] Mismatch detection and reporting

**Asset Shader Integration (30 TODOs)**
- [ ] GLSL to Metal conversion tool
- [ ] Shader asset pipeline
- [ ] Format validation
- [ ] Error messages
- [ ] Batch conversion support

### Success Criteria
- [ ] Shader variants generate and compile
- [ ] Hot reload works for shader changes
- [ ] Function reflection accurate
- [ ] Asset shaders compatible

---

## Phase 3 Integration Checklist - UPDATED

All agents must verify:
- [ ] All Metal GPU commands properly generated
- [ ] No Vulkan/D3D12 references
- [ ] Resource lifecycle complete
- [ ] Thread-safe operations
- [ ] Error handling comprehensive
- [ ] Compilation succeeds without warnings
- [ ] All stub functions removed
- [ ] ~1,320 Phase 3 TODOs resolved
- [ ] GPU rendering functional end-to-end
- [ ] No resource hazards

---

## Phase 3 Completion Criteria - UPDATED

**ORIGINAL AGENTS (3.1-3.6):** ~600 TODOs
- Render graph framework
- G-buffer structure
- Deferred lighting framework
- Forward rendering framework
- Shadow system framework
- Shader system framework

**REMEDIAL AGENTS (3.7-3.11):** ~920 TODOs
- [ ] Agent 3.7 (Graph Execution): 200 TODOs - Compilation & execution
- [ ] Agent 3.8 (G-Buffer & Deferred): 200 TODOs - Rendering & lighting
- [ ] Agent 3.9 (Forward & Transparency): 180 TODOs - Forward pass & effects
- [ ] Agent 3.10 (Shadow Integration): 160 TODOs - Shadow system hookup
- [ ] Agent 3.11 (Shader System): 180 TODOs - Variants & hot reload

**TOTAL PHASE 3:** ~1,320 TODOs

## Deliverables After Full Phase 3

- ✅ Render graph compiles and executes correctly
- ✅ G-buffer renders geometry with all data
- ✅ Deferred lighting illuminates with PBR
- ✅ Forward rendering works for transparency
- ✅ Cascaded shadows cast correctly
- ✅ Shader variants generate at runtime
- ✅ Shader hot reload works
- ✅ Complete integrated rendering pipeline
- ✅ ~1,320 TODOs completed
- ✅ **Ready for Phase 4 (Materials & Global Illumination)**

**IMPORTANT NOTE:** Phase 3 must be 95%+ complete before advancing to Phase 4. These remedial agents address critical GPU rendering gaps.

