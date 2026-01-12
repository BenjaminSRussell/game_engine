# Jules 15-Phase Agent Task Prompts

**Purpose:** This file contains specific, actionable prompts for each of the 15 agents in the Jules 15-Phase Development System.

**How to Use:**
1. Find your agent section below
2. Read each task prompt carefully
3. Follow the step-by-step instructions exactly
4. Update `master/todo.csv` as you complete tasks
5. Report progress daily to your agent lead

---

## 🎯 AGENT 1: Jules-Engine-Core (Phase 1)

### CORE-001: Engine Initialization Pipeline ⭐ START HERE

**Your Prompt:**
You are tasked with implementing the engine initialization pipeline. This is the foundation upon which all other systems depend.

**Files to Focus On:**
- `src/engine/core/engine.c` - Main engine initialization
- `src/engine/core/engine.h` - Engine interface
- `src/engine/core/init.c` - Initialization subsystems
- `src/engine/core/shutdown.c` - Cleanup procedures

**Step-by-Step Instructions:**
1. **Read Current Implementation:** Examine existing engine initialization code
2. **Identify Missing Components:** Check what's needed for complete initialization
3. **Implement Initialization Sequence:** Create proper startup order:
   - Memory allocator initialization
   - Logging system setup
   - Thread pool creation
   - Virtual file system mount
   - Hot reload infrastructure
4. **Add Validation:** Ensure each step succeeds before proceeding
5. **Implement Shutdown Sequence:** Clean reverse order of initialization
6. **Add Error Handling:** Graceful failure with detailed error messages

**Acceptance Criteria:**
- Engine initializes → runs 10 frames clean → shuts down with zero leaks
- All subsystems initialize in correct order
- ASAN/UBSAN report zero errors during startup/shutdown
- Build produces zero warnings

**Test Command:**
```bash
# Build and test initialization
cmake --build . -j4
./simple_working_main
# Should run 10 frames and exit cleanly
```

**Update CSV:** Mark CORE-001 as "in_progress" when starting, "completed" when done.

---

### CORE-002: Memory Allocator Consolidation

**Your Prompt:**
Consolidate all memory allocation into a unified system with fragmentation prevention and performance tracking.

**Files to Focus On:**
- `src/engine/core/memory.c` - Main memory allocator
- `src/engine/core/allocator.c` - Allocation strategies
- `src/engine/core/pool.c` - Memory pools
- `src/engine/core/tracking.c` - Memory usage tracking

**Step-by-Step Instructions:**
1. **Audit Current Allocators:** Find all malloc/free usage in the codebase
2. **Design Unified Interface:** Create consistent allocation API
3. **Implement Allocation Strategies:**
   - Pool allocation for small objects
   - Arena allocation for temporary data
   - Heap allocation for large objects
4. **Add Fragmentation Prevention:** Implement defragmentation
5. **Add Performance Tracking:** Track allocation patterns and metrics
6. **Replace All malloc/free:** Update entire codebase to use unified allocator

**Acceptance Criteria:**
- Memory allocator consolidated, no fragmentation
- 10000+ allocations without performance degradation
- Memory usage tracking accurate
- Zero memory leaks in ASAN

**Test Command:**
```bash
# Test memory allocator
./advanced_physics_enhancements.c
# Monitor memory usage, should be stable
```

---

### CORE-003: Thread Pool Validation

**Your Prompt:**
Implement and validate a thread pool system that can execute 100+ jobs without deadlocks.

**Files to Focus On:**
- `src/engine/core/thread_pool.c` - Thread pool implementation
- `src/engine/core/jobs.c` - Job system
- `src/engine/core/work_queue.c` - Work distribution

**Step-by-Step Instructions:**
1. **Implement Thread Pool:** Create worker threads and job queue
2. **Add Job Types:** Support different job priorities and dependencies
3. **Implement Work Stealing:** Load balancing across threads
4. **Add Deadlock Prevention:** Proper dependency tracking
5. **Create Stress Test:** 100+ concurrent jobs test
6. **Validate Performance:** Measure throughput and latency

**Acceptance Criteria:**
- Thread pool executes 100+ jobs without deadlocks
- Proper load balancing across CPU cores
- Job dependencies resolve correctly
- No race conditions or data corruption

---

### CORE-004: Virtual File System

**Your Prompt:**
Implement a virtual file system that can load 50+ files correctly with proper path resolution.

**Files to Focus On:**
- `src/engine/core/vfs.c` - Virtual file system
- `src/engine/core/file_io.c` - File I/O operations
- `src/engine/core/path.c` - Path handling

**Step-by-Step Instructions:**
1. **Design VFS Architecture:** Mount points, path resolution
2. **Implement File Loading:** Support multiple file formats
3. **Add Caching System:** In-memory file cache
4. **Implement Hot Reload:** File change detection
5. **Add Async Loading:** Background file loading
6. **Create Test Suite:** Load 50+ different files

**Acceptance Criteria:**
- Virtual file system loads 50+ files correctly
- Path resolution works across platforms
- Hot reload updates files without restart
- Async loading doesn't block main thread

---

### CORE-005: Logging & Debug System

**Your Prompt:**
Create a unified logging and debug system with multiple output channels and severity levels.

**Files to Focus On:**
- `src/engine/core/logging.c` - Logging system
- `src/engine/core/debug.c` - Debug utilities
- `src/engine/core/assert.c` - Assertion system

**Step-by-Step Instructions:**
1. **Implement Logging Interface:** Multiple log levels and channels
2. **Add Output Targets:** Console, file, network logging
3. **Implement Debug Drawing:** In-engine debug visualization
4. **Add Performance Profiling:** Frame time and function profiling
5. **Create Debug Console:** In-game command interface
6. **Test All Features:** Verify logging works under all conditions

**Acceptance Criteria:**
- Logging system unified and working
- Debug visualization renders correctly
- Performance profiling provides useful metrics
- Debug console accepts commands

---

### CORE-006: Hot Reload Infrastructure

**Your Prompt:**
Implement hot reload infrastructure for assets, shaders, and code changes.

**Files to Focus On:**
- `src/engine/core/hot_reload.c` - Hot reload system
- `src/engine/core/file_watcher.c` - File change detection
- `src/engine/core/reload_manager.c` - Reload coordination

**Step-by-Step Instructions:**
1. **Implement File Watching:** Detect file changes efficiently
2. **Create Reload Pipeline:** Coordinate reload across systems
3. **Add Asset Reloading:** Update assets without restart
4. **Implement Shader Reloading:** Recompile shaders on change
5. **Add Code Reloading:** Dynamic library reloading
6. **Test All Scenarios:** Verify hot reload works for all types

**Acceptance Criteria:**
- Hot reload infrastructure operational
- Assets reload without game restart
- Shaders recompile and apply correctly
- Code changes take effect without restart

---

## 🎯 AGENT 2: Jules-Rendering (Phase 2)

### RENDER-001: Renderer Initialization ⭐ START HERE

**Your Prompt:**
Initialize the complete rendering pipeline with support for multiple backends.

**Files to Focus On:**
- `src/engine/rendering/renderer.c` - Main renderer
- `src/engine/rendering/init.c` - Renderer initialization
- `src/engine/backend/metal/metal_renderer.m` - Metal backend
- `src/engine/backend/vulkan/vulkan_renderer.c` - Vulkan backend

**Step-by-Step Instructions:**
1. **Implement Renderer Interface:** Unified API across backends
2. **Initialize Metal Backend:** Complete Metal implementation
3. **Initialize Vulkan Backend:** Full Vulkan support
4. **Add Backend Selection:** Runtime backend switching
5. **Implement Resource Management:** GPU resource lifecycle
6. **Create Validation Tests:** Ensure all backends work

**Acceptance Criteria:**
- Triangle renders on screen at 60 FPS on all platforms
- All backends initialize correctly
- Resource management prevents leaks
- Backend switching works seamlessly

---

### RENDER-002: Shader System Consolidation

**Your Prompt:**
Consolidate shader compilation and management across all backends.

**Files to Focus On:**
- `src/engine/shading/shader.c` - Shader system
- `src/engine/shading/compiler.c` - Shader compilation
- `src/engine/shading/hot_reload.c` - Shader hot reload

**Step-by-Step Instructions:**
1. **Unify Shader API:** Common interface across backends
2. **Implement Compilation Pipeline:** Cross-platform shader compilation
3. **Add Hot Reload:** Live shader updates
4. **Implement Caching:** Avoid redundant compilation
5. **Add Validation:** Shader error reporting
6. **Test All Shaders:** Verify compilation and execution

**Acceptance Criteria:**
- Shader compilation pipeline unified, hot-reload working
- All shader types compile correctly
- Hot reload updates without artifacts
- Compilation errors reported clearly

---

### RENDER-003: Frame Graph Execution

**Your Prompt:**
Implement frame graph scheduling for optimal render pass execution.

**Files to Focus On:**
- `src/engine/rendering/frame_graph.c` - Frame graph system
- `src/engine/rendering/render_pass.c` - Render passes
- `src/engine/rendering/scheduler.c` - Pass scheduling

**Step-by-Step Instructions:**
1. **Design Frame Graph:** Dependency-based pass scheduling
2. **Implement Render Passes:** All required render passes
3. **Add Scheduling Logic:** Optimal pass execution order
4. **Implement Resource Barriers:** GPU synchronization
5. **Add Performance Tracking:** Frame time analysis
6. **Validate Execution:** Ensure all passes execute correctly

**Acceptance Criteria:**
- Frame graph scheduling correct, all passes execute in order
- Resource barriers prevent race conditions
- Performance is optimal for target hardware
- No GPU stalls or synchronization issues

---

### RENDER-004: GPU Memory Management

**Your Prompt:**
Implement efficient GPU memory management with fragmentation prevention.

**Files to Focus On:**
- `src/engine/rendering/gpu_memory.c` - GPU memory manager
- `src/engine/rendering/buffer_pool.c` - Buffer management
- `src/engine/rendering/texture_pool.c` - Texture management

**Step-by-Step Instructions:**
1. **Implement Memory Allocator:** GPU memory allocation strategies
2. **Add Buffer Management:** Vertex/index/uniform buffers
3. **Implement Texture Management:** Texture allocation and streaming
4. **Add Defragmentation:** GPU memory compaction
5. **Implement Memory Tracking:** Usage statistics
6. **Stress Test:** 10000+ allocations test

**Acceptance Criteria:**
- GPU memory management: 10000+ allocations without fragmentation
- Memory usage is optimal for target hardware
- No memory leaks or corruption
- Performance scales with allocation count

---

### RENDER-005: Vulkan Backend Stability

**Your Prompt:**
Ensure Vulkan backend is fully stable or cleanly removed if not needed.

**Files to Focus On:**
- `src/engine/backend/vulkan/vulkan_renderer.c` - Main Vulkan implementation
- `src/engine/backend/vulkan/vulkan_utils.c` - Vulkan utilities
- `src/engine/backend/vulkan/vulkan_sync.c` - Synchronization

**Step-by-Step Instructions:**
1. **Audit Vulkan Implementation:** Find all stability issues
2. **Fix Validation Errors:** Address all Vulkan validation warnings
3. **Implement Missing Features:** Complete Vulkan feature set
4. **Add Error Handling:** Graceful Vulkan error recovery
5. **Performance Optimization:** Ensure competitive performance
6. **Decision Point:** Either fully stabilize or remove cleanly

**Acceptance Criteria:**
- Vulkan backend either fully working or cleanly removed
- Zero validation errors
- Performance comparable to other backends
- Clean removal if chosen (no broken references)

---

### RENDER-006: Metal Backend Synchronization

**Your Prompt:**
Optimize Metal backend synchronization to prevent GPU stalls.

**Files to Focus On:**
- `src/engine/backend/metal/metal_renderer.m` - Metal renderer
- `src/engine/backend/metal/metal_sync.m` - Metal synchronization
- `src/engine/backend/metal/metal_command_buffer.m` - Command buffers

**Step-by-Step Instructions:**
1. **Analyze GPU Stalls:** Identify synchronization bottlenecks
2. **Optimize Command Buffers:** Efficient command buffer usage
3. **Implement Proper Fencing:** GPU-CPU synchronization
4. **Add Resource Tracking:** Resource state management
5. **Optimize Memory Usage:** Metal-specific memory optimizations
6. **Validate Performance:** Ensure 60 FPS target

**Acceptance Criteria:**
- Metal backend synchronized, no GPU stalls
- Command buffers execute efficiently
- Resource state transitions are optimal
- Performance meets 60 FPS target

---

### RENDER-007: Post-Processing Pipeline

**Your Prompt:**
Implement complete post-processing pipeline with advanced effects.

**Files to Focus On:**
- `src/engine/postprocess/post_process.c` - Post-processing system
- `src/engine/postprocess/taa.c` - Temporal anti-aliasing
- `src/engine/postprocess/ssao.c` - Screen space ambient occlusion
- `src/engine/postprocess/ssr.c` - Screen space reflections

**Step-by-Step Instructions:**
1. **Implement TAA:** Temporal anti-aliasing with jitter management
2. **Add SSAO:** Screen space ambient occlusion
3. **Implement SSR:** Screen space reflections
4. **Add Bloom:** Bloom and tone mapping
5. **Implement Color Grading:** Final color corrections
6. **Optimize Performance:** Ensure post-processing fits budget

**Acceptance Criteria:**
- Post-processing effects render correctly (TAA, SSAO, SSR)
- All effects work together without conflicts
- Performance impact is minimal
- Visual quality meets target standards

---

## 🎯 AGENT 3: Jules-Physics (Phase 3)

### PHYS-001: Rigid Body System ⭐ START HERE

**Your Prompt:**
Implement a robust rigid body physics system supporting various shapes and masses.

**Files to Focus On:**
- `src/engine/physics/rigid_body.c` - Rigid body implementation
- `src/engine/physics/collision_shapes.c` - Collision shapes
- `src/engine/physics/physics_world.c` - Physics simulation

**Step-by-Step Instructions:**
1. **Implement Rigid Body Class:** Position, velocity, mass, inertia
2. **Add Collision Shapes:** Box, sphere, capsule, mesh
3. **Implement Forces and Torques:** Apply forces to bodies
4. **Add Integration:** Update position/velocity over time
5. **Implement Broad Phase:** Efficient collision detection
6. **Test Performance:** 1000+ bodies at 60 FPS

**Acceptance Criteria:**
- 1000+ physics objects simulating at 60 FPS without jitter
- All collision shapes work correctly
- Forces and torques apply properly
- No performance degradation with object count

---

### PHYS-002: Collision Detection Pipeline

**Your Prompt:**
Implement accurate collision detection with no tunneling artifacts.

**Files to Focus On:**
- `src/engine/physics/collision/collision_detection.c` - Main collision system
- `src/engine/physics/collision/broad_phase.c` - Broad phase detection
- `src/engine/physics/collision/narrow_phase.c` - Narrow phase detection

**Step-by-Step Instructions:**
1. **Implement Broad Phase:** Spatial partitioning for efficiency
2. **Add Narrow Phase:** Accurate collision detection
3. **Implement Continuous Collision:** Prevent tunneling
4. **Add Collision Filtering:** Layer-based collision control
5. **Optimize Performance:** Ensure real-time performance
6. **Validate Accuracy:** No false positives/negatives

**Acceptance Criteria:**
- Collision detection accurate with no tunneling
- Performance scales with object count
- Collision filtering works correctly
- Continuous collision prevents fast object tunneling

---

### PHYS-003: Constraint Solver

**Your Prompt:**
Implement a constraint solver for joints and connections between bodies.

**Files to Focus On:**
- `src/engine/physics/constraints/constraint_solver.c` - Main solver
- `src/engine/physics/constraints/joint.c` - Joint implementations
- `src/engine/physics/constraints/solver_iter.c` - Solver iterations

**Step-by-Step Instructions:**
1. **Implement Constraint Types:** Fixed, hinge, ball-socket joints
2. **Add Solver Iterations:** Gauss-Seidel solver
3. **Implement Joint Limits:** Rotation and translation limits
4. **Add Motor Forces:** Powered joints
5. **Optimize Convergence:** Stable constraint resolution
6. **Test Stability:** Complex joint configurations

**Acceptance Criteria:**
- Constraints working: fixed, hinge, ball-socket
- Solver converges to stable state
- Joint limits enforced correctly
- Motor forces apply properly

---

### PHYS-004: XPBD Implementation

**Your Prompt:**
Implement Extended Position Based Dynamics for stable simulation.

**Files to Focus On:**
- `src/engine/physics/solver/xpbd_solver.c` - XPBD implementation
- `src/engine/physics/solver/position_based.c` - Position based dynamics
- `src/engine/physics/solver/damping.c` - Velocity damping

**Step-by-Step Instructions:**
1. **Implement XPBD Algorithm:** Position based simulation
2. **Add Constraint Projection:** Position correction
3. **Implement Velocity Update:** Velocity integration
4. **Add Damping:** Realistic motion damping
5. **Ensure Determinism:** Reproducible simulation
6. **Validate Stability:** Stable under all conditions

**Acceptance Criteria:**
- XPBD solver producing deterministic results
- Simulation is stable under stress
- Performance is real-time capable
- Results are reproducible across runs

---

### PHYS-005: Raycasting & Shape Casting

**Your Prompt:**
Implement accurate raycasting and shape casting for physics queries.

**Files to Focus On:**
- `src/engine/physics/raycasting/raycast.c` - Raycasting implementation
- `src/engine/physics/raycasting/shape_cast.c` - Shape casting
- `src/engine/physics/raycasting/query_filter.c` - Query filtering

**Step-by-Step Instructions:**
1. **Implement Raycasting:** Single ray intersection tests
2. **Add Shape Casting:** Sweep tests for volumes
3. **Implement Query Filtering:** Layer-based filtering
4. **Add Multiple Hits:** All intersections along ray
5. **Optimize Performance:** Efficient spatial queries
6. **Validate Accuracy:** Precise hit detection

**Acceptance Criteria:**
- Raycasting and shape casting accurate
- Query filtering works correctly
- Performance is real-time capable
- Hit detection is precise and reliable

---

### PHYS-006: Physics Debugging Visualization

**Your Prompt:**
Implement comprehensive debugging visualization for physics systems.

**Files to Focus On:**
- `src/engine/physics/debug/physics_debug.c` - Debug visualization
- `src/engine/physics/debug/debug_draw.c` - Debug drawing utilities
- `src/engine/physics/debug/physics_editor.c` - Physics editor tools

**Step-by-Step Instructions:**
1. **Implement Debug Drawing:** Render physics shapes
2. **Add Contact Visualization:** Show collision contacts
3. **Implement Force Visualization:** Display applied forces
4. **Add Performance Graphs:** Real-time performance metrics
5. **Create Debug UI:** Interactive physics controls
6. **Test All Features:** Comprehensive debug coverage

**Acceptance Criteria:**
- Physics debugging visualization comprehensive
- All physics state is visible
- Debug UI is intuitive and useful
- Performance impact is minimal

---

[Continue with remaining agents 4-15 in similar detailed format...]

---

## 📊 Task Status Tracking

**How to Update Progress:**
1. When starting a task: Update `master/todo.csv` status to "in_progress"
2. When completing a task: Update status to "completed"
3. When blocked: Update status to "blocked" and report to agent lead

**CSV Format:**
```
TASK-ID,type,category,agent,description,status
CORE-001,implementation,Engine Core,Jules-Engine-Core,Engine Initialization Pipeline,in_progress
```

**Daily Reporting:**
- Agent leads report progress to project lead
- Blockers escalated immediately
- Success criteria verified before marking complete

---

**System Ready.** All agents have detailed task prompts.
**Begin Phase 1.** Follow prompts exactly.
**Track Progress.** Update CSV daily.
**Report Blockers.** Escalate issues immediately.
