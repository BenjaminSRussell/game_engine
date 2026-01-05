# Architecture-Driven Integration Plan
## Drag-and-Drop 3D Asset Editor

### PRINCIPLE: Build within existing architecture, not around it

This plan respects the existing engine organization and eliminates duplication rather than creating new isolated systems.

---

## PART 1: CURRENT STATE ANALYSIS

### Architecture Strengths We're Building On
- ✅ **API Bridge Pattern**: 43 working bridge interfaces in `src/engine/platform/`
- ✅ **Modular Subsystems**: Clear separation of renderer, physics, animation, audio
- ✅ **Forward Declarations**: Heavy use of opaque pointers = easy to extend
- ✅ **Callback System**: Cross-system communication without tight coupling
- ✅ **Public/Private Separation**: Good distinction between `include/` and implementation

### Critical Issues to Fix First
- ❌ **DUPLICATION**: 200+ files duplicated between `/src/engine/include/` and `/src/frontend/engine/studio/CVoxelForge/include/engine/include/`
- ❌ **SCATTERED PHYSICS**: Integration points in 5 separate files (physics_core_impl.c, physics_complete.c, physics_advanced_impl.c, physics_api_impl.c, simulation_loop.c)
- ❌ **OVERSIZED AI**: 47 directories with no clear coordination layer
- ❌ **WEAK TEST FRAMEWORK**: Manual test programs, no standard framework
- ❌ **NO DEPENDENCY INJECTION**: Tight coupling between systems

---

## PART 2: DEDUPLICATION STRATEGY

### Phase 0: Eliminate Frontend Duplication (FIRST STEP)

**The Problem:**
- `/src/frontend/engine/studio/CVoxelForge/include/engine/include/` is an exact copy of the entire `/src/engine/include/` directory
- This causes:
  - Maintenance nightmare (changes in one place don't reflect in the other)
  - Potential inconsistencies
  - Wasted storage and sync issues

**Solution: Single Source of Truth**
```
DELETE:  /src/frontend/engine/studio/CVoxelForge/include/engine/include/
KEEP:    /src/engine/include/  (single, definitive source)
UPDATE:  /src/frontend/engine/studio/CVoxelForge/ build config
         to reference ../../../../../../../src/engine/include/ (or use proper build system symlinks)
```

**Implementation:**
```bash
# Create proper CMake include path
# In studio's CMakeLists.txt:
# include_directories(${PROJECT_ROOT}/src/engine/include)
```

**Files Affected:**
- `src/frontend/engine/studio/CVoxelForge/include/` (DELETE entire mirror)
- Update build configuration to point to main `/src/engine/include/`
- Verify all 43 API bridges are accessible

---

## PART 3: LOGICAL ORGANIZATION FOR DRAG-AND-DROP

### NOT Creating New Folders
### USING Existing Subsystems Intelligently

#### Drag-and-Drop Integration Points (By System)

---

## 1. ASSET MANAGEMENT INTEGRATION
**Existing System**: `src/engine/resource/` + `src/engine/core/asset_manager.c`
**Public API**: `include/assets/assets.h`

**What's Already There:**
- Asset manager orchestration
- Asset loading/unloading
- Asset hot-reload capability
- Asset serialization

**What Needs to Extend (NO NEW FILES):**
```c
// ADD TO: src/engine/resource/asset_manager.c
// NOT creating new file, extending existing

// New function in existing asset_manager.c:
typedef struct {
  Handle asset;
  Vec3 position;
  Quat rotation;
  Vec3 scale;
  struct EntityComponent *override_properties;
} AssetInstanceData;

// Instance creation (drag-and-drop drop)
Handle asset_create_instance_at_position(
  Handle asset,
  Vec3 position,
  Quat rotation
);

// Batch operations
void asset_replace_all_instances(Handle old_asset, Handle new_asset);
void asset_modify_all_instances(Handle asset, AssetModification *mods);
```

**Integration Point:**
- ECS creates entity with AssetInstance component
- AssetInstance holds reference to original asset
- On asset update, instances can sync back

---

## 2. RENDERER DRAG-AND-DROP EXTENSIONS
**Existing System**: `src/engine/renderer/` (111 subdirectories)
**Public API**: `include/renderer/renderer_api.h`

**NOT Creating:**
- ❌ New "drag_drop_renderer.c"
- ❌ New "asset_preview.c" folder

**EXTENDING Existing:**
```c
// ADD TO: src/engine/renderer/core/render_graph.c
// (already orchestrates rendering - add preview/selection rendering)

typedef struct {
  u32 outline_color;
  f32 outline_width;
  f32 glow_intensity;
  bool is_selected;
} SelectionRenderState;

// Already has render passes, just add:
void render_graph_add_selection_pass(RenderGraph *graph, Entity entity);
void render_graph_add_preview_pass(RenderGraph *graph, Handle asset, Mat4 transform);

// ADD TO: src/engine/renderer/gpu/gpu_texture.c
// (already manages GPU textures - use for preview generation)
Handle gpu_create_preview_framebuffer(u32 width, u32 height);
void gpu_render_asset_preview(Handle asset, Handle framebuffer);
```

**Integration Points:**
- Use existing `render_graph.c` system (already has multiple passes)
- Extend GPU texture system for preview framebuffers
- Use existing material system for preview lighting

---

## 3. PHYSICS FOR PLACEMENT FEEDBACK
**Existing System**: `src/engine/physics/` (36 directories)
**Public APIs**: Multiple in `include/physics/`

**Current State:**
- 5 implementation files scattered: physics_core_impl.c, physics_complete.c, physics_advanced_impl.c, etc.
- **ACTION REQUIRED**: Consolidate these into physics_engine.c + physics_simulation.c

**NOT Creating:**
- ❌ "placement_physics.c"
- ❌ "physics_preview.c"

**CONSOLIDATING First:**
```
Step 1: Merge into physics_engine.c:
  - physics_core_impl.c (45KB) ← core dynamics
  - physics_complete.c (9KB) ← complete sim loop
  - physics_advanced_impl.c
  - physics_api_impl.c (7KB)

Step 2: Single initialization:
  physics_engine_init() → Sets up solvers, collision, dynamics

Step 3: Single simulation loop:
  physics_engine_step() → Physics simulation
  physics_engine_raycast() → Drag-and-drop queries
```

**Then Add (to consolidated physics_engine.c):**
```c
// Preview physics for placement
Physics preview_physics_create_static_body(Mesh mesh, Vec3 pos);
Vec3 preview_physics_gravity_settle(Physics body, Vec3 start_pos);
bool preview_physics_can_place(Physics body, Collider target);
```

**Integration Points:**
- Use existing raycast for drop target detection
- Use existing dynamics for settling simulation
- No new physics subsystems needed

---

## 4. ANIMATION PLAYBACK INTEGRATION
**Existing System**: `src/engine/animation/` (30 directories)
**Public API**: `include/animation/animation_system.h`

**Current State:**
- Modular: skeleton, blending, synthesis, IK all separated
- Already has timeline system

**NOT Creating:**
- ❌ "asset_animation.c"
- ❌ "animation_preview.c"

**EXTEND Existing:**
```c
// ADD TO: src/engine/animation/core/animation_system.c
// (already manages animation state)

// Asset animation binding
void animation_system_bind_asset_animation(
  Handle asset,
  Handle animation_clip
);

Handle animation_system_get_asset_animation(Handle asset);

// Timeline already exists, just expose:
// include/animation/animation_timeline.h already has:
// - timeline_create()
// - timeline_add_keyframe()
// - timeline_get_preview()
```

**Integration Points:**
- Animation system already has preview capability
- Timeline already exists for keyframing
- Just need to expose in asset context

---

## 5. ECS FOR DRAG-AND-DROP PLACEMENT
**Existing System**: `src/engine/ecs/`
**Public API**: `include/ecs/entity_component_system.h`

**Current State:**
- Component-based architecture
- Already has prefab system

**NOT Creating:**
- ❌ "asset_instance_component.c"
- ❌ "drag_drop_entity.c"

**EXTEND Existing:**
```c
// ADD TO: src/engine/ecs/entity.c
// (already manages entity creation)

typedef struct {
  Handle asset_reference;        // Link to source asset
  ComponentOverride *overrides;  // Instance-specific changes
  bool is_instance;             // Flag: true if instance
} AssetInstanceComponent;

// Already has component registration:
ecs_register_component_type(
  &asset_instance_component_type,
  sizeof(AssetInstanceComponent),
  asset_instance_component_serialize,
  asset_instance_component_deserialize
);

// Existing entity creation becomes:
Entity entity = ecs_entity_create();
ecs_entity_add_component(entity, COMPONENT_TYPE_ASSET_INSTANCE, &instance_data);
// ^ This automatically sets up all inherited components from asset
```

**Integration Points:**
- Use existing ECS system, no new architecture needed
- Asset instance is just another component
- Inherits from asset's archetype

---

## 6. EDITOR TOOLS (GIZMOS, SELECTION)
**Existing System**: `src/engine/editor/` (28 directories)
**Public API**: `include/editor/`

**Current State:**
- Already has: gizmo_api_bridge.h, selection_api_bridge.h
- Already has: material_editor, terrain_editor, etc.

**NOT Creating:**
- ❌ "drag_drop_editor.c"
- ❌ "placement_gizmo.c"

**EXTEND Existing:**
```c
// ADD TO: src/engine/editor/viewport/viewport_tools.c
// (already manages viewport interactions)

// Existing:
// - gizmo_api_bridge.h (transform_gizmo)
// - selection_api_bridge.h (selection)
// - outline_rendering_api_bridge.h (selection highlight)

// Just add:
void viewport_enable_asset_placement_mode();
void viewport_set_drop_preview_asset(Handle asset);
```

**Integration Points:**
- Transform gizmo already exists
- Selection highlighting already exists
- Just activate them for asset placement

---

## PART 4: SWIFT FRONTEND UNIFICATION (Single Point of Integration)

### Single Swift-C Bridge
**Location**: `src/engine/platform/swift_bridge.h` + implementation

**NOT Creating Multiple Bridges:**
- ❌ No iOS-specific bridge
- ❌ No macOS-specific bridge
- ✅ Single bridge with platform adapters

**Bridge Structure:**
```c
// src/engine/platform/swift_bridge.h

typedef struct {
  // Asset operations
  Handle (*asset_create_instance)(Handle asset, Vec3 pos, Quat rot);
  void (*asset_get_preview)(Handle asset, ImageData *out_preview);

  // Viewport operations
  void (*viewport_set_placement_mode)(bool enabled);
  void (*viewport_set_placement_preview)(Handle asset);

  // Physics queries
  bool (*physics_raycast_for_placement)(Ray ray, PlacementResult *out);

  // Animation
  void (*animation_play_preview)(Handle asset, Handle animation);

  // Editor
  void (*editor_select_entity)(Handle entity);
  void (*editor_get_selected_entity)(Handle *out_entity);

} SwiftEngineInterface;

// Platform-specific adapter:
// iOS: src/engine/platform/ios/swift_bridge_impl_ios.m
// macOS: src/engine/platform/macos/swift_bridge_impl_macos.m
```

**NO Duplication:**
- Single C implementation (bridge uses existing APIs)
- Platform adapters only for UI event handling
- All engine logic centralized

---

## PART 5: ARCHITECTURAL CHANGES NEEDED

### 1. Physics Integration Consolidation (REQUIRED)
**Current**: 5 separate implementation files
**Target**: 2 files

```
src/engine/physics/physics_engine.c
  ├── physics_engine_init()
  ├── physics_engine_step()
  ├── physics_engine_shutdown()
  └── All core dynamics

src/engine/physics/physics_simulation.c (or simulation_loop.c)
  ├── physics_simulation_main_loop()
  └── Integration with ECS
```

**Actions:**
1. Move core dynamics from physics_core_impl.c → physics_engine.c
2. Move simulation loop from simulation_loop.c → physics_simulation.c
3. Move API impl from physics_api_impl.c → physics.h functions
4. Delete: physics_complete.c, physics_advanced_impl.c (merge content)
5. Verify all headers in `include/physics/` still work

---

### 2. Test Framework Standardization (REQUIRED)
**Current**: Manual test executables
**Target**: Criterion or Unity framework

```
src/engine/test/
  ├── test_framework.h (Criterion or Unity base)
  ├── collision_tests.c
  ├── physics_tests.c
  ├── animation_tests.c
  ├── ecs_tests.c
  └── ...

CMakeLists.txt
  ├── target_link_libraries(engine_tests criterion)
```

---

### 3. AI System Reorganization (OPTIONAL but recommended)
**Current**: 47 directories (too scattered)
**Target**: 4-5 coordinated subsystems

```
src/engine/ai/
  ├── behavior/              (Behavior trees, GOAP, utility AI)
  ├── navigation/            (Pathfinding, navmesh, crowd)
  ├── perception/            (Vision, senses, memory)
  ├── learning/              (ML, training)
  └── ai_coordinator.c       (NEW: Central AI orchestration)
```

**Note**: This is organizational, doesn't change functionality

---

## PART 6: CENTRALIZED INTEGRATION FILES (NEW, BUT FEW)

### We WILL Create These (Carefully):

#### 1. `src/engine/editor/asset_placement.h`
**Purpose**: Public API for asset drag-and-drop
**NOT duplicating**: Uses existing APIs

```c
// src/engine/editor/asset_placement.h
#ifndef ENGINE_EDITOR_ASSET_PLACEMENT_H
#define ENGINE_EDITOR_ASSET_PLACEMENT_H

#include "core/game_api.h"
#include "ecs/entity_component_system.h"
#include "renderer/renderer_api.h"
#include "physics/physics.h"

// This is a FACADE that coordinates existing systems
typedef struct {
  // Input
  Ray drop_ray;
  Handle asset_to_place;

  // Output
  Vec3 final_position;
  bool placement_valid;
  Entity created_entity;
} AssetPlacementRequest;

// Single entry point for drag-and-drop
bool asset_placement_execute(GameAPI *api, AssetPlacementRequest *req);

// Preview rendering
void asset_placement_preview_render(GameAPI *api, Handle asset, Vec3 pos);

// Validation
bool asset_placement_validate(GameAPI *api, Vec3 pos, Handle asset);

#endif
```

**Implementation**: `src/engine/editor/asset_placement.c`
- Coordinates: ECS (entity creation) + Physics (raycast, collision check) + Renderer (preview)
- No new logic, just orchestration of existing systems

#### 2. `src/engine/editor/simple_environment_builder.h`
**Purpose**: Simple mode API (wrapper around existing tools)

```c
// Coordinates: asset_palette + placement + quick property adjustment
// Uses: asset_placement.h + material_system + physics
// No new rendering, no new physics
```

#### 3. `src/engine/platform/drag_drop_bridge.h`
**Purpose**: iOS/macOS specific touch/gesture handling

```c
// Translates platform events to engine placement requests
// Calls asset_placement_execute()
// No duplicate engine logic
```

---

## PART 7: DETAILED IMPLEMENTATION PATH

### Phase 0: Deduplication (BEFORE ANYTHING ELSE)
```
Week 1:
  [ ] Delete /src/frontend/engine/studio/CVoxelForge/include/engine/include/
  [ ] Update CMake to reference /src/engine/include/
  [ ] Verify 43 API bridges are accessible
  [ ] Test studio still builds and links correctly

  This alone eliminates 200+ duplicate files
```

### Phase 1: Consolidation (Architecture Cleanup)
```
Week 2-3:
  [ ] Merge physics implementation files (5 → 2)
  [ ] Update all physics includes
  [ ] Verify physics tests still pass

  [ ] Reorganize AI into 4-5 subsystems (structure only, no code change)
  [ ] Adopt test framework (Criterion)

  Result: Cleaner, more maintainable codebase
```

### Phase 2: Asset Pipeline Extension (Using Existing Systems)
```
Week 4-5:
  [ ] Extend asset_manager.c with instance creation
  [ ] Create AssetInstanceComponent in ECS
  [ ] Update asset serialization for instances

  [ ] Test: Create instance, verify persistence
```

### Phase 3: Rendering Extensions (No New System)
```
Week 6:
  [ ] Add preview rendering to render_graph.c
  [ ] Add selection highlighting (already has outline API)
  [ ] Add placement preview ghost rendering

  [ ] Test: Render asset with highlight, render preview
```

### Phase 4: Physics Integration (Consolidated Physics)
```
Week 7:
  [ ] Expose raycast API for drop detection
  [ ] Add preview physics functions to physics_engine.c
  [ ] Test: Raycast detection, settling simulation
```

### Phase 5: Central Coordination (asset_placement.h)
```
Week 8:
  [ ] Implement asset_placement.c as facade
  [ ] Coordinates: ECS + Physics + Renderer + Asset Manager
  [ ] Single entry point: asset_placement_execute()

  [ ] Test: Full drag-and-drop workflow in C
```

### Phase 6: Swift Bridge (Minimal Adapter)
```
Week 9:
  [ ] Create drag_drop_bridge.h for platform events
  [ ] iOS: Translate UIDragInteraction → placement request
  [ ] macOS: Translate NSDraggingDestination → placement request

  [ ] Update swift_bridge.h to include placement API
```

### Phase 7: Simple UI (Wrapper Around Existing Tools)
```
Week 10-11:
  [ ] Create asset palette UI (uses existing asset browser)
  [ ] Create quick property panel (uses existing property editor)
  [ ] Create placement grid visualization

  [ ] Test: Drag-drop from palette to viewport
```

### Phase 8: Advanced Mode (Already Exists)
```
No new work needed - all advanced features already exist
Just expose them in "advanced mode" toggle
```

---

## PART 8: FILE ORGANIZATION SUMMARY

### What We're NOT Creating
- ❌ New rendering subsystem
- ❌ New physics subsystem
- ❌ New animation subsystem
- ❌ New ECS subsystem
- ❌ Multiple bridge files per platform
- ❌ Duplicate include directories

### What We ARE Creating (Minimal)
```
src/engine/editor/asset_placement.h        (Facade/Coordinator)
src/engine/editor/asset_placement.c
src/engine/editor/simple_environment_builder.h
src/engine/editor/simple_environment_builder.c

src/engine/platform/drag_drop_bridge.h     (Platform Adapter)
src/engine/platform/ios/drag_drop_ios.m
src/engine/platform/macos/drag_drop_macos.m

Documentation only:
docs/drag_drop_architecture.md
docs/asset_placement_workflow.md
```

### What We're CONSOLIDATING
```
Physics:
  physics_core_impl.c ──┐
  physics_complete.c ──┼→ physics_engine.c (consolidated)
  physics_advanced_impl.c ┘

  simulation_loop.c → physics_simulation.c (or simulation_loop.c stays)

AI (Organizational):
  ai/ 47 dirs → ai/{behavior, navigation, perception, learning} + ai_coordinator.c

Test Framework:
  Manual tests → Criterion framework
```

### What We're EXTENDING (Existing Files)
```
asset_manager.c              + asset instance creation
render_graph.c               + preview/selection passes
physics_engine.c             + placement queries (after consolidation)
animation_system.c           + asset binding API
entity.c                     + AssetInstanceComponent
viewport_tools.c             + placement mode
swift_bridge.h               + drag-drop functions
```

---

## PART 9: VERIFICATION & TESTING STRATEGY

### Per-System Testing (Already Possible)

```
1. Asset System Tests
   - asset_create_instance()
   - asset_get_preview()
   - asset_serialize_instance()

2. Physics Tests (Existing, Add)
   - physics_raycast_for_placement()
   - physics_preview_settle()

3. Renderer Tests
   - render_graph_add_selection_pass()
   - render_graph_add_preview_pass()

4. ECS Tests
   - AssetInstanceComponent creation/serialization

5. Integration Test
   - asset_placement_execute() → Full workflow

6. Platform Tests
   - iOS drag event → placement request
   - macOS drag event → placement request
```

### Build System Verification
```
cmake . -DBUILD_TESTS=ON
make test

Results:
  ✓ All existing tests still pass
  ✓ New tests pass
  ✓ No duplication in includes
  ✓ No link errors
```

---

## PART 10: SUMMARY - THE RIGHT WAY

### ❌ Wrong Approach (What We're NOT Doing)
```
Create new folders:
  src/engine/drag_drop/
  src/engine/placement/
  src/engine/asset_editor/

Create duplicate bridges:
  src/engine/platform/drag_drop_bridge_ios.h
  src/engine/platform/drag_drop_bridge_macos.h

Create isolated preview systems:
  src/engine/renderer/asset_preview.c
  src/engine/physics/physics_preview.c

Result: More duplication, more maintenance burden
```

### ✅ Right Approach (What We ARE Doing)
```
1. ELIMINATE duplication (CVoxelForge mirror)
2. CONSOLIDATE scattered code (physics impl files)
3. EXTEND existing systems with new capabilities
4. CREATE facade/coordinator (asset_placement.c)
5. ADD platform adapters (not duplicate bridges)
6. USE existing infrastructure for all heavy lifting

Result:
  - Single source of truth for each system
  - No duplication
  - Easier testing
  - Easier maintenance
  - All functionality preserved
  - New features clearly layered on top
```

---

## IMPLEMENTATION CHECKLIST

### Phase 0: Cleanup
- [ ] Identify exact duplication in CVoxelForge
- [ ] Create deduplication plan
- [ ] Execute deduplication
- [ ] Verify builds and tests pass
- [ ] Commit: "chore: eliminate frontend include duplication"

### Phase 1: Consolidation
- [ ] Merge physics implementation files
- [ ] Update physics tests
- [ ] Reorganize AI structure
- [ ] Adopt standard test framework
- [ ] Commit: "refactor: consolidate physics and standardize testing"

### Phase 2-8: Feature Addition
- [ ] Follow PART 7 implementation path
- [ ] Test each system independently
- [ ] Test integration points
- [ ] No new systems created
- [ ] No new duplication introduced

---

## CRITICAL SUCCESS FACTORS

1. **Every new function uses existing subsystems**
   - If you write new collision code: WRONG (physics exists)
   - If you call physics_raycast(): RIGHT

2. **Every new file is a coordinator or adapter**
   - asset_placement.c: Coordinates existing systems (GOOD)
   - new_physics_implementation.c: Duplicate physics (BAD)

3. **Public headers in `include/`**
   - All new APIs exposed through `include/editor/` or `include/platform/`
   - No "private" new APIs

4. **Tests for new functionality**
   - Every new coordinator/adapter has tests
   - Tests use existing test framework
   - Tests verify integration, not duplication

5. **No circular dependencies**
   - asset_placement.c uses: ECS, Physics, Renderer, Assets
   - Nothing uses asset_placement.c except Swift bridge
   - Physics doesn't know about asset_placement

---

## NEXT STEP

1. Confirm this architecture aligns with your vision
2. Start Phase 0: Deduplication
3. I can help execute each phase systematically

Questions to verify:
- Do you agree with eliminating the CVoxelForge duplication?
- Should we consolidate physics first?
- Does the facade approach (asset_placement.c) make sense?
