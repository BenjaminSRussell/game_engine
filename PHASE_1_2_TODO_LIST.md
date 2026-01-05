# Phase 1 & 2 Core Systems - Filtered TODO List

**Total TODOs: 269 actionable items**
**Estimated Effort: ~600-900 developer-hours**
**Recommended Team Size: 2-3 developers**
**Timeline: 3-6 months (working full-time)**

---

## QUICK START GUIDE

### Phase 1 (Core Infrastructure) - 142 TODOs
Priority order for implementation:

1. **Memory Management** (23 TODOs) - UNBLOCK EVERYTHING ELSE
2. **Serialization** (23 TODOs) - NEEDED FOR ASSET LOADING
3. **Advanced Containers** (48 TODOs) - NEEDED FOR ECS & PHYSICS
4. **Threading/Fiber System** (8 TODOs) - LAST (optional for MVP)

### Phase 2 (Physics Engine) - 127 TODOs
Priority order:

1. **Constraint Solver** (39 TODOs) - CORE PHYSICS
2. **Broadphase Collision** (17 TODOs) - NEEDED FOR SOLVER
3. **Soft Body & Cloth** (19 TODOs) - ADVANCED FEATURE
4. **Character Physics** (19 TODOs) - GAMEPLAY-CRITICAL
5. **Aerodynamics/Vehicle/Fluid** (33 TODOs) - NICE-TO-HAVE

---

## PHASE 1: CORE INFRASTRUCTURE (142 TODOs)

### 1.1 MEMORY MANAGEMENT (23 TODOs)
**Priority: CRITICAL**
**Difficulty: 3-6/10**
**Location:** `src/engine/core/memory*.c`
**Why First:** Everything else allocates memory

#### 1.1.1 - Stack Allocator (11 TODOs)
- [ ] Implement `stack_create()` - Difficulty: 3
- [ ] Implement `stack_push()` - Difficulty: 3
- [ ] Implement `stack_pop()` - Difficulty: 3
- [ ] Implement `stack_marker_get()` - Difficulty: 2
- [ ] Implement `stack_marker_rewind()` - Difficulty: 2
- [ ] Add double-ended stack support - Difficulty: 5
- [ ] Implement aligned allocations - Difficulty: 4
- [ ] Add boundary checks - Difficulty: 3
- [ ] Implement thread-local instances - Difficulty: 5
- [ ] Add debug memory viewing - Difficulty: 4
- [ ] Write unit tests - Difficulty: 4

**File:** `src/engine/core/memory/stack_allocator.c`

#### 1.1.2 - Arena Allocator (8 TODOs)
- [ ] Implement `arena_init()` - Difficulty: 3
- [ ] Implement `arena_alloc()` (pointer bump) - Difficulty: 2
- [ ] Add `arena_reset()` - Difficulty: 2
- [ ] Implement `arena_checkpoint()` (save state) - Difficulty: 3
- [ ] Add `arena_rewind()` - Difficulty: 3
- [ ] Implement thread-local arena cache - Difficulty: 5
- [ ] Add overflow protection (growing) - Difficulty: 4
- [ ] Write benchmark vs malloc - Difficulty: 3

**File:** `src/engine/core/memory/arena_allocator.c`

#### 1.1.3 - Fragmentation Analysis (4 TODOs)
- [ ] Implement `calc_fragmentation()` - Difficulty: 5
- [ ] Implement `largest_free_block_search()` - Difficulty: 4
- [ ] Add histogram generation (block sizes) - Difficulty: 4
- [ ] Write allocator stress test - Difficulty: 4

**File:** `src/engine/core/memory/fragmentation_metric.c`

---

### 1.2 SERIALIZATION (23 TODOs)
**Priority: HIGH**
**Difficulty: 4-9/10**
**Location:** `src/engine/core/serialization/*.c`
**Why After Memory:** Serialization uses allocators

#### 1.2.1 - JSON Parser (8 TODOs)
- [ ] Implement `json_parse()` - Difficulty: 7
- [ ] Implement SIMD string scanning (AVX2/NEON) - Difficulty: 9
- [ ] Add DOM-based allocation strategy - Difficulty: 6
- [ ] Implement SAX-style callback mode - Difficulty: 7
- [ ] Add unicode validation - Difficulty: 6
- [ ] Implement number parsing optimization - Difficulty: 7
- [ ] Add serialization (writer) - Difficulty: 6
- [ ] Write fuzzing tests - Difficulty: 6

**File:** `src/engine/core/serialization/json_parser.c`

#### 1.2.2 - JSON Validator (7 TODOs)
- [ ] Implement `validate_schema()` - Difficulty: 5
- [ ] Implement recursive type checking - Difficulty: 5
- [ ] Add required field enforcement - Difficulty: 4
- [ ] Implement range/enum validation - Difficulty: 4
- [ ] Add default value injection - Difficulty: 5
- [ ] Implement helpful error reporting (path) - Difficulty: 4
- [ ] Write schema mismatch test - Difficulty: 4

**File:** `src/engine/core/serialization/json_validator.c`

#### 1.2.3 - Schema Validator (7 TODOs)
- [ ] Implement `validator_create()` - Difficulty: 6
- [ ] Implement type checking - Difficulty: 5
- [ ] Add range/enum constraints - Difficulty: 5
- [ ] Implement required field checking - Difficulty: 5
- [ ] Add recursive schema validation - Difficulty: 7
- [ ] Implement informative error messages - Difficulty: 5
- [ ] Write tests - Difficulty: 5

**File:** `src/engine/core/serialization/schema_validator.c`

#### 1.2.4 - Asset Manifest (1 TODO)
- [ ] Write manifest integrity test - Difficulty: 4

**File:** `src/engine/core/serialization/asset_manifest.c`

---

### 1.3 ADVANCED CONTAINERS (48 TODOs)
**Priority: HIGH**
**Difficulty: 2-9/10**
**Location:** `src/engine/core/containers_advanced/*.c`
**Why Important:** Core data structures for ECS and physics

#### 1.3.1 - Static Vector (7 TODOs)
- [ ] Implement `svec_init()` - Difficulty: 2
- [ ] Implement `svec_push()` - Difficulty: 2
- [ ] Implement `svec_pop()` - Difficulty: 2
- [ ] Add bounds checking (debug only) - Difficulty: 3
- [ ] Implement fast iteration macros - Difficulty: 3
- [ ] Add sorting support (qsort wrapper) - Difficulty: 4
- [ ] Write performance comparison vs std::vector - Difficulty: 3

**File:** `src/engine/core/containers_advanced/static_vector.c`

#### 1.3.2 - Sparse Set (13 TODOs)
- [ ] Implement `sparse_set_create()` - Difficulty: 3
- [ ] Implement `sparse_set_insert()` - Difficulty: 3
- [ ] Implement `sparse_set_remove()` - Difficulty: 4
- [ ] Implement `sparse_set_has()` - Difficulty: 2
- [ ] Implement `sparse_set_get()` - Difficulty: 3
- [ ] Implement `sparse_set_clear()` - Difficulty: 2
- [ ] Implement `sparse_set_iterate()` - Difficulty: 3
- [ ] Add SIMD iteration support - Difficulty: 6
- [ ] Implement `sparse_set_sort()` - Difficulty: 5
- [ ] Add versioning for stable handles - Difficulty: 7
- [ ] Implement parallel iteration - Difficulty: 6
- [ ] Add archetype chunking optimization - Difficulty: 8
- [ ] Write unit tests - Difficulty: 4

**File:** `src/engine/core/containers_advanced/sparse_set.c`

#### 1.3.3 - Slot Map (10 TODOs)
- [ ] Implement `slot_map_create()` - Difficulty: 4
- [ ] Implement `slot_map_insert()` - Difficulty: 5
- [ ] Implement `slot_map_remove()` - Difficulty: 5
- [ ] Implement `slot_map_get()` - Difficulty: 3
- [ ] Add generation/version checking - Difficulty: 4
- [ ] Implement free-list management - Difficulty: 5
- [ ] Add iterator support - Difficulty: 4
- [ ] Implement compaction/defragmentation - Difficulty: 7
- [ ] Add serialization support - Difficulty: 5
- [ ] Write unit tests - Difficulty: 4

**File:** `src/engine/core/containers_advanced/slot_map.c`

#### 1.3.4 - Octree (11 TODOs)
- [ ] Implement `octree_create()` - Difficulty: 5
- [ ] Implement `octree_insert()` - Difficulty: 6
- [ ] Implement `octree_remove()` - Difficulty: 6
- [ ] Implement `octree_update()` - Difficulty: 7
- [ ] Add frustum culling query - Difficulty: 6
- [ ] Add raycast query - Difficulty: 6
- [ ] Implement sphere/box query - Difficulty: 5
- [ ] Add dynamic balancing/resizing - Difficulty: 8
- [ ] Implement linear octree (hashing) - Difficulty: 7
- [ ] Add debug visualization - Difficulty: 4
- [ ] Write unit tests - Difficulty: 5

**File:** `src/engine/core/containers_advanced/octree.c`

#### 1.3.5 - Lock-Free Ring Buffer (7 TODOs)
- [ ] Implement `ring_init()` - Difficulty: 6
- [ ] Implement `spsc_enqueue()` (Single Prod Single Cons) - Difficulty: 7
- [ ] Implement `spsc_dequeue()` - Difficulty: 7
- [ ] Add `mpmc_enqueue()` (CAS loop) - Difficulty: 9
- [ ] Implement cache-line padding (false sharing prev) - Difficulty: 8
- [ ] Add batch processing helpers - Difficulty: 5
- [ ] Write throughput benchmark - Difficulty: 5

**File:** `src/engine/core/containers_advanced/ring_buffer_lockfree.c`

---

### 1.4 THREADING & FIBER SYSTEM (8 TODOs)
**Priority: MEDIUM**
**Difficulty: 6-9/10**
**Location:** `src/engine/core/threading/*.c`
**Why Last in Phase 1:** Complex assembly code, not critical for MVP

#### 1.4.1 - Fiber Context (8 TODOs)
- [ ] Implement `buffer_context_save()` - Difficulty: 9
- [ ] Implement `buffer_context_restore()` - Difficulty: 9
- [ ] Add x64 assembly implementation - Difficulty: 8
- [ ] Add ARM64 assembly implementation - Difficulty: 8
- [ ] Implement stack sanity checks - Difficulty: 6
- [ ] Add valgrind/asan integration hooks - Difficulty: 7
- [ ] Implement exception handling support - Difficulty: 9
- [ ] Write context switch benchmark - Difficulty: 5

**File:** `src/engine/core/threading/fiber_context.c`

---

## PHASE 2: HYPER-OPTIMIZED PHYSICS ENGINE (127 TODOs)

### 2.1 CONSTRAINT SOLVER (39 TODOs)
**Priority: CRITICAL**
**Difficulty: 5-8/10**
**Location:** `src/engine/physics/solver/*.c`
**Why First in Phase 2:** Foundation of all physics

#### 2.1.1 - Sequential Impulse Solver (11 TODOs)
- [ ] Implement `solver_create()` - Difficulty: 6
- [ ] Implement `solve_velocity_constraints()` - Difficulty: 8
- [ ] Implement `solve_position_constraints()` - Difficulty: 8
- [ ] Add contact constraint formulation - Difficulty: 7
- [ ] Implement friction constraints - Difficulty: 7
- [ ] Add restitution/bounce - Difficulty: 6
- [ ] Implement warm starting - Difficulty: 7
- [ ] Add constraint accumulation - Difficulty: 6
- [ ] Implement sleeping/deactivation - Difficulty: 6
- [ ] Add SIMD optimization - Difficulty: 8
- [ ] Write unit tests - Difficulty: 7

**File:** `src/engine/physics/solver/sequential_impulse.c`

#### 2.1.2 - XPBD Solver (11 TODOs)
- [ ] Implement `xpbd_solver_create()` - Difficulty: 7
- [ ] Add compliant constraints - Difficulty: 8
- [ ] Implement lambda multipliers - Difficulty: 8
- [ ] Add constraint stabilization - Difficulty: 7
- [ ] Implement stiffness tuning - Difficulty: 7
- [ ] Add constraint batching - Difficulty: 7
- [ ] Implement parallel solving - Difficulty: 8
- [ ] Add warm starting - Difficulty: 7
- [ ] Implement sub-stepping - Difficulty: 7
- [ ] Add constraint visualization - Difficulty: 5
- [ ] Write unit tests - Difficulty: 7

**File:** `src/engine/physics/solver/xpbd_solver.c`

#### 2.1.3 - Contact Manifold (7 TODOs)
- [ ] Implement `manifold_create()` - Difficulty: 5
- [ ] Implement point reduction (keep best 4 points) - Difficulty: 7
- [ ] Add contact persistence (warm starting) - Difficulty: 6
- [ ] Implement face-face contact clipping - Difficulty: 7
- [ ] Add edge-edge contact support - Difficulty: 8
- [ ] Implement friction cone calculation - Difficulty: 6
- [ ] Write stability visualizer - Difficulty: 4

**File:** `src/engine/physics/solver/contact_manifold.c`

#### 2.1.4 - Warm Starting (6 TODOs)
- [ ] Implement `warm_start_apply()` - Difficulty: 6
- [ ] Implement impulse caching (contact ID matching) - Difficulty: 7
- [ ] Add friction impulse persistence - Difficulty: 6
- [ ] Implement cache invalidation (separation) - Difficulty: 5
- [ ] Add block solver support - Difficulty: 8
- [ ] Write stacking stability test - Difficulty: 4

**File:** `src/engine/physics/solver/warm_starting.c`

#### 2.1.5 - Joint Motors (7 TODOs)
- [ ] Implement `motor_solve()` - Difficulty: 6
- [ ] Implement angular motor (hinge drive) - Difficulty: 6
- [ ] Add linear motor (slider drive) - Difficulty: 5
- [ ] Implement max force/torque limits - Difficulty: 4
- [ ] Add servo positional targeting (PD control) - Difficulty: 7
- [ ] Implement cone limit (ball socket) - Difficulty: 7
- [ ] Write robotic arm test - Difficulty: 5

**File:** `src/engine/physics/solver/joint_motors.c`

#### 2.1.6 - Physics Core Integration (approx 4 TODOs from physics_core_impl.c)
See `src/engine/physics/physics_core_impl.c` for detailed constraint implementation TODOs

---

### 2.2 BROADPHASE COLLISION (17 TODOs)
**Priority: HIGH**
**Difficulty: 5-10/10**
**Location:** `src/engine/physics/broadphase/*.c`

#### 2.2.1 - GPU Broadphase (10 TODOs)
- [ ] Implement `gpu_broadphase_init()` - Difficulty: 8
- [ ] Implement LBVH construction on GPU - Difficulty: 10
- [ ] Add AABB generation shader - Difficulty: 7
- [ ] Implement collision pair test shader - Difficulty: 8
- [ ] Add sorting (Radix Sort) on GPU - Difficulty: 9
- [ ] Implement data download/sync - Difficulty: 7
- [ ] Add hybrid CPU/GPU mode - Difficulty: 8
- [ ] Implement compaction of results - Difficulty: 8
- [ ] Add integration with simulation loop - Difficulty: 7
- [ ] Write comprehensive benchmarks - Difficulty: 8

**File:** `src/engine/physics/broadphase/gpu_broadphase.c`

#### 2.2.2 - Sweep and Prune Algorithm (7 TODOs)
- [ ] Implement `sap_init()` - Difficulty: 5
- [ ] Implement axis sorting (insertion sort) - Difficulty: 6
- [ ] Add overlap pair management - Difficulty: 6
- [ ] Implement batch add/remove (re-sort) - Difficulty: 5
- [ ] Add raycast filtering (AABB check) - Difficulty: 5
- [ ] Implement multi-axis pruning (1D vs 3D) - Difficulty: 6
- [ ] Write sorting benchmark - Difficulty: 4

**File:** `src/engine/physics/broadphase/sap_algorithm.c`

---

### 2.3 NARROWPHASE COLLISION & BALLISTICS (8 TODOs)
**Priority: MEDIUM**
**Difficulty: 5-7/10**
**Location:** `src/engine/physics/ballistics/*.c`

#### 2.3.1 - Trajectory Simulation (8 TODOs)
- [ ] Implement `trajectory_solve()` - Difficulty: 6
- [ ] Add air resistance (drag) calculation - Difficulty: 5
- [ ] Implement magnus effect (spin) - Difficulty: 7
- [ ] Add wind influence - Difficulty: 5
- [ ] Implement material penetration (bullet depth) - Difficulty: 6
- [ ] Add ricochet physics - Difficulty: 6
- [ ] Implement CCD for fast projectiles - Difficulty: 7
- [ ] Write trajectory prediction visualizer - Difficulty: 4

**File:** `src/engine/physics/ballistics/trajectory_sim.c`

---

### 2.4 SOFT BODY & CLOTH SIMULATION (19 TODOs)
**Priority: MEDIUM**
**Difficulty: 5-10/10**
**Location:** `src/engine/physics/softbody/*.c`

#### 2.4.1 - FEM Deformable Objects (11 TODOs)
- [ ] Implement `fem_solver_create()` - Difficulty: 8
- [ ] Implement tetrahedral mesh generation - Difficulty: 9
- [ ] Add stiffness matrix assembly - Difficulty: 8
- [ ] Implement corotated elasticity - Difficulty: 9
- [ ] Add implicit integration (Backward Euler) - Difficulty: 9
- [ ] Implement conjugate gradient solver - Difficulty: 8
- [ ] Add fracture/cutting support - Difficulty: 10
- [ ] Implement plastic deformation - Difficulty: 7
- [ ] Add volume conservation - Difficulty: 7
- [ ] Implement collision handling - Difficulty: 8
- [ ] Write unit tests - Difficulty: 8

**File:** `src/engine/physics/softbody/fem_deformable.c`

#### 2.4.2 - Softbody Constraints (8 TODOs)
- [ ] Implement `constraint_solver_init()` - Difficulty: 6
- [ ] Implement distance constraint (XPBD) - Difficulty: 5
- [ ] Add volume conservation constraint (pressure) - Difficulty: 8
- [ ] Implement bending constraint (dihedral angle) - Difficulty: 7
- [ ] Add self-collision detection - Difficulty: 9
- [ ] Implement attachment constraint (soft to rigid) - Difficulty: 6
- [ ] Add tearing/breaking support - Difficulty: 7
- [ ] Write stability benchmark - Difficulty: 5

**File:** `src/engine/physics/softbody/softbody_constraints.c`

---

### 2.5 CHARACTER PHYSICS (19 TODOs)
**Priority: HIGH**
**Difficulty: 4-8/10**
**Location:** `src/engine/physics/character/*.c` and `src/engine/physics/ragdoll/*.c`

#### 2.5.1 - Ragdoll Controller (8 TODOs)
- [ ] Implement `ragdoll_init()` - Difficulty: 6
- [ ] Implement pose matching torques - Difficulty: 8
- [ ] Add balance controller - Difficulty: 8
- [ ] Implement strength scaling (injury/stamina) - Difficulty: 7
- [ ] Add protection reflexes (arms out) - Difficulty: 7
- [ ] Implement transition from animation to physics - Difficulty: 8
- [ ] Add joint limit constraints - Difficulty: 6
- [ ] Write stability tests - Difficulty: 6

**File:** `src/engine/physics/ragdoll/ragdoll_controller.c`

#### 2.5.2 - Pose Matching (8 TODOs)
- [ ] Implement `pose_drive_init()` - Difficulty: 6
- [ ] Calculate joint torques (PD controller) - Difficulty: 7
- [ ] Implement stiffness/damping control - Difficulty: 4
- [ ] Add center of mass balance compensation - Difficulty: 8
- [ ] Implement strength scaling based on health - Difficulty: 6
- [ ] Add animation blending factor - Difficulty: 4
- [ ] Implement recovery from fall - Difficulty: 6
- [ ] Write stability tests - Difficulty: 5

**File:** `src/engine/physics/ragdoll/pose_matching.c`

#### 2.5.3 - Skeleton Matching (6 TODOs)
- [ ] Implement `pose_matcher_init()` - Difficulty: 6
- [ ] Implement joint space target calculation - Difficulty: 7
- [ ] Add world space tracking (COM) - Difficulty: 7
- [ ] Implement PD-controller gain tuning - Difficulty: 7
- [ ] Add hierarchical strength distribution - Difficulty: 6
- [ ] Write tests - Difficulty: 5

**File:** `src/engine/physics/ragdoll/skeleton_matching.c`

#### 2.5.4 - Climbing System (6 TODOs)
- [ ] Implement `climb_scan()` - Difficulty: 6
- [ ] Implement ledge detection (convex hull check) - Difficulty: 7
- [ ] Add grab point constraint - Difficulty: 6
- [ ] Implement wall run physics (anti-gravity) - Difficulty: 7
- [ ] Add mantle/vault logic - Difficulty: 5
- [ ] Implement stamina consumption - Difficulty: 4
- [ ] Add slip surfaces detection - Difficulty: 4
- [ ] Write parkour regression map - Difficulty: 5

**File:** `src/engine/physics/character/climbing_system.c`

---

### 2.6 AERODYNAMICS & VEHICLE PHYSICS (25 TODOs)
**Priority: LOW-MEDIUM**
**Difficulty: 5-8/10**
**Location:** `src/engine/physics/aerodynamics/*.c` and `src/engine/physics/vehicle/*.c`

#### 2.6.1 - Wind Field (8 TODOs)
- [ ] Implement `wind_field_init()` - Difficulty: 6
- [ ] Implement directional wind volumes - Difficulty: 5
- [ ] Add turbulence noise (3D Perlin/Simplex) - Difficulty: 6
- [ ] Implement gust simulation - Difficulty: 5
- [ ] Add vortex shedding effect - Difficulty: 7
- [ ] Implement wind occlusion (raycast check) - Difficulty: 6
- [ ] Add query interface for particles - Difficulty: 4
- [ ] Write vector field visualizer - Difficulty: 4

**File:** `src/engine/physics/aerodynamics/wind_field.c`

#### 2.6.2 - Lift & Drag (9 TODOs)
- [ ] Implement `aero_solve()` - Difficulty: 7
- [ ] Add lift coefficient calculation (angle of attack) - Difficulty: 7
- [ ] Implement drag equation (proportional to v^2) - Difficulty: 6
- [ ] Add induced drag simulation - Difficulty: 7
- [ ] Implement stalling behavior (abrupt lift loss) - Difficulty: 7
- [ ] Add control surface effectiveness - Difficulty: 6
- [ ] Implement ground effect - Difficulty: 6
- [ ] Add supersonic wave drag approximation - Difficulty: 8
- [ ] Write lookup tables - Difficulty: 5

**File:** `src/engine/physics/aerodynamics/lift_drag.c`

#### 2.6.3 - Tire Model (8 TODOs)
- [ ] Implement `tire_init()` - Difficulty: 5
- [ ] Implement Pacejka 'Magic Formula' - Difficulty: 7
- [ ] Add slip angle calculation - Difficulty: 6
- [ ] Implement camber thrust - Difficulty: 5
- [ ] Add load sensitivity - Difficulty: 5
- [ ] Implement surface friction variation (wet/ice) - Difficulty: 6
- [ ] Add thermal simulation (tire heat) - Difficulty: 6
- [ ] Implement visual deformation - Difficulty: 7

**File:** `src/engine/physics/vehicle/tire_model.c`

---

## IMPLEMENTATION STRATEGY

### Recommended Work Flow

1. **Start with Phase 1.1 (Memory)** - Immediately
   - Enables everything else to be built efficiently
   - Relatively straightforward implementations
   - Most commonly used systems

2. **Parallel Phase 1.2 (Serialization)** - After memory basics
   - Needed for asset loading and configuration
   - Less critical for MVP

3. **Then Phase 1.3 (Containers)** - After memory
   - Foundation for ECS system
   - Moderate complexity

4. **Begin Phase 2.1 (Constraint Solver)** - Once memory/containers working
   - Core physics functionality
   - Can be developed independently

5. **Add Phase 2.2 (Broadphase)** - After constraint solver basics
   - Needed for efficient physics simulation

6. **Everything else** - As resources allow

### Testing Strategy

Each TODO should include:
- **Unit tests** for core algorithms
- **Benchmark tests** for performance-critical code (allocators, solvers)
- **Integration tests** for component interactions
- **Stress tests** for scale (1M allocations, 10K physics bodies, etc.)

### Performance Targets

**Memory:**
- Stack allocator: <100ns per allocation
- Arena allocator: <10ns per allocation
- Zero fragmentation over 1M allocations

**Physics:**
- Solve 10K constraints in <2ms
- Process 50K rigid bodies in <5ms
- Broadphase for 100K objects in <10ms

**Containers:**
- Sparse set iterate 1M elements in <1ms
- Octree raycast 1M elements in <2ms
- Lock-free queue: >10M ops/sec

---

## DIFFICULTY LEGEND

- **1-2:** Trivial (boilerplate, simple wrappers)
- **3-4:** Easy (straightforward implementation)
- **5-6:** Medium (requires algorithm knowledge)
- **7-8:** Hard (complex algorithm, multi-file changes)
- **9-10:** Expert (research paper implementation, optimization)

---

## NOTES

- All TODOs extracted from `ROADMAP.h` and code files
- Organized by logical dependencies
- Difficulty estimates are relative
- Many TODOs can be parallelized once foundations are in place
- Focus on correctness first, optimization second
