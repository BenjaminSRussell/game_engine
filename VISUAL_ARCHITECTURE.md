# Visual Architecture Diagrams

## Current Architecture (What We Have)

### Engine Subsystems Overview
```
                          ┌─────────────────────────────────┐
                          │   MINECRAFT V2 ENGINE CORE       │
                          │     (40,000+ lines)              │
                          └──────────────┬────────────────────┘
                                          │
                ┌─────────────────────────┼─────────────────────────┐
                │                         │                         │
        ┌───────▼────────┐      ┌─────────▼──────────┐   ┌──────────▼────────┐
        │   RENDERING    │      │   PHYSICS ENGINE   │   │   ANIMATION SYS   │
        │ 111 dirs       │      │  36 dirs scattered │   │   30 dirs         │
        │ ✅ Clean       │      │  ❌ 5 impl files   │   │   ✅ Modular      │
        └────────────────┘      └────────────────────┘   └───────────────────┘

        ┌───────────────────┐   ┌──────────────┐      ┌────────────────┐
        │   AI SYSTEM       │   │  ECS SYSTEM  │      │  AUDIO SYSTEM  │
        │  47 dirs          │   │  ✅ Good     │      │  39 dirs       │
        │  ❌ Oversized     │   └──────────────┘      │  ✅ Good       │
        └───────────────────┘                         └────────────────┘

        ┌────────────────────┐   ┌────────────┐    ┌─────────────────┐
        │   EDITOR TOOLS     │   │ SCRIPTING  │    │  NETWORKING     │
        │   28 dirs          │   │  ✅ Good   │    │  14 dirs        │
        │   ✅ Modular       │   └────────────┘    │  ✅ Modular     │
        └────────────────────┘                     └─────────────────┘
```

---

## The Problem (Why We Need a Plan)

### Duplication Problem
```
BEFORE (What We Have Now):
┌──────────────────────┐          ┌────────────────────────────────┐
│  src/engine/include/ │          │  CVoxelForge/include/engine/   │
│  (REAL)              │          │  include/                      │
│  200 files           │   ❌COPY │  200 files (DUPLICATE)         │
│  Single source       │◄─────────┤  Out of sync!                  │
└──────────────────────┘          └────────────────────────────────┘

Result: 200 duplicate files, maintenance nightmare, inconsistency

AFTER (What We'll Have):
┌──────────────────────┐
│  src/engine/include/ │
│  (SINGLE SOURCE)     │
│  200 files           │◄─────┐
│  Shared access       │      │
└──────────────────────┘      │
                              │
                    ┌─────────┴────────┐
                    │ Build CMake      │
                    │ symlink/path     │
                    ├─────────┬────────┤
                    │Studio   │Game    │
                    └─────────┴────────┘
```

### Physics Scatter Problem
```
BEFORE (Current Chaos):
┌─────────────────────────────────────────┐
│    Where is physics initialized?        │
│                                         │
│  physics_core_impl.c (45KB) ─┐          │
│  physics_complete.c ────────┼→ CONFUSION
│  physics_advanced_impl.c ───┤           │
│  physics_api_impl.c ────────┼→ 5 files! │
│  simulation_loop.c ─────────┘           │
│                                         │
│  Where does integration happen?         │
│  Search codebase → multiple places      │
└─────────────────────────────────────────┘

AFTER (Clean):
┌─────────────────────────────────────┐
│    physics_engine.c                 │
│    (single, unified)                │
│                                     │
│  All dynamics here                  │
│  All API here                       │
│  Clear entry point                  │
│                                     │
├─────────────────────────────────────┤
│  physics_simulation.c               │
│  (simulation loop)                  │
└─────────────────────────────────────┘
```

---

## Our Solution Architecture

### Layers (Clean Separation)
```
┌─────────────────────────────────────────────────────────────┐
│                    SWIFT UI LAYER                           │
│  (Asset Palette, Viewport, Quick Properties)                │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│           PLATFORM BRIDGE LAYER                             │
│  (swift_bridge.h + iOS/macOS adapters)                      │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ Calls
                     ▼
┌────────────────────────────────────────────────────────────┐
│            ASSET PLACEMENT COORDINATOR                      │
│              asset_placement.c                              │
│                                                              │
│  bool asset_placement_execute(GameAPI*, Request*) {        │
│    1. physics_raycast()                                    │
│    2. physics_validate()                                   │
│    3. physics_settle()                                     │
│    4. ecs_entity_create()                                  │
│    5. render()                                             │
│  }                                                          │
└────────────────────┬────────────────────────────────────────┘
                     │ Uses APIs
        ┌────────────┼────────────┬────────────┐
        ▼            ▼            ▼            ▼
    ┌────────┐  ┌────────┐  ┌──────────┐ ┌──────────┐
    │  ECS   │  │Physics │  │Renderer  │ │ Assets   │
    │(create)│  │ (query)│  │(preview) │ │(instance)│
    └────────┘  └────────┘  └──────────┘ └──────────┘
        ▲            ▲            ▲          ▲
        └────────────┴────────────┴──────────┘
           Existing Engine Systems
              (Unchanged)
```

---

## Data Flow: User Places Asset

```
USER ACTION:
┌────────────────────────────────────────┐
│  Drag asset from palette to viewport   │
│  Release (drop) at position            │
└────────────────────┬───────────────────┘
                     │
                     ▼
    ┌────────────────────────────────┐
    │  iOS/macOS Platform Event      │
    │  UIDragInteraction / NSDrops   │
    └────────────┬───────────────────┘
                 │
                 ▼
    ┌──────────────────────────────┐
    │  drag_drop_ios.m / .m        │
    │  (Platform Adapter)          │
    │  Extracts: asset, position   │
    └────────────┬─────────────────┘
                 │
                 ▼
    ┌──────────────────────────────┐
    │  Swift Bridge Call           │
    │  asset_placement_create()    │
    │  Pass: asset, pos, rot       │
    └────────────┬─────────────────┘
                 │
                 ▼
    ┌──────────────────────────────────────┐
    │  C Function: asset_placement_execute │
    │                                      │
    │  1. Raycast: physics_raycast()       │
    │     → Find surface                   │
    │     → Get normal                     │
    │                                      │
    │  2. Validate: physics_validate()     │
    │     → Check collision-free          │
    │     → Check bounds                  │
    │                                      │
    │  3. Settle: physics_simulate()       │
    │     → Gravity simulation             │
    │     → Find rest position             │
    │                                      │
    │  4. Create: ecs_entity_create()      │
    │     → New entity                     │
    │     → Add AssetInstanceComponent     │
    │     → Add Transform component        │
    │     → Add Mesh/Renderer component    │
    │                                      │
    │  5. Return: entity handle + position │
    └────────────┬────────────────────────┘
                 │
                 ▼
    ┌──────────────────────────────┐
    │  Back to Swift               │
    │  Update Scene Hierarchy      │
    │  Viewport updates (renders)  │
    └────────────┬─────────────────┘
                 │
                 ▼
    ┌──────────────────────────────┐
    │  RESULT: Asset visible in    │
    │  viewport at placement pos   │
    │  Can select/modify/delete    │
    └──────────────────────────────┘
```

---

## Phase Progression

```
PHASE 0: Deduplication (Week 1)
┌──────────────────────┐
│ DELETE 200 duplicate │
│ files in CVoxelForge │
│ Update CMake         │
│ Build still works    │
└──────────┬───────────┘
           │ Clean foundation
           ▼
PHASE 1: Consolidation (Weeks 2-3)
┌──────────────────────────┐
│ Merge 5 physics files→2  │
│ Cleanup scattered code   │
│ Add test framework       │
└──────────┬───────────────┘
           │ Clear architecture
           ▼
PHASE 2: Asset Instances (Week 4)
┌──────────────────────────┐
│ Extend asset_manager.c   │
│ Instance creation API    │
│ Instance tracking        │
└──────────┬───────────────┘
           │ Asset system ready
           ▼
PHASE 3: ECS Component (Week 5)
┌──────────────────────────┐
│ Create AssetInstance     │
│ Component type           │
│ Registration + serialize │
└──────────┬───────────────┘
           │ Instances in world
           ▼
PHASE 4: Physics Queries (Week 6)
┌──────────────────────────┐
│ Expose raycast API       │
│ Add validation functions │
│ Add settling simulation  │
└──────────┬───────────────┘
           │ Placement validation ready
           ▼
PHASE 5: Coordinator (Week 7) ⭐
┌──────────────────────────┐
│ Create asset_placement.c │
│ Single entry point       │
│ Orchestrate all systems  │
└──────────┬───────────────┘
           │ Heart of system
           ▼
PHASE 6: Swift Bridge (Week 8)
┌──────────────────────────┐
│ Extend swift_bridge.h    │
│ Add platform adapters    │
│ iOS + macOS handlers     │
└──────────┬───────────────┘
           │ UI can call placement
           ▼
PHASE 7: UI (Weeks 9-10)
┌──────────────────────────┐
│ Asset palette UI         │
│ Viewport drop target     │
│ Property panel           │
└──────────┬───────────────┘
           │ User-facing features
           ▼
PHASE 8: Testing (Week 11)
┌──────────────────────────┐
│ Integration tests        │
│ Performance validation   │
│ Production ready         │
└──────────────────────────┘
```

---

## System Integration Points

### Before (Scattered)
```
                    Asset System
                          │
    ┌──────────┬──────────┼──────────┬──────────┐
    │          │          │          │          │
    ▼          ▼          ▼          ▼          ▼
  Place1      Place2     Place3     Place4     Place5
  (unclear where
   logic goes)
```

### After (Centralized)
```
┌───────────────────────────────────┐
│   Swift UI                        │
└──────────────┬────────────────────┘
               │
               ▼
        ┌──────────────┐
        │ Swift Bridge │
        └──────┬───────┘
               │
               ▼
    ┌────────────────────────────┐
    │  asset_placement_execute() │  ← Single integration point
    │  (ONLY place placement     │     Everything goes through here
    │   logic exists)            │
    └────────┬───────────────────┘
             │
    ┌────────┼────────┬──────────┬──────────┐
    ▼        ▼        ▼          ▼          ▼
   ECS    Physics  Renderer   Assets   Animation
   │        │        │         │        │
   └────┬───┴────┬───┴────┬────┴───┬────┴────┐
        │        │        │        │         │
        └────────┴────────┴────────┴─────────┘
             Engine Systems (Unchanged)
```

---

## File Organization Improvement

```
BEFORE: Confusing
┌─────────────────────────────────────────┐
│  Where do I add drag-and-drop?          │
│  - New directory? (wrong)               │
│  - New renderer system? (wrong)         │
│  - New physics system? (wrong)          │
│  - Duplicate bridge? (wrong)            │
│  - Modify multiple files? (messy)       │
│                                         │
│  Result: Duplicated code, hard to test │
└─────────────────────────────────────────┘

AFTER: Clear
┌─────────────────────────────────────────┐
│  asset_placement.c                      │
│  ├─ Orchestrates placement              │
│  ├─ Single entry point                  │
│  ├─ Easy to understand                  │
│  ├─ Easy to test                        │
│  └─ Easy to extend                      │
│                                         │
│  Result: Clean, testable, maintainable  │
└─────────────────────────────────────────┘
```

---

## Dependencies Tree

```
TIGHT COUPLING (❌ What We Avoid):
                Swift UI
                   │
     ┌─────────────┼─────────────┐
     │             │             │
   ECS         Physics       Renderer
     │             │             │
     └─────────────┼─────────────┘
           Asset Manager

Everything depends on everything


CLEAN COUPLING (✅ What We Do):
                Swift UI
                   │
            Swift Bridge
                   │
        asset_placement_execute()
           (Single entry point)
           ┌─────┬─────┬─────┬─────┐
           ▼     ▼     ▼     ▼     ▼
          ECS Physics Render Asset Animation

          (Only asset_placement knows
           about placement logic)
```

---

## Testing Coverage

```
                  Integration Test
                  (Full workflow)
                        │
        ┌───────────────┼───────────────┐
        │               │               │
    ┌───▼────┐     ┌────▼─────┐   ┌────▼─────┐
    │ asset_ │     │    UI    │   │ Platform │
    │placement│     │   Tests  │   │  Tests   │
    └────┬────┘    └──────────┘   └──────────┘
        │
    ┌───┴───────┬────────┬─────────┬──────────┐
    │           │        │         │          │
 ┌──▼──┐   ┌───▼──┐  ┌──▼─┐   ┌──▼──┐   ┌───▼───┐
 │ ECS │   │Physics│  │ Rend│   │Asset│   │Anim   │
 │Test │   │ Test │  │Test │   │Test │   │Test   │
 └─────┘   └──────┘  └─────┘   └─────┘   └───────┘

Each system independently testable
Plus integration tests
100% coverage of new functionality
```

---

## Performance Profile

```
OPERATION: Place Asset via Drag-Drop

Timeline (targets):
0ms    ├─ User initiates drag
       │  (handler ready)
       │
10ms   ├─ Preview updated
       │  (ghost asset shown)
       │  Target: <16ms per frame
       │
20ms   ├─ Raycast detection
       │  Target: <1ms
       │
25ms   ├─ Validation
       │  Target: <2ms
       │
30ms   ├─ Physics settling (if needed)
       │  Target: <5ms
       │
40ms   ├─ Entity created
       │  Target: <10ms
       │
50ms   ├─ Rendered in viewport
       │  (visible to user)
       │
60ms   └─ Next frame ready
        (60 FPS smooth)

Overall: User drag-to-visual should be <50ms
```

---

## Success Visualization

```
BEFORE:
┌─────────────────────────────────┐
│ Confused Architecture           │
│ 200 duplicate files             │
│ Scattered implementations       │
│ Hard to test                    │
│ Hard to maintain                │
│ No clear integration point      │
│ Feature scattered across code   │
└─────────────────────────────────┘


AFTER:
┌─────────────────────────────────┐
│ Clean Architecture              │
│ Single source of truth          │
│ Consolidated implementations    │
│ Easy to test                    │
│ Easy to maintain                │
│ Clear integration point         │
│ Feature cohesive and centered   │
└─────────────────────────────────┘

+ Can add new features easily
+ Can understand code quickly
+ Can modify without breaking
+ Can test independently
+ Can extend systematically
```

---

This visual representation shows:
1. The problems we're solving
2. How our solution works
3. The progression through phases
4. The clean architecture we're building
5. The data flow through the system
6. The testing strategy
7. The performance targets
8. The before/after improvement

All 8 planning documents are complete. Ready to implement? 🚀
