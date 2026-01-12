# 15 Jules Agents - Quick Reference Guide

## The Architecture

**Goal:** Parallelize engine development across 15 specialized teams. Each agent owns one engine subsystem with clear file boundaries, zero overlap, and independent task assignments.

---

## Agent Matrix

| # | Agent | System | Files | Tasks | Start | Priority |
|---|-------|--------|-------|-------|-------|----------|
| 1 | Jules-Engine-Core | Core Engine | 198 | 6 | PHASE 1 | P0 |
| 2 | Jules-Rendering | Graphics | 361 | 7 | PHASE 1 | P0 |
| 3 | Jules-Physics | Physics | 224 | 6 | PHASE 1 | P0 |
| 4 | Jules-Animation | Animation | 55 | 5 | PHASE 2 | P1 |
| 5 | Jules-Audio | Audio | 67 | 4 | PHASE 2 | P1 |
| 6 | Jules-AI | AI/NPC | 108 | 4 | PHASE 2 | P1 |
| 7 | Jules-Assets | Assets | 140 | 4 | PHASE 2 | P1 |
| 8 | Jules-Networking | Network | 44 | 4 | PHASE 2 | P1 |
| 9 | Jules-Gameplay | Gameplay | 22 | 5 | PHASE 3 | P1 |
| 10 | Jules-Character | Character | 130 | 4 | PHASE 3 | P1 |
| 11 | Jules-Geometry | Geometry | 134 | 4 | PHASE 3 | P1 |
| 12 | Jules-Materials | Materials | 89 | 4 | PHASE 3 | P1 |
| 13 | Jules-Lighting | Lighting | 131 | 4 | PHASE 3 | P1 |
| 14 | Jules-Effects | VFX/Particles | 79 | 4 | PHASE 3 | P1 |
| 15 | Jules-Tools | Editor | 308 | 6 | PHASE 4 | P2 |

**Total:** 2,385 files, 65+ critical tasks

---

## Directory Ownership

```
src/engine/
├── core/                          → Jules-Engine-Core
├── rendering/                     → Jules-Rendering
├── backend/                       → Jules-Rendering (Metal/Vulkan)
├── shading/                       → Jules-Materials
├── physics/                       → Jules-Physics
├── animation/                     → Jules-Animation
├── audio/                         → Jules-Audio
├── ai/                            → Jules-AI
├── assets/                        → Jules-Assets
├── network/                       → Jules-Networking
├── networking/                    → Jules-Networking
├── gameplay/                      → Jules-Gameplay
├── character/                     → Jules-Character
├── geometry/                      → Jules-Geometry
├── materials/                     → Jules-Materials
├── lighting/                      → Jules-Lighting
├── effects/                       → Jules-Effects
├── editor/                        → Jules-Tools
├── tools/                         → Jules-Tools
├── debug/                         → Jules-Tools
├── ecs/                           → Jules-Engine-Core
├── ui/                            → Jules-Gameplay
├── scene/                         → Jules-Engine-Core
├── platform/                      → Jules-Rendering (backend)
└── postprocess/                   → Jules-Rendering
```

---

## PHASE 1: Foundation (Start Here)

**Duration:** 2 weeks  
**Goal:** Get engine running with pixels on screen and physics simulating

### Jules-Engine-Core (6 critical tasks)
1. **CORE-001:** Engine lifecycle (init → update → shutdown)
2. **CORE-002:** Memory allocator consolidation
3. **CORE-003:** Thread pool validation
4. **CORE-004:** Virtual file system
5. **CORE-005:** Logging consolidation
6. **CORE-006:** Hot reload infrastructure

**Success:** Engine initializes, runs 10 frames clean, shuts down with zero leaks

### Jules-Rendering (7 critical tasks)
1. **RENDER-001:** Renderer init (Metal/Vulkan/OpenGL)
2. **RENDER-002:** Shader compilation pipeline
3. **RENDER-003:** Frame graph execution
4. **RENDER-004:** GPU memory management
5. **RENDER-005:** Vulkan backend (fix or remove)
6. **RENDER-006:** Metal synchronization
7. **RENDER-007:** Post-processing pipeline

**Success:** Triangle renders on screen at 60 FPS on all platforms

### Jules-Physics (6 critical tasks)
1. **PHYS-001:** Physics solver consolidation
2. **PHYS-002:** Collision detection
3. **PHYS-003:** Constraint solving
4. **PHYS-004:** Deterministic simulation
5. **PHYS-005:** Performance optimization
6. **PHYS-006:** Continuous collision detection

**Success:** 1000+ objects simulating at 60 FPS, no jitter

---

## PHASE 2: Systems (Parallel these - start Week 3)

Can run in parallel once Phase 1 foundation is solid.

### Jules-Animation (5 tasks) - depends on Engine-Core
- Skeletal animation system
- Animation state machines
- IK solvers
- Motion matching
- Procedural animation

### Jules-Audio (4 tasks) - depends on Engine-Core
- Audio playback system
- Spatial audio (3D)
- Music streaming
- DSP effects

### Jules-AI (4 tasks) - depends on Physics
- Pathfinding system (A*)
- Behavior trees
- GOAP planning
- Perception system

### Jules-Assets (4 tasks) - depends on Engine-Core
- Asset loader (FBX, glTF, OBJ)
- Texture management
- Animation import
- Asset streaming

### Jules-Networking (4 tasks) - depends on Physics
- Socket layer
- State replication
- Client prediction
- Server reconciliation

---

## PHASE 3: Integration (Start Week 5)

Content and high-level systems - can parallelize.

### Jules-Gameplay (5 tasks)
- Crafting system
- Quest system
- Combat system
- Food system
- Interaction system

### Jules-Character (4 tasks) - depends on Animation
- Skeletal mesh system
- Character customization
- Ragdoll physics
- Equipment system

### Jules-Geometry (4 tasks) - depends on Rendering
- Mesh system
- Normal/tangent calculation
- LOD generation
- CSG operations

### Jules-Materials (4 tasks) - depends on Rendering + Shading
- Material system
- Shader compilation
- Material editor
- Texture sampling

### Jules-Lighting (4 tasks) - depends on Rendering
- Light system
- Shadow mapping
- Global illumination
- Lightmapping

### Jules-Effects (4 tasks) - depends on Rendering
- Particle system
- Emitters & behaviors
- Trail rendering
- Decal system

---

## PHASE 4: Polish (Week 7+)

### Jules-Tools (6 tasks) - depends on all others
- Level editor
- Gizmo system
- Property editor
- Asset browser
- Debug visualization
- Developer console

---

## Success Checkpoints

### End of PHASE 1 (Week 2)
✅ Engine compiles and runs  
✅ Pixels on screen  
✅ Physics objects moving  
✅ Zero memory leaks  
✅ No race conditions  

### End of PHASE 2 (Week 4)
✅ All 5 systems operational  
✅ Animation working  
✅ Audio playing  
✅ AI pathfinding functional  
✅ Assets loading from files  
✅ Network basic structure  

### End of PHASE 3 (Week 6)
✅ Character in game world  
✅ Game mechanics working (craft, kill, quest)  
✅ Complex levels possible  
✅ Beautiful materials and lighting  
✅ Visual effects polished  

### End of PHASE 4+ (Week 8+)
✅ Full editor functional  
✅ Game playable start-to-finish  
✅ Performance targets met  
✅ All systems stable  
✅ Codebase maintainable  

---

## How to Check Agent Boundaries

1. Open `master/JULES_15_AGENTS_ASSIGNMENTS.md`
2. Find your agent section
3. Review "Ownership" section
4. Check "Directory Ownership" section in this guide
5. **Before editing any file:**
   - Verify your agent owns that subsystem
   - Check no overlap with other agents
   - Update `master/todo.csv` with task ID

---

## Key Rules

🚫 **Do NOT:**
- Work on files you don't own
- Create duplicate implementations
- Disable/backup files - fix in-place
- Start PHASE 2 tasks if PHASE 1 isn't solid
- Commit without updating `master/todo.csv`

✅ **Do:**
- Verify ownership before editing
- Report blockers immediately if dependent task incomplete
- Update task status weekly
- Consolidate code ONLY after build is green
- Test your changes end-to-end

---

## Contact & Coordination

**Daily:**
- Check `master/JULES_15_AGENTS_ASSIGNMENTS.md` for latest status
- Verify your agent's dependencies are progressing

**Weekly:**
- Report % complete, blockers, what's next
- Flag any cross-agent issues
- Validate no duplicate work

**If Blocked:**
- Check if you're waiting on another agent
- Open issue in master/ folder
- Request help in sync-up

---

## Ownership Quick Lookup

**Looking for which agent owns a file?**

Use this pattern matching:

- `src/engine/core/` → Jules-Engine-Core
- `src/engine/rendering/` → Jules-Rendering
- `src/engine/backend/` → Jules-Rendering
- `src/engine/physics/` → Jules-Physics
- `src/engine/animation/` → Jules-Animation
- `src/engine/audio/` → Jules-Audio
- `src/engine/ai/` → Jules-AI
- `src/engine/assets/` → Jules-Assets
- `src/engine/network/` + `src/engine/networking/` → Jules-Networking
- `src/engine/gameplay/` → Jules-Gameplay
- `src/engine/character/` → Jules-Character
- `src/engine/geometry/` → Jules-Geometry
- `src/engine/materials/` + `src/engine/shading/` → Jules-Materials
- `src/engine/lighting/` → Jules-Lighting
- `src/engine/effects/` → Jules-Effects
- `src/engine/editor/` + `src/engine/tools/` + `src/engine/debug/` → Jules-Tools

---

## Example: Starting a Task

1. **Choose your agent** (e.g., Jules-Rendering)
2. **Pick a task** (e.g., RENDER-001: Renderer Initialization)
3. **Read full spec** in `JULES_15_AGENTS_ASSIGNMENTS.md` under your agent
4. **Verify ownership:**
   - Files: `src/engine/rendering/*` ✅ (owned by Jules-Rendering)
   - Files: `src/engine/backend/*` ✅ (owned by Jules-Rendering)
5. **Check dependencies:**
   - Depends on: Jules-Engine-Core completion ✅ (check status)
6. **Plan implementation:**
   - Review acceptance criteria
   - List functions to implement
   - Create test plan
7. **Update master/todo.csv:**
   - Add: `RENDER-001,implementation,Rendering,Jules-Rendering,Renderer initialization,in_progress`
8. **Implement & test**
9. **Mark complete:**
   - Update CSV: status = "completed"
   - Add note to todo.md with test results
   - Commit with reference to RENDER-001

---

End of Quick Reference
