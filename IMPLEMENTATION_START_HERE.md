# START HERE: Implementation Guide

## Overview
You now have 3 complementary planning documents that work together:

1. **ARCHITECTURE_INTEGRATION_PLAN.md** - Why and how
2. **FOCUSED_IMPLEMENTATION_TODOS.md** - Detailed checklist
3. **This document** - Quick reference

---

## The Right Way (What We're Doing)

### ❌ Wrong Approach
Create new systems, new folders, new bridges, duplicate code
```
Result: 500 new files, maintenance nightmare, duplication
```

### ✅ Right Approach
1. **Eliminate duplication** (CVoxelForge mirror)
2. **Consolidate scattered code** (5 physics files → 2)
3. **Extend existing systems** (add instance API to asset_manager)
4. **Create facades/coordinators** (asset_placement.c)
5. **Add platform adapters** (not duplicate bridges)

```
Result: +5-10 files, clean architecture, zero duplication, fully testable
```

---

## Understanding the Architecture

### Current Engine Structure (GOOD!)
```
src/engine/
├── core/                 (Memory, logging, containers, engine lifecycle)
├── renderer/             (111 dirs - rendering pipeline with 43+ API bridges)
├── physics/              (36 dirs - but scattered across 5 impl files)
├── animation/            (30 dirs - well organized, modular)
├── ai/                   (47 dirs - too scattered, but functional)
├── ecs/                  (Entity component system - good)
├── audio/                (39 dirs - well organized)
├── editor/               (28 dirs - 43 API bridges for tools)
├── scripting/            (Lua + visual scripting)
├── networking/           (14 dirs - multiplayer framework)
└── ... (80+ subsystems total)

include/                   (43 API bridges: *_api_bridge.h)
```

### Current Problems (CRITICAL!)
1. **Duplication**: `/src/frontend/engine/studio/CVoxelForge/include/engine/include/` is complete copy of `/src/engine/include/` (200+ duplicate files)
2. **Scattered Physics**: 5 implementation files (physics_core_impl.c, physics_complete.c, physics_advanced_impl.c, physics_api_impl.c, simulation_loop.c)
3. **No Centralization**: Where does physics integrate? Answer: 5 different places
4. **Weak Testing**: Manual test programs, no standard framework

### Our Solution
1. **Single Include Source**: Delete CVoxelForge mirror, use shared includes
2. **Unified Physics**: physics_engine.c + physics_simulation.c (consolidated)
3. **Single Integration Point**: asset_placement.c orchestrates all systems
4. **Proper Testing**: Standard C test framework (Criterion)

---

## The Implementation Path

### Phase 0: DEDUPLICATION (Week 1) 🎯 START HERE
**Goal**: Eliminate 200 duplicate files
**Files**:
- `/src/frontend/engine/studio/CVoxelForge/include/engine/include/` → DELETE
- Update CMake to use `/src/engine/include/` (shared)

**Result**: Single source of truth, -200 files

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 0.1

---

### Phase 1: CONSOLIDATION (Week 2-3)
**Goal**: Clean up scattered implementations
**Tasks**:
- Merge 5 physics files into 2 unified files
- Standardize test framework (Criterion)
- Reorganize AI structure (optional but clean)

**Result**: Clear, maintainable physics architecture

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 1.1-1B.1

---

### Phase 2: ASSET SYSTEM (Week 4-5)
**Goal**: Support asset instances
**Extend**: `src/engine/resource/asset_manager.c`
**Add**:
```c
Handle asset_create_instance(Handle asset, Vec3 pos, Quat rot);
AssetInstanceData* asset_get_instance_data(Handle instance);
void asset_get_instances(Handle asset, Handle *out, u32 *out_count);
```

**Result**: Asset system can create and track instances

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 2.1-2.2

---

### Phase 3: ECS INTEGRATION (Week 6)
**Goal**: Instances work with world
**Create**: AssetInstanceComponent (struct)
**Extend**: `src/engine/ecs/entity.c` to register component

**Result**: Instances are first-class ECS entities

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 3.1

---

### Phase 4: PHYSICS QUERIES (Week 7)
**Goal**: Placement validation and settling
**Extend**: `src/engine/physics/physics_engine.c`
**Add**:
```c
bool physics_raycast(Ray ray, RaycastHit *out);
bool physics_validate_placement(Collider, Vec3, Layer, Entity*);
Vec3 physics_simulate_gravity_settle(Vec3, Collider, f32);
```

**Result**: Physics supports placement operations

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 4.1

---

### Phase 5: CENTRAL COORDINATOR (Week 8) 🌟 CORE
**Goal**: Single entry point for drag-and-drop
**Create**: `src/engine/editor/asset_placement.c` (NEW)
**Facade**:
```c
bool asset_placement_execute(GameAPI *api, AssetPlacementRequest *req) {
  // 1. Raycast to surface (physics)
  // 2. Validate placement (physics)
  // 3. Simulate settling (physics)
  // 4. Create entity (ECS)
  // 5. Set transform (renderer/transform)
  // Return: created entity + position
}
```

**This is the heart of the system.**

**Result**: Drag-and-drop logic centralized, testable, uses all systems

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 5.1

---

### Phase 6: SWIFT BRIDGE (Week 9)
**Goal**: UI can call drag-and-drop
**Extend**: `src/engine/platform/swift_bridge.h`
**Add**:
```c
Entity asset_placement_create(Handle asset, Vec3 pos, Quat rot);
void asset_placement_preview_render(Handle asset, Vec3 pos);
ImageData* asset_get_preview_image(Handle asset, u32 w, u32 h);
```

**Create**: Platform handlers:
- `src/engine/platform/ios/drag_drop_ios.m`
- `src/engine/platform/macos/drag_drop_macos.m`

**Result**: Swift can trigger placement

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 6.1-6.2

---

### Phase 7: UI (Weeks 10-11)
**Goal**: User-facing interface
**Build**:
1. Asset palette (grid view, draggable)
2. Viewport as drop target
3. Quick properties panel

**Uses**: Existing Swift UI code, just connects to asset_placement bridge

**Result**: Complete drag-and-drop workflow

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 7.1-7.3

---

### Phase 8: TESTING (Week 12)
**Goal**: Validate everything works
**Test**:
- Full drag-drop workflow (C)
- Asset persistence (save/load)
- Error conditions
- Performance (60 FPS preview)
- iOS and macOS

**Result**: Production-ready system

**See**: `FOCUSED_IMPLEMENTATION_TODOS.md` Section 8.1-8.2

---

## Key Architectural Decisions

### 1. NO NEW RENDERING SYSTEM
- Use existing `render_graph.c` (already has multiple passes)
- Use existing GPU texture system (already has framebuffers)
- Just add: preview pass + selection highlight

### 2. NO NEW PHYSICS SYSTEM
- Consolidate existing physics code
- Use existing raycast API
- Add: placement validation functions

### 3. NO NEW ECS SYSTEM
- Use existing ECS completely
- Add: AssetInstanceComponent (just another component)
- Instances are normal entities with special component

### 4. SINGLE COORDINATOR
- `asset_placement.c` is the ONLY place that knows about drag-and-drop
- Everything else is unaware of placement feature
- Clean separation of concerns

### 5. SINGLE SWIFT BRIDGE
- One bridge for ALL platform communication
- Platform-specific adapters (iOS/macOS) are thin layers
- No duplicate bridge files

---

## What Each File Does

### Consolidation Phase
```
physics_engine.c              Contains: All core physics from 5 impl files
physics_simulation.c          Contains: Main simulation loop
(these 2 files replace 5 scattered files)
```

### Extension Phase
```
asset_manager.c               Extended: +instance creation API
render_graph.c                Extended: +preview rendering pass
entity.c                      Extended: +AssetInstanceComponent registration
swift_bridge.h                Extended: +placement API functions
```

### New Files (Minimal)
```
asset_placement.c             Coordinator: Orchestrates placement workflow
asset_placement.h             Public API for placement

drag_drop_bridge.h            Platform abstraction
ios/drag_drop_ios.m           Platform handler
macos/drag_drop_macos.m       Platform handler
```

### Swift (Using Existing)
```
AssetPaletteView.swift        Uses existing asset browser or minimal new
ViewportView.swift            Uses existing viewport
QuickPropertyPanel.swift      Uses existing property editor
```

---

## Testing Strategy

### Per-System Testing (Each Phase)
```
Phase 2: Test asset_create_instance() works
Phase 3: Test AssetInstanceComponent serialization
Phase 4: Test physics raycast and validation
Phase 5: Test asset_placement_execute() end-to-end
Phase 6: Test bridge functions callable from Swift
Phase 7: Test UI placement workflow
Phase 8: Test full integration
```

### NO Integration Hell
- Each phase is independently testable
- All previous phases still pass their tests
- Tests use standard framework (Criterion)

---

## Critical Success Factors

### DO's
✅ Extend existing systems
✅ Create facades/coordinators
✅ Add platform adapters
✅ Test each phase independently
✅ Use existing APIs and structures
✅ Keep things in logical places

### DON'Ts
❌ Create new rendering system
❌ Create new physics system
❌ Duplicate headers/bridges
❌ Create platform-specific duplicates
❌ Scatter logic across files
❌ Skip consolidation phase

---

## Execution Checklist

### Before Starting
- [ ] Read ARCHITECTURE_INTEGRATION_PLAN.md (understand why)
- [ ] Read FOCUSED_IMPLEMENTATION_TODOS.md (detailed steps)
- [ ] Understand Phase 0 is critical foundation
- [ ] Verify Phase 0 will delete 200 files (then rebuild)

### Phase 0
- [ ] Delete CVoxelForge include mirror
- [ ] Update CMake paths
- [ ] Build and verify no errors
- [ ] Commit deduplication

### Phase 1
- [ ] Merge physics files
- [ ] Adopt test framework
- [ ] All tests pass
- [ ] Commit consolidation

### Phases 2-8
- [ ] Follow FOCUSED_IMPLEMENTATION_TODOS.md step by step
- [ ] Test each phase
- [ ] Verify no regressions
- [ ] Commit per phase

---

## How to Read the Documents

### ARCHITECTURE_INTEGRATION_PLAN.md
**Use When**: You want to understand the WHY
- Current state analysis
- What's wrong (duplication, scattering)
- What's right (existing architecture)
- High-level approach

**Read**: Parts 1-3 (why we're doing this)

### FOCUSED_IMPLEMENTATION_TODOS.md
**Use When**: You're implementing and need detailed steps
- Section by section
- Specific files to modify
- Exact functions to add
- Test instructions

**Read**: Section for current phase

### This Document
**Use When**: You need quick reference
- Which phase to do
- How long it takes
- What each file does
- Key decisions

**Read**: Before starting, for overview

---

## Timeline

| Phase | Duration | Focus | Result |
|-------|----------|-------|--------|
| 0 | 1 week | Deduplication | -200 files |
| 1 | 1 week | Consolidation | -3 files, cleaner |
| 2 | 1 week | Asset instances | Instance API |
| 3 | 1 week | ECS integration | Instances in world |
| 4 | 1 week | Physics queries | Placement validation |
| 5 | 1 week | Coordinator | Single entry point |
| 6 | 1 week | Swift bridge | UI can call C |
| 7 | 2 weeks | UI implementation | User-facing features |
| 8 | 1 week | Testing | Production ready |
| **Total** | **10 weeks** | **End-to-end** | **Working system** |

---

## Next Steps

1. **Confirm you agree with this approach**
   - Eliminate duplication first?
   - Consolidate physics?
   - Single coordinator pattern?

2. **Start Phase 0**
   - Begin deduplication (detailed in FOCUSED_IMPLEMENTATION_TODOS.md)
   - I can help execute this step-by-step

3. **I can help with**
   - Identifying exact files to consolidate
   - Writing consolidation code
   - Running tests
   - Fixing issues
   - Each phase implementation

---

## Questions to Verify

- ✅ Do you want to eliminate the CVoxelForge include duplication?
- ✅ Should we consolidate scattered physics files first?
- ✅ Does the facade approach (asset_placement.c) make sense?
- ✅ Ready to start Phase 0?

Once you confirm, we can begin immediately with concrete implementation.

---

## Key Insight

**You already have everything you need in the engine.**

This isn't about adding new rendering, physics, animation, or AI systems.

It's about:
1. Eliminating duplication (deduplication phase)
2. Organizing existing code better (consolidation phase)
3. Exposing APIs for placement operations (extension phase)
4. Building a thin coordination layer (facade phase)
5. Connecting to Swift UI (bridge phase)

The hard work is already done. We're just connecting it logically.

---

## Ready to Begin?

The three documents above provide everything needed. You can:

1. **Start now** - Begin Phase 0 (deduplication)
2. **Ask questions** - About any phase or decision
3. **Request help** - I can implement any phase
4. **Adjust plan** - If you see a better approach

What would you like to do next?
