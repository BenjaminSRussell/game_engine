# Phase 4: Materials, Lighting & Advanced GI (100+ Tasks Per Agent)

---

## Agent 4.1: PBR Material System (~100 TODOs)

**Material Definition (25 TODOs)**
- [ ] Material structure (albedo, metallic, roughness, AO)
- [ ] Material instances (per-object parameters)
- [ ] Texture slots (diffuse, normal, roughness, metallic, AO, emissive)
- [ ] Scalar parameters
- [ ] Vector parameters
- [ ] Material flags
- [ ] Shader reference

**Material Compilation (20 TODOs)**
- [ ] Shader variant generation
- [ ] Parameter binding
- [ ] Texture binding
- [ ] Constant buffers
- [ ] Sampler state
- [ ] Error validation

**Material Library (20 TODOs)**
- [ ] Material creation/destruction
- [ ] Material cloning
- [ ] Parameter overrides
- [ ] Global material registry
- [ ] Material caching
- [ ] Memory management

**Runtime Updates (20 TODOs)**
- [ ] Update material parameters
- [ ] Update textures
- [ ] Realtime shader editing
- [ ] Persistence to disk
- [ ] Material import

**Specialized Materials (15 TODOs)**
- [ ] Transparent materials
- [ ] Emissive materials
- [ ] Displacement mapping
- [ ] Parallax occlusion
- [ ] Subsurface scattering

---

## Agent 4.2: Clustered Light Management (~100 TODOs)

**Light Source Creation (25 TODOs)**
- [ ] Directional light structure
- [ ] Point light structure
- [ ] Spot light structure
- [ ] Light creation API
- [ ] Light destruction
- [ ] Light activation/deactivation
- [ ] Light parameters
- [ ] IES profile support

**Light Culling System (30 TODOs)**
- [ ] 3D grid initialization
- [ ] Cell bounds calculation
- [ ] Light assignment to cells
- [ ] Per-cell light list
- [ ] Max lights per cell
- [ ] Overflow handling
- [ ] Depth range per cell
- [ ] GPU list generation
- [ ] Sorting optimization

**Light Parameters (20 TODOs)**
- [ ] Intensity/brightness
- [ ] Color
- [ ] Position
- [ ] Direction
- [ ] Range/falloff
- [ ] Cone angle (spot)
- [ ] Shadow casting
- [ ] IES data

**Dynamic Lighting (15 TODOs)**
- [ ] Update light positions
- [ ] Culling updates
- [ ] Removal/addition
- [ ] Performance budgets
- [ ] Statistics

**Performance Optimization (10 TODOs)**
- [ ] Reduce light count
- [ ] Sorting strategies
- [ ] Cache optimization
- [ ] Profiling

---

## Agent 4.3: Irradiance Probes & Global Illumination (~100 TODOs)

**Probe Grid Setup (25 TODOs)**
- [ ] Grid initialization
- [ ] Probe placement
- [ ] Probe density
- [ ] Bounds definition
- [ ] Memory allocation
- [ ] GPU transfer
- [ ] Updates

**Probe Baking (20 TODOs)**
- [ ] Irradiance computation
- [ ] Distance field computation
- [ ] Ray casting
- [ ] Hemicube rendering
- [ ] Sphere rendering
- [ ] Filtering
- [ ] Validation

**Runtime Sampling (20 TODOs)**
- [ ] Trilinear interpolation
- [ ] Probe selection
- [ ] Blending
- [ ] Normals weighting
- [ ] Parallax correction
- [ ] Smooth transitions

**Indirect Lighting (20 TODOs)**
- [ ] Diffuse indirect
- [ ] Specular indirect (from probes)
- [ ] Glossy reflections
- [ ] Ambient occlusion blending
- [ ] Energy conservation

**Optimization (15 TODOs)**
- [ ] Sparse probe grids
- [ ] Adaptive grids
- [ ] Temporal updates
- [ ] Multi-level hierarchy
- [ ] Cache coherence

---

## Agent 4.4: Volumetric Fog & Froxels (~100 TODOs)

**Froxel Grid Setup (25 TODOs)**
- [ ] 3D froxel grid definition
- [ ] Depth slicing
- [ ] Near/far plane
- [ ] Froxel size
- [ ] Memory layout
- [ ] GPU allocation
- [ ] Updates

**Fog Volume Definition (20 TODOs)**
- [ ] Volume shape (box, sphere, capsule)
- [ ] Fog density
- [ ] Absorption
- [ ] Scattering
- [ ] Anisotropy
- [ ] Color
- [ ] Emission

**Volumetric Rendering (20 TODOs)**
- [ ] Froxel traversal
- [ ] Light accumulation
- [ ] Scattering integration
- [ ] Multiple bounces
- [ ] Temporal filtering
- [ ] Performance

**Lighting Integration (20 TODOs)**
- [ ] Direct light contribution
- [ ] Indirect light
- [ ] Shadow application
- [ ] Phase functions
- [ ] Anisotropic scattering

**Optimization (15 TODOs)**
- [ ] Reprojection
- [ ] Resolution reduction
- [ ] Filtering
- [ ] Cache optimization
- [ ] Profiling

---

## Agent 4.5: Screen-Space Reflections (~100 TODOs)

**SSR Ray Tracing (25 TODOs)**
- [ ] Ray generation
- [ ] Screen-space ray marching
- [ ] Hierarchical Z-buffer
- [ ] Ray-plane intersection
- [ ] Early termination
- [ ] Hit detection

**Reflection Sampling (20 TODOs)**
- [ ] Stochastic sampling
- [ ] Sample count
- [ ] Noise patterns
- [ ] Reflection filtering
- [ ] Roughness variation
- [ ] Edge preservation

**History & Reprojection (20 TODOs)**
- [ ] Previous frame texture
- [ ] Velocity buffer
- [ ] Temporal reprojection
- [ ] Invalid pixel detection
- [ ] Disocclusion handling

**Filtering & Denoising (20 TODOs)**
- [ ] Bilateral filtering
- [ ] Edge-preserving blur
- [ ] Variance clipping
- [ ] Temporal filtering
- [ ] Confidence weighting

**Integration (15 TODOs)**
- [ ] Blend with environment
- [ ] Material-aware reflection
- [ ] Fallback for off-screen
- [ ] Fade at edges

---

## Agent 4.6: Post-Processing: Bloom & Tonemapping (~100 TODOs)

**Bloom Effect (30 TODOs)**
- [ ] Luminance threshold
- [ ] Downsampling passes
- [ ] Blur (13-tap tent filter)
- [ ] Multiple bloom levels
- [ ] Upsampling
- [ ] Reconstruction filter
- [ ] Blend with original
- [ ] Intensity control
- [ ] Color shift

**Tone Mapping (25 TODOs)**
- [ ] ACES tone curve
- [ ] Linear tone mapping
- [ ] Reinhard tone mapping
- [ ] Unreal tone mapping
- [ ] Exposure control
- [ ] Contrast adjustment
- [ ] Saturation
- [ ] Color grading LUT

**Exposure Control (20 TODOs)**
- [ ] Auto exposure
- [ ] Metering modes
- [ ] Exposure history
- [ ] Adaptation speed
- [ ] Clamp range
- [ ] User override

**Color Grading (15 TODOs)**
- [ ] 3D LUT texture
- [ ] LUT blending
- [ ] Per-segment grading
- [ ] Custom curves
- [ ] Export/import

**Optimization (10 TODOs)**
- [ ] Shared blur passes
- [ ] Resolution reduction
- [ ] Caching
- [ ] Performance profiling

---

## Phase 4 Integration Checklist

All agents verify:
- [ ] Materials render correctly
- [ ] Lights illuminate properly
- [ ] GI contributes
- [ ] Volumetrics work
- [ ] SSR functional
- [ ] Post-processing applied
- [ ] ~600 TODOs completed
- [ ] Ready for Phase 5

