# Phase 3: Metal Rendering Pipeline (100+ Tasks Per Agent)

---

## Agent 3.1: Render Graph & Frame Graph (~100 TODOs)

### Objective
Implement automatic render graph with resource management and pass scheduling.

**Core Tasks (30 TODOs)**
- [ ] Define render pass node structure
- [ ] Define resource node structure
- [ ] Graph construction API
- [ ] Add pass to graph
- [ ] Add resource to graph
- [ ] Connect passes (dependencies)
- [ ] Validate graph acyclic
- [ ] Detect unused resources
- [ ] Optimize pass order

**Resource Aliasing & Reuse (25 TODOs)**
- [ ] Track resource lifetime
- [ ] Detect non-overlapping uses
- [ ] Alias resources when safe
- [ ] Reuse memory efficiently
- [ ] Memory budget enforcement
- [ ] Fragmentation analysis
- [ ] Statistics reporting

**Barrier Insertion (20 TODOs)**
- [ ] Analyze resource hazards
- [ ] Insert memory barriers
- [ ] Insert image layout transitions
- [ ] Optimize barrier placement
- [ ] Validate memory safety
- [ ] Performance measurement

**Compilation & Execution (15 TODOs)**
- [ ] Compile graph to execution plan
- [ ] Allocate transient resources
- [ ] Schedule command encoding
- [ ] Frame pacing integration
- [ ] Statistics collection

**Debugging & Visualization (10 TODOs)**
- [ ] Graph visualization
- [ ] Pass timing
- [ ] Resource tracking
- [ ] Memory reporting

---

## Agent 3.2: G-Buffer & Deferred Rendering Setup (~100 TODOs)

### Objective
Implement deferred rendering G-buffer with multiple render targets.

**G-Buffer Structure (25 TODOs)**
- [ ] Albedo texture (RGBA8)
- [ ] Normal texture (RG16F)
- [ ] Material texture (RGBA8: metallic, roughness, AO, flags)
- [ ] Depth texture (Depth32Float)
- [ ] Velocity texture (RG16F)
- [ ] Emissive texture (RGBA8)
- [ ] Texture creation
- [ ] Format validation
- [ ] Resize handling

**G-Buffer Pass (25 TODOs)**
- [ ] Render pass descriptor
- [ ] Material assignment
- [ ] Shader binding
- [ ] Vertex/fragment setup
- [ ] Multiple render targets
- [ ] Depth testing
- [ ] Viewport setup
- [ ] Clear colors
- [ ] Statistics

**Normal Encoding (15 TODOs)**
- [ ] Spheremap encoding
- [ ] Oct-tree encoding
- [ ] RGBA8 quantization
- [ ] Reconstruction
- [ ] Tangent space handling

**Deferred Lighting Setup (20 TODOs)**
- [ ] Lighting pass descriptor
- [ ] Full-screen triangle
- [ ] Texture inputs
- [ ] Buffer inputs
- [ ] Output render target
- [ ] Blend modes

**Performance Optimization (15 TODOs)**
- [ ] Bandwidth reduction
- [ ] Cache efficiency
- [ ] MSAA handling
- [ ] Early-Z
- [ ] Viewport optimization

---

## Agent 3.3: Deferred Lighting Pass (~100 TODOs)

### Objective
Implement PBR deferred lighting with light culling.

**Clustered Light Culling (30 TODOs)**
- [ ] 3D light grid setup
- [ ] Cell division
- [ ] Light assignment compute
- [ ] Per-cell light list
- [ ] Depth range per cell
- [ ] Statistics
- [ ] Memory layout
- [ ] GPU upload

**PBR BRDF Lighting (25 TODOs)**
- [ ] Lambertian diffuse
- [ ] Specular GGX BRDF
- [ ] Metallic workflow
- [ ] Roughness handling
- [ ] Fresnel computation
- [ ] Visibility functions
- [ ] Ambient occlusion
- [ ] Energy conservation

**Light Types (20 TODOs)**
- [ ] Directional lights
- [ ] Point lights
- [ ] Spot lights
- [ ] Area lights
- [ ] IES profiles
- [ ] Light attenuation
- [ ] Geometric falloff

**Shadows in Deferred (15 TODOs)**
- [ ] Shadow sampling
- [ ] PCF filtering
- [ ] Cascade blending
- [ ] Contact hardening
- [ ] Performance optimization

**Advanced Features (10 TODOs)**
- [ ] Lightmap integration
- [ ] Probe blending
- [ ] Dynamic range handling
- [ ] Optimization passes

---

## Agent 3.4: Forward Rendering & Transparency (~100 TODOs)

### Objective
Implement forward rendering for transparency and special materials.

**Forward Rendering Path (25 TODOs)**
- [ ] Opaque forward pass
- [ ] Transparent forward pass
- [ ] Sorting
- [ ] Batch optimization
- [ ] Shader variants
- [ ] State management
- [ ] Performance

**Transparency Handling (20 TODOs)**
- [ ] Alpha blending
- [ ] Alpha masking
- [ ] Alpha to coverage
- [ ] Weighted blended OIT
- [ ] Order-independent transparency
- [ ] MSAA interaction

**Refraction & Distortion (15 TODOs)**
- [ ] Refraction vectors
- [ ] Screen-space refraction
- [ ] IOR mapping
- [ ] Thickness
- [ ] Caustics

**Glass & Special Materials (20 TODOs)**
- [ ] Glass shaders
- [ ] Water shaders
- [ ] Particle effects
- [ ] Additive blending
- [ ] Custom blend modes

**Performance (20 TODOs)**
- [ ] Draw call batching
- [ ] State caching
- [ ] Sorting strategies
- [ ] Overdraw reduction
- [ ] Optimization

---

## Agent 3.5: Cascaded Shadow Mapping (~100 TODOs)

### Objective
Implement cascaded shadow maps with filtering and optimization.

**Shadow Atlas Setup (20 TODOs)**
- [ ] Atlas texture creation
- [ ] Cascade allocation
- [ ] Resolution management
- [ ] Mipmap generation
- [ ] Format (Depth32, RGBA shadow)
- [ ] Resize handling

**Cascade Configuration (20 TODOs)**
- [ ] Cascade count (4 typical)
- [ ] Split planes calculation
- [ ] PSNM distribution
- [ ] View frustum fitting
- [ ] Texel density
- [ ] Stability

**Shadow Rendering (25 TODOs)**
- [ ] Depth prepass
- [ ] Rasterize to cascades
- [ ] Front face depth bias
- [ ] Polygon offset
- [ ] Near-plane clipping
- [ ] Back-face rendering
- [ ] Partial updates

**Shadow Sampling (20 TODOs)**
- [ ] Cascade selection
- [ ] PCF filtering
- [ ] Poisson disk
- [ ] Voronoi sampling
- [ ] Contact hardening
- [ ] Fade at edges

**Optimization (15 TODOs)**
- [ ] Partial updates
- [ ] Temporal reprojection
- [ ] Scene-based refinement
- [ ] Cache coherence
- [ ] Bandwidth

---

## Agent 3.6: Shader Compilation & Hot Reload (~100 TODOs)

### Objective
Implement Metal shader compilation with hot reload capability.

**Shader Library Loading (25 TODOs)**
- [ ] Load .metallib files
- [ ] Function extraction
- [ ] Function names
- [ ] Function signatures
- [ ] Error handling
- [ ] Library caching
- [ ] Hot reload detection
- [ ] File watching
- [ ] Compilation errors

**Function Reflection (20 TODOs)**
- [ ] Extract argument information
- [ ] Buffer argument reflection
- [ ] Texture argument reflection
- [ ] Sampler argument reflection
- [ ] Thread group size
- [ ] SIMD group size

**Pipeline Creation with Shaders (20 TODOs)**
- [ ] Fetch vertex function
- [ ] Fetch fragment function
- [ ] Fetch compute function
- [ ] Pipeline creation
- [ ] Error reporting
- [ ] Fallback handling
- [ ] Function validation

**Hot Reload (20 TODOs)**
- [ ] File system watcher
- [ ] Library recompilation
- [ ] Function re-extraction
- [ ] Pipeline invalidation
- [ ] Graceful fallback
- [ ] No frame drops

**Shader Variants (15 TODOs)**
- [ ] Variant definitions
- [ ] Constant specialization
- [ ] Feature flags
- [ ] Permutation management
- [ ] Caching

---

## Phase 3 Integration Checklist

All agents verify:
- [ ] Render graph compiles
- [ ] G-buffer renders
- [ ] Lighting works
- [ ] Shadows cast correctly
- [ ] Transparency renders
- [ ] Shaders load
- [ ] ~600 TODOs completed
- [ ] Ready for Phase 4

