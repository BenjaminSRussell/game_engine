# Focused Implementation TODOs
## Architecture-Respecting Drag-and-Drop Integration

---

## PHASE 0: DEDUPLICATION & CLEANUP (Start Here!)

### 0.1 Eliminate Frontend Include Duplication
**Location**: `/src/frontend/engine/studio/CVoxelForge/include/engine/include/`
**Status**: 🔴 CRITICAL - 200+ duplicate files

#### Todo List:
- [ ] TODO: Document exact file structure in CVoxelForge includes
  - **File**: `CVoxelForge/include/` directory listing
  - **Expected**: List all mirrored directories
  - **Reason**: Verify complete duplication scope

- [ ] TODO: Create CMake deduplication plan
  - **File**: `src/frontend/engine/studio/CVoxelForge/CMakeLists.txt`
  - **Action**: Replace local includes with `../../../../../../../src/engine/include`
  - **Verify**: Can reference exact same headers

- [ ] TODO: Delete duplicate include directories
  - **File**: `CVoxelForge/include/engine/include/` entire directory
  - **Action**: Remove after CMake verified
  - **Commit**: "chore: eliminate frontend include duplication - single source of truth"

- [ ] TODO: Verify studio build works with shared includes
  - **Command**: `cd build_xcode && cmake .. && make`
  - **Expected**: Studio builds without errors
  - **Commit**: "test: verify studio builds with deduplicated includes"

- [ ] TODO: Verify studio can access all 43 API bridges
  - **Check**: All `platform/*_api_bridge.h` files accessible
  - **Test**: Swift can call bridge functions
  - **Commit**: "test: verify all API bridges accessible from studio"

**Result**: -200 files, single include directory, no duplication

---

## PHASE 1: PHYSICS ENGINE CONSOLIDATION

### 1.1 Consolidate Scattered Physics Implementations
**Current State**: 5 implementation files (45KB + 9KB + 7KB + scattered)
**Target**: 2 files (physics_engine.c, physics_simulation.c)

#### Todo List:

- [ ] TODO: Audit physics implementation files
  - **Files**:
    - `src/engine/physics/physics_core_impl.c` (45KB)
    - `src/engine/physics/physics_complete.c` (9KB)
    - `src/engine/physics/physics_advanced_impl.c` (unknown)
    - `src/engine/physics/physics_api_impl.c` (7KB)
    - `src/engine/physics/simulation_loop.c` (19KB)
  - **Action**: Read each and document what each implements
  - **Document in**: `docs/physics_consolidation_plan.md`

- [ ] TODO: Identify redundancy in physics implementations
  - **Question**: What's in physics_complete.c vs physics_core_impl.c?
  - **Question**: What's in physics_advanced_impl.c vs physics_api_impl.c?
  - **Question**: What goes where?
  - **Document**: Consolidation mapping

- [ ] TODO: Create unified physics_engine.c structure
  - **File**: `src/engine/physics/physics_engine.c`
  - **Contains**: All core dynamics from the 5 files
  - **Functions**:
    ```c
    bool physics_engine_init(EngineCore *engine);
    void physics_engine_shutdown(EngineCore *engine);
    void physics_engine_step(f32 delta_time);
    void physics_engine_raycast(Ray ray, RaycastResult *out);
    void physics_engine_create_rigid_body(...);
    ```
  - **Verify**: Headers in `include/physics/` reference these

- [ ] TODO: Create unified physics_simulation.c
  - **File**: `src/engine/physics/physics_simulation.c`
  - **Contains**: Main simulation loop from simulation_loop.c
  - **Functions**:
    ```c
    void physics_simulation_main_loop(f32 delta_time, World *world);
    void physics_simulation_update_entities(World *world);
    void physics_simulation_sync_ecs(ECS *ecs);
    ```

- [ ] TODO: Consolidate physics_api_impl.c functions
  - **Action**: Move all API implementations to physics_engine.c
  - **Update**: `include/physics/physics.h` to point to consolidated functions
  - **Verify**: All physics/* bridge files still work

- [ ] TODO: Delete redundant physics files
  - **Delete**:
    - `physics_complete.c` (content merged)
    - `physics_advanced_impl.c` (content merged)
    - Keep: `physics_engine.c`, `physics_simulation.c`, `simulation_loop.c` (or rename)
  - **Verify**: No other files reference deleted files

- [ ] TODO: Update CMakeLists.txt physics sources
  - **File**: Root `CMakeLists.txt`
  - **Update**: PHYSICS_SOURCES list to only include:
    - physics_engine.c
    - physics_simulation.c
    - solver/ subdirectory
    - collision/ subdirectory
    - constraint/ subdirectory
    - (other modular components)
  - **Remove**: Old impl files

- [ ] TODO: Test physics build and functionality
  - **Run**: Physics tests
  - **Run**: Physics demo (if exists)
  - **Verify**: No behavioral changes
  - **Commit**: "refactor: consolidate physics implementations into unified physics_engine.c"

- [ ] TODO: Update physics documentation
  - **File**: `docs/physics_system.md`
  - **Update**: Structure now shows consolidated files
  - **Update**: Integration points documented

**Result**: -3 files, clearer physics architecture, single integration point

---

## PHASE 1B: TEST FRAMEWORK STANDARDIZATION (Optional but Recommended)

### 1B.1 Adopt Standard C Testing Framework

- [ ] TODO: Choose testing framework (Criterion recommended)
  - **Options**:
    - Criterion (modern, macOS-friendly)
    - Unity (lightweight)
    - Catch2 (but C++ focused)
  - **Decision**: Which one?
  - **Document**: `docs/testing_framework_choice.md`

- [ ] TODO: Setup test framework in CMakeLists.txt
  - **File**: Root `CMakeLists.txt`
  - **Add**: Criterion or chosen framework
  - **Create**: `enable_testing()` and test discovery

- [ ] TODO: Migrate existing physics tests
  - **From**: Manual test executables
  - **To**: Criterion test suite
  - **Location**: `src/engine/physics/tests/`
  - **Result**: `make test` runs all physics tests

- [ ] TODO: Create test template for new systems
  - **Template**: `src/engine/test_template.c`
  - **Shows**: How to write test for new functionality

**Result**: Professional test infrastructure, easier to validate changes

---

## PHASE 2: ASSET SYSTEM EXTENSIONS

### 2.1 Extend Asset Manager for Instances
**File**: `src/engine/resource/asset_manager.c`
**Header**: `include/assets/asset_manager.h`

#### Todo List:

- [ ] TODO: Define AssetInstanceData structure
  - **File**: `include/assets/asset_manager.h`
  - **Add**:
    ```c
    typedef struct {
      Handle asset_handle;              // Reference to source asset
      Vec3 position;
      Quat rotation;
      Vec3 scale;
      struct EntityComponent *overrides; // Instance-specific changes
      bool dirty;                        // Needs serialization
    } AssetInstanceData;
    ```
  - **Location**: Add after existing asset structures

- [ ] TODO: Create asset instance functions in asset_manager.c
  - **Add Functions**:
    ```c
    // Create instance of asset at position
    Handle asset_create_instance(Handle asset, Vec3 pos, Quat rot);

    // Get instance data
    AssetInstanceData* asset_get_instance_data(Handle instance);

    // Track all instances of asset
    void asset_get_instances(Handle asset, Handle *out_handles, u32 *out_count);

    // Batch operations
    void asset_replace_all_instances(Handle old_asset, Handle new_asset);
    void asset_modify_all_instances(Handle asset, PropertyOverride *mods);
    ```

- [ ] TODO: Add asset instance serialization
  - **File**: `src/engine/core/serialization/asset_manifest.c`
  - **Add**: Instance serialization to asset manifest
  - **Verify**: Can save and load instances

- [ ] TODO: Test asset instance creation and persistence
  - **Test**: Create instance, save, load, verify
  - **Commit**: "feat: add asset instance management to asset_manager"

**Result**: Asset system supports instances, foundations for drag-and-drop

---

### 2.2 Asset Preview Generation
**File**: `src/engine/renderer/core/render_graph.c`
**Header**: `include/renderer/renderer_api.h`

#### Todo List:

- [ ] TODO: Extend render_graph for preview rendering
  - **File**: `src/engine/renderer/core/render_graph.c`
  - **Add Function**:
    ```c
    // Add preview rendering pass
    void render_graph_add_preview_pass(
      RenderGraph *graph,
      Handle asset,
      Mat4 transform,
      RenderTarget *out_target
    );
    ```

- [ ] TODO: Create preview framebuffer management
  - **Use Existing**: `src/engine/renderer/gpu/gpu_texture.c` (already manages framebuffers)
  - **Add Function**:
    ```c
    Handle gpu_create_preview_framebuffer(u32 width, u32 height);
    void gpu_render_to_framebuffer(Handle asset, Handle framebuffer);
    ImageData* gpu_read_framebuffer_pixels(Handle framebuffer);
    ```

- [ ] TODO: Test asset preview generation
  - **Test**: Generate preview image for asset
  - **Test**: Save preview to file
  - **Verify**: Preview quality acceptable
  - **Commit**: "feat: add asset preview rendering to render_graph"

**Result**: Can generate thumbnails for UI asset browser

---

## PHASE 3: ECS COMPONENT FOR ASSET INSTANCES

### 3.1 Create AssetInstanceComponent
**File**: `src/engine/ecs/entity.c`
**Header**: `include/ecs/entity_component_system.h`

#### Todo List:

- [ ] TODO: Define AssetInstanceComponent structure
  - **File**: `include/ecs/entity_component_system.h`
  - **Add**:
    ```c
    typedef struct {
      ComponentType type = COMPONENT_TYPE_ASSET_INSTANCE;
      Handle asset_reference;
      ComponentOverride *instance_overrides;
    } AssetInstanceComponent;
    ```

- [ ] TODO: Register AssetInstanceComponent type
  - **File**: `src/engine/ecs/entity.c`
  - **In**: `ecs_core_init()` or component registration
  - **Register**:
    ```c
    ecs_register_component_type(
      &asset_instance_component_type,
      sizeof(AssetInstanceComponent),
      asset_instance_serialize,
      asset_instance_deserialize
    );
    ```

- [ ] TODO: Implement component serialization
  - **Functions**:
    ```c
    void asset_instance_serialize(Component *comp, JSONBuilder *json);
    Component* asset_instance_deserialize(JSONValue *json);
    ```

- [ ] TODO: Create component lifecycle
  - **When entity created with AssetInstanceComponent**:
    - Load asset definition
    - Inherit components from asset archetype
    - Apply instance overrides

- [ ] TODO: Test component creation and persistence
  - **Test**: Create entity with AssetInstanceComponent
  - **Test**: Save and load
  - **Commit**: "feat: add AssetInstanceComponent to ECS"

**Result**: Asset instances integrated with ECS, full serialization support

---

## PHASE 4: PHYSICS INTEGRATION (Using Consolidated Physics)

### 4.1 Physics Queries for Placement
**File**: `src/engine/physics/physics_engine.c` (after consolidation)
**Header**: `include/physics/physics.h`

#### Todo List:

- [ ] TODO: Expose raycast API for drop detection
  - **Verify Existing**: Check if raycast already exists in physics_engine.c
  - **Add Function** (if needed):
    ```c
    bool physics_raycast(Ray ray, RaycastHit *out_hit);
    ```

- [ ] TODO: Add collision validation for placement
  - **Function**:
    ```c
    bool physics_validate_placement(
      Collider placement_collider,
      Vec3 position,
      PhysicsLayer target_layer,
      Entity *out_blocking_entity  // What's blocking?
    );
    ```

- [ ] TODO: Add preview physics settling
  - **Function**:
    ```c
    // Simulate physics to find settled position
    Vec3 physics_simulate_gravity_settle(
      Vec3 start_pos,
      Collider collider,
      f32 max_fall_distance
    );
    ```

- [ ] TODO: Test physics placement queries
  - **Test**: Raycast for placement target
  - **Test**: Validate placement at position
  - **Test**: Simulate settling
  - **Commit**: "feat: add physics queries for asset placement"

**Result**: Physics system exposes placement queries

---

## PHASE 5: CENTRAL ASSET PLACEMENT COORDINATOR (NEW)

### 5.1 Create asset_placement.c Facade
**File**: `src/engine/editor/asset_placement.c` (NEW)
**Header**: `src/engine/editor/asset_placement.h` (NEW)
**Public**: `include/editor/asset_placement.h`

#### Todo List:

- [ ] TODO: Create asset_placement.h public API
  - **File**: `include/editor/asset_placement.h`
  - **Define**:
    ```c
    typedef struct {
      Ray drop_ray;
      Handle asset_to_place;
      Vec3 suggested_position;
      Entity *out_created_entity;
      bool *out_placement_valid;
    } AssetPlacementRequest;

    // Main entry point - coordinates all systems
    bool asset_placement_execute(GameAPI *api, AssetPlacementRequest *req);

    // Preview rendering
    void asset_placement_render_preview(GameAPI *api, Handle asset, Vec3 pos);

    // Validation without placement
    bool asset_placement_validate_position(GameAPI *api, Handle asset, Vec3 pos);
    ```

- [ ] TODO: Implement asset_placement.c
  - **File**: `src/engine/editor/asset_placement.c` (NEW)
  - **Implementation**:
    ```c
    bool asset_placement_execute(GameAPI *api, AssetPlacementRequest *req) {
      // 1. Raycast to find placement surface
      RaycastHit hit;
      if (!physics_raycast(req->drop_ray, &hit)) {
        *req->out_placement_valid = false;
        return false;
      }

      // 2. Validate placement at position
      Handle asset = req->asset_to_place;
      Collider collider = asset_get_collider(asset);
      if (!physics_validate_placement(collider, hit.position, LAYER_GAMEPLAY, NULL)) {
        *req->out_placement_valid = false;
        return false;
      }

      // 3. Simulate gravity settling
      Vec3 final_pos = physics_simulate_gravity_settle(hit.position, collider, 10.0f);

      // 4. Create entity with AssetInstanceComponent
      Entity entity = ecs_entity_create();
      AssetInstanceComponent instance = {
        .asset_reference = asset,
        .instance_overrides = NULL
      };
      ecs_entity_add_component(entity, COMPONENT_TYPE_ASSET_INSTANCE, &instance);

      // 5. Set transform
      Transform transform = {
        .position = final_pos,
        .rotation = quat_identity(),
        .scale = vec3_one()
      };
      ecs_entity_add_component(entity, COMPONENT_TYPE_TRANSFORM, &transform);

      // 6. Result
      *req->out_created_entity = entity;
      *req->out_placement_valid = true;
      *req->out_suggested_position = final_pos;
      return true;
    }
    ```

- [ ] TODO: Implement preview rendering
  - **Function**: `asset_placement_render_preview()`
  - **Action**: Calls `render_graph_add_preview_pass()` (from Phase 2.2)
  - **Shows**: Ghost of asset at proposed position

- [ ] TODO: Add validation-only function
  - **Function**: `asset_placement_validate_position()`
  - **Use**: Called during drag hover to show valid/invalid feedback

- [ ] TODO: Test asset placement workflow
  - **Test**: asset_placement_execute() full workflow
  - **Test**: asset_placement_validate_position() feedback
  - **Test**: asset_placement_render_preview() visual
  - **Commit**: "feat: add central asset_placement coordinator"

**Result**: Single, well-tested entry point for drag-and-drop workflow

---

## PHASE 6: SWIFT BRIDGE FOR DRAG-AND-DROP

### 6.1 Extend swift_bridge.h with placement API
**File**: `src/engine/platform/swift_bridge.h`
**Header**: `include/platform/swift_bridge.h`

#### Todo List:

- [ ] TODO: Add placement functions to SwiftEngineInterface
  - **File**: `include/platform/swift_bridge.h`
  - **Add**:
    ```c
    typedef struct {
      // ... existing functions ...

      // Asset placement
      bool (*asset_placement_raycast)(
        Vec3 ray_origin,
        Vec3 ray_direction,
        PlacementResult *out
      );
      Entity (*asset_placement_create)(
        Handle asset,
        Vec3 position,
        Quat rotation
      );
      void (*asset_placement_preview_render)(
        Handle asset,
        Vec3 position
      );

      // Asset preview
      ImageData* (*asset_get_preview_image)(Handle asset, u32 width, u32 height);

      // Selection
      void (*editor_select_entity)(Entity entity);
      Entity (*editor_get_selected_entity)(void);
    } SwiftEngineInterface;
    ```

- [ ] TODO: Implement bridge functions
  - **File**: `src/engine/platform/swift_bridge.c`
  - **Implementation**: Thin wrapper calling asset_placement.c and other APIs
  - **Thread Safety**: Ensure thread-safe for Swift calls

- [ ] TODO: Test bridge from Swift
  - **Call**: Each bridge function from Swift code
  - **Verify**: Correct parameters, return values
  - **Commit**: "feat: add drag-and-drop API to swift_bridge"

**Result**: Swift can call placement functions

---

### 6.2 Platform-Specific Gesture Handling
**File**: `src/engine/platform/drag_drop_bridge.h` (NEW)
**iOS**: `src/engine/platform/ios/drag_drop_ios.m`
**macOS**: `src/engine/platform/macos/drag_drop_macos.m`

#### Todo List:

- [ ] TODO: Create drag_drop_bridge.h abstraction
  - **File**: `include/platform/drag_drop_bridge.h` (NEW)
  - **Define**: Platform-agnostic drag event interface
  - **Functions**:
    ```c
    void drag_drop_bridge_init(SwiftEngineInterface *engine_api);
    void drag_drop_bridge_on_drag_entered(Handle asset, Vec3 position);
    void drag_drop_bridge_on_drag_updated(Vec3 position);
    void drag_drop_bridge_on_drag_exited(void);
    Entity drag_drop_bridge_on_drop(Handle asset, Vec3 position);
    ```

- [ ] TODO: Implement iOS drag-drop handler
  - **File**: `src/engine/platform/ios/drag_drop_ios.m` (NEW)
  - **Handle**: UIDragInteraction events
  - **Translate**: UIKit events → engine placement requests
  - **Call**: `drag_drop_bridge_on_drop()` on drop

- [ ] TODO: Implement macOS drag-drop handler
  - **File**: `src/engine/platform/macos/drag_drop_macos.m` (NEW)
  - **Handle**: NSDraggingDestination protocol
  - **Translate**: AppKit events → engine placement requests
  - **Call**: `drag_drop_bridge_on_drop()` on drop

- [ ] TODO: Test platform handlers
  - **iOS**: Drag asset in app, drop on viewport
  - **macOS**: Drag asset in app, drop on viewport
  - **Verify**: Entity created at drop position
  - **Commit**: "feat: add platform-specific drag-drop handlers"

**Result**: iOS and macOS can drag-drop assets into viewport

---

## PHASE 7: SIMPLE UI LAYER

### 7.1 Asset Palette UI
**File**: `src/frontend/engine/studio/Sources/Components/AssetPaletteView.swift` (or extend existing)

#### Todo List:

- [ ] TODO: Examine existing asset browser in Swift
  - **Find**: Where is current asset browser implemented?
  - **Understand**: How does it display assets?
  - **Plan**: Extend for drag-and-drop

- [ ] TODO: Create categorized asset palette
  - **Categories**:
    - Terrain (blocks, natural)
    - Structures (buildings, props)
    - Interactive (doors, switches)
    - Effects (particles, lights)
  - **Display**: Grid view, drag-enabled

- [ ] TODO: Implement asset drag source
  - **iOS**: UIDragInteraction on collection cells
  - **macOS**: NSDraggingSource on table rows
  - **Data**: Asset handle in drag pasteboard

- [ ] TODO: Test palette dragging
  - **Test**: Can drag asset from palette
  - **Test**: Drop preview shows ghost
  - **Commit**: "feat: create asset palette with drag sources"

**Result**: Can drag assets from palette to viewport

---

### 7.2 Viewport Drop Target
**File**: `src/frontend/engine/studio/Sources/SceneView/ViewportView.swift` (or extend)

#### Todo List:

- [ ] TODO: Make viewport a drop target
  - **iOS**: UIDropInteraction on metal view
  - **macOS**: NSDraggingDestination on metal view
  - **Action**: Accept asset handles

- [ ] TODO: Implement drop feedback during drag
  - **Show**: Ghost preview of asset
  - **Show**: Placement validity (green/red)
  - **Show**: Grid snap points

- [ ] TODO: Implement drop handling
  - **On Drop**: Call bridge function
  - **Result**: Entity created, shown in viewport
  - **Undo**: Support undo for placement

- [ ] TODO: Test viewport drop target
  - **Test**: Drag from palette to viewport
  - **Test**: Preview shows during drag
  - **Test**: Entity created on drop
  - **Commit**: "feat: make viewport drop target for assets"

**Result**: Can drop assets into viewport and see them placed

---

### 7.3 Quick Property Adjustment
**File**: `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` (or extend)

#### Todo List:

- [ ] TODO: Create minimal property editor
  - **Properties**:
    - Position (XYZ)
    - Rotation (Euler)
    - Scale (XYZ with lock)
  - **Display**: Simple input fields

- [ ] TODO: Bind property changes to selected entity
  - **On Change**: Update entity component
  - **Update**: Viewport rendering
  - **Support**: Undo/redo

- [ ] TODO: Add keyboard shortcuts
  - **G**: Grab/move (activate position edit)
  - **R**: Rotate (activate rotation edit)
  - **S**: Scale (activate scale edit)

- [ ] TODO: Test quick properties
  - **Test**: Select entity in viewport
  - **Test**: Change position, see update
  - **Commit**: "feat: add quick property panel for placed assets"

**Result**: Can quickly adjust placed asset properties

---

## PHASE 8: VALIDATION & INTEGRATION TESTING

### 8.1 Full Workflow Integration Test

#### Todo List:

- [ ] TODO: Create integration test case
  - **Scenario**: Full drag-and-drop workflow
  - **Steps**:
    1. Create asset (exists or mock)
    2. Get asset preview
    3. Drag asset from palette
    4. Preview shows in viewport
    5. Drop asset
    6. Entity created with correct position
    7. Can select and modify
    8. Can undo placement
  - **Verify**: All steps work end-to-end

- [ ] TODO: Test asset persistence
  - **Save**: Scene with placed assets
  - **Load**: Scene from file
  - **Verify**: Assets in correct positions with correct properties

- [ ] TODO: Test error conditions
  - **Invalid placement**: Try to place colliding
  - **Out of bounds**: Try to place off map
  - **Missing asset**: Try to place deleted asset
  - **Verify**: Graceful failure, error messaging

- [ ] TODO: Performance testing
  - **Drag speed**: Smooth 60 FPS while dragging
  - **Drop creation**: Entity created in <50ms
  - **Preview rendering**: Preview updates in <16ms
  - **Commit**: "test: add integration tests for asset placement workflow"

**Result**: Full drag-and-drop workflow tested and working

---

### 8.2 Build Verification

#### Todo List:

- [ ] TODO: Verify all builds work
  - **Build**: Clean build on macOS
  - **Build**: Clean build on iOS
  - **Verify**: No compilation errors
  - **Verify**: No link errors
  - **Verify**: No new warnings introduced

- [ ] TODO: Verify no duplication introduced
  - **Check**: No duplicate include directories
  - **Check**: No duplicate API bridges
  - **Check**: No duplicate implementations
  - **Count**: Total new files = ~5-10 max

- [ ] TODO: Final commit and summary
  - **Commit**: "feat: complete drag-and-drop asset placement system"
  - **Create**: PR with all changes
  - **Document**: What was added, what was consolidated

**Result**: Clean, working integration complete

---

## SUMMARY: WHAT WE'RE ACTUALLY BUILDING

### Files Deleted (Deduplication)
- ❌ `/src/frontend/engine/studio/CVoxelForge/include/engine/include/` (-200 files)
- ❌ `physics_complete.c` (content consolidated)
- ❌ `physics_advanced_impl.c` (content consolidated)

### Files Consolidated
- 📦 `physics_core_impl.c` + `physics_api_impl.c` → `physics_engine.c`
- 📦 `simulation_loop.c` stays or → `physics_simulation.c`

### Files Extended (Existing)
- 📝 `asset_manager.c` + asset instance functions
- 📝 `render_graph.c` + preview rendering
- 📝 `entity.c` + AssetInstanceComponent
- 📝 `swift_bridge.h` + placement API
- 📝 Asset pipeline + Swift UI (existing or minimal new)

### Files Created (New, but Minimal)
```
New Implementations:
  ✨ src/engine/editor/asset_placement.c       (Facade/Coordinator)
  ✨ include/editor/asset_placement.h           (Public API)
  ✨ include/platform/drag_drop_bridge.h        (Platform Adapter)
  ✨ src/engine/platform/ios/drag_drop_ios.m   (iOS Handler)
  ✨ src/engine/platform/macos/drag_drop_macos.m (macOS Handler)

Documentation:
  📄 docs/asset_placement_architecture.md
  📄 docs/drag_drop_workflow.md
  📄 docs/physics_consolidation_summary.md
```

### Total New Code
- **Consolidated**: -3 files
- **Extended**: 5 existing files
- **Created**: 5 new files (2 C, 3 Swift, 5 headers/docs)
- **Net**: +~2 files, better organized

### Key Properties
✅ **No Duplication**: Single source for everything
✅ **Well Organized**: Each piece in logical location
✅ **Modular**: Each component testable independently
✅ **Centralized**: Single entry point (asset_placement.c)
✅ **Preserved**: All existing functionality intact

---

## EXECUTION ORDER

1. **Phase 0** (1 week): Deduplication - the foundation
2. **Phase 1** (1 week): Physics consolidation - clean architecture
3. **Phase 2** (1 week): Asset extensions - platform for instances
4. **Phase 3** (1 week): ECS component - integration with world
5. **Phase 4** (1 week): Physics queries - placement validation
6. **Phase 5** (1 week): Central coordinator - everything comes together
7. **Phase 6** (1 week): Swift bridge - UI integration
8. **Phase 7** (2 weeks): UI implementation - user-facing features
9. **Phase 8** (1 week): Testing - validation and performance

**Total**: ~10 weeks

---

## GUARDRAILS

### Don't Do:
- ❌ Create new rendering system (use existing)
- ❌ Create new physics system (use existing)
- ❌ Duplicate headers (single source)
- ❌ Create platform-specific duplicates (single bridge + adapters)
- ❌ Skip consolidation phase (foundation matters)

### Do Do:
- ✅ Extend existing systems
- ✅ Create facades/coordinators
- ✅ Add platform adapters
- ✅ Test each phase
- ✅ Document as you go
- ✅ Use existing APIs

This is the right way forward.
