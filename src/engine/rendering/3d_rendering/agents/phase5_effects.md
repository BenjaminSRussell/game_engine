# Phase 5: Effects, Environment & Post-Processing (100+ Tasks Per Agent)

---

## Agent 5.1: GPU Particle System (~100 TODOs)

**Particle Pool Management (25 TODOs)**
- [ ] Particle buffer structure
- [ ] Dead particle list (free list)
- [ ] Alive particle list
- [ ] Reuse pool
- [ ] Allocation strategies
- [ ] Memory layout
- [ ] GPU transfer

**Particle Emission (20 TODOs)**
- [ ] Emitter structure
- [ ] Emission rate
- [ ] Lifetime
- [ ] Initial velocity
- [ ] Size
- [ ] Color
- [ ] Rotation

**GPU Simulation (30 TODOs)**
- [ ] Compute shader simulation
- [ ] Velocity update
- [ ] Position update
- [ ] Lifetime update
- [ ] Acceleration (gravity)
- [ ] Drag
- [ ] Collision detection
- [ ] Despawn logic
- [ ] Multi-threaded dispatch

**Rendering (15 TODOs)**
- [ ] Billboard quads
- [ ] Instanced rendering
- [ ] Sort by camera distance
- [ ] Blending modes
- [ ] Texture atlasing

**Optimization (10 TODOs)**
- [ ] Spatial hashing
- [ ] Culling
- [ ] Cache coherence
- [ ] Profiling

---

## Agent 5.2: FFT Ocean Simulation (~100 TODOs)

**Phillips Spectrum (20 TODOs)**
- [ ] Spectrum generation
- [ ] Wind parameters
- [ ] Amplitude scaling
- [ ] Wave statistics
- [ ] Dispersion

**FFT Computation (25 TODOs)**
- [ ] 2D FFT compute shader
- [ ] Butterfly operations
- [ ] Bit reversal
- [ ] Radix-2 FFT
- [ ] In-place computation
- [ ] Bit reversal permutation

**Wave Displacement (20 TODOs)**
- [ ] Height field computation
- [ ] Horizontal displacement
- [ ] Normal computation
- [ ] Foam factor
- [ ] Surface reconstruction

**Ocean Rendering (20 TODOs)**
- [ ] Tessellated grid
- [ ] Displacement mapping
- [ ] Normal mapping
- [ ] Fresnel reflections
- [ ] Refraction

**Optimization (15 TODOs)**
- [ ] Cascade FFTs
- [ ] Multi-scale waves
- [ ] Temporal coherence
- [ ] LOD meshes
- [ ] Profiling

---

## Agent 5.3: Atmospheric Scattering & Sky (~100 TODOs)

**Rayleigh & Mie Scattering (25 TODOs)**
- [ ] Rayleigh coefficient
- [ ] Mie coefficient
- [ ] Wavelength dependence
- [ ] Optical depth
- [ ] Transmittance

**LUT Precomputation (20 TODOs)**
- [ ] Transmittance LUT
- [ ] Multi-scattering LUT
- [ ] Sky view LUT
- [ ] Precompute shaders
- [ ] Storage optimization

**Sky Rendering (20 TODOs)**
- [ ] Full-screen sky pass
- [ ] Sunlight direction
- [ ] Sun disk
- [ ] Clouds
- [ ] Night sky
- [ ] Stars

**Aerial Perspective (20 TODOs)**
- [ ] Depth-based fog
- [ ] Scattering lookup
- [ ] Blending with scene
- [ ] Distance falloff
- [ ] Color grading

**Optimization (15 TODOs)**
- [ ] Reprojection
- [ ] Low-res prepass
- [ ] Temporal filtering
- [ ] Cache optimization

---

## Agent 5.4: Temporal Anti-Aliasing (~100 TODOs)

**Frame Setup (20 TODOs)**
- [ ] Halton sequence jitter
- [ ] Jitter per-frame
- [ ] Projection matrix modification
- [ ] Shader constant update
- [ ] Frame index

**Velocity Buffer (20 TODOs)**
- [ ] Velocity computation
- [ ] Motion vectors
- [ ] Disocclusion detection
- [ ] Velocity scaling
- [ ] Validity

**Reprojection (25 TODOs)**
- [ ] Previous frame lookup
- [ ] Bilinear sampling
- [ ] Motion compensated lookup
- [ ] Invalid pixel detection
- [ ] Neighborhood clamping
- [ ] Variance clipping

**Filtering (20 TODOs)**
- [ ] 3x3 neighborhood sampling
- [ ] Temporal accumulation
- [ ] Feedback weight
- [ ] History blending
- [ ] Ghosting prevention

**Optimization (15 TODOs)**
- [ ] Efficient lookup
- [ ] Cache coherence
- [ ] Shared compute
- [ ] Memory bandwidth

---

## Agent 5.5: Decal System (~100 TODOs)

**Decal Structures (20 TODOs)**
- [ ] Decal buffer
- [ ] Position/rotation/scale
- [ ] Normal/AO/roughness maps
- [ ] Blend modes
- [ ] Lifetime

**Decal Projection (25 TODOs)**
- [ ] Projection matrix setup
- [ ] Screen-space bounds
- [ ] Frustum culling
- [ ] Z-test against G-buffer
- [ ] Mask generation

**Decal Blending (25 TODOs)**
- [ ] Apply to G-buffer
- [ ] Normal blending
- [ ] AO application
- [ ] Roughness modification
- [ ] Blend modes (add, multiply, etc.)

**Optimization (20 TODOs)**
- [ ] Sort by distance
- [ ] Culling
- [ ] Atlas textures
- [ ] Memory budgeting
- [ ] Decay/removal

**Debug Visualization (10 TODOs)**
- [ ] Show decal bounds
- [ ] Show projected area
- [ ] Statistics

---

## Agent 5.6: Debug Visualization & Profiling (~100 TODOs)

**Debug Rendering (25 TODOs)**
- [ ] Debug line renderer
- [ ] Debug box renderer
- [ ] Debug sphere renderer
- [ ] Debug grid
- [ ] Debug text 3D
- [ ] Color coding

**Visualization Systems (25 TODOs)**
- [ ] Light visualization (spheres)
- [ ] Shadow frustum visualization
- [ ] Probe grid visualization
- [ ] Cluster grid visualization
- [ ] BVH visualization
- [ ] G-buffer channels

**GPU Profiling (20 TODOs)**
- [ ] GPU timestamp queries
- [ ] Per-pass timing
- [ ] Overlay display
- [ ] Frame graph
- [ ] Memory statistics
- [ ] Bandwidth tracking

**CPU-GPU Timing (15 TODOs)**
- [ ] CPU frame time
- [ ] GPU frame time
- [ ] Stall detection
- [ ] Budget tracking
- [ ] Alert system

**Performance Analysis (15 TODOs)**
- [ ] Bottleneck identification
- [ ] Bandwidth analysis
- [ ] ALU utilization
- [ ] Cache efficiency
- [ ] Optimization hints

---

## Phase 5 Integration Checklist

All agents verify:
- [ ] Particles simulate
- [ ] Ocean waves
- [ ] Sky renders
- [ ] TAA stable
- [ ] Decals work
- [ ] Profiling functional
- [ ] ~600 TODOs completed
- [ ] Ready for Phase 6

