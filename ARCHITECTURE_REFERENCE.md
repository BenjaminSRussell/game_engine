# Architecture Reference Guide

## Quick Lookup for File Locations and Dependencies

---

## Current Engine Organization

```
src/engine/
│
├── core/                           (Engine foundation: memory, logging, lifecycle)
│   ├── engine.c                    Entry point
│   ├── engine_core.c               Core systems
│   ├── game_loop.c                 Main loop
│   ├── asset_manager.c             💰 WE EXTEND THIS
│   └── (73+ dirs for utilities)
│
├── renderer/                        (Graphics pipeline - 111 directories)
│   ├── core/
│   │   ├── render_graph.c          💰 WE EXTEND THIS
│   │   ├── renderer.c
│   │   └── material_system.c
│   ├── gpu/
│   │   ├── gpu_texture.c           💰 WE USE FOR PREVIEWS
│   │   ├── gpu_memory.c
│   │   └── gpu_particles.c
│   ├── lighting/
│   ├── materials/                  (15 dirs)
│   ├── post_processing/
│   ├── shadows/
│   ├── particles/
│   └── ... (many more)
│
├── physics/                         (36 directories - CONSOLIDATE)
│   ├── physics_core_impl.c         🔴 CONSOLIDATE
│   ├── physics_complete.c          🔴 CONSOLIDATE
│   ├── physics_advanced_impl.c     🔴 CONSOLIDATE
│   ├── physics_api_impl.c          🔴 CONSOLIDATE
│   ├── simulation_loop.c           🔴 CONSOLIDATE
│   ├── physics_engine.c            ✨ NEW (consolidated)
│   ├── physics_simulation.c        ✨ NEW (consolidated)
│   ├── solver/                     (19 dirs)
│   ├── collision/                  (10 dirs)
│   ├── constraints/                (9 dirs)
│   └── ... (many more)
│
├── animation/                       (30 directories - GOOD)
│   ├── core/
│   │   ├── skeleton.c
│   │   ├── animation_system.c      💰 WE USE FOR ASSET ANIMATION
│   │   └── blending.c
│   ├── ik/
│   ├── skeletal/
│   ├── blend_trees/
│   └── ... (more)
│
├── ecs/                             (Entity component system - USE AS-IS)
│   ├── entity.c                    💰 WE EXTEND WITH AssetInstanceComponent
│   ├── entity_component_system.h
│   ├── archetype.c
│   └── ...
│
├── editor/                          (28 directories)
│   ├── asset_placement.c           ✨ NEW (coordinator)
│   ├── asset_placement.h           ✨ NEW (public API)
│   ├── viewport_tools.c
│   ├── gizmo_api_bridge.h
│   ├── selection_api_bridge.h
│   ├── outline_rendering_api_bridge.h
│   └── ... (many more)
│
├── platform/                        (67 directories)
│   ├── swift_bridge.h              💰 WE EXTEND
│   ├── engine_bridge.h
│   ├── [43 *_api_bridge.h files]
│   ├── drag_drop_bridge.h          ✨ NEW (adapter)
│   ├── ios/
│   │   ├── drag_drop_ios.m         ✨ NEW
│   │   └── ...
│   ├── macos/
│   │   ├── drag_drop_macos.m       ✨ NEW
│   │   └── ...
│   └── ...
│
├── audio/                           (39 directories)
│   └── (use for spatial audio)
│
├── ai/                              (47 directories)
│   └── (keep as-is, may reorganize)
│
├── networking/                      (14 directories)
│   └── (use for multiplayer framework)
│
├── world_building/                  (8 directories)
│   └── (use for terrain/environment)
│
├── scripting/                       (core, lua, visual_scripting)
│   └── (use existing)
│
└── [40+ more subsystems]
```

---

## Include Directory (Public APIs)

```
include/
├── renderer/
│   └── renderer_api.h              (Renderer public API)
│
├── physics/
│   └── physics.h                   (Physics public API)
│
├── editor/
│   ├── asset_placement.h           ✨ NEW
│   └── (existing editors)
│
├── platform/
│   ├── swift_bridge.h              (Extended)
│   ├── drag_drop_bridge.h          ✨ NEW
│   └── [43 *_api_bridge.h]
│
├── assets/
│   └── asset_manager.h
│
├── ecs/
│   └── entity_component_system.h
│
├── audio/
├── ai/
├── animation/
└── [40+ more]
```

---

## Files to DELETE (Phase 0)

```
DELETE ENTIRE DIRECTORY:
/src/frontend/engine/studio/CVoxelForge/include/engine/include/

This is an exact copy of /src/engine/include/
Contains: 200+ duplicate files
Reason: Single source of truth
After: Build CMake to use /src/engine/include/ instead
```

---

## Files to CONSOLIDATE (Phase 1)

```
FROM (5 files):
  src/engine/physics/physics_core_impl.c      (45KB)
  src/engine/physics/physics_complete.c       (9KB)
  src/engine/physics/physics_advanced_impl.c  (unknown)
  src/engine/physics/physics_api_impl.c       (7KB)
  src/engine/physics/simulation_loop.c        (19KB)

TO (2 files):
  src/engine/physics/physics_engine.c         (consolidated dynamics)
  src/engine/physics/physics_simulation.c     (consolidated loop)

DELETE:
  physics_complete.c
  physics_advanced_impl.c
  physics_api_impl.c (functions moved to physics_engine.c)

KEEP EXISTING:
  solver/ subdirectory
  collision/ subdirectory
  constraints/ subdirectory
  All other modular components
```

---

## Files to EXTEND (Phases 2-6)

### Asset Manager - Phase 2
```
FILE: src/engine/resource/asset_manager.c
EXTEND WITH:
  + Handle asset_create_instance(Handle asset, Vec3 pos, Quat rot)
  + AssetInstanceData* asset_get_instance_data(Handle instance)
  + void asset_get_instances(Handle asset, Handle *out, u32 *out_count)
  + void asset_replace_all_instances(Handle old, Handle new)

HEADER: include/assets/asset_manager.h
ADD:
  typedef struct AssetInstanceData { ... }
```

### Render Graph - Phase 2.2
```
FILE: src/engine/renderer/core/render_graph.c
EXTEND WITH:
  + void render_graph_add_preview_pass(RenderGraph *g, Handle asset, Mat4 xf, RenderTarget *out)

USE EXISTING:
  src/engine/renderer/gpu/gpu_texture.c for framebuffers
```

### Entity System - Phase 3
```
FILE: src/engine/ecs/entity.c
EXTEND WITH:
  + AssetInstanceComponent registration
  + asset_instance_serialize()
  + asset_instance_deserialize()

HEADER: include/ecs/entity_component_system.h
ADD:
  typedef struct AssetInstanceComponent { ... }
  #define COMPONENT_TYPE_ASSET_INSTANCE
```

### Physics Engine - Phase 4
```
FILE: src/engine/physics/physics_engine.c (after consolidation)
EXTEND WITH:
  + bool physics_raycast(Ray ray, RaycastHit *out)
  + bool physics_validate_placement(Collider, Vec3, Layer, Entity*)
  + Vec3 physics_simulate_gravity_settle(Vec3, Collider, f32)

HEADER: include/physics/physics.h
UPDATE: Add new function declarations
```

### Swift Bridge - Phase 6
```
FILE: src/engine/platform/swift_bridge.h
EXTEND WITH:
  + Entity asset_placement_create(Handle, Vec3, Quat)
  + void asset_placement_preview_render(Handle, Vec3)
  + ImageData* asset_get_preview_image(Handle, u32, u32)

IMPLEMENTATION: src/engine/platform/swift_bridge.c
ADD: Bridge function implementations
```

---

## Files to CREATE (Phases 5-6)

### Phase 5: Central Coordinator (HEART OF SYSTEM)
```
NEW FILE: src/engine/editor/asset_placement.c
PUBLIC HEADER: include/editor/asset_placement.h

PURPOSE: Orchestrate all systems for placement
COORDINATES: ECS + Physics + Renderer + Asset Manager
ENTRY POINT: bool asset_placement_execute(GameAPI*, AssetPlacementRequest*)

FUNCTIONS:
  bool asset_placement_execute(...)
  void asset_placement_render_preview(...)
  bool asset_placement_validate_position(...)
```

### Phase 6: Platform Adapters
```
NEW FILE: include/platform/drag_drop_bridge.h
PURPOSE: Platform-agnostic drag-drop interface

PLATFORM IMPLEMENTATIONS:
  NEW: src/engine/platform/ios/drag_drop_ios.m
    - Translates UIDragInteraction → engine placement

  NEW: src/engine/platform/macos/drag_drop_macos.m
    - Translates NSDraggingDestination → engine placement
```

---

## Swift Files (Mostly Existing)

### Extend or Create
```
src/frontend/engine/studio/Sources/
├── Components/
│   ├── AssetPaletteView.swift      (extend or new)
│   └── QuickPropertyPanel.swift    (extend or new)
│
├── SceneView/
│   ├── ViewportView.swift          (extend - add drop target)
│   └── [existing]
│
└── [use all existing UI infrastructure]
```

---

## Dependencies Map

### asset_placement.c (Coordinator) Depends On:
```
        ┌─────────────────────────────────┐
        │  asset_placement.c              │
        │  (Central Coordinator)          │
        └──────────────┬────────────────────┘
        ┌──────────────┼──────────────┬──────────────┐
        ▼              ▼              ▼              ▼
    ┌────────┐   ┌─────────┐   ┌──────────┐   ┌──────────┐
    │  ECS   │   │ Physics │   │ Renderer │   │  Asset   │
    │ entity │   │raycast  │   │ preview  │   │ manager  │
    │creation│   │validate │   │  render  │   │instances │
    └────────┘   └─────────┘   └──────────┘   └──────────┘
        ▲            ▲              ▲              ▲
        │            │              │              │
        ├────────────┼──────────────┼──────────────┤
        │   Existing, just expose APIs             │
        └─────────────────────────────────────────┘
```

### NO System Knows About asset_placement.c
```
Physics ─┐
         ├─→ asset_placement.c ──→ Swift Bridge
Renderer ┤
         ├─→ Only asset_placement
ECS ─────┤   knows about drag-drop
Assets ──┘
```

This is CLEAN SEPARATION OF CONCERNS.

---

## Integration Points (Where Systems Connect)

### 1. Asset Manager Integration
```
asset_manager.c
  ├─ asset_create_instance()        (NEW - creates instance)
  ├─ asset_get_instance_data()      (NEW - retrieves instance)
  └─ asset_get_instances()          (NEW - finds all instances)
```

### 2. ECS Integration
```
entity.c
  ├─ AssetInstanceComponent         (NEW - component type)
  └─ ecs_entity_add_component()     (EXISTING - used for instances)
```

### 3. Physics Integration
```
physics_engine.c (consolidated)
  ├─ physics_raycast()              (EXISTING - exposed)
  ├─ physics_validate_placement()   (NEW)
  └─ physics_simulate_gravity_settle() (NEW)
```

### 4. Renderer Integration
```
render_graph.c
  ├─ render_graph_add_preview_pass() (NEW)
  └─ Existing passes continue to work
```

### 5. Swift Bridge
```
swift_bridge.h
  ├─ asset_placement_create()       (NEW)
  ├─ asset_placement_preview_render() (NEW)
  └─ All existing bridge functions continue to work
```

---

## Data Flow: Drag-and-Drop Operation

```
User drags asset from palette
    ↓
iOS/macOS drag event
    ↓
drag_drop_ios.m / drag_drop_macos.m (platform adapters)
    ↓
drag_drop_bridge_on_drop() call
    ↓
Swift UI calls: asset_placement_create(asset, pos, rot)
    ↓
swift_bridge.c calls: asset_placement_execute()
    ↓
asset_placement.c:
  1. physics_raycast() → find surface
  2. physics_validate_placement() → check valid
  3. physics_simulate_gravity_settle() → find final position
  4. ecs_entity_create() → create entity
  5. ecs_entity_add_component(AssetInstanceComponent)
  6. ecs_entity_add_component(Transform)
    ↓
Entity rendered by existing renderer
    ↓
User sees asset placed in viewport
```

---

## Testing at Each Layer

```
                    Swift UI Tests
                          ↓
              Swift Bridge Tests
                          ↓
            asset_placement Tests (C)
                          ↓
    ┌──────────┬──────────┼──────────┬──────────┐
    ↓          ↓          ↓          ↓          ↓
 Asset      Physics      ECS     Renderer   Animation
 Tests      Tests       Tests     Tests      Tests
```

Each layer independently testable.

---

## Build System Changes

### CMakeLists.txt

**Remove:**
```cmake
# PHYSICS_SOURCES
list(APPEND PHYSICS_SOURCES
  src/engine/physics/physics_core_impl.c    # REMOVE
  src/engine/physics/physics_complete.c     # REMOVE
  src/engine/physics/physics_advanced_impl.c # REMOVE
)
```

**Update:**
```cmake
# PHYSICS_SOURCES (consolidated)
list(APPEND PHYSICS_SOURCES
  src/engine/physics/physics_engine.c       # NEW
  src/engine/physics/physics_simulation.c   # CONSOLIDATED
  src/engine/physics/solver/...             # KEEP
  src/engine/physics/collision/...          # KEEP
)

# EDITOR_SOURCES
list(APPEND EDITOR_SOURCES
  src/engine/editor/asset_placement.c       # NEW
)

# PLATFORM_SOURCES
list(APPEND PLATFORM_SOURCES
  src/engine/platform/drag_drop_bridge.c    # NEW
  src/engine/platform/ios/drag_drop_ios.m   # NEW
  src/engine/platform/macos/drag_drop_macos.m # NEW
)
```

---

## Verification Checklist

### After Phase 0 (Deduplication)
- [ ] CVoxelForge include mirror deleted
- [ ] Studio builds with shared includes
- [ ] 43 API bridges accessible from Studio
- [ ] No include files in two places

### After Phase 1 (Consolidation)
- [ ] 5 physics files merged to 2
- [ ] All physics tests pass
- [ ] Demos still work
- [ ] No behavioral changes

### After Phase 5 (Coordinator)
- [ ] asset_placement_execute() fully functional
- [ ] Coordinates ECS + Physics + Renderer + Assets
- [ ] Single entry point for placement
- [ ] All tests pass

### After Phase 6 (Bridge)
- [ ] Swift can call all placement functions
- [ ] iOS handlers work
- [ ] macOS handlers work
- [ ] Platform events translate correctly

### After Phase 7 (UI)
- [ ] Can drag from palette
- [ ] Can see preview while dragging
- [ ] Can drop to place
- [ ] Can select and modify
- [ ] 60 FPS smooth interaction

### After Phase 8 (Testing)
- [ ] Full integration tests pass
- [ ] Save/load works
- [ ] Error handling works
- [ ] Performance targets met
- [ ] Production ready

---

## File Count Summary

```
DELETED (Phase 0):        -200 files (CVoxelForge mirror)
CONSOLIDATED (Phase 1):    -3 files (physics)
EXTENDED (Phases 2-6):     +5 existing files updated
CREATED (Phases 5-6):     +5-10 new files

NET CHANGE:               -198 files removed, +10 added = -188 net
                          Much cleaner codebase!
```

---

## Quick Links

```
For Understanding:
  Read: ARCHITECTURE_INTEGRATION_PLAN.md

For Implementation:
  Read: FOCUSED_IMPLEMENTATION_TODOS.md

For Quick Reference:
  Read: IMPLEMENTATION_START_HERE.md

For Big Picture:
  Read: PLAN_SUMMARY.md

For File Locations:
  Read: ARCHITECTURE_REFERENCE.md (this document)
```

---

## Commands for Implementation

### Phase 0 - Deduplication
```bash
rm -rf src/frontend/engine/studio/CVoxelForge/include/engine/include/
cd build && cmake .. && make
```

### Phase 1 - Physics Build
```bash
cd build && cmake .. && make
make test  # Run physics tests
```

### Each Phase
```bash
cd build && cmake .. && make
make test
git add .
git commit -m "phase N: [description]"
```

---

## Success Criteria

| Criterion | Target |
|-----------|--------|
| Duplication | 0 duplicate files |
| Integration Points | 1 (asset_placement.c) |
| Phases Complete | 8/8 |
| Tests Passing | 100% |
| Build Time | < 2 minutes |
| FPS While Dragging | 60 |
| Entity Creation Latency | < 50ms |
| Codebase Quality | Better organized |
| Maintainability | Significantly improved |

---

Done! You now have:

1. **Full understanding** (ARCHITECTURE_INTEGRATION_PLAN.md)
2. **Detailed steps** (FOCUSED_IMPLEMENTATION_TODOS.md)
3. **Quick reference** (IMPLEMENTATION_START_HERE.md)
4. **Big picture** (PLAN_SUMMARY.md)
5. **File locations** (ARCHITECTURE_REFERENCE.md)

Ready to implement?
