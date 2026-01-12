# Jules 15-Phase Development System

**Strategy:** One dedicated phase per agent. Each agent owns a complete phase where they deliver all their tasks end-to-end.

**Timeline:** ~15-16 weeks (phases can overlap where dependencies allow)

**Structure:**
- Tier 1 Phases (1-3): Must complete in order - foundation systems
- Tier 2 Phases (4-8): Can start after Phase 3 complete - core gameplay systems
- Tier 3 Phases (9-14): Can start after Phase 3 complete - visual/audio systems
- Tier 4 Phase (15): Must be last - editor and tools

---

## 🔵 PHASE 1: Engine Core Foundation

### Phase Objective
Build and validate the engine's core infrastructure. All other systems depend on this.

### Agent
**Jules-Engine-Core** (6 critical tasks)

### Files Owned
- `src/engine/core/` (198 files)
- `src/engine/ecs/` (11 files)
- `src/engine/scene/` (23 files)

### Phase 1 Success Criteria
✅ Engine initializes → runs 10 frames clean → shuts down with zero leaks
✅ Memory allocator consolidated, no fragmentation
✅ Thread pool executes 100+ jobs without deadlocks
✅ Virtual file system loads 50+ files correctly
✅ Logging system unified and working
✅ Hot reload infrastructure operational
✅ All 6 tasks (CORE-001 through CORE-006) marked completed in `master/todo.csv`
✅ Build: `cmake --build . -j4` produces zero warnings
✅ Tests: All core subsystem tests pass with ASAN/UBSAN enabled

### Phase 1 Deadline
**End of Week 2**

### Phase 1 Critical Dependencies
⚠️ **BLOCKING**: All 14 other agents waiting on this
⚠️ Phases 2-15 cannot start until Phase 1 complete

### Your Tasks (Jules-Engine-Core)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 1: Jules-Engine-Core

**Task List:**
1. CORE-001: Engine Initialization Pipeline ⭐ START HERE
2. CORE-002: Memory Allocator Consolidation
3. CORE-003: Thread Pool Validation
4. CORE-004: Virtual File System
5. CORE-005: Logging & Debug System
6. CORE-006: Hot Reload Infrastructure

### How to Execute This Phase
```
1. Read this entire section
2. Go to master/JULIUS_AGENT_TASK_PROMPTS.md → Agent 1
3. Open each task file reference
4. Follow "Your Prompt" instructions exactly
5. Update master/todo.csv as you complete tasks
6. Report daily progress
7. Do not start Phase 2 until all 6 tasks completed
```

### Verification Commands
```bash
# Count completed CORE tasks
grep "^CORE-" master/todo.csv | grep "completed" | wc -l
# Should equal 6

# Check for in_progress > 7 days old
grep "^CORE-" master/todo.csv | grep "in_progress"
# Should be empty before phase ends
```

---

## 🟡 PHASE 2: Rendering Pipeline

### Phase Objective
Implement complete unified rendering pipeline supporting all backends with proper synchronization and frame composition.

### Agent
**Jules-Rendering** (7 critical tasks)

### Files Owned
- `src/engine/rendering/` (361 files)
- `src/engine/backend/metal/` (Metal backend)
- `src/engine/backend/vulkan/` (Vulkan backend)
- `src/engine/shading/` (Shader system)
- `src/engine/postprocess/` (Post-processing)

### Phase 2 Success Criteria
✅ Triangle renders on screen at 60 FPS on all platforms
✅ Shader compilation pipeline unified, hot-reload working
✅ Frame graph scheduling correct, all passes execute in order
✅ GPU memory management: 10000+ allocations without fragmentation
✅ Vulkan backend either fully working or cleanly removed
✅ Metal backend synchronized, no GPU stalls
✅ Post-processing effects render correctly (TAA, SSAO, SSR)
✅ All 7 tasks (RENDER-001 through RENDER-007) marked completed
✅ Tests: Render 60 FPS for 10 minutes without crashes

### Phase 2 Deadline
**End of Week 4** (2 weeks after Phase 1 complete)

### Phase 2 Dependencies
🔴 **BLOCKED by:** Phase 1 (Jules-Engine-Core)
✅ **Enables:** Phases 3, 4, 5, 6+ (all visual systems need rendering)

### Your Tasks (Jules-Rendering)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 2: Jules-Rendering

**Task List:**
1. RENDER-001: Renderer Initialization ⭐ START HERE
2. RENDER-002: Shader System Consolidation
3. RENDER-003: Frame Graph Execution
4. RENDER-004: GPU Memory Management
5. RENDER-005: Vulkan Backend Stability
6. RENDER-006: Metal Backend Synchronization
7. RENDER-007: Post-Processing Pipeline

### How to Execute This Phase
```
1. Wait for Phase 1 (Jules-Engine-Core) to complete
2. Read this entire section
3. Go to master/JULIUS_AGENT_TASK_PROMPTS.md → Agent 2
4. Open each task file reference
5. Follow "Your Prompt" instructions exactly
6. Update master/todo.csv as you complete tasks
7. Do not start Phase 3 until all 7 tasks completed
```

### Verification Commands
```bash
# Count completed RENDER tasks
grep "^RENDER-" master/todo.csv | grep "completed" | wc -l
# Should equal 7

# Verify Phase 1 still complete
grep "^CORE-" master/todo.csv | grep -v "completed" | wc -l
# Should equal 0
```

---

## 🔴 PHASE 3: Physics Engine

### Phase Objective
Build robust physics simulation with rigid body dynamics, constraints, collision detection, and deterministic behavior.

### Agent
**Jules-Physics** (6 critical tasks)

### Files Owned
- `src/engine/physics/` (224 files)
- `src/engine/physics/solver/` (Solvers)
- `src/engine/physics/collision/` (Collision detection)
- `src/engine/physics/constraints/` (Constraints)

### Phase 3 Success Criteria
✅ 1000+ physics objects simulating at 60 FPS without jitter
✅ Collision detection accurate with no tunneling
✅ Rigid body solver converged to stable state
✅ Constraints working: fixed, hinge, ball-socket
✅ XPBD solver producing deterministic results
✅ Raycasting and shape casting accurate
✅ All 6 tasks (PHYS-001 through PHYS-006) marked completed
✅ Tests: Drop 1000 boxes, measure frame time, verify stability

### Phase 3 Deadline
**End of Week 6** (2 weeks after Phase 2 complete)

### Phase 3 Dependencies
🔴 **BLOCKED by:** Phases 1, 2 (Jules-Engine-Core, Jules-Rendering)
✅ **Enables:** All gameplay, character, and effects systems

### Your Tasks (Jules-Physics)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 3: Jules-Physics

**Task List:**
1. PHYS-001: Rigid Body System ⭐ START HERE
2. PHYS-002: Collision Detection Pipeline
3. PHYS-003: Constraint Solver
4. PHYS-004: XPBD Implementation
5. PHYS-005: Raycasting & Shape Casting
6. PHYS-006: Physics Debugging Visualization

### How to Execute This Phase
```
1. Wait for Phase 2 (Jules-Rendering) to complete
2. Read this entire section
3. Go to master/JULIUS_AGENT_TASK_PROMPTS.md → Agent 3
4. Open each task file reference
5. Follow "Your Prompt" instructions exactly
6. Update master/todo.csv as you complete tasks
7. Phases 4-14 can now start in parallel
```

### Verification Commands
```bash
# Count completed PHYS tasks
grep "^PHYS-" master/todo.csv | grep "completed" | wc -l
# Should equal 6

# Verify Phases 1-2 still complete
grep "^CORE-\|^RENDER-" master/todo.csv | grep -v "completed" | wc -l
# Should equal 0
```

---

## 🟢 PHASE 4: Animation Systems

### Phase Objective
Implement skeletal animation, blend trees, IK solvers, and animation state machines.

### Agent
**Jules-Animation** (5 critical tasks)

### Files Owned
- `src/engine/animation/` (55 files)
- `src/engine/character/animation/` (Character animation)
- `src/engine/animation/ik_advanced/` (IK solvers)
- `src/engine/animation/state_machines/` (State machines)

### Phase 4 Success Criteria
✅ Skeletal animation plays correctly on character models
✅ Blend trees interpolate between animations smoothly
✅ IK solvers compute limb positions accurately
✅ Animation state machine transitions work without popping
✅ Additive animation blending correct
✅ All 5 tasks (ANIM-001 through ANIM-005) marked completed
✅ Tests: Play 50+ animation sequences, verify smooth blending

### Phase 4 Deadline
**End of Week 8** (2 weeks, can start after Phase 3)

### Phase 4 Dependencies
🟡 **REQUIRES:** Phases 1, 2, 3 (Engine, Rendering, Physics)
🟢 **CAN START:** After Phase 3
✅ **Enables:** Character systems, visual polish

### Your Tasks (Jules-Animation)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 4: Jules-Animation

**Task List:**
1. ANIM-001: Skeletal Animation Playback ⭐ START HERE
2. ANIM-002: Blend Tree System
3. ANIM-003: IK Solver Suite
4. ANIM-004: Animation State Machine
5. ANIM-005: Additive Blending

### How to Execute This Phase
```
1. Wait for Phase 3 (Jules-Physics) to complete
2. Read this entire section
3. Go to master/JULIUS_AGENT_TASK_PROMPTS.md → Agent 4
4. Open each task file reference
5. Follow "Your Prompt" instructions exactly
6. Update master/todo.csv as you complete tasks
```

### Verification Commands
```bash
# Count completed ANIM tasks
grep "^ANIM-" master/todo.csv | grep "completed" | wc -l
# Should equal 5

# Verify foundation phases complete
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv | grep -v "completed" | wc -l
# Should equal 0
```

---

## 🟢 PHASE 5: Audio Systems

### Phase Objective
Implement spatial audio, music systems, sound effects, and DSP effects.

### Agent
**Jules-Audio** (4 critical tasks)

### Files Owned
- `src/engine/audio/` (67 files)
- `src/engine/audio/dsp/` (DSP effects)
- `src/engine/audio/spatial/` (Spatial audio)

### Phase 5 Success Criteria
✅ Sound effects play with correct volume and panning
✅ Spatial audio: 3D sound positioning works
✅ Music system loops correctly
✅ DSP effects: reverb, EQ, compression operational
✅ All 4 tasks (AUDIO-001 through AUDIO-004) marked completed
✅ Tests: Play 100+ simultaneous sound events

### Phase 5 Deadline
**End of Week 10** (2 weeks, can start after Phase 3)

### Phase 5 Dependencies
🟡 **REQUIRES:** Phases 1, 2, 3 (Engine, Rendering, Physics)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-Audio)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 5: Jules-Audio

**Task List:**
1. AUDIO-001: Audio Playback System ⭐ START HERE
2. AUDIO-002: Spatial Audio
3. AUDIO-003: Music System
4. AUDIO-004: DSP Effects

### Verification Commands
```bash
# Count completed AUDIO tasks
grep "^AUDIO-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🟢 PHASE 6: AI & NPC Systems

### Phase Objective
Implement behavior trees, GOAP planning, pathfinding, and NPC AI subsystems.

### Agent
**Jules-AI** (4 critical tasks)

### Files Owned
- `src/engine/ai/` (108 files)
- `src/engine/ai/behavior_trees/` (Behavior trees)
- `src/engine/ai/planning/` (GOAP planning)
- `src/engine/ai/pathfinding/` (Pathfinding)
- `src/engine/ai/perception/` (NPC perception)

### Phase 6 Success Criteria
✅ Behavior trees execute and branch correctly
✅ GOAP planner generates action sequences
✅ Pathfinding: A* finds optimal paths
✅ NPC perception detects players/threats
✅ All 4 tasks (AI-001 through AI-004) marked completed
✅ Tests: Simulate 50 NPCs with AI

### Phase 6 Deadline
**End of Week 12** (2 weeks, can start after Phase 3)

### Phase 6 Dependencies
🟡 **REQUIRES:** Phases 1, 2, 3 (Engine, Rendering, Physics)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-AI)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 6: Jules-AI

**Task List:**
1. AI-001: Behavior Tree System ⭐ START HERE
2. AI-002: GOAP Planner
3. AI-003: Pathfinding & Navigation
4. AI-004: NPC Perception System

### Verification Commands
```bash
# Count completed AI tasks
grep "^AI-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🟢 PHASE 7: Asset Management

### Phase Objective
Implement asset importing, loading, streaming, and bundling systems.

### Agent
**Jules-Assets** (4 critical tasks)

### Files Owned
- `src/engine/assets/` (140 files)
- `src/engine/assets/importer/` (Asset importers)
- `src/engine/assets/io/` (I/O and streaming)
- `src/engine/assets/system/` (Asset system)

### Phase 7 Success Criteria
✅ Assets import from FBX, glTF, PNG correctly
✅ Asset streaming loads content on demand
✅ Bundling system packages assets efficiently
✅ Hot reload updates assets without restart
✅ All 4 tasks (ASSET-001 through ASSET-004) marked completed
✅ Tests: Load 500+ assets, verify references correct

### Phase 7 Deadline
**End of Week 14** (2 weeks, can start after Phase 3)

### Phase 7 Dependencies
🟡 **REQUIRES:** Phases 1, 2, 3 (Engine, Rendering, Physics)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-Assets)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 7: Jules-Assets

**Task List:**
1. ASSET-001: Asset Importer ⭐ START HERE
2. ASSET-002: Asset Streaming
3. ASSET-003: Asset Bundling
4. ASSET-004: Hot Reload System

### Verification Commands
```bash
# Count completed ASSET tasks
grep "^ASSET-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🟢 PHASE 8: Networking & Replication

### Phase Objective
Implement network protocol, entity replication, prediction, and multiplayer systems.

### Agent
**Jules-Networking** (4 critical tasks)

### Files Owned
- `src/engine/network/` (44 files)
- `src/engine/network/protocol/` (Network protocol)
- `src/engine/network/replication/` (Entity replication)
- `src/engine/network/prediction/` (Client prediction)

### Phase 8 Success Criteria
✅ Network protocol sends/receives packets reliably
✅ Entity replication synchronizes state across clients
✅ Client prediction smooths movement
✅ Interpolation handles latency correctly
✅ All 4 tasks (NET-001 through NET-004) marked completed
✅ Tests: Connect 4 clients, verify synchronization

### Phase 8 Deadline
**End of Week 16** (2 weeks, can start after Phase 3)

### Phase 8 Dependencies
🟡 **REQUIRES:** Phases 1, 2, 3 (Engine, Rendering, Physics)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-Networking)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 8: Jules-Networking

**Task List:**
1. NET-001: Network Protocol ⭐ START HERE
2. NET-002: Entity Replication
3. NET-003: Client Prediction
4. NET-004: Synchronization

### Verification Commands
```bash
# Count completed NET tasks
grep "^NET-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🟢 PHASE 9: Gameplay Systems

### Phase Objective
Implement crafting, quests, combat, food systems, and core gameplay loops.

### Agent
**Jules-Gameplay** (5 critical tasks)

### Files Owned
- `src/engine/gameplay/` (22 files)
- `src/engine/gameplay/crafting/` (Crafting system)
- `src/engine/gameplay/combat/` (Combat system)
- `src/engine/gameplay/quests/` (Quest system)

### Phase 9 Success Criteria
✅ Crafting recipes work end-to-end
✅ Quest system tracks objectives and completion
✅ Combat system handles damage, health, death
✅ Food system affects player stats
✅ Inventory management working
✅ All 5 tasks (GAME-001 through GAME-005) marked completed
✅ Tests: Complete full gameplay loop (craft, quest, combat, eat)

### Phase 9 Deadline
**End of Week 18** (2 weeks, can start after Phase 3)

### Phase 9 Dependencies
🟡 **REQUIRES:** Phases 1, 2, 3 (Engine, Rendering, Physics)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-Gameplay)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 9: Jules-Gameplay

**Task List:**
1. GAME-001: Crafting System ⭐ START HERE
2. GAME-002: Quest System
3. GAME-003: Combat System
4. GAME-004: Food/Health System
5. GAME-005: Inventory Management

### Verification Commands
```bash
# Count completed GAME tasks
grep "^GAME-" master/todo.csv | grep "completed" | wc -l
# Should equal 5
```

---

## 🟢 PHASE 10: Character Systems

### Phase Objective
Implement skeletal meshes, character customization, and character-specific IK and physics.

### Agent
**Jules-Character** (4 critical tasks)

### Files Owned
- `src/engine/character/` (130 files)
- `src/engine/character/skeletal_mesh/` (Skeletal mesh rendering)
- `src/engine/character/customization/` (Character customization)
- `src/engine/character/eyes/` (Eye rendering)
- `src/engine/character/hair/` (Hair simulation)

### Phase 10 Success Criteria
✅ Skeletal meshes render with skinning
✅ Character customization working (body parts, colors)
✅ Character IK: foot placement, hand placement
✅ Ragdoll physics activated on death
✅ All 4 tasks (CHAR-001 through CHAR-004) marked completed
✅ Tests: Create 20 unique characters, verify rendering

### Phase 10 Deadline
**End of Week 20** (2 weeks, can start after Phase 3)

### Phase 10 Dependencies
🟡 **REQUIRES:** Phases 1, 2, 3, 4 (Engine, Rendering, Physics, Animation)
🟢 **CAN START:** After Phase 4

### Your Tasks (Jules-Character)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 10: Jules-Character

**Task List:**
1. CHAR-001: Skeletal Mesh Rendering ⭐ START HERE
2. CHAR-002: Character Customization
3. CHAR-003: Character IK System
4. CHAR-004: Ragdoll Physics

### Verification Commands
```bash
# Count completed CHAR tasks
grep "^CHAR-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🟢 PHASE 11: Geometry & Meshes

### Phase Objective
Implement mesh processing, LOD generation, CSG operations, and vertex optimization.

### Agent
**Jules-Geometry** (4 critical tasks)

### Files Owned
- `src/engine/geometry/` (134 files)
- `src/engine/geometry/mesh/` (Mesh processing)
- `src/engine/geometry/lod/` (LOD generation)
- `src/engine/geometry/csg/` (CSG operations)

### Phase 11 Success Criteria
✅ Mesh loading and processing correct
✅ LOD system generates lower poly models
✅ CSG operations (union, intersection, subtraction)
✅ Vertex normal calculation accurate
✅ All 4 tasks (GEO-001 through GEO-004) marked completed
✅ Tests: Process 100+ meshes, verify LOD quality

### Phase 11 Deadline
**End of Week 22** (2 weeks, can start after Phase 3)

### Phase 11 Dependencies
🟡 **REQUIRES:** Phases 1, 2, 3 (Engine, Rendering, Physics)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-Geometry)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 11: Jules-Geometry

**Task List:**
1. GEO-001: Mesh Processing ⭐ START HERE
2. GEO-002: LOD Generation
3. GEO-003: CSG Operations
4. GEO-004: Vertex Optimization

### Verification Commands
```bash
# Count completed GEO tasks
grep "^GEO-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🟢 PHASE 12: Materials & Shaders

### Phase Objective
Implement material system, shader editor, shader compilation, and material properties.

### Agent
**Jules-Materials** (4 critical tasks)

### Files Owned
- `src/engine/materials/` (89 files)
- `src/engine/materials/editor/` (Material editor)
- `src/engine/materials/compilation/` (Shader compilation)

### Phase 12 Success Criteria
✅ Material editor creates and modifies materials
✅ Shader compilation produces valid GPU programs
✅ Material properties affect rendering correctly
✅ Shader hot-reload works without artifacts
✅ All 4 tasks (MAT-001 through MAT-004) marked completed
✅ Tests: Create 50+ materials, verify appearance

### Phase 12 Deadline
**End of Week 24** (2 weeks, can start after Phase 3)

### Phase 12 Dependencies
🟡 **REQUIRES:** Phases 1, 2 (Engine, Rendering)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-Materials)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 12: Jules-Materials

**Task List:**
1. MAT-001: Material System ⭐ START HERE
2. MAT-002: Shader Editor
3. MAT-003: Shader Compilation
4. MAT-004: Material Properties

### Verification Commands
```bash
# Count completed MAT tasks
grep "^MAT-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🟢 PHASE 13: Lighting & Global Illumination

### Phase Objective
Implement real-time lighting, shadows, global illumination, and light baking.

### Agent
**Jules-Lighting** (4 critical tasks)

### Files Owned
- `src/engine/lighting/` (131 files)
- `src/engine/lighting/shadows/` (Shadow systems)
- `src/engine/lighting/gi/` (Global illumination)
- `src/engine/lighting/probes/` (Light probes)

### Phase 13 Success Criteria
✅ Real-time lights illuminate scene correctly
✅ Shadow mapping produces sharp, artifact-free shadows
✅ Global illumination bounces light realistically
✅ Light probes provide fast indirect lighting
✅ All 4 tasks (LIGHT-001 through LIGHT-004) marked completed
✅ Tests: Render complex lit scene at 60 FPS

### Phase 13 Deadline
**End of Week 26** (2 weeks, can start after Phase 3)

### Phase 13 Dependencies
🟡 **REQUIRES:** Phases 1, 2 (Engine, Rendering)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-Lighting)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 13: Jules-Lighting

**Task List:**
1. LIGHT-001: Real-Time Lighting ⭐ START HERE
2. LIGHT-002: Shadow Systems
3. LIGHT-003: Global Illumination
4. LIGHT-004: Light Probes

### Verification Commands
```bash
# Count completed LIGHT tasks
grep "^LIGHT-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🟢 PHASE 14: VFX & Particles

### Phase Objective
Implement particle systems, trails, decals, explosions, and visual effects.

### Agent
**Jules-Effects** (4 critical tasks)

### Files Owned
- `src/engine/effects/` (79 files)
- `src/engine/effects/particles/` (Particle systems)
- `src/engine/effects/decals/` (Decal system)
- `src/engine/effects/trails/` (Trail effects)

### Phase 14 Success Criteria
✅ Particle emitters spawn and animate correctly
✅ Particles collide with geometry
✅ Decals render on surfaces
✅ Explosion effects visually compelling
✅ All 4 tasks (FX-001 through FX-004) marked completed
✅ Tests: Simulate 50,000+ particles at 60 FPS

### Phase 14 Deadline
**End of Week 28** (2 weeks, can start after Phase 3)

### Phase 14 Dependencies
🟡 **REQUIRES:** Phases 1, 2 (Engine, Rendering)
🟢 **CAN START:** After Phase 3

### Your Tasks (Jules-Effects)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 14: Jules-Effects

**Task List:**
1. FX-001: Particle System ⭐ START HERE
2. FX-002: Particle Collision
3. FX-003: Decal System
4. FX-004: Trail Effects

### Verification Commands
```bash
# Count completed FX tasks
grep "^FX-" master/todo.csv | grep "completed" | wc -l
# Should equal 4
```

---

## 🔵 PHASE 15: Editor & Tools (Final Phase)

### Phase Objective
Build editor and development tools to make the engine usable for content creation, debugging, and deployment.

### Agent
**Jules-Tools** (6 critical tasks)

### Files Owned
- `src/engine/editor/` (308 files)
- `src/engine/tools/` (47 files)
- `src/engine/debug/` (8 files)

### Phase 15 Success Criteria
✅ Level editor builds and saves levels
✅ Gizmo system moves/rotates/scales objects
✅ Asset browser browses all content
✅ Property editor edits entity properties live
✅ Debug visualization shows physics/AI
✅ Developer console runs commands
✅ All 6 tasks (TOOLS-001 through TOOLS-006) marked completed
✅ Full game playable start-to-finish from editor
✅ Build compiles, zero warnings, all tests pass

### Phase 15 Deadline
**End of Week 30+** (2 weeks, only starts after all other phases)

### Phase 15 Critical Dependencies
⚠️ **DO NOT START until ALL phases 1-14 complete**
⚠️ Depends on all 14 prior agents and their deliverables

### Your Tasks (Jules-Tools)
**Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 15: Jules-Tools

**Task List:**
1. TOOLS-001: Level Editor ⭐ START HERE
2. TOOLS-002: Gizmo System
3. TOOLS-003: Property Editor
4. TOOLS-004: Asset Browser
5. TOOLS-005: Debug Visualization
6. TOOLS-006: Developer Console

### How to Execute This Phase
```
1. Verify ALL phases 1-14 complete (all 64 tasks done)
2. Read this entire section
3. Go to master/JULIUS_AGENT_TASK_PROMPTS.md → Agent 15
4. Open each task file reference
5. Follow "Your Prompt" instructions exactly
6. Update master/todo.csv as you complete tasks
7. This is the final phase - ship the editor
```

### Verification Commands
```bash
# Count completed TOOLS tasks
grep "^TOOLS-" master/todo.csv | grep "completed" | wc -l
# Should equal 6

# Verify ALL phases 1-14 complete (64 total tasks)
grep -v "completed" master/todo.csv | grep "CORE-\|RENDER-\|PHYS-\|ANIM-\|AUDIO-\|AI-\|ASSET-\|NET-\|GAME-\|CHAR-\|GEO-\|MAT-\|LIGHT-\|FX-\|TOOLS-" | wc -l
# Should equal 0 (all tasks completed)
```

---

## 📊 15-Phase Timeline Overview

| Phase | Agent | Files | Tasks | Weeks | Deadline | Status |
|-------|-------|-------|-------|-------|----------|--------|
| 1 | Jules-Engine-Core | 198 | 6 | 1-2 | Week 2 | **CRITICAL** |
| 2 | Jules-Rendering | 361 | 7 | 3-4 | Week 4 | **CRITICAL** |
| 3 | Jules-Physics | 224 | 6 | 5-6 | Week 6 | **CRITICAL** |
| 4 | Jules-Animation | 55 | 5 | 7-8 | Week 8 | Parallel* |
| 5 | Jules-Audio | 67 | 4 | 9-10 | Week 10 | Parallel* |
| 6 | Jules-AI | 108 | 4 | 11-12 | Week 12 | Parallel* |
| 7 | Jules-Assets | 140 | 4 | 13-14 | Week 14 | Parallel* |
| 8 | Jules-Networking | 44 | 4 | 15-16 | Week 16 | Parallel* |
| 9 | Jules-Gameplay | 22 | 5 | 17-18 | Week 18 | Parallel* |
| 10 | Jules-Character | 130 | 4 | 19-20 | Week 20 | Parallel* |
| 11 | Jules-Geometry | 134 | 4 | 21-22 | Week 22 | Parallel* |
| 12 | Jules-Materials | 89 | 4 | 23-24 | Week 24 | Parallel* |
| 13 | Jules-Lighting | 131 | 4 | 25-26 | Week 26 | Parallel* |
| 14 | Jules-Effects | 79 | 4 | 27-28 | Week 28 | Parallel* |
| 15 | Jules-Tools | 363 | 6 | 29-30 | Week 30+ | **FINAL** |

*Phases 4-14 can overlap significantly after Phase 3 completes.

**Total:** 65 tasks across 2,385 files
**Minimum Timeline:** 30+ weeks (with full parallelization)
**Critical Path:** Phase 1 → Phase 2 → Phase 3 → Phase 15

---

## 🚀 How to Use This 15-Phase System

### For Project Lead
```
1. Keep master/todo.csv current - it's the source of truth
2. Check phase deadlines above
3. Verify each phase completion with bash commands provided
4. Do not advance to Phase 15 until all phases 1-14 complete
5. Coordinate phase starts: phases 4-14 can overlap after phase 3
```

### For Agent Lead (Any Phase)
```
1. Read your phase section above
2. Understand dependencies and blockers
3. Go to master/JULIUS_AGENT_TASK_PROMPTS.md → Your Agent
4. Read each task prompt carefully
5. Update master/todo.csv as you work
6. Report progress daily
7. Do not start next phase until this phase complete
```

### For Developer Assigned a Task
```
1. Agent Lead assigns you a task ID (e.g., CORE-001)
2. Search master/JULIUS_AGENT_TASK_PROMPTS.md for that task
3. Read "Your Prompt" section - exact instructions
4. Open file referenced in task
5. Follow step-by-step instructions
6. Test according to acceptance criteria
7. Update CSV: mark in_progress when starting, completed when done
8. Report blockers to agent lead immediately
```

### Quick Navigation
- **"What's my phase?"** → Find your agent above
- **"What's my task?"** → Go to `master/JULIUS_AGENT_TASK_PROMPTS.md` → Your Agent
- **"What's my deadline?"** → See timeline above
- **"What do I do first?"** → Search for "⭐ START HERE" in your agent section
- **"How do I know I'm done?"** → See "Success Criteria" in your phase
- **"What blocks me?"** → See "Dependencies" in your phase

---

## 📋 Master Index for All Documentation

| Document | Purpose | When to Read |
|----------|---------|--------------|
| `JULES_15_PHASE_SYSTEM.md` (THIS FILE) | Phase-by-phase breakdown | Start here for phase info |
| `master/JULIUS_AGENT_TASK_PROMPTS.md` | Specific task instructions | When assigned a task |
| `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` | Full agent specifications | Understand full scope |
| `master/JULIUS_EXECUTION_GUIDE.md` | Quick start guide | First time using system |
| `JULIUS_15_QUICK_REFERENCE.md` | 1-page lookup tables | Quick reference |
| `master/todo.csv` | Task status database | Track progress |
| `master/master.md` | Workflow rules | Before starting work |
| `master/expectations.md` | Quality bar | Understand "done" |

---

**System Ready.** All 15 agents have dedicated phases.
**Begin Phase 1.** Do not advance to Phase 2 until Phase 1 complete.
**Track Progress.** Update `master/todo.csv` daily.
**Report Blockers.** Escalate blocked tasks immediately.

✅ **15-Phase System Complete**
