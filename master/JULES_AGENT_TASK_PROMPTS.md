# Jules Agents - Specific Task Prompts & CSV References

**Purpose:** Each agent knows exactly what to do with specific prompts and CSV task IDs  
**Updated:** 2026-01-12

---

## Agent 1: Jules-Engine-Core

### Mission
Build and validate the engine's core infrastructure: lifecycle management, memory systems, threading, and utilities that all other systems depend on.

### Ownership
- `src/engine/core/` (198 files)
- `src/engine/ecs/` (11 files - entity component system)
- `src/engine/scene/` (23 files - scene management)

### Your Specific Tasks

#### Task 1: CORE-001 - Engine Initialization Pipeline ⭐ START HERE
**Status:** In master/todo.csv  
**CSV Reference:** Search for `CORE-001` in master/todo.csv

**Your Prompt:**
```
Verify the engine initialization and shutdown pipeline works end-to-end:
1. Open src/engine/core/engine.c
2. Trace the complete flow: engine_init() → engine_update() → engine_shutdown()
3. Verify all subsystem initializations happen in correct order
4. Test: Initialize engine, run 10 frames, shut down cleanly
5. Verify: Zero memory leaks using AddressSanitizer
6. Acceptance: Engine starts without errors, runs 10 frames clean, no dangling pointers

What's broken:
- Subsystem init order may be wrong
- Some subsystems may not have shutdown functions
- Potential memory leaks in cleanup

How to know you're done:
- Binary runs to completion with zero ASAN violations
- All systems report initialized/shutdown correctly
- Log shows clean lifecycle
```

**Update CSV when starting:**
```
CORE-001,implementation,Engine Core > Initialization,Jules-Engine-Core,Engine initialization pipeline validation,in_progress
```

**Update CSV when done:**
```
CORE-001,implementation,Engine Core > Initialization,Jules-Engine-Core,Engine initialization pipeline validation,completed
```

#### Task 2: CORE-002 - Memory Allocator Consolidation
**Status:** In master/todo.csv  
**CSV Reference:** Search for `CORE-002`

**Your Prompt:**
```
Consolidate all memory allocators into single unified system:
1. Find all allocator implementations in src/engine/core/common/memory/
2. Identify: allocator.c (main), any duplicates, device-specific allocators
3. Consolidate: Route all allocations through single allocator
4. Add tracking: Track allocation size, type, lifetime
5. Test: Allocate 100,000+ small/large objects, verify no fragmentation

What's broken:
- Multiple allocators may exist (temporary, static, dynamic)
- Fragmentation issues possible
- Memory tracking unclear

How to know you're done:
- All allocations use single allocator
- Memory profiler shows no fragmentation
- Leak detector catches all dangling pointers
```

#### Task 3: CORE-003 - Thread Pool Validation
**Your Prompt:**
```
Verify thread pool and job system work correctly:
1. Test src/engine/core/sync/thread_pool.c
2. Submit 100 jobs across all threads
3. Verify: Jobs execute in parallel, no deadlocks, proper synchronization
4. Test: Queue jobs faster than they execute, verify queue handling
5. Test: Shutdown pool, verify all jobs complete

Acceptance:
- Jobs execute in parallel on available cores
- No race conditions (test with TSan)
- No deadlocks even under heavy load
```

#### Task 4: CORE-004 - Virtual File System (VFS)
**Your Prompt:**
```
Verify VFS can load files and manage paths:
1. Test src/engine/core/resource/vfs.c
2. Mount multiple directories and packages
3. Load 50+ files from different mounts
4. Test async loading, file streaming
5. Test hot-reload file changes

Acceptance:
- Load files from VFS paths without errors
- Path handling works on Windows/Mac/Linux
- Async I/O doesn't stall main thread
```

#### Task 5: CORE-005 - Logging Consolidation
**Your Prompt:**
```
Consolidate logging backends:
1. Find all logger implementations (src/engine/core/logger.c)
2. Verify single logging system used everywhere
3. Test log levels: DEBUG, INFO, WARN, ERROR
4. Test log output: console, file, debugger
5. Performance: Logging doesn't drop frames

Acceptance:
- All subsystems use same logger interface
- Log output consistent across modules
- Performance impact < 0.5ms per frame
```

#### Task 6: CORE-006 - Hot Reload Infrastructure
**Your Prompt:**
```
Verify code hot reload works:
1. Implement runtime function/data reloading
2. Test: Modify simple subsystem, reload without crash
3. Test: Reload preserves state (entities, config)
4. Test: Reload fails gracefully if incompatible

Acceptance:
- Can reload non-critical code during development
- State preserved across reload
- Graceful error on incompatible changes
```

### Phase & Dependencies
- **Phase:** PHASE 1 (Start immediately)
- **Duration:** 2 weeks
- **Blocks:** All other 14 agents (they depend on this!)
- **Priority:** P0 CRITICAL

### Success Criteria
✅ Engine runs 10 clean frames  
✅ Zero memory leaks (ASAN clean)  
✅ No race conditions (TSan clean)  
✅ All tasks in CSV marked completed  
✅ Build compiles with -Wall -Werror

---

## Agent 2: Jules-Rendering

### Mission
Implement complete rendering pipeline with proper GPU memory management, shader handling, and cross-platform backend support (Metal/Vulkan/OpenGL).

### Ownership
- `src/engine/rendering/` (361 files)
- `src/engine/backend/` (30 files - Metal, Vulkan, OpenGL)
- `src/engine/postprocess/` (66 files)

### Your Specific Tasks

#### Task 1: RENDER-001 - Renderer Initialization
**Status:** In master/todo.csv  
**CSV Reference:** Search for `RENDER-001`

**Your Prompt:**
```
Get the renderer working and rendering to screen:
1. Verify src/engine/rendering/renderer.c renderer_init() works
2. Initialize graphics backend (Metal on macOS, Vulkan/OpenGL on Linux)
3. Create render window and swap chain
4. Compile basic shader (triangle shader)
5. Test: Render single triangle to screen at 60 FPS

What's broken:
- Backend initialization may be incomplete
- Shader compilation may fail
- Swap chain may not synchronize properly

Acceptance:
- Triangle visible on screen
- Renders at 60 FPS (16.67ms per frame)
- Works on all target platforms
```

**Update CSV:**
```
RENDER-001,implementation,Rendering > Initialization,Jules-Rendering,Renderer initialization and setup,in_progress
```

#### Task 2: RENDER-002 - Shader Compilation
**Your Prompt:**
```
Consolidate and fix shader compilation system:
1. Find shader compiler in src/engine/shading/ and src/engine/rendering/
2. Verify single compilation pipeline (no duplicates)
3. Test: Load 50+ shaders, handle compilation errors gracefully
4. Implement hot-reload: Modify shader, reload without restarting engine
5. Test: Shader errors reported clearly in log

Acceptance:
- All shaders compile without warnings
- Hot-reload works (< 1 second reload time)
- Error messages helpful for debugging
- Performance: Shader compilation doesn't block rendering
```

#### Task 3: RENDER-003 - Frame Graph Execution
**Your Prompt:**
```
Implement frame graph scheduling:
1. Design frame graph (render passes in order)
2. Implement pass dependencies and scheduling
3. Test: Execute 10+ render passes in correct dependency order
4. Verify GPU/CPU synchronization (no stalls)
5. Profile: Measure frame composition time

Acceptance:
- Render passes execute in correct order
- No GPU/CPU stalls
- Frame composition < 5ms
```

#### Task 4-7: Continue with GPU Memory, Vulkan/Metal, Post-Processing...

### Phase & Dependencies
- **Phase:** PHASE 1 (Start immediately, parallel with Engine-Core and Physics)
- **Duration:** 2 weeks
- **Blocks:** Materials, Lighting, Effects, Tools
- **Priority:** P0 CRITICAL

---

## Agent 3: Jules-Physics

### Mission
Implement robust physics simulation with deterministic behavior for networking and multiplayer support.

### Ownership
- `src/engine/physics/` (224 files)

### Your Specific Tasks

#### Task 1: PHYS-001 - Physics Solver Consolidation
**Your Prompt:**
```
Choose and consolidate physics solver:
1. Compare implementations: src/engine/physics/solver/xpbd_solver.c vs sequential_impulse.c
2. Identify which is more stable/performant
3. Consolidate into single canonical solver
4. Test: Simulate 500 rigid bodies
5. Benchmark: Achieve 60 FPS with 1000+ bodies

What's broken:
- May have duplicate solvers
- One may be disabled or incomplete
- Performance may not scale

Acceptance:
- Single unified solver (no duplication)
- 1000+ bodies at 60 FPS
- No instability (bodies don't jitter)
```

**Update CSV:**
```
PHYS-001,implementation,Physics > Solver,Jules-Physics,Physics solver consolidation,in_progress
```

#### Task 2: PHYS-002 - Collision Detection
**Your Prompt:**
```
Verify collision detection works accurately:
1. Test broad phase (BVH or SAP)
2. Test narrow phase (GJK, contact manifold)
3. Stack 100 boxes, verify no penetration
4. Test edge cases (thin objects, fast objects, stacks)
5. Profile: Broad phase < 2ms, narrow phase < 3ms

Acceptance:
- No missed collisions
- No false positives
- Stack test stable
- Performance within budget
```

#### Task 3: PHYS-003 - Constraint Solving
**Your Prompt:**
```
Verify joint constraints work:
1. Test hinges (doors, wheels)
2. Test ball joints (rope, chains)
3. Test sliders (pistons, drawers)
4. Build complex mechanism, verify stability
5. Test constraint error < 0.1% tolerance

Acceptance:
- Constraints maintain position/rotation
- No jitter or instability
- Can build chains 10+ bodies long
```

#### Tasks 4-6: Continue with Determinism, Optimization, CCD...

### Phase & Dependencies
- **Phase:** PHASE 1 (Start immediately, parallel with Engine-Core and Rendering)
- **Duration:** 2 weeks
- **Blocks:** AI, Networking, Gameplay, Character, Effects
- **Priority:** P0 CRITICAL

---

## Agent 4: Jules-Animation

### Mission
Implement skeletal animation, blending, IK, and advanced features like motion matching.

### Ownership
- `src/engine/animation/` (55 files)

### Your Specific Tasks

#### Task 1: ANIM-001 - Skeletal Animation
**Your Prompt:**
```
Verify skeletal animation works:
1. Load character skeleton from src/engine/animation/skeleton.c
2. Load animation clips from FBX/glTF
3. Sample animations at different playback speeds
4. Test: Play walk, run, jump animations
5. Test: Switch between animations smoothly

What's broken:
- Skeleton may not load properly
- Animation sampling may skip keyframes
- Transitions may be jerky

Acceptance:
- Character animates smoothly
- 10+ different animations play correctly
- Playback speed adjustable
- Smooth animation blending
```

#### Task 2: ANIM-002 - Animation State Machine
**Your Prompt:**
```
Implement animation state transitions:
1. Create state machine in src/engine/animation/state_machines/
2. Define states: Idle, Walk, Run, Jump, Land
3. Implement transitions with conditions
4. Test: Character transitions walk → run → jump → land → idle smoothly

Acceptance:
- State transitions smooth (blend over 0.2-0.5 seconds)
- Correct animations play for each state
- Conditions trigger transitions correctly
```

#### Tasks 3-5: Continue with IK, Motion Matching, Procedural...

### Phase & Dependencies
- **Phase:** PHASE 2 (Start Week 3, depends on Engine-Core completion)
- **Duration:** 1-2 weeks
- **Blocks:** Character, Gameplay
- **Priority:** P1 HIGH

---

## Agent 5: Jules-Audio

### Mission
Implement spatial audio, music systems, and DSP effects.

### Ownership
- `src/engine/audio/` (67 files)

### Your Specific Tasks

#### Task 1: AUDIO-001 - Audio Playback
**Your Prompt:**
```
Get audio playing:
1. Initialize audio system in src/engine/audio/audio_system.c
2. Load WAV and OGG files
3. Play 20 simultaneous audio streams
4. Test: Volume, pitch, panning controls
5. Verify audio output to speakers

What's broken:
- Audio system may not initialize
- File format support may be incomplete
- Mixing may have artifacts

Acceptance:
- Audio plays without crackling/popping
- 20+ simultaneous sounds at full quality
- Volume/pitch controls work
```

#### Tasks 2-4: Spatial audio, Music streaming, DSP effects...

### Phase & Dependencies
- **Phase:** PHASE 2 (Start Week 3, depends on Engine-Core)
- **Blocks:** Character, Effects
- **Priority:** P1 HIGH

---

## Agents 6-15 (Continued...)

[Similar detailed prompts for each remaining agent]

---

## CSV Integration

### How to Update master/todo.csv

When you start a task:
```csv
AGENT-###,implementation,Category,Jules-AgentName,Task description,in_progress
```

When you complete a task:
```csv
AGENT-###,implementation,Category,Jules-AgentName,Task description,completed
```

### Example Entry
```csv
CORE-001,implementation,Engine Core > Initialization,Jules-Engine-Core,Engine initialization pipeline validation,in_progress
```

**Fields:**
- ID: Task identifier (CORE-001, RENDER-001, etc)
- Type: "implementation" or "bug_fix"
- Category: Subsystem path (Engine Core > Initialization)
- Source: Agent name
- Description: What you're doing
- Status: pending/in_progress/completed

### Weekly Status
Run this to see all YOUR agent's tasks:
```bash
grep "Jules-Engine-Core" master/todo.csv
```

This shows all your tasks and their status.

---

## Your Daily Workflow

### Each Morning:
1. Check this document for your current task
2. Review the **Your Prompt** section
3. Open the referenced files
4. Read the **Acceptance Criteria**
5. Start implementing

### As You Work:
1. Update CSV to `in_progress` if not already
2. Implement according to prompt
3. Test against acceptance criteria
4. Commit frequently with task ID reference

### When Complete:
1. Update CSV to `completed`
2. Add completion note to master/todo.md with test results
3. Commit final changes
4. Move to next task

### Each Friday:
1. Report progress to team
2. Update % complete for your agent
3. Flag any blockers
4. Adjust next week's plan if needed

---

## Quick Task Reference

### All 65+ Tasks in One Place

**PHASE 1 Tasks (15 critical):**
- CORE-001 through CORE-006 (6 tasks)
- RENDER-001 through RENDER-007 (7 tasks)
- PHYS-001 through PHYS-006 (6 tasks)

**PHASE 2 Tasks (20 tasks):**
- ANIM-001 through ANIM-005 (5 tasks)
- AUDIO-001 through AUDIO-004 (4 tasks)
- AI-001 through AI-004 (4 tasks)
- ASSET-001 through ASSET-004 (4 tasks)
- NET-001 through NET-004 (4 tasks)

**PHASE 3 Tasks (24 tasks):**
- GAME-001 through GAME-005 (5 tasks)
- CHAR-001 through CHAR-004 (4 tasks)
- GEO-001 through GEO-004 (4 tasks)
- MAT-001 through MAT-004 (4 tasks)
- LIGHT-001 through LIGHT-004 (4 tasks)
- FX-001 through FX-004 (4 tasks)

**PHASE 4 Tasks (6 tasks):**
- TOOLS-001 through TOOLS-006 (6 tasks)

**Total: 65+ critical tasks**

---

## Never Forget

- ✅ Update CSV when starting (in_progress)
- ✅ Update CSV when done (completed)
- ✅ Test against acceptance criteria
- ✅ Commit with task ID reference
- ✅ Report blockers immediately
- ✅ Stay in your agent's directory
- ✅ Don't overlap with other agents

---

End of Jules Agent Task Prompts
