# Jules Agents - Phase Execution Prompts

**Purpose:** Each phase has a clear prompt explaining what agents do and where to find all data
**Updated:** 2026-01-12

---

## 🚀 PHASE 1: Foundation (Weeks 1-2)

### Phase 1 Prompt

```
PHASE 1 OBJECTIVE:
Build and validate the core engine infrastructure so all other systems have
a solid foundation. Get the engine running, pixels on screen, and physics
objects simulating.

3 AGENTS WORK IN PARALLEL:
1. Jules-Engine-Core   - Engine backbone (memory, threading, lifecycle)
2. Jules-Rendering     - Graphics pipeline (shaders, GPU, frame composition)
3. Jules-Physics       - Physics simulation (solvers, collision, constraints)

PHASE 1 SUCCESS:
✅ Engine initializes → runs 10 frames clean → shuts down with zero leaks
✅ Triangle renders on screen at 60 FPS on all platforms
✅ 1000+ physics objects simulating at 60 FPS without jitter
✅ All 19 tasks (6+7+6) marked completed in master/todo.csv
✅ Build compiles with -Wall -Werror, zero warnings

DEADLINE: End of Week 2
BLOCKING: All 12 agents in Phase 2, 3, 4 (entire project depends on this)
```

---

## WHERE TO FIND PHASE 1 DATA

### Agent Task Lists
**Location:** `master/JULIUS_AGENT_TASK_PROMPTS.md`
**Find:** Sections for Agent 1, Agent 2, Agent 3 (Engine-Core, Rendering, Physics)
**Contains:** Each agent's 4-6 specific tasks with prompts

**Quick access:**
```bash
grep -A 100 "Agent 1: Jules-Engine-Core" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 2: Jules-Rendering" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 3: Jules-Physics" master/JULIUS_AGENT_TASK_PROMPTS.md
```

### Detailed Agent Specifications
**Location:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
**Find:** Sections for TIER 1: Critical Foundation
**Contains:** Full mission, ownership, 4-6 tasks each with file locations

**Quick access:**
```bash
grep -A 200 "TIER 1: Critical Foundation" master/JULIUS_15_AGENTS_ASSIGNMENTS.md
```

### Task Status Tracking
**Location:** `master/todo.csv`
**Search:** All rows with task IDs starting with CORE-, RENDER-, PHYS-
**Example:**
```bash
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv
```

**CSV Format:**
```
ID,type,category,source,description,status
CORE-001,implementation,Engine Core > Initialization,Jules-Engine-Core,Engine initialization pipeline validation,pending
CORE-002,implementation,Engine Core > Memory,Jules-Engine-Core,Memory allocator consolidation,pending
...
RENDER-001,implementation,Rendering > Initialization,Jules-Rendering,Renderer initialization and setup,pending
...
PHYS-001,implementation,Physics > Solver,Jules-Physics,Physics solver consolidation,pending
...
```

### File Ownership for Phase 1
**Location:** `JULIUS_15_QUICK_REFERENCE.md` → "Directory Ownership" section
**Or:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` → Each agent's "Ownership" section

**CORE-001 files:**
- `src/engine/core/engine.c` - Main engine lifecycle
- `src/engine/core/` (all files) - Core subsystems
- `src/engine/ecs/` - Entity component system
- `src/engine/scene/` - Scene management

**RENDER-001 files:**
- `src/engine/rendering/renderer.c` - Main renderer
- `src/engine/backend/` - Metal/Vulkan/OpenGL backends
- `src/engine/rendering/` (all files) - Graphics subsystem

**PHYS-001 files:**
- `src/engine/physics/` (all files) - All physics code

### Phase 1 Success Criteria
**Location:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
**Section:** Each agent's "Success Criteria" subsection
**Contains:** Acceptance tests and validation checkpoints

**Access:**
```bash
grep -A 10 "Success Criteria" master/JULIUS_15_AGENTS_ASSIGNMENTS.md | head -20
```

### Phase 1 Task Details (19 total)

**Jules-Engine-Core (6 tasks):**
1. CORE-001: Engine Initialization Pipeline
2. CORE-002: Memory Allocator Consolidation
3. CORE-003: Thread Pool Validation
4. CORE-004: Virtual File System
5. CORE-005: Logging Consolidation
6. CORE-006: Hot Reload Infrastructure

**Jules-Rendering (7 tasks):**
1. RENDER-001: Renderer Initialization
2. RENDER-002: Shader Compilation
3. RENDER-003: Frame Graph Execution
4. RENDER-004: GPU Memory Management
5. RENDER-005: Vulkan Backend
6. RENDER-006: Metal Synchronization
7. RENDER-007: Post-Processing Pipeline

**Jules-Physics (6 tasks):**
1. PHYS-001: Physics Solver Consolidation
2. PHYS-002: Collision Detection
3. PHYS-003: Constraint Solving
4. PHYS-004: Deterministic Simulation
5. PHYS-005: Performance Optimization
6. PHYS-006: Continuous Collision Detection

---

## How Phase 1 Agents Use This Information

### Agent Lead View:
1. Read this prompt (above)
2. Go to `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
3. Find your agent section
4. Read full mission and all 4-6 task specs
5. Track progress in `master/todo.csv`

### Developer View:
1. Agent lead assigns you Task #1
2. Open `master/JULIUS_AGENT_TASK_PROMPTS.md`
3. Find "Agent 1: Jules-Engine-Core" section
4. Search for "Task 1: CORE-001"
5. Read **Your Prompt** - exact instructions
6. Implement according to prompt
7. Test against acceptance criteria
8. Update CSV status: in_progress → completed

### Project Lead View:
1. Check `master/todo.csv` for CORE-, RENDER-, PHYS- tasks
2. Count status=completed tasks
3. Success = 19 tasks completed
4. Blocker check = Any in_progress > 2 weeks?
5. Quality check = All acceptance tests pass

---

---

## 📊 PHASE 2: Systems (Weeks 3-4)

### Phase 2 Prompt

```
PHASE 2 OBJECTIVE:
Build 5 major engine systems that depend on Phase 1 foundation being solid.
These systems enable animation, audio, AI, content, and networking.

5 AGENTS WORK IN PARALLEL (after Phase 1 complete):
1. Jules-Animation    - Character motion (skeletons, blend trees, IK)
2. Jules-Audio        - Sound systems (music, spatial audio, DSP)
3. Jules-AI           - NPC behavior (pathfinding, behavior trees, GOAP)
4. Jules-Assets       - Content pipeline (import, loading, streaming)
5. Jules-Networking   - Multiplayer support (sockets, replication, prediction)

PHASE 2 SUCCESS:
✅ All 20 tasks (5+4+4+4+4) marked completed in master/todo.csv
✅ Animation system works: Load character, play walk/run/jump animations
✅ Audio system works: Play 20+ simultaneous sounds
✅ AI system works: Pathfind 50 NPCs without collision
✅ Assets load: Import FBX/glTF files with skeletons and animations
✅ Network basics work: Send/receive packets between 2 clients
✅ All systems integrate with Phase 1 without crashing

CRITICAL DEPENDENCY:
⚠️  DO NOT START until Phase 1 is 100% complete and stable
⚠️  Phase 1 blockers = Phase 2 blockers (cascading failure)

DEADLINE: End of Week 4
BLOCKING: All 6 agents in Phase 3 (depends on Phase 2 + Phase 1)
```

---

## WHERE TO FIND PHASE 2 DATA

### Agent Task Lists
**Location:** `master/JULIUS_AGENT_TASK_PROMPTS.md`
**Find:** Sections for Agent 4, 5, 6, 7, 8 (Animation, Audio, AI, Assets, Networking)
**Contains:** Each agent's 4-5 specific tasks with prompts

**Quick access:**
```bash
grep -A 100 "Agent 4: Jules-Animation" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 5: Jules-Audio" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 6: Jules-AI" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 7: Jules-Assets" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 8: Jules-Networking" master/JULIUS_AGENT_TASK_PROMPTS.md
```

### Detailed Agent Specifications
**Location:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
**Find:** Sections for TIER 2: Major Systems
**Contains:** Full mission, ownership, 4-5 tasks each with file locations

**Quick access:**
```bash
grep -A 150 "TIER 2: Major Systems" master/JULIUS_15_AGENTS_ASSIGNMENTS.md
```

### Task Status Tracking
**Location:** `master/todo.csv`
**Search:** All rows with task IDs starting with ANIM-, AUDIO-, AI-, ASSET-, NET-
**Example:**
```bash
grep "^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv
```

### File Ownership for Phase 2
**Location:** `JULIUS_15_QUICK_REFERENCE.md` → "Directory Ownership" section

**Phase 2 Directories:**
- Animation: `src/engine/animation/` (55 files)
- Audio: `src/engine/audio/` (67 files)
- AI: `src/engine/ai/` (108 files)
- Assets: `src/engine/assets/` (140 files)
- Networking: `src/engine/network/` + `src/engine/networking/` (44 files)

### Dependencies on Phase 1
**Location:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` → Each agent's "Depends On" section

**Animation depends on:** Jules-Engine-Core (ECS, threading)
**Audio depends on:** Jules-Engine-Core (threading, VFS)
**AI depends on:** Jules-Physics (collision detection, pathfinding)
**Assets depends on:** Jules-Engine-Core (VFS, memory)
**Networking depends on:** Jules-Physics (determinism, synchronization)

### Phase 2 Task Details (20 total)

**Jules-Animation (5 tasks):**
1. ANIM-001: Skeletal Animation System
2. ANIM-002: Animation State Machine
3. ANIM-003: IK Solver Systems
4. ANIM-004: Motion Matching
5. ANIM-005: Procedural Animation

**Jules-Audio (4 tasks):**
1. AUDIO-001: Audio Playback System
2. AUDIO-002: Spatial Audio
3. AUDIO-003: Music System
4. AUDIO-004: DSP Effects

**Jules-AI (4 tasks):**
1. AI-001: Pathfinding System
2. AI-002: Behavior Trees
3. AI-003: GOAP Planning
4. AI-004: Perception System

**Jules-Assets (4 tasks):**
1. ASSET-001: Asset Loader
2. ASSET-002: Texture Management
3. ASSET-003: Animation Import
4. ASSET-004: Asset Streaming

**Jules-Networking (4 tasks):**
1. NET-001: Socket Layer
2. NET-002: State Replication
3. NET-003: Client Prediction
4. NET-004: Reconciliation

---

## How Phase 2 Agents Use This Information

### Pre-Phase 2 Checklist:
- [ ] All Phase 1 agents report 100% complete
- [ ] No Phase 1 critical bugs remaining
- [ ] Phase 1 build is green (compiles, zero warnings)
- [ ] Jules-Engine-Core: 6/6 tasks done
- [ ] Jules-Rendering: 7/7 tasks done
- [ ] Jules-Physics: 6/6 tasks done

### Agent Lead View:
1. Verify Phase 1 complete above
2. Go to `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` → TIER 2
3. Find your agent section
4. Read full mission and 4-5 task specs
5. Review Phase 1 integration points
6. Track progress in `master/todo.csv`

### Developer View:
1. Agent lead assigns you Task #1
2. Open `master/JULIUS_AGENT_TASK_PROMPTS.md`
3. Find your agent section
4. Search for your task ID (ANIM-001, AUDIO-001, etc.)
5. Read **Your Prompt** and dependencies
6. Verify Phase 1 systems available
7. Implement according to prompt
8. Test against acceptance criteria
9. Update CSV: in_progress → completed

### Phase 1 Coordinator View:
Before Phase 2 starts:
1. Verify all Phase 1 tasks = completed in CSV
2. Run Phase 1 integration tests
3. Check for memory leaks (ASAN clean)
4. Check for race conditions (TSan clean)
5. Only then greenlight Phase 2 start

---

---

## 📊 PHASE 3: Integration (Weeks 5-6)

### Phase 3 Prompt

```
PHASE 3 OBJECTIVE:
Build 6 major content and polish layers that bring together all Phase 1
and Phase 2 systems. Players can now create characters, build levels,
craft items, and play the game.

6 AGENTS WORK IN PARALLEL (after Phase 2 complete):
1. Jules-Gameplay    - Game mechanics (crafting, quests, combat, food)
2. Jules-Character   - Character systems (customization, skeletal meshes, ragdoll)
3. Jules-Geometry    - Mesh systems (LOD, normals, CSG operations)
4. Jules-Materials   - Material system (shaders, textures, editor)
5. Jules-Lighting    - Lighting & GI (shadows, baking, real-time)
6. Jules-Effects     - VFX & Particles (explosions, trails, decals)

PHASE 3 SUCCESS:
✅ All 24 tasks (5+4+4+4+4+4) marked completed in master/todo.csv
✅ Character visible in game world with animations
✅ Game mechanics work: Craft items, kill enemies, complete quests
✅ Complex levels buildable: CSG operations, multiple materials, lighting
✅ Beautiful visuals: Materials render correctly, shadows work, effects polish
✅ All systems integrate with Phase 1 + Phase 2 without crashing
✅ Core gameplay loop functional: Explore → Collect → Craft → Fight → Progress

CRITICAL DEPENDENCIES:
⚠️  Jules-Character depends on Jules-Animation complete
⚠️  Jules-Geometry depends on Jules-Rendering complete
⚠️  Jules-Materials depends on Jules-Rendering complete
⚠️  Jules-Lighting depends on Jules-Rendering complete
⚠️  Jules-Effects depends on Jules-Rendering complete

DEADLINE: End of Week 6
BLOCKING: Jules-Tools (Phase 4) - needs all systems ready
```

---

## WHERE TO FIND PHASE 3 DATA

### Agent Task Lists
**Location:** `master/JULIUS_AGENT_TASK_PROMPTS.md`
**Find:** Sections for Agent 9, 10, 11, 12, 13, 14 (Gameplay, Character, Geometry, Materials, Lighting, Effects)
**Contains:** Each agent's 4-5 specific tasks with prompts

**Quick access:**
```bash
grep -A 100 "Agent 9: Jules-Gameplay" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 10: Jules-Character" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 11: Jules-Geometry" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 12: Jules-Materials" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 13: Jules-Lighting" master/JULIUS_AGENT_TASK_PROMPTS.md
grep -A 100 "Agent 14: Jules-Effects" master/JULIUS_AGENT_TASK_PROMPTS.md
```

### Detailed Agent Specifications
**Location:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
**Find:** Sections for TIER 2: Major Systems (continued) and task listings
**Contains:** Full mission, ownership, 4-5 tasks each with file locations

**Quick access:**
```bash
grep -A 200 "Jules-Gameplay" master/JULIUS_15_AGENTS_ASSIGNMENTS.md
grep -A 150 "Jules-Character" master/JULIUS_15_AGENTS_ASSIGNMENTS.md
```

### Task Status Tracking
**Location:** `master/todo.csv`
**Search:** All rows with task IDs starting with GAME-, CHAR-, GEO-, MAT-, LIGHT-, FX-
**Example:**
```bash
grep "^GAME-\|^CHAR-\|^GEO-\|^MAT-\|^LIGHT-\|^FX-" master/todo.csv
```

### File Ownership for Phase 3
**Location:** `JULIUS_15_QUICK_REFERENCE.md` → "Directory Ownership" section

**Phase 3 Directories:**
- Gameplay: `src/engine/gameplay/` (22 files)
- Character: `src/engine/character/` (130 files)
- Geometry: `src/engine/geometry/` (134 files)
- Materials: `src/engine/materials/` (89 files)
- Lighting: `src/engine/lighting/` (131 files)
- Effects: `src/engine/effects/` (79 files)

### Phase 3 Dependencies

**Location:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` → Each agent's section

**Dependency Chart:**
```
Jules-Rendering (Phase 1)
  ├→ Jules-Geometry (Phase 3)
  ├→ Jules-Materials (Phase 3)
  ├→ Jules-Lighting (Phase 3)
  └→ Jules-Effects (Phase 3)

Jules-Animation (Phase 2)
  └→ Jules-Character (Phase 3)

Jules-Physics (Phase 1) + Jules-Animation (Phase 2)
  └→ Jules-Gameplay (Phase 3)
```

### Phase 3 Task Details (24 total)

**Jules-Gameplay (5 tasks):**
1. GAME-001: Crafting System
2. GAME-002: Quest System
3. GAME-003: Combat System
4. GAME-004: Food System
5. GAME-005: Interaction System

**Jules-Character (4 tasks):**
1. CHAR-001: Skeletal Mesh System
2. CHAR-002: Character Customization
3. CHAR-003: Ragdoll System
4. CHAR-004: Equipment System

**Jules-Geometry (4 tasks):**
1. GEO-001: Mesh System
2. GEO-002: Normal & Tangent Calculation
3. GEO-003: LOD System
4. GEO-004: CSG Operations

**Jules-Materials (4 tasks):**
1. MAT-001: Material System
2. MAT-002: Shader Compilation
3. MAT-003: Material Editor
4. MAT-004: Texture Sampling

**Jules-Lighting (4 tasks):**
1. LIGHT-001: Light System
2. LIGHT-002: Shadow Mapping
3. LIGHT-003: Global Illumination
4. LIGHT-004: Lightmapping

**Jules-Effects (4 tasks):**
1. FX-001: Particle System
2. FX-002: Emitters & Behaviors
3. FX-003: Trail Rendering
4. FX-004: Decal System

---

## How Phase 3 Agents Use This Information

### Pre-Phase 3 Checklist:
- [ ] All Phase 1 agents: 19/19 tasks done
- [ ] All Phase 2 agents: 20/20 tasks done
- [ ] Phase 1 + Phase 2 build is green
- [ ] Phase 2 integration tests pass
- [ ] Animation + Audio + AI + Assets + Networking working

### Agent Lead View:
1. Verify Phase 1 + Phase 2 complete
2. Go to `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
3. Find your agent section
4. Review dependencies on Phase 1 + 2
5. Plan which Phase 1/2 systems you need
6. Schedule synchronization points with dependent agents
7. Track progress in `master/todo.csv`

### Developer View:
1. Agent lead assigns you Task #1
2. Verify prerequisite Phase 1/2 systems working
3. Open `master/JULIUS_AGENT_TASK_PROMPTS.md`
4. Find your agent section
5. Search for your task ID
6. Read dependencies section carefully
7. Implement, test, update CSV

### Content Creator View (new role in Phase 3):
1. Geometry agent builds CSG tools
2. Gameplay agent enables crafting
3. Character agent enables customization
4. Materials agent enables visual design
5. Lighting agent enables atmosphere
6. Effects agent enables polish

---

---

## 🎯 PHASE 4: Polish (Weeks 7+)

### Phase 4 Prompt

```
PHASE 4 OBJECTIVE:
Build editor and development tools to make the engine usable for content
creation, debugging, and deployment. All game systems are complete; now
polish the developer experience.

1 AGENT (after all others complete):
1. Jules-Tools - Editor & Tools (level editor, gizmos, asset browser, console)

PHASE 4 SUCCESS:
✅ All 6 tasks marked completed in master/todo.csv
✅ Level editor functional: Build levels visually
✅ Gizmo system working: Move/rotate/scale objects
✅ Asset browser functional: Browse all content
✅ Property editor working: Edit entity properties live
✅ Debug visualization: See physics shapes, AI paths
✅ Developer console: Run commands during development
✅ Full game playable start-to-finish with editor
✅ Build compiles, zero warnings, all tests pass

CRITICAL DEPENDENCIES:
⚠️  DO NOT START until ALL other phases complete
⚠️  Jules-Tools depends on:
    - Jules-Engine-Core (core systems)
    - Jules-Rendering (UI rendering)
    - Jules-Physics (debug visualization)
    - Jules-Animation (animation preview)
    - Jules-Gameplay (entity editing)
    - Jules-Character (character preview)
    - Jules-Geometry (mesh preview)
    - Jules-Materials (material preview)
    - Jules-Lighting (lighting preview)
    - Jules-Effects (effect preview)

DEADLINE: Week 8+
SUCCESS: Full game shipped and playable
```

---

## WHERE TO FIND PHASE 4 DATA

### Agent Task Lists
**Location:** `master/JULIUS_AGENT_TASK_PROMPTS.md`
**Find:** Section for Agent 15 (Jules-Tools)
**Contains:** 6 specific tasks with prompts

**Quick access:**
```bash
grep -A 200 "Agent 15: Jules-Tools" master/JULIUS_AGENT_TASK_PROMPTS.md
```

### Detailed Agent Specification
**Location:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
**Find:** Section "Agent 15: Jules-Tools"
**Contains:** Full mission, ownership, 6 tasks with file locations

**Quick access:**
```bash
grep -A 150 "Agent 15: Jules-Tools" master/JULIUS_15_AGENTS_ASSIGNMENTS.md
```

### Task Status Tracking
**Location:** `master/todo.csv`
**Search:** All rows with task IDs starting with TOOLS-
**Example:**
```bash
grep "^TOOLS-" master/todo.csv
```

### File Ownership for Phase 4
**Location:** `JULIUS_15_QUICK_REFERENCE.md` → "Directory Ownership" section

**Phase 4 Directories:**
- Editor: `src/engine/editor/` (308 files)
- Tools: `src/engine/tools/` (47 files)
- Debug: `src/engine/debug/` (8 files)

### Phase 4 Task Details (6 total)

**Jules-Tools (6 tasks):**
1. TOOLS-001: Level Editor
2. TOOLS-002: Gizmo System
3. TOOLS-003: Property Editor
4. TOOLS-004: Asset Browser
5. TOOLS-005: Debug Visualization
6. TOOLS-006: Developer Console

---

## How Phase 4 Agent Uses This Information

### Pre-Phase 4 Checklist:
- [ ] Phase 1 complete: 19/19 tasks
- [ ] Phase 2 complete: 20/20 tasks
- [ ] Phase 3 complete: 24/24 tasks
- [ ] All prior systems integrated and working
- [ ] Build is green (compiles, zero warnings)
- [ ] No critical blockers or bugs

### Agent Lead View (Jules-Tools):
1. Verify all Phase 1-3 complete
2. Read `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` → Agent 15
3. Understand 6 tasks and dependencies
4. Plan editor architecture
5. Schedule integration points with all 14 other agents
6. Track progress in `master/todo.csv`

### Developer View:
1. Jules-Tools agent lead assigns you Task #1
2. Open `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 15
3. Read your task prompt
4. Review dependencies (requires input from all other systems)
5. Implement, test, update CSV
6. Coordinate with other agent leads for preview integration

---

---

## 📍 COMPLETE DATA REFERENCE

### Master Index
**File:** `master/JULIUS_COMPLETE_INDEX.md`
**Purpose:** Navigation hub for all documentation
**Contains:** Links to all 5 documents, quick reference, learning path

### Quick Reference
**File:** `JULIUS_15_QUICK_REFERENCE.md`
**Purpose:** 1-page lookup tables
**Contains:** Agent matrix, directory ownership, phase timeline

### Detailed Assignments
**File:** `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
**Purpose:** Full specifications for all 15 agents
**Contains:** Mission, ownership, tasks, dependencies, timeline

### Task Prompts
**File:** `master/JULIUS_AGENT_TASK_PROMPTS.md`
**Purpose:** Exact prompts for each agent's tasks
**Contains:** Your Prompt section, CSV entries, workflows

### Phase Prompts
**File:** `master/JULIUS_PHASE_PROMPTS.md` (this file)
**Purpose:** Phase-level coordination
**Contains:** Phase objectives, data locations, checklists

### Status Tracking
**File:** `master/todo.csv`
**Purpose:** Task status database
**Contains:** All 65+ tasks with status (pending/in_progress/completed)

### Task Notes
**File:** `master/todo.md`
**Purpose:** Agent checklist and completion notes
**Contains:** Checklist of all agents, completion notes

### Workflow Rules
**File:** `master/master.md`
**Purpose:** Developer rules and constraints
**Contains:** Git rules, consolidation rules, quality standards

### Quality Expectations
**File:** `master/expectations.md`
**Purpose:** Unreal Engine-level quality bar
**Contains:** What "done" means, testing requirements

---

## 🔍 How to Find Specific Information

### "What's the first task for my agent?"
→ `master/JULIUS_AGENT_TASK_PROMPTS.md` → Find your agent → Task 1

### "What files do I own?"
→ `JULIUS_15_QUICK_REFERENCE.md` → Directory Ownership table

### "What's the status of all Phase 2 tasks?"
→ `master/todo.csv` → `grep "^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv`

### "When does Phase 3 start?"
→ `master/JULIUS_PHASE_PROMPTS.md` → PHASE 2 Prompt → Deadline

### "What systems does my task depend on?"
→ `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` → Your agent → Your task → "Depends On" section

### "What's the acceptance criteria for RENDER-003?"
→ `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 2: Jules-Rendering → Task 3: RENDER-003 → Acceptance section

### "What should I do when I finish my task?"
→ `master/JULIUS_PHASE_PROMPTS.md` → Your phase section → "How Agents Use This" → Workflow

---

## ✅ Phase Completion Verification

### PHASE 1 Completion Checklist
```bash
# Count completed tasks
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv | grep "completed" | wc -l
# Should equal 19 (6 + 7 + 6)

# Verify no in_progress > 2 weeks old
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv | grep "in_progress"
# Should be empty before Phase 2 starts
```

### PHASE 2 Completion Checklist
```bash
# Count completed tasks
grep "^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv | grep "completed" | wc -l
# Should equal 20 (5 + 4 + 4 + 4 + 4)

# Verify Phase 1 still 100%
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv | grep -v "completed" | wc -l
# Should equal 0
```

### PHASE 3 Completion Checklist
```bash
# Count completed tasks
grep "^GAME-\|^CHAR-\|^GEO-\|^MAT-\|^LIGHT-\|^FX-" master/todo.csv | grep "completed" | wc -l
# Should equal 24 (5 + 4 + 4 + 4 + 4 + 4)

# Verify Phase 1 + 2 still 100%
grep "^CORE-\|^RENDER-\|^PHYS-\|^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv | grep -v "completed" | wc -l
# Should equal 0
```

### PHASE 4 Completion Checklist
```bash
# Count completed tasks
grep "^TOOLS-" master/todo.csv | grep "completed" | wc -l
# Should equal 6

# Verify ALL phases complete
grep -v "completed" master/todo.csv | grep "CORE-\|RENDER-\|PHYS-\|ANIM-\|AUDIO-\|AI-\|ASSET-\|NET-\|GAME-\|CHAR-\|GEO-\|MAT-\|LIGHT-\|FX-\|TOOLS-" | wc -l
# Should equal 0 (all tasks completed)
```

---

## 🚀 Ready to Execute

**To start your phase:**

1. Verify previous phase complete (see checklists above)
2. Read the Phase X Prompt (above) for your objectives
3. Go to `master/JULIUS_AGENT_TASK_PROMPTS.md` and find your agent
4. Read each task in order
5. Update `master/todo.csv` as you work
6. Report progress weekly

**Questions?** Everything is in these 6 documents:
- `master/JULIUS_PHASE_PROMPTS.md` (this file)
- `master/JULIUS_COMPLETE_INDEX.md`
- `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
- `master/JULIUS_AGENT_TASK_PROMPTS.md`
- `JULIUS_15_QUICK_REFERENCE.md`
- `master/todo.csv`

---

End of Phase Prompts
