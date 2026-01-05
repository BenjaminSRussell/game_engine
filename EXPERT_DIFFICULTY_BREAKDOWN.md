# Expert Difficulty TODOs (9-10/10) - Phase 1 & 2

**Total Expert TODOs in Phase 1 & 2:** 26
**Estimated Hours:** 75-100 hours each (1,950-2,600 total)
**Recommended:** Only 2-3 of these per developer per month

---

## Overview: Why These Are Expert-Level

Expert-level work typically requires:
1. **Research paper implementation** - Non-trivial algorithms (FABRIK IK, LBVH, SPH)
2. **Low-level systems knowledge** - Assembly (fiber context), GPU compute shaders
3. **Numerical stability** - Physics solvers, fluid simulation, ray tracing denoising
4. **Multi-component integration** - Pieces depend on other complex systems
5. **Extreme performance requirements** - Must hit tight performance budgets
6. **Cutting-edge techniques** - Nanite, ray tracing denoising, XPBD

---

## Phase 1: Core Infrastructure - Expert TODOs (6 total)

### 1. Fiber Context Save/Restore (3 TODOs)
**Files:** `src/engine/core/threading/fiber_context.c`
**Difficulty:** 9/10 each
**Estimated Hours:** 100+ hours

#### Why Expert?
- Requires **assembly language** (x64 and ARM64)
- **Undefined behavior** in C - need bare metal control
- **CPU-specific** - different registers, calling conventions per architecture
- **Exception handling** - need to work with stack unwinding
- **Subtle bugs** - memory corruption if stack not saved correctly

#### The Three TODOs:
```c
// TODO(AGENT_CORE_1): Implement buffer_context_save() [Difficulty: 9]
//   Save x64/ARM64 register state (RBP, RSP, RIP, non-volatiles)
//   Must preserve return address, handle red zone on x64
//   Assembly-level work: ~40-60 hours per architecture

// TODO(AGENT_CORE_1): Implement buffer_context_restore() [Difficulty: 9]
//   Restore saved context and jump to new execution point
//   Must prevent stack corruption, handle signal safety
//   Assembly-level work: ~40-60 hours per architecture

// TODO(AGENT_CORE_1): Implement exception handling support [Difficulty: 9]
//   Make fiber contexts work with C++ exceptions
//   Need to integrate with DWARF/SEH exception mechanisms
//   ~30-40 hours of research and integration
```

#### Why It's Hard:
- **x64 calling convention** has complex rules (red zone, alignment)
- **ARM64** has different register layout (30 general regs)
- **Stack unwinding** with exceptions requires frame pointer chains
- **Testing is nearly impossible** - bugs appear intermittently in production
- **No portable solution** - every architecture needs custom assembly

#### Research Required:
- x64 AMD64 ABI specification
- ARM64 AAPCS calling convention
- DWARF unwinding format
- Signal safety in fiber contexts

#### Recommended Approach:
1. **Start with existing implementations** - Look at Boost Fiber, coroutines
2. **Single architecture first** - Get x64 stable, then ARM64
3. **Extensive testing** - Stress test with 1000s of context switches
4. **Fuzzing** - Random context save/restore sequences
5. **Valgrind/ASAN** - Constant memory corruption detection

---

### 2. Lock-Free Ring Buffer MPMC (1 TODO)
**File:** `src/engine/core/containers_advanced/ring_buffer_lockfree.c`
**Difficulty:** 9/10
**Estimated Hours:** 40-50 hours

#### Why Expert?
- **Lock-free data structures** are notoriously bug-prone
- **Compare-and-swap (CAS)** loops require deep understanding of memory ordering
- **False sharing** - cache line contention kills performance
- **Multiple producer/consumer** (MPMC) = 4 concurrent access patterns
- **Subtle race conditions** - bugs only appear under specific thread timing

#### The TODO:
```c
// TODO(AGENT_CORE_2): Add mpmc_enqueue() (CAS loop) [Difficulty: 9]
//   Implement wait-free or lock-free multi-producer multi-consumer enqueue
//   Must use atomic_compare_and_swap for head pointer updates
//   Handle ABA problem (same index pointer used twice)
//   Padding for cache line false sharing (128 bytes)
//   ~40-50 hours
```

#### Why It's Hard:
- **ABA problem** - If thread A reads pointer, another thread enqueues/dequeues/enqueues same value, A can corrupt
- **Memory ordering** - Need acquire/release semantics on reads/writes
- **Cache coherency** - Multiple cores fighting over same memory location
- **Testing methodology** - ThreadSanitizer barely catches these bugs
- **Platform differences** - x64 vs ARM memory models differ significantly

#### Research Required:
- C11 atomic operations and memory ordering
- Lock-free programming patterns (Barnes, Herlihy)
- Cache coherency protocols (MESI/MOESI)
- ABA problem solutions

#### Recommended Approach:
1. **Start with single-producer/single-consumer (SPSC)** - Much simpler
2. **Add only one thread at a time** - First MPSC, then SPMC, then MPMC
3. **Use ThreadSanitizer** - `clang -fsanitize=thread`
4. **Stress test heavily** - 1000 iterations of 1000 threads
5. **Study existing code** - folly/concurrency, Boost.Lockfree
6. **Consider hybrid** - Lock-free for common case, lock for rare

---

### 3. AVX-512 Matrix Operations (2 TODOs)
**File:** `src/engine/core/simd/avx512_kernels.c`
**Difficulty:** 9-10/10
**Estimated Hours:** 60-80 hours

#### Why Expert?
- **SIMD intrinsics** have limited portability (AVX-512 not on all CPUs)
- **Matrix operations** require cache-aware optimization
- **Batch processing** - Process multiple 4x4 matrices simultaneously
- **Numerical precision** - Floating point error accumulation in chains
- **CPU-specific tuning** - Instruction scheduling per microarchitecture

#### The TODOs:
```c
// TODO(AGENT_PERF_2): Implement 4x4 matrix inverse [Difficulty: 10]
//   Compute inverse of 4 matrices in parallel (8 floats at once)
//   Use Gauss-Jordan elimination with row pivoting
//   Must handle near-singular matrices (det close to 0)
//   ~30-40 hours

// TODO(AGENT_PERF_2): Implement AABB frustum culling (16 AABBs) [Difficulty: 9]
//   Test 16 axis-aligned bounding boxes against view frustum simultaneously
//   Use SIMD dot products and comparisons
//   Return 16-bit mask of visible AABBs
//   ~20-30 hours
```

#### Why It's Hard:
- **AVX-512 is bleeding edge** - Not available on many CPUs (requires Xeon/high-end)
- **Instruction scheduling** matters enormously (CPU can't execute 2 vaddps in parallel if data dependent)
- **Register pressure** - 32 ZMM registers look like plenty until you use them
- **Different CPU models** - Skylake-X vs Cannon Lake have different uop counts
- **Floating point precision** - Matrix inverse near singularities is numerically unstable

#### Research Required:
- Intel intrinsic guide
- Agner Fog's microarchitecture manuals
- Cache-oblivious algorithms
- Numerical analysis for matrix operations

#### Recommended Approach:
1. **Start with scalar (non-SIMD) version** - Get correctness first
2. **Benchmark scalar version** - Know what you're optimizing for
3. **Use inline assembly sparingly** - Mostly use intrinsics
4. **Tune for one CPU first** - Then generalize (Skylake, then others)
5. **Profile with VTune** - See actual instruction uops and latencies
6. **Consider fallback** - Use SSE3 if AVX-512 unavailable

---

## Phase 2: Physics Engine - Expert TODOs (20 total)

### A. GPU Broadphase - LBVH Construction (1 TODO)
**File:** `src/engine/physics/broadphase/gpu_broadphase.c`
**Difficulty:** 10/10
**Estimated Hours:** 80-120 hours

#### Why Expert (Maximum Difficulty)?
- **Compute shader development** - GPU programming is fundamentally different from CPU
- **LBVH (Linear BVH)** - State-of-the-art but poorly documented
- **GPU memory management** - Must fit millions of AABBs and tree nodes
- **Synchronization points** - GPU→CPU readback causes pipeline stalls
- **Multiple algorithms** - Many ways to build BVH, each with tradeoffs

#### The TODO:
```c
// TODO(AGENT_PHYSICS_1): Implement LBVH construction on GPU [Difficulty: 10]
//   Build bounding volume hierarchy on GPU from 100K-1M AABBs
//   Use prefix sum for treelet assignments
//   Compute shader for tree node creation and linking
//   Synchronize between compute kernel invocations
//   Synchronize GPU→CPU for physics pipeline
//   ~80-120 hours
```

#### Why It's Hard:
- **LBVH papers** (Lauterbach et al.) assume you know GPU algorithms deeply
- **Compute shader debugging** is nearly impossible (print statements won't help)
- **Synchronization overhead** kills performance if done wrong
- **GPU memory layout** matters - Cache-unfriendly data structure = 10x slower
- **Driver quirks** - AMD/NVIDIA/Intel have different performance characteristics

#### Research Required:
- LBVH construction algorithm (Lauterbach 2009)
- GPU prefix sum (Blelloch scan algorithm)
- Compute shader fundamentals
- GPU memory bandwidth optimization
- Vulkan/DirectX ray tracing APIs

#### Recommended Approach:
1. **Skip GPU version for MVP** - Use SAP/AABB tree on CPU first
2. **Only GPU if hitting broadphase bottleneck** - Profile before optimizing
3. **Use existing library** - Nvidia PhysX GPU code or Bullet GPU branch
4. **Start with small set** - Get working for 1K objects, then scale
5. **Measure: is it actually faster?** - CPU+GPU sync overhead is huge
6. **Consider hybrid approach** - CPU broadphase, GPU narrowphase

---

### B. Soft Body Physics - SPH & FLIP Fluid (3 TODOs)
**File:** `src/engine/physics/softbody/cloth_simulation_advanced.c`
**Difficulty:** 9-10/10 each
**Estimated Hours:** 100-150 hours each (300-450 total)

#### Why Expert?
- **SPH (Smoothed Particle Hydrodynamics)** - Research paper algorithm
- **FLIP (Fluid-Implicit Particle)** - Hybrid Lagrangian/Eulerian, very complex
- **Numerical stability** - Fluid simulations blow up if not careful with time stepping
- **Many parameters** - Smoothing radius, viscosity, artificial pressure, etc.
- **Extreme performance requirements** - Must handle 10K+ particles in real-time

#### The TODOs:
```c
// TODO(AGENT_PHYSICS_2): Implement SPH fluid simulation [Difficulty: 10]
//   Smoothed Particle Hydrodynamics for water/liquid
//   Implement kernel functions (Poly6, Spiky)
//   Density calculation (sum neighbors weighted by kernel)
//   Pressure forces from Tait equation
//   Viscosity from Laplacian smoothing
//   Surface tension via curvature
//   ~120-150 hours

// TODO(AGENT_PHYSICS_2): Implement FLIP fluid solver [Difficulty: 10]
//   Fluid-Implicit Particle method
//   Eulerian velocity grid + Lagrangian particles
//   Pressure projection on grid
//   Transfer particle velocities to/from grid
//   Advection of particles through velocity field
//   Handle boundary conditions (free surface, obstacles)
//   ~150-200 hours

// TODO(AGENT_PHYSICS_2): Implement fluid-rigid coupling [Difficulty: 9]
//   Particles exert pressure on rigid bodies
//   Rigid bodies displace fluid
//   Buoyancy forces on submerged objects
//   Two-way coupling solver
//   ~80-100 hours
```

#### Why They're Hard:
- **Parameter tuning** - Dozens of magic numbers that break stability
- **Boundary handling** - Particles at container edges are edge cases
- **CFL condition** - Time step must be tiny if particles move fast (expensive)
- **Kernel choice** - Different kernels have different smoothness/performance
- **Incompressibility** - True incompressibility is unsolvable, must approximate

#### Research Required:
- SPH surveys (Monaghan, Müller et al.)
- FLIP/PIC papers (Zhu & Bridson)
- Divergence-free velocity fields
- Pressure projection (Poisson solver)
- GPU SPH implementations (for performance)

#### Recommended Approach:
1. **Skip for MVP** - Use rigid body physics only initially
2. **Start with SPH** - Simpler than FLIP, easier to debug
3. **CPU-only first** - Get algorithm correct, then optimize
4. **Use small particle counts** - Start with 100-500 particles
5. **Visualize forces** - Debug by drawing pressure/velocity fields
6. **Use existing solver library** - FLIP^PIC library, OpenVDB
7. **Consider particles optional feature** - Not critical for core gameplay

---

### C. FEM Deformable Objects - Fracture (1 TODO)
**File:** `src/engine/physics/softbody/fem_deformable.c`
**Difficulty:** 10/10
**Estimated Hours:** 80-120 hours

#### Why Expert (Maximum Difficulty)?
- **FEM (Finite Element Method)** itself is PhD-level material
- **Fracture simulation** adds plasticity + crack propagation
- **Mesh modifications** - Tetrahedral mesh must be updated dynamically
- **Numerical stability** - Matrix solvers can diverge easily
- **Real-time constraints** - Can't solve full Navier-Stokes every frame

#### The TODO:
```c
// TODO(AGENT_PHYSICS_2): Add fracture/cutting support [Difficulty: 10]
//   Detect when stress exceeds material yield strength
//   Split tetrahedral elements along fracture surface
//   Remesh around fracture boundary
//   Separate mesh pieces, add new collision geometry
//   Create surface mesh for visual rendering
//   Maintain simulation continuity across fracture
//   ~100-120 hours
```

#### Why It's Hard:
- **Tetrahedralization** - Generating valid tet mesh from arbitrary triangles is non-trivial
- **Stress computation** - Need to solve for stress tensor at integration points
- **Plasticity** - Track permanent deformation (very complex)
- **Mesh coherency** - Breaking mesh and re-connecting is error-prone
- **Visual quality** - Fracture surfaces must look good, not just physically correct

#### Research Required:
- FEM fundamentals (Hughes FEM book)
- Plasticity theory
- Mesh generation algorithms
- Level set methods for fracture
- GPU FEM implementations

#### Recommended Approach:
1. **Skip fracture for MVP** - Deformables without breaking are valuable
2. **Start with elasticity only** - Get basic FEM working first
3. **Use Tetgen library** - Don't hand-write tetrahedralization
4. **CPU-only initially** - GPU FEM is research territory
5. **Consider pre-fractured meshes** - Artist creates pieces, simulation glues them
6. **Fallback to rigid body** - If FEM unstable, switch object to rigid

---

### D. Character Physics - FABRIK & Full-Body IK (2 TODOs)
**File:** `src/engine/animation/ik_advanced/fabrik_solver.c`
**Difficulty:** 9/10 each
**Estimated Hours:** 50-80 hours each (100-160 total)

#### Why Expert?
- **FABRIK (Forward And Backward Reaching IK)** - Elegant algorithm but complex
- **Full-body constraints** - Multiple limbs must cooperate
- **Numerical precision** - Floating point errors accumulate with chain length
- **Multi-constraint satisfaction** - Pole targets, joint limits, foot placement
- **Real-time performance** - Must solve in <1ms

#### The TODOs:
```c
// TODO(AGENT_ANIM_1): Implement FABRIK for tree structures [Difficulty: 9]
//   Chain-based IK sufficient for simple limbs
//   But skeletal rigs have branches (ribs, fingers)
//   FABRIK extension to tree topology
//   Solve multiple chains sharing parent joints
//   ~50-60 hours

// TODO(AGENT_ANIM_1): Add Jacobian-based solver [Difficulty: 9]
//   Inverse Jacobian method for overdetermined systems
//   Damped Least Squares (DLS) for singularities
//   Selectively relax constraints (foot > hip)
//   ~50-60 hours
```

#### Why They're Hard:
- **FABRIK basics** - Forward/backward passes with distance constraints (OK)
- **Tree structures** - Multiple branches from single parent (hard)
- **Joint limits** - Clamping rotations while maintaining chain length (very hard)
- **Performance** - 50+ bone chains need fast solution
- **Blending** - Blend IK solution with animation smoothly (very hard)

#### Research Required:
- FABRIK paper (Aristidou & Lasenby)
- Jacobian transpose method
- Damped least squares (SVD-based)
- Quaternion constraints
- Animation blending techniques

#### Recommended Approach:
1. **Start with 2-bone IK** - Arms, legs are simple
2. **Add FABRIK iteratively** - Forward pass, then backward, then forward
3. **Handle joint limits naively first** - Clamp after iteration (not perfect but fast)
4. **Use DLS only if needed** - Simpler methods often sufficient
5. **Test on real character rigs** - 60+ bones with constraints
6. **Blend with key-framed animations** - Manual animation more reliable for cutscenes

---

### E. Soft Body Constraints - Self-Collision (1 TODO)
**File:** `src/engine/physics/softbody/softbody_constraints.c`
**Difficulty:** 9/10
**Estimated Hours:** 80-100 hours

#### Why Expert?
- **Self-collision detection** is O(n²) naively
- **Cloth-cloth collisions** - Garment folding on itself
- **Continuous collision detection** - CCD needed to prevent tunneling
- **Constraint generation** - Must create many fine-grained collision constraints
- **Numerical complexity** - Solver must handle contact + cloth constraints

#### The TODO:
```c
// TODO(AGENT_PHYSICS_2): Add self-collision detection [Difficulty: 9]
//   Prevent cloth from passing through itself
//   Build spatial data structure (octree/BVH) of particles
//   Test particle-triangle collisions
//   Generate contact constraints (separation, friction)
//   Integrate with cloth constraint solver
//   ~80-100 hours
```

#### Why It's Hard:
- **Continuous collision detection (CCD)** - Discrete checks miss fast movement
- **Constraint stiffness** - Too stiff = jittering, too loose = penetration
- **Spatial coherency** - Rebuild structure every frame is expensive
- **Performance cliff** - Works fine for 1K particles, breaks at 10K
- **Debugging impossible** - Cloth explosion = could be 10 different bugs

#### Research Required:
- CCD algorithms (Bridson et al.)
- Broadphase spatial data structures
- Constraint-based collision response
- PBD contact handling

#### Recommended Approach:
1. **Skip self-collision for MVP** - Single-layer cloth doesn't need it
2. **Implement without CCD first** - Discrete collision checks only
3. **Use simple spatial grid** - Fast to update, good enough for particles
4. **Conservative constraint stiffness** - Slightly loose is better than jitter
5. **Profile before optimizing** - Self-collision might not be bottleneck
6. **Consider simplified geometry** - Use coarse collision mesh, not all particles

---

## Summary: Expert TODO Difficulty Ranking

### Tier 1: Absolute Maximum Difficulty (10/10) - Research-Level Work
1. **LBVH construction on GPU** - Requires GPU algorithm research
2. **SPH fluid simulation** - Needs deep numerical analysis knowledge
3. **FLIP fluid solver** - Very complex multi-step algorithm
4. **FEM fracture/cutting** - PhD-level FEM knowledge
5. **DXR/Vulkan RT integration** - Requires ray tracing API expertise

**Total: 5 items, ~500 hours**

### Tier 2: Very Hard (9/10) - Senior Engineer Level
1. **Fiber context save/restore** - Assembly language + exception handling
2. **Lock-free MPMC queue** - Subtle concurrency bugs
3. **AVX-512 matrix operations** - CPU microarchitecture knowledge
4. **FABRIK IK solver** - Complex mathematics + performance tuning
5. **Self-collision detection** - CCD algorithms + spatial data structures
6. **Full-body IK solver** - Mathematical optimization + blending
7. **Cloth simulation (PBD)** - Constraint-based physics
8. **Tetrahedral mesh generation** - Complex 3D geometry
9. **Corotated elasticity** - Advanced material deformation
10. **Fluid-rigid coupling** - Two-way physics solver integration
11. **Soft body constraints** - Multi-constraint satisfaction
12. **GPU fluid acceleration** - Compute shader optimization
13. **GPU-accelerated ray marching** - Shader complexity
14. **Procedural ocean FFT** - Signal processing + GPU
15. **FABRIK tree structures** - Multi-chain IK

**Total: 15 items, ~1,100 hours**

---

## Recommendation: Skip Most Expert TODOs for Phase 1 & 2 MVP

These 26 expert TODOs are:
- **Not critical for MVP** - Core gameplay works without them
- **High risk** - Bugs are subtle and hard to find
- **Long tail** - Each one could take 3-6 months of senior engineer time
- **Performance-sensitive** - Optimization takes huge time investment

### Recommended Scope Reduction:

**For MVP, you NEED:**
- ✅ Stack allocator (medium)
- ✅ Arena allocator (medium)
- ✅ Sparse set container (medium-hard)
- ✅ Sequential impulse solver (hard)
- ✅ Broadphase SAP algorithm (medium-hard) - NOT GPU version

**For MVP, SKIP:**
- ❌ LBVH/GPU broadphase
- ❌ SPH/FLIP fluids
- ❌ FEM deformables with fracture
- ❌ Advanced IK solvers
- ❌ Self-collision detection
- ❌ All fiber context assembly code
- ❌ AVX-512 optimization

**Instead, use:**
- **CPU broadphase** - SAP algorithm (9/10 instead of 10/10)
- **Rigid bodies only** - No soft bodies, no fluids
- **Simple IK** - 2-bone IK solver (5/10 difficulty)
- **No cloth simulation** - Maybe add later
- **Simple SIMD** - SSE3 instead of AVX-512
- **Standard threading** - Pthreads instead of fiber system

### Realistic Phase 1 & 2 Timeline with Reduced Scope:

**Phase 1: Core Infrastructure** (500 hours)
- Memory allocators ✅ (60 hours)
- Containers ✅ (100 hours)
- JSON serialization ✅ (80 hours)
- **Skip fiber system** (avoid 100 hours of assembly)

**Phase 2: Physics Engine** (400 hours)
- Constraint solver ✅ (150 hours)
- Broadphase SAP ✅ (50 hours)
- Character physics ✅ (100 hours)
- **Skip GPU broadphase** (avoid 120 hours)
- **Skip fluids** (avoid 450 hours)
- **Skip FEM** (avoid 200 hours)

**Total: ~900 hours (3 developers, ~3-4 months)**
vs. **2,100+ hours** with expert features

---

## Final Recommendation

**Focus on these 6 expert TODOs (if time permits):**
1. Lock-free ring buffer (40 hours) - Small and valuable
2. FABRIK 2-bone IK (30 hours) - Gameplay critical
3. Full-body IK (50 hours) - Polishing feature
4. Self-collision detection (80 hours) - Only if cloth needed
5. Fluid-rigid coupling (60 hours) - If fluid simulation chosen
6. GPU broadphase (100 hours) - Only if physics slow

**Total: ~360 hours** - Achievable with 1-2 senior engineers over 6-9 months

**Everything else:** Phase 3 and beyond (year 2+)

