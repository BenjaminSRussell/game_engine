# 15 Specialized Jules Agents - Engine Architecture

**Created:** 2026-01-12
**Strategy:** Split the Minecraft v2 engine into 15 specialized agents, each owning a critical subsystem
**Goal:** Parallel development with zero overlap and clear ownership boundaries

---

## Agent Assignment Overview

| Agent | Subsystem | Files | Priority | Focus |
|-------|-----------|-------|----------|-------|
| Jules-Engine-Core | Core Engine | 198 files | P0 | Engine lifecycle, memory, threading |
| Jules-Rendering | Rendering Pipeline | 361 files | P0 | Graphics, shaders, frame composition |
| Jules-Physics | Physics & Collision | 224 files | P0 | Rigid bodies, constraints, simulation |
| Jules-Animation | Animation Systems | 55 files | P1 | Skeletal, blend trees, IK systems |
| Jules-Audio | Audio & DSP | 67 files | P1 | Sound, music, spatial audio |
| Jules-AI | AI/NPC/Pathfinding | 108 files | P1 | Behavior trees, GOAP, navmesh |
| Jules-Assets | Asset Management | 140 files | P1 | Import, loading, streaming, bundling |
| Jules-Networking | Network & Replication | 44 files | P1 | Sockets, packets, prediction, netcode |
| Jules-Gameplay | Gameplay Systems | 22 files | P1 | Crafting, quests, combat, food |
| Jules-Character | Character Systems | 130 files | P1 | Skeletal meshes, customization, IK |
| Jules-Geometry | Geometry & Meshes | 134 files | P1 | Vertex layouts, normals, LOD, CSG |
| Jules-Materials | Materials & Shaders | 89 files | P1 | Material editor, shader compilation |
| Jules-Lighting | Lighting & GI | 131 files | P1 | Shadows, GI, baking, light probes |
| Jules-Effects | VFX & Particles | 79 files | P1 | Particles, trails, decals, explosions |
| Jules-Tools | Editor & Tools | 308 files | P2 | Gizmos, property editors, asset browser |

---

## TIER 1: Critical Foundation (P0)

### Agent 1: Jules-Engine-Core
**Subsystem:** Core Engine Infrastructure
**Files:** `src/engine/core/` (198 files)
**Status:** ACTIVE - Build foundation before gameplay

#### Mission
Ensure the engine lifecycle, memory management, threading, and core utilities work reliably. This is the foundation all other systems depend on.

#### Ownership
- Core engine initialization and shutdown
- Memory allocation and pooling systems
- Thread pool and job system
- Logging and profiling infrastructure
- Virtual file system and resource paths
- String utilities and data structures

#### Critical Tasks

**CORE-001: Engine Initialization Pipeline** (CRITICAL)
- File: `src/engine/core/engine.c`
- Verify: `engine_init()` → `engine_update()` → `engine_shutdown()` complete data flow
- Acceptance: Engine starts, runs for 10 frames, shuts down cleanly with zero memory leaks
- Test: Run with address sanitizer enabled, no errors

**CORE-002: Memory Allocator Consolidation** (CRITICAL)
- File: `src/engine/core/common/memory/allocator.c`
- Task: Verify single unified allocator; remove duplicates
- Acceptance: All allocations routed through one allocator with tracking
- Test: Memory profiling shows correct allocation patterns

**CORE-003: Thread Pool Validation** (CRITICAL)
- File: `src/engine/core/sync/thread_pool.c`
- Task: Verify thread pool works, job dispatch, synchronization
- Acceptance: Jobs execute in parallel, no race conditions, proper cleanup
- Test: Submit 100 jobs, verify all complete without deadlocks

**CORE-004: Virtual File System** (HIGH)
- File: `src/engine/core/resource/vfs.c`
- Task: Verify VFS can load files, mount directories, handle async I/O
- Acceptance: Load 50+ files across multiple mounts without errors
- Test: Load engine resources, assets, scripts from VFS

**CORE-005: Logging & Debug System** (HIGH)
- File: `src/engine/core/logger.c`
- Task: Consolidate logging, ensure no duplicate implementations
- Acceptance: Log levels work, output formats consistent
- Test: All systems log appropriately, timestamps accurate

**CORE-006: Hot Reload Infrastructure** (MEDIUM)
- File: `src/engine/core/hot_reload.c`
- Task: Verify code hot reload doesn't break systems
- Acceptance: Reload systems without crashing, state preserved
- Test: Modify shader, reload, verify it recompiles

---

### Agent 2: Jules-Rendering
**Subsystem:** Rendering Pipeline
**Files:** `src/engine/rendering/` (361 files)
**Status:** ACTIVE - Critical path for visuals

#### Mission
Implement a complete, unified rendering pipeline supporting Metal/Vulkan/OpenGL with proper synchronization, shader management, and frame composition.

#### Ownership
- Renderer initialization and frame submission
- Backend abstraction (Metal/Vulkan/OpenGL)
- Shader compilation and hot-reload
- Render graph and frame graph execution
- Memory management (VRAM allocation, resource lifetime)
- Synchronization (command buffers, fences, semaphores)

#### Critical Tasks

**RENDER-001: Renderer Initialization** (CRITICAL)
- Files: `src/engine/rendering/renderer.c`, `src/engine/backend/`
- Task: Verify `renderer_init()` initializes Metal/Vulkan backends correctly
- Acceptance: Create window, compile shaders, render triangle to screen
- Test: Start engine, see triangle rendered on all platforms

**RENDER-002: Shader System Consolidation** (CRITICAL)
- Files: `src/engine/shading/`, `src/engine/rendering/shader.c`
- Task: Verify single shader compilation pipeline, remove duplicates
- Acceptance: Load 50+ shaders, recompile on change, no stale shaders
- Test: Shader hot-reload works, materials update on recompile

**RENDER-003: Frame Graph Execution** (CRITICAL)
- File: `src/engine/rendering/frame_graph.c`
- Task: Implement frame graph scheduling and execution
- Acceptance: Execute passes in correct order, synchronize GPU/CPU properly
- Test: Render complex scene with multiple render passes

**RENDER-004: GPU Memory Management** (CRITICAL)
- Files: `src/engine/rendering/gpu_memory.c`
- Task: Verify GPU memory allocation, defragmentation, lifetime tracking
- Acceptance: Allocate/deallocate 10000+ resources without fragmentation
- Test: Monitor VRAM usage, no memory leaks

**RENDER-005: Vulkan Backend Stability** (HIGH)
- Files: `src/engine/backend/vulkan/`
- Task: Fix or fully remove Vulkan backend (currently broken)
- Acceptance: Either Vulkan renders correctly or is removed cleanly
- Test: Render on Vulkan-capable hardware without crashes

**RENDER-006: Metal Backend Synchronization** (HIGH)
- Files: `src/engine/backend/metal/`
- Task: Fix Metal synchronization issues (command encoding, semaphores)
- Acceptance: Metal renders at 60 FPS, no synchronization stalls
- Test: Render on macOS, verify GPU/CPU sync

**RENDER-007: Post-Processing Pipeline** (MEDIUM)
- Files: `src/engine/postprocess/`
- Task: Implement TAA, SSAO, SSR compute shaders
- Acceptance: Post effects render correctly, no GPU stalls
- Test: Enable all post effects, verify visuals and performance

---

### Agent 3: Jules-Physics
**Subsystem:** Physics & Collision
**Files:** `src/engine/physics/` (224 files)
**Status:** ACTIVE - Core gameplay mechanic

#### Mission
Implement a robust physics simulation with proper collision detection, constraint solving, and deterministic behavior for networking.

#### Ownership
- Physics solver (XPBD, impulse-based)
- Rigid body dynamics
- Collision detection (broad/narrow phase)
- Constraint solving (joints, contacts)
- Material properties and friction
- Deterministic simulation for networked play

#### Critical Tasks

**PHYS-001: Physics Solver Consolidation** (CRITICAL)
- Files: `src/engine/physics/solver/xpbd_solver.c`, `sequential_impulse.c`
- Task: Choose canonical solver, consolidate duplicate implementations
- Acceptance: Single unified solver, 500+ objects simulating
- Test: Drop 1000 cubes, verify stability and performance

**PHYS-002: Collision Detection** (CRITICAL)
- Files: `src/engine/physics/collision/`
- Task: Verify broad phase (BVH/SAP), narrow phase (GJK/Manifold)
- Acceptance: Detect collisions accurately, no missed or false positives
- Test: Complex collision scenarios (stack stability, edge cases)

**PHYS-003: Constraint Solving** (CRITICAL)
- Files: `src/engine/physics/constraints/`
- Task: Verify joint constraints work (hinges, ball joints, sliders)
- Acceptance: Constraints maintain stability, no jitter
- Test: Build complex mechanism, verify constraints hold

**PHYS-004: Deterministic Simulation** (HIGH)
- File: `src/engine/physics/determinism.c`
- Task: Ensure physics is deterministic for networked replay
- Acceptance: Same input sequence = same output on all machines
- Test: Replay physics, verify frame-by-frame identical

**PHYS-005: Performance Optimization** (HIGH)
- File: `src/engine/physics/profiling/`
- Task: Profile physics, implement SIMD optimizations
- Acceptance: 1000+ bodies at 16.67ms budget (60 FPS)
- Test: Benchmark complex scenes, verify frame budget

**PHYS-006: Continuous Collision Detection** (MEDIUM)
- File: `src/engine/physics/ccd/`
- Task: Implement CCD for fast-moving objects
- Acceptance: High-speed objects don't pass through geometry
- Test: Bullet at 100m/s doesn't penetrate wall

---

## TIER 2: Major Systems (P1)

### Agent 4: Jules-Animation
**Subsystem:** Animation Systems
**Files:** `src/engine/animation/` (55 files)
**Status:** CRITICAL - Character motion

#### Mission
Implement skeletal animation, blending, and IK with support for advanced features like motion matching and procedural animation.

#### Ownership
- Skeleton data and bone transformations
- Animation state machines and transitions
- Blend trees and animation blending
- Inverse kinematics (IK) solvers
- Motion matching and animation synthesis
- Procedural animation (breathing, recoil, footsteps)

#### Critical Tasks

**ANIM-001: Skeletal Animation System** (CRITICAL)
- Files: `src/engine/animation/skeleton.c`, `skeletal_animation.c`
- Task: Verify skeleton loading, bone transforms, skinning
- Acceptance: Load 50+ skeleton assets, animate smoothly
- Test: Import rigged character, play animations without artifacts

**ANIM-002: Animation State Machine** (CRITICAL)
- File: `src/engine/animation/state_machines/anim_state_machine.c`
- Task: Implement state transitions, blend tree evaluation
- Acceptance: Complex animation states blend smoothly
- Test: Walk → run → jump → land animations transition correctly

**ANIM-003: IK Solver Systems** (HIGH)
- Files: `src/engine/animation/ik/`, `src/engine/animation/ik_advanced/`
- Task: Implement multiple IK solvers (2-bone, FABRIK, full-body)
- Acceptance: IK targets reached with correct constraint behavior
- Test: Character foot placement on slopes, hand reaching

**ANIM-004: Motion Matching** (HIGH)
- File: `src/engine/animation/motion_matching/motion_database.c`
- Task: Implement motion matching system for smooth locomotion
- Acceptance: Character transitions between animations smoothly
- Test: Complex movement blending with motion matching

**ANIM-005: Procedural Animation** (MEDIUM)
- Files: `src/engine/animation/procedural/`
- Task: Implement procedural breathing, recoil, turn-in-place
- Acceptance: Procedural layers blend with base animations
- Test: Breathing animation works, recoil animates correctly

---

### Agent 5: Jules-Audio
**Subsystem:** Audio & DSP
**Files:** `src/engine/audio/` (67 files)
**Status:** HIGH - Sound design

#### Mission
Implement spatial audio, music systems, and DSP effects with proper mixing and synchronization.

#### Ownership
- Audio core (decode, playback, mixing)
- Spatial audio (3D sound positioning, HRTF)
- Music system (streaming, looping, transitions)
- DSP effects (reverb, delay, compression, EQ)
- Audio analysis and visualization
- Synchronization with gameplay events

#### Critical Tasks

**AUDIO-001: Audio Playback System** (CRITICAL)
- File: `src/engine/audio/audio_system.c`
- Task: Verify audio initialization, playback, cleanup
- Acceptance: Play 20+ simultaneous audio streams without errors
- Test: Load and play WAV/OGG files, verify audio output

**AUDIO-002: Spatial Audio** (HIGH)
- File: `src/engine/audio/spatial_audio.c`
- Task: Implement 3D sound positioning with HRTF
- Acceptance: Sound position matches visual source in 3D space
- Test: Move sound source, verify 3D positioning

**AUDIO-003: Music System** (HIGH)
- File: `src/engine/audio/music_system.c`
- Task: Implement music streaming and crossfading
- Acceptance: Smooth music transitions, no hitches
- Test: Play long music file, transition between tracks

**AUDIO-004: DSP Effects** (MEDIUM)
- File: `src/engine/audio/dsp/`
- Task: Implement reverb, delay, compression, EQ
- Acceptance: Effects apply without artifacts or pops
- Test: Apply effects to audio, verify quality

---

### Agent 6: Jules-AI
**Subsystem:** AI/NPC/Pathfinding
**Files:** `src/engine/ai/` (108 files)
**Status:** HIGH - NPC behavior

#### Mission
Implement NPC behavior systems with pathfinding, decision-making, and emergent interactions.

#### Ownership
- Pathfinding (A*, smooth paths, crowd)
- Behavior trees and state machines
- GOAP (Goal-Oriented Action Planning)
- Perception systems (vision, hearing, memory)
- Dialogue and interaction systems
- Procedural behavior synthesis

#### Critical Tasks

**AI-001: Pathfinding System** (CRITICAL)
- Files: `src/engine/ai/pathfinding/astar_pathfinder.c`
- Task: Verify A* pathfinding, navmesh generation, smooth paths
- Acceptance: NPCs navigate complex terrain without getting stuck
- Test: 50 NPCs pathfinding simultaneously, optimal routes

**AI-002: Behavior Trees** (HIGH)
- Files: `src/engine/ai/behavior_trees.c`
- Task: Implement behavior tree execution, compositing
- Acceptance: Complex NPC behaviors work reliably
- Test: NPCs follow patrol routes, respond to stimuli

**AI-003: GOAP Planning** (MEDIUM)
- Files: `src/engine/ai/npc_advanced/goap_planner.c`
- Task: Implement GOAP for goal-oriented NPC actions
- Acceptance: NPCs plan action sequences to achieve goals
- Test: NPC executes multi-step plan (find item, use item, report)

**AI-004: Perception System** (MEDIUM)
- Files: `src/engine/ai/perception/visual_cortex.c`
- Task: Implement vision, hearing, memory
- Acceptance: NPCs detect player at proper distances
- Test: Hide from NPC line-of-sight, sound detection

---

### Agent 7: Jules-Assets
**Subsystem:** Asset Management
**Files:** `src/engine/assets/` (140 files)
**Status:** HIGH - Content pipeline

#### Mission
Implement robust asset loading, streaming, and management for all content types.

#### Ownership
- Asset importers (FBX, glTF, OBJ)
- Format-specific loading (meshes, textures, animations)
- Streaming and LOD systems
- Asset bundling and packaging
- Hot-reload for development
- Dependency tracking

#### Critical Tasks

**ASSET-001: Asset Loader** (CRITICAL)
- Files: `src/engine/assets/importer/asset_importer.c`
- Task: Verify asset loading for all major formats
- Acceptance: Load 1000+ assets without errors
- Test: Import character model with skeleton and animations

**ASSET-002: Texture Management** (HIGH)
- Files: `src/engine/assets/textures/`
- Task: Implement texture compression, streaming, virtual texturing
- Acceptance: Large textures stream efficiently, no hitches
- Test: Load 2K/4K textures, verify memory usage

**ASSET-003: Animation Import** (HIGH)
- Files: `src/engine/assets/system/asset_system/import/animation_importer.c`
- Task: Verify animation import and retargeting
- Acceptance: Import animations from external tools
- Test: Import walk/run from mocap, retarget to character

**ASSET-004: Asset Streaming** (MEDIUM)
- Files: `src/engine/assets/io/streaming/`
- Task: Implement streaming LOD, prefetch, prioritization
- Acceptance: Large scenes load smoothly without pops
- Test: Walk through large level, verify streaming

---

### Agent 8: Jules-Networking
**Subsystem:** Network & Replication
**Files:** `src/engine/network/` (44 files)
**Status:** HIGH - Multiplayer

#### Mission
Implement networking infrastructure with prediction, reconciliation, and deterministic state replication.

#### Ownership
- Socket layer and packet handling
- State replication architecture
- Client-side prediction
- Server reconciliation
- Lag compensation
- RPC systems

#### Critical Tasks

**NET-001: Socket Layer** (CRITICAL)
- Files: `src/engine/network/network_manager.c`
- Task: Verify socket communication, packet assembly
- Acceptance: Send 100 packets/sec reliably
- Test: Network two clients, verify packet delivery

**NET-002: State Replication** (HIGH)
- Files: `src/engine/network/replication.c`
- Task: Implement entity state replication, interpolation
- Acceptance: Client accurately mirrors server state
- Test: Player position replicates with latency compensation

**NET-003: Client Prediction** (HIGH)
- Files: `src/engine/network/prediction/`
- Task: Implement client-side movement prediction
- Acceptance: Smooth movement despite 100-200ms latency
- Test: Move player with 200ms ping, verify responsiveness

**NET-004: Reconciliation** (MEDIUM)
- Files: `src/engine/network/prediction/net_prediction_impl.c`
- Task: Implement server reconciliation, rollback correction
- Acceptance: Prediction errors corrected smoothly
- Test: Server disagrees with client, correction is imperceptible

---

### Agent 9: Jules-Gameplay
**Subsystem:** Gameplay Systems
**Files:** `src/engine/gameplay/` (22 files)
**Status:** HIGH - Core loops

#### Mission
Implement core gameplay mechanics: crafting, quests, combat, food systems.

#### Ownership
- Crafting and recipe systems
- Quest systems with chains and markers
- Combat damage and status effects
- Inventory and item management
- Food/hunger systems
- Interaction systems

#### Critical Tasks

**GAME-001: Crafting System** (CRITICAL)
- File: `src/engine/gameplay/crafting/crafting_system.c`
- Task: Implement recipe database, grid validation, crafting
- Acceptance: Players can craft shaped/shapeless recipes
- Test: Craft pickaxe, verify tool works

**GAME-002: Quest System** (CRITICAL)
- File: `src/engine/gameplay/quest_system.c`
- Task: Implement quest chains, rewards, markers
- Acceptance: Complete quest chain, receive rewards
- Test: Quest giver quest → 3-quest chain → completion

**GAME-003: Combat System** (CRITICAL)
- Files: `src/engine/gameplay/combat/`
- Task: Implement damage, status effects, hitbox integration
- Acceptance: Deal damage, apply effects, kill entities
- Test: Attack enemy, enemy dies with correct animations

**GAME-004: Food System** (HIGH)
- File: `src/game/blockgame/survival/hunger.c`
- Task: Implement hunger/saturation, food poisoning, starvation
- Acceptance: Hunger affects health, starvation kills
- Test: Starve without eating, die correctly

**GAME-005: Interaction System** (MEDIUM)
- File: `src/engine/gameplay/interaction/interaction_system.c`
- Task: Implement entity interactions (doors, chests, NPCs)
- Acceptance: Players can interact with game objects
- Test: Open chest, speak to NPC, pick up item

---

### Agent 10: Jules-Character
**Subsystem:** Character Systems
**Files:** `src/engine/character/` (130 files)
**Status:** HIGH - Character customization

#### Mission
Implement character customization, skeletal meshes, and character-specific features.

#### Ownership
- Character creation and customization
- Skeletal mesh rendering
- Character movement systems
- Ragdoll physics integration
- Clothing and equipment systems
- Character animations and IK

#### Critical Tasks

**CHAR-001: Skeletal Mesh System** (CRITICAL)
- Files: `src/engine/character/skeletal_mesh/`
- Task: Verify skeletal mesh rendering, skinning
- Acceptance: Characters render with skeletal animation
- Test: Load character model, animate without glitches

**CHAR-002: Character Customization** (HIGH)
- Files: `src/engine/character/appearance/`
- Task: Implement character creation with customization
- Acceptance: Create and save character with custom appearance
- Test: Customize face, body, gear, verify persistence

**CHAR-003: Ragdoll System** (HIGH)
- Files: `src/engine/character/cloth/cloth_simulation.c`
- Task: Implement ragdoll physics for character death
- Acceptance: Dead characters ragdoll realistically
- Test: Kill character, watch ragdoll death animation

**CHAR-004: Equipment System** (MEDIUM)
- Files: `src/engine/character/equipment/`
- Task: Implement armor/weapons as visible equipment
- Acceptance: Equipped items appear on character
- Test: Equip armor, see it on character model

---

### Agent 11: Jules-Geometry
**Subsystem:** Geometry & Meshes
**Files:** `src/engine/geometry/` (134 files)
**Status:** HIGH - Mesh processing

#### Mission
Implement mesh generation, manipulation, and optimization systems.

#### Ownership
- Mesh data structures and vertex layouts
- Normal and tangent space calculations
- LOD generation
- Mesh optimization and compression
- CSG (Constructive Solid Geometry)
- Procedural mesh generation

#### Critical Tasks

**GEO-001: Mesh System** (CRITICAL)
- Files: `src/engine/geometry/mesh/`
- Task: Verify mesh loading, vertex layout, rendering
- Acceptance: Load 1000+ meshes, render without artifacts
- Test: Complex character model with multiple LODs

**GEO-002: Normal & Tangent Calculation** (HIGH)
- Files: `src/engine/geometry/normals/`
- Task: Implement normal and tangent space generation
- Acceptance: Correct normal mapping on all surfaces
- Test: Detailed normal-mapped object looks correct

**GEO-003: LOD System** (HIGH)
- Files: `src/engine/geometry/lod/`
- Task: Implement automatic LOD generation
- Acceptance: Distant objects use simplified meshes
- Test: Performance improves with LOD, quality near camera

**GEO-004: CSG Operations** (MEDIUM)
- Files: `src/engine/geometry/csg/`
- Task: Implement boolean operations for level design
- Acceptance: Union/subtract/intersect meshes correctly
- Test: Build level using CSG operations

---

### Agent 12: Jules-Materials
**Subsystem:** Materials & Shaders
**Files:** `src/engine/materials/` (89 files)
**Status:** HIGH - Material system

#### Mission
Implement material editor, shader compilation, and material properties.

#### Ownership
- Material definitions and properties
- Shader compilation and validation
- Material editor UI
- Material blending and transitions
- Texture sampling and filtering
- Material persistence and hot-reload

#### Critical Tasks

**MAT-001: Material System** (CRITICAL)
- Files: `src/engine/materials/material.c`
- Task: Implement material loading, property evaluation
- Acceptance: 100+ unique materials render correctly
- Test: Load material with complex properties, verify visuals

**MAT-002: Shader Compilation** (CRITICAL)
- Files: `src/engine/materials/shader_compiler.c`
- Task: Verify shader compilation, error handling
- Acceptance: Compile 50+ shaders, hot-reload works
- Test: Modify shader, hot-reload, verify changes

**MAT-003: Material Editor** (HIGH)
- Files: `src/engine/editor/material_editor/`
- Task: Implement in-editor material editing
- Acceptance: Edit material, see changes in real-time
- Test: Adjust material properties, preview updates

**MAT-004: Texture Sampling** (MEDIUM)
- Files: `src/engine/assets/textures/sampling/`
- Task: Implement anisotropic filtering, mipmap LOD
- Acceptance: Textures render without artifacts
- Test: Distant ground textures remain crisp

---

### Agent 13: Jules-Lighting
**Subsystem:** Lighting & GI
**Files:** `src/engine/lighting/` (131 files)
**Status:** HIGH - Illumination

#### Mission
Implement static and dynamic lighting with global illumination support.

#### Ownership
- Light types (directional, point, spot)
- Shadow mapping (directional, cascaded, omnidirectional)
- Global illumination systems
- Light probes and baking
- Real-time GI approximations
- Performance optimization for lighting

#### Critical Tasks

**LIGHT-001: Light System** (CRITICAL)
- Files: `src/engine/lighting/light_system.c`
- Task: Implement light rendering, shadow casting
- Acceptance: 100+ lights render with shadows
- Test: Complex lit scene with multiple shadow-casting lights

**LIGHT-002: Shadow Mapping** (CRITICAL)
- Files: `src/engine/lighting/shadows/shadow_map.c`
- Task: Implement directional, omnidirectional, cascaded shadows
- Acceptance: High-quality shadows without artifacts
- Test: Cascaded shadow maps on large terrain

**LIGHT-003: Global Illumination** (HIGH)
- Files: `src/engine/lighting/gi/`
- Task: Implement light probe GI or screen-space GI
- Acceptance: Indirect lighting looks correct
- Test: Light bounces correctly in complex scenes

**LIGHT-004: Lightmapping** (MEDIUM)
- Files: `src/engine/lighting/baking/`
- Task: Implement lightmap baking and sampling
- Acceptance: Static geometry pre-lit correctly
- Test: Baked lighting matches real-time reference

---

### Agent 14: Jules-Effects
**Subsystem:** VFX & Particles
**Files:** `src/engine/effects/` (79 files)
**Status:** HIGH - Visual effects

#### Mission
Implement unified particle and effects systems for all visual effects.

#### Ownership
- Particle systems (GPU/CPU)
- Particle emitters and behaviors
- Trail rendering
- Decal systems
- Explosion and destruction effects
- Effect performance optimization

#### Critical Tasks

**FX-001: Particle System** (CRITICAL)
- Files: `src/engine/effects/particles/`
- Task: Implement unified particle system (GPU-preferred)
- Acceptance: 100,000+ particles at 60 FPS
- Test: Large explosion with many particles, smooth frame rate

**FX-002: Emitters & Behaviors** (HIGH)
- Files: `src/engine/effects/particles/emitters/`
- Task: Implement emitter types, particle forces, lifetime
- Acceptance: Complex particle effects work smoothly
- Test: Fire, smoke, sparkle effects all render correctly

**FX-003: Trail Rendering** (HIGH)
- Files: `src/engine/effects/trails/`
- Task: Implement dynamic trail rendering
- Acceptance: Sword trails, projectile trails render correctly
- Test: Melee weapon leaves visual trail

**FX-004: Decal System** (MEDIUM)
- Files: `src/engine/effects/decals/`
- Task: Implement decal rendering (bullet holes, blood)
- Acceptance: Decals persist on surfaces without performance hit
- Test: Bullet holes appear on walls, don't accumulate infinitely

---

### Agent 15: Jules-Tools
**Subsystem:** Editor & Tools
**Files:** `src/engine/editor/` (308 files)
**Status:** MEDIUM - Development experience

#### Mission
Implement editor tools for level design, asset management, and debugging.

#### Ownership
- Level editor UI and workflow
- Gizmo systems (move, rotate, scale)
- Property editors
- Asset browser and explorer
- Debug visualization systems
- Developer console and commands

#### Critical Tasks

**TOOLS-001: Level Editor** (HIGH)
- Files: `src/engine/editor/level_editor/`
- Task: Implement level editing with save/load
- Acceptance: Create level in editor, verify in-game
- Test: Build level, compile, play in-game

**TOOLS-002: Gizmo System** (HIGH)
- Files: `src/engine/editor/gizmos/`
- Task: Implement 3D gizmos (move, rotate, scale)
- Acceptance: Manipulate entities with visual feedback
- Test: Move objects in editor with gizmos

**TOOLS-003: Property Editor** (HIGH)
- Files: `src/engine/editor/property_editor/`
- Task: Implement property inspection and editing
- Acceptance: Edit entity properties, see changes live
- Test: Edit material property in inspector, see change in editor

**TOOLS-004: Asset Browser** (HIGH)
- Files: `src/engine/editor/asset_browser/`
- Task: Implement asset exploration and preview
- Acceptance: Browse all assets with thumbnails
- Test: Find any asset type, see preview

**TOOLS-005: Debug Visualization** (MEDIUM)
- Files: `src/engine/debug/`
- Task: Implement debug drawing (collision shapes, paths, etc)
- Acceptance: Toggle debug visualizations on/off
- Test: Show physics shapes, AI paths, light bounds

**TOOLS-006: Developer Console** (MEDIUM)
- Files: `src/engine/scripting/console.c`
- Task: Implement in-game console with commands
- Acceptance: Execute commands, see results
- Test: Console commands work (spawn entity, setvar, etc)

---

## Task Dependency Matrix

```
Foundation (must complete first):
  Jules-Engine-Core (CORE-001 to CORE-006)
  ├── Jules-Rendering depends on Core
  ├── Jules-Physics depends on Core
  ├── Jules-Animation depends on Core
  └── Jules-Audio depends on Core

Graphics Stack:
  Jules-Rendering
  ├── Jules-Materials depends on Rendering
  ├── Jules-Lighting depends on Rendering
  ├── Jules-Effects depends on Rendering
  └── Jules-Geometry depends on Rendering

Gameplay Stack:
  Jules-Physics
  ├── Jules-Gameplay depends on Physics
  ├── Jules-AI depends on Physics
  ├── Jules-Character depends on Animation + Physics
  └── Jules-Assets provides content for all

Networking:
  Jules-Networking (independent, but needs Physics for determinism)

Tools:
  Jules-Tools depends on all other systems
```

---

## Execution Strategy

### Phase 1: Foundation (Weeks 1-2)
- **Jules-Engine-Core**: Get engine running
- **Jules-Rendering**: Get pixels on screen
- **Jules-Physics**: Get objects moving

### Phase 2: Systems (Weeks 3-4)
- Parallelize: Animation, Audio, AI, Assets, Networking
- All depend on Foundation being solid

### Phase 3: Integration (Weeks 5-6)
- **Jules-Character**: Character in game
- **Jules-Gameplay**: Game mechanics working
- **Jules-Geometry**: Complex levels
- **Jules-Materials**: Beautiful materials
- **Jules-Lighting**: Proper lighting
- **Jules-Effects**: Polish with VFX

### Phase 4: Tools & Polish (Weeks 7+)
- **Jules-Tools**: Make editor good
- Cross-system integration tests
- Performance optimization
- Bug fixes and stability

---

## Success Criteria (Overall)

✅ All 15 agents deliver their assigned systems
✅ Zero file ownership overlap
✅ All systems compile and link
✅ Core gameplay loop works end-to-end
✅ Performance meets targets (60 FPS, 16.67ms budget)
✅ No critical bugs blocking progression
✅ Codebase is maintainable and well-documented

---

## Check-in Schedule

**Weekly (All Jules agents):**
- Status update: % complete, blockers
- Report any dependencies blocking other agents
- Verify no duplicated work

**Bi-weekly (Agent leads):**
- Integration testing across systems
- Dependency validation
- Cross-system issue resolution

**End of Phase (Full team):**
- Comprehensive integration test
- Performance profiling
- Stability verification

---

End of Jules 15 Agents Assignment Document
