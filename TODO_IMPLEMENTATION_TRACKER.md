# Implementation TODO Tracker

This document tracks all specific TODOs identified for integrating the engine with the drag-and-drop interface. Each TODO is tagged with:
- **Phase**: Which phase of the roadmap
- **Priority**: Critical, High, Medium, Low
- **Status**: Not Started, In Progress, Blocked, Completed
- **Est. LOC**: Estimated lines of code
- **File**: Target file(s)

---

## Phase 1: Audit and Integration Baseline

### 1.1 Engine System Audit

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Document all 80+ subsystems with interfaces | `docs/integration_audit.md` | Not Started | - | Create dependency matrix |
| Critical | Create engine dependency graph | `docs/integration_audit.md` | Not Started | - | Map initialization order |
| High | Verify all editor integrations | Various | Not Started | 0 | Integration test |
| High | Document all exposed C APIs | `docs/integration_audit.md` | Not Started | - | Audit all includes/ headers |

### 1.2 Swift Frontend Assessment

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Audit all 181 Swift files for duplication | `docs/frontend_audit.md` | Not Started | - | Identify iOS/macOS duplicates |
| Critical | Create feature matrix (iOS vs macOS) | `docs/frontend_audit.md` | Not Started | - | Document feature gaps |
| High | Identify missing drag-and-drop infrastructure | `docs/frontend_audit.md` | Not Started | - | Check UIDragInteraction, NSDraggingDestination |
| High | Document all C bridge calls | `docs/frontend_audit.md` | Not Started | - | Trace Swift→C interactions |

### 1.3 Asset Pipeline Review

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Map current asset loading flow | `docs/asset_pipeline_audit.md` | Not Started | - | Manifest→serialization→caching |
| High | Identify asset types for drag-and-drop | `docs/asset_pipeline_audit.md` | Not Started | - | 3D models, textures, animations, etc. |
| High | Document asset hot-reload capabilities | `docs/asset_pipeline_audit.md` | Not Started | - | Check reload system |

### 1.4 Integration Points Documentation

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Create integration checklist with all systems | `docs/integration_checklist.md` | Not Started | - | Master checklist |
| High | Document all existing TODOs in codebase | `docs/integration_checklist.md` | Not Started | - | Grep for "TODO" comments |

---

## Phase 2: Unified Asset Pipeline

### 2.1 Asset Registry Enhancement

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Add asset type enumeration | `src/engine/resource/asset_manager.c/h` | Not Started | 100 | Enum all drag-and-drop types |
| High | Implement asset categorization | `src/engine/resource/asset_manager.c` | Not Started | 200 | Category system |
| High | Add asset tagging system | `src/engine/resource/asset_manager.c` | Not Started | 150 | Tag-based filtering |
| High | Create asset search/discovery API | `src/engine/resource/asset_manager.c` | Not Started | 300 | Search by type, tag, name |
| Medium | Implement asset versioning | `src/engine/resource/asset_manager.c` | Not Started | 250 | Backwards compatibility |

### 2.2 Drag-and-Drop Asset Metadata

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Extend asset manifest for drag-and-drop metadata | `src/engine/core/serialization/asset_manifest.c` | Not Started | 200 | Preview UUID, operations, properties |
| High | Add quick-load metadata system | `src/engine/core/serialization/asset_manifest.c` | Not Started | 150 | UI responsiveness optimization |
| Medium | Implement async metadata loading | `src/engine/core/serialization/asset_manifest.c` | Not Started | 100 | Background loading |

### 2.3 Asset Instancing System

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Create AssetInstance component | `src/engine/ecs/entity_component_system.c` | Not Started | 300 | Core component type |
| Critical | Implement asset reference vs copy mechanics | `src/engine/ecs/entity_component_system.c` | Not Started | 200 | Choose when to reference/copy |
| High | Add instance override system | `src/engine/ecs/entity_component_system.c` | Not Started | 250 | Instance-specific overrides |
| High | Support multi-level prefab nesting | `src/engine/ecs/entity_component_system.c` | Not Started | 200 | Nested hierarchies |
| High | Implement instance-to-asset sync | `src/engine/ecs/entity_component_system.c` | Not Started | 180 | Update instances when asset changes |

### 2.4 Asset Streaming and Caching

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Optimize asset loading for rapid loading | `src/engine/streaming/chunk_streaming.c` | Not Started | 200 | Target: <100ms |
| High | Implement smart caching based on viewport | `src/engine/streaming/chunk_streaming.c` | Not Started | 150 | Spatial caching |
| Medium | Add memory pressure notifications | `src/engine/streaming/chunk_streaming.c` | Not Started | 100 | Alert when memory high |
| Medium | Create asset preloading hints | `src/engine/streaming/chunk_streaming.c` | Not Started | 120 | Predictive loading |
| Low | Implement LRU eviction for asset cache | `src/engine/streaming/chunk_streaming.c` | Not Started | 100 | Memory management |

### 2.5 Asset Validation Pipeline

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create validators for each asset type | `src/engine/tools/asset_validator.c` | Not Started | 400 | Type-specific validation |
| High | Implement dependency checking | `src/engine/tools/asset_validator.c` | Not Started | 200 | Detect broken references |
| Medium | Add asset health reporting | `src/engine/tools/asset_validator.c` | Not Started | 150 | Health dashboard |
| Medium | Create migration tools for legacy assets | `src/engine/tools/asset_validator.c` | Not Started | 250 | Asset format updates |
| Medium | Implement automatic asset repair | `src/engine/tools/asset_validator.c` | Not Started | 200 | Auto-fix common issues |

---

## Phase 3: Swift Frontend Unification

### 3.1 Unified Drag-and-Drop System

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Create abstract DragDropTarget protocol | `src/frontend/engine/studio/Sources/DragDrop/DragDropSystem.swift` | Not Started | 100 | Platform-agnostic protocol |
| Critical | Implement iOS drag-drop adapter | `src/frontend/engine/studio/Sources/DragDrop/DragDropSystem.swift` | Not Started | 200 | UIDragInteraction wrapper |
| Critical | Implement macOS drag-drop adapter | `src/frontend/engine/studio/Sources/DragDrop/DragDropSystem.swift` | Not Started | 200 | NSDraggingDestination wrapper |
| High | Create uniform drag event handling | `src/frontend/engine/studio/Sources/DragDrop/DragDropSystem.swift` | Not Started | 150 | Common event model |
| High | Implement preview rendering | `src/frontend/engine/studio/Sources/DragDrop/DragDropSystem.swift` | Not Started | 180 | Custom preview generation |
| High | Add drag state machine | `src/frontend/engine/studio/Sources/DragDrop/DragDropSystem.swift` | Not Started | 120 | State tracking |

### 3.2 Unified Asset Browser

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create single asset browser view | `src/frontend/engine/studio/Sources/Components/AssetBrowser.swift` | Not Started | 300 | Master asset browser |
| High | Implement platform-specific layouts | `src/frontend/engine/studio/Sources/Components/AssetBrowser.swift` | Not Started | 250 | iOS grid, macOS split |
| High | Add search/filter functionality | `src/frontend/engine/studio/Sources/Components/AssetBrowser.swift` | Not Started | 150 | Search by name, type, tag |
| High | Implement multi-asset selection | `src/frontend/engine/studio/Sources/Components/AssetBrowser.swift` | Not Started | 100 | Bulk operations |
| Medium | Create asset preview panel | `src/frontend/engine/studio/Sources/Components/AssetBrowser.swift` | Not Started | 120 | Preview on selection |
| Medium | Add tagging and categorization UI | `src/frontend/engine/studio/Sources/Components/AssetBrowser.swift` | Not Started | 80 | Tag management |

### 3.3 Unified Viewport

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Create shared rendering backend (Metal) | `src/frontend/engine/studio/Sources/SceneView/UnifiedViewport.swift` | Not Started | 400 | Shared Metal renderer |
| High | Implement iOS gesture handling | `src/frontend/engine/studio/Sources/SceneView/UnifiedViewport.swift` | Not Started | 150 | Pan, pinch, rotation |
| High | Implement macOS mouse/trackpad handling | `src/frontend/engine/studio/Sources/SceneView/UnifiedViewport.swift` | Not Started | 150 | Mouse + trackpad events |
| High | Implement drop zone visualization | `src/frontend/engine/studio/Sources/SceneView/UnifiedViewport.swift` | Not Started | 120 | Drop target highlighting |
| High | Add snap/alignment grid display | `src/frontend/engine/studio/Sources/SceneView/UnifiedViewport.swift` | Not Started | 100 | Grid rendering |
| High | Create placement feedback (preview ghosts) | `src/frontend/engine/studio/Sources/SceneView/UnifiedViewport.swift` | Not Started | 150 | Translucent placement preview |
| Medium | Implement undo/redo for viewport actions | `src/frontend/engine/studio/Sources/SceneView/UnifiedViewport.swift` | Not Started | 200 | Action history |

### 3.4 Property Inspector Unification

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create single property editor view | `src/frontend/engine/studio/Sources/Components/PropertyInspector.swift` | Not Started | 400 | Master property editor |
| High | Implement platform-specific layouts | `src/frontend/engine/studio/Sources/Components/PropertyInspector.swift` | Not Started | 150 | iOS modal, macOS panel |
| High | Implement all property types | `src/frontend/engine/studio/Sources/Components/PropertyInspector.swift` | Not Started | 300 | vec3, quat, material, etc. |
| High | Add live preview for property changes | `src/frontend/engine/studio/Sources/Components/PropertyInspector.swift` | Not Started | 100 | Real-time viewport update |
| Medium | Create collapsible sections | `src/frontend/engine/studio/Sources/Components/PropertyInspector.swift` | Not Started | 80 | Organize properties |
| Medium | Implement drag-to-modify for numeric values | `src/frontend/engine/studio/Sources/Components/PropertyInspector.swift` | Not Started | 120 | Slider-like modification |

### 3.5 UI Layer Architecture

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create abstract UI component protocol | `src/frontend/engine/studio/Sources/UI/UIArchitecture.swift` | Not Started | 200 | Platform-agnostic UI |
| High | Implement platform adapters for controls | `src/frontend/engine/studio/Sources/UI/UIArchitecture.swift` | Not Started | 250 | Buttons, toggles, sliders |
| High | Implement platform adapters for dialogs | `src/frontend/engine/studio/Sources/UI/UIArchitecture.swift` | Not Started | 200 | Dialogs, popovers |
| High | Implement platform adapters for menus | `src/frontend/engine/studio/Sources/UI/UIArchitecture.swift` | Not Started | 150 | Context menus, contextual actions |
| High | Implement platform adapters for notifications | `src/frontend/engine/studio/Sources/UI/UIArchitecture.swift` | Not Started | 100 | Alerts, toasts |
| Medium | Ensure consistent styling across platforms | `src/frontend/engine/studio/Sources/UI/UIArchitecture.swift` | Not Started | 100 | Unified visual language |
| Medium | Implement dark/light theme support | `src/frontend/engine/studio/Sources/UI/UIArchitecture.swift` | Not Started | 120 | Theme system |

### 3.6 C/Swift Bridge Enhancement

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Add drag-and-drop specific C functions | `include/swift_bridge_wrapper.h` | Not Started | 50 | Function declarations |
| Critical | Implement drag preview generation function | `include/swift_bridge_wrapper.c` | Not Started | 100 | asset_get_drag_preview() |
| Critical | Implement viewport raycast for drop function | `include/swift_bridge_wrapper.c` | Not Started | 80 | viewport_raycast_for_drop() |
| Critical | Implement asset instance creation function | `include/swift_bridge_wrapper.c` | Not Started | 120 | asset_create_instance_at_position() |
| High | Implement drop zone list function | `include/swift_bridge_wrapper.c` | Not Started | 60 | get_drop_zone_list() |
| High | Implement batch asset operations | `include/swift_bridge_wrapper.c` | Not Started | 200 | Batch create, delete, modify |
| High | Add progress callbacks for async operations | `include/swift_bridge_wrapper.c` | Not Started | 150 | Async operation tracking |
| High | Create thread-safe callbacks for Swift events | `include/swift_bridge_wrapper.c` | Not Started | 200 | Swift event callbacks |

---

## Phase 4: 3D Asset Rendering & Interaction Layer

### 4.1 Asset Preview Rendering

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create off-screen rendering for previews | `src/engine/renderer/asset_preview_renderer.c` | Not Started | 250 | Framebuffer-based preview |
| High | Implement quick thumbnail generation | `src/engine/renderer/asset_preview_renderer.c` | Not Started | 150 | Small preview images |
| High | Add custom lighting for previews | `src/engine/renderer/asset_preview_renderer.c` | Not Started | 100 | Standard preview lighting |
| High | Cache preview images | `src/engine/renderer/asset_preview_renderer.c` | Not Started | 80 | Prevent regeneration |
| Medium | Support animated previews for skeletal meshes | `src/engine/renderer/asset_preview_renderer.c` | Not Started | 120 | Animated preview playback |

### 4.2 Transform Gizmo System

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Extend gizmo system for drag-and-drop | `src/engine/editor/transform_gizmo.c` | Not Started | 200 | Gizmo enhancements |
| High | Implement translation gizmo | `src/engine/editor/transform_gizmo.c` | Not Started | 150 | XYZ axis manipulation |
| High | Implement rotation gizmo | `src/engine/editor/transform_gizmo.c` | Not Started | 180 | All axes, plane, free rotate |
| High | Implement scale gizmo | `src/engine/editor/transform_gizmo.c` | Not Started | 150 | Uniform and per-axis |
| Medium | Implement touch-friendly gizmo sizing | `src/engine/editor/transform_gizmo.c` | Not Started | 80 | iOS touch target sizing |
| Medium | Add coordinate system toggle | `src/engine/editor/transform_gizmo.c` | Not Started | 60 | World/local/parent modes |
| Medium | Create smart axis highlighting on hover | `src/engine/editor/transform_gizmo.c` | Not Started | 100 | Visual feedback |
| Medium | Implement grid snapping | `src/engine/editor/transform_gizmo.c` | Not Started | 120 | Grid-based movement |
| Medium | Implement angle snapping | `src/engine/editor/transform_gizmo.c` | Not Started | 80 | Snap to 15°, 45°, 90° |
| Medium | Implement vertex snapping | `src/engine/editor/transform_gizmo.c` | Not Started | 150 | Model-to-model snap |

### 4.3 Physics Asset Preview

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create preview physics bodies | `src/engine/physics/physics_preview.c` | Not Started | 150 | Temporary physics representation |
| High | Implement soft-body visualization | `src/engine/physics/physics_preview.c` | Not Started | 100 | Deformable body preview |
| High | Add collision wireframe display | `src/engine/physics/physics_preview.c` | Not Started | 120 | Physics shape visualization |
| High | Show physics properties inline | `src/engine/physics/physics_preview.c` | Not Started | 100 | Mass, friction, restitution display |
| Medium | Implement physics material swatches | `src/engine/physics/physics_preview.c` | Not Started | 80 | Material preset UI |
| Medium | Add quick physics property assignment UI | `src/engine/physics/physics_preview.c` | Not Started | 100 | Quick property panel |

### 4.4 Placement Feedback System

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Implement ghost preview | `src/engine/editor/placement_feedback.c` | Not Started | 150 | Translucent placement preview |
| High | Add placement validity indicator | `src/engine/editor/placement_feedback.c` | Not Started | 120 | Green/yellow/red feedback |
| High | Create snap target visualization | `src/engine/editor/placement_feedback.c` | Not Started | 100 | Show snap targets |
| High | Implement alignment grid display | `src/engine/editor/placement_feedback.c` | Not Started | 80 | Grid visualization |
| Medium | Add distance/measurement display | `src/engine/editor/placement_feedback.c` | Not Started | 100 | Measurement tool |

### 4.5 Selection and Highlight System

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Implement outline rendering | `src/engine/renderer/selection_highlight.c` | Not Started | 150 | Selected asset outline |
| High | Add selection glow effect | `src/engine/renderer/selection_highlight.c` | Not Started | 100 | Glow/bloom on selection |
| High | Create hierarchy breadcrumb visualization | `src/engine/renderer/selection_highlight.c` | Not Started | 120 | Hierarchy path display |
| High | Implement multi-selection highlighting | `src/engine/renderer/selection_highlight.c` | Not Started | 100 | Multiple asset selection |
| Medium | Add selection cycling | `src/engine/renderer/selection_highlight.c` | Not Started | 80 | Overlap resolution |

### 4.6 Interactive Viewport Features

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create orthographic mode | `src/engine/editor/viewport_editor.c` | Not Started | 100 | Orthographic camera |
| High | Add reference images/grids | `src/engine/editor/viewport_editor.c` | Not Started | 120 | Background reference |
| High | Implement camera bookmarks | `src/engine/editor/viewport_editor.c` | Not Started | 80 | Save/restore camera positions |
| High | Create focus-on-selection with framing | `src/engine/editor/viewport_editor.c` | Not Started | 100 | Frame selected assets |
| Medium | Add viewport split view | `src/engine/editor/viewport_editor.c` | Not Started | 200 | Top/front/side + perspective |
| Medium | Implement fog of war for visibility testing | `src/engine/editor/viewport_editor.c` | Not Started | 150 | Visibility debugging |

---

## Phase 5: Environment Builder

### 5.1 Simple Environment Builder UI

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Create main environment builder view | `src/frontend/engine/studio/Sources/Gameplay/EnvironmentBuilder.swift` | Not Started | 400 | Main UI |
| High | Implement simple mode | `src/frontend/engine/studio/Sources/Gameplay/EnvironmentBuilder.swift` | Not Started | 300 | Drag-and-drop mode |
| High | Implement advanced mode | `src/frontend/engine/studio/Sources/Gameplay/EnvironmentBuilder.swift` | Not Started | 300 | Full system access |
| High | Add mode toggle | `src/frontend/engine/studio/Sources/Gameplay/EnvironmentBuilder.swift` | Not Started | 50 | Switch between modes |
| Medium | Implement scene templates | `src/frontend/engine/studio/Sources/Gameplay/EnvironmentBuilder.swift` | Not Started | 150 | Template selection |

### 5.2 Asset Palette System

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create categorized asset palettes | `src/engine/editor/asset_palette.c` | Not Started | 250 | Categories: terrain, structures, etc. |
| High | Implement custom palette creation | `src/engine/editor/asset_palette.c` | Not Started | 150 | User palette creation |
| High | Add palette search and filtering | `src/engine/editor/asset_palette.c` | Not Started | 120 | Search/filter UI |
| Medium | Support palette import/export | `src/engine/editor/asset_palette.c` | Not Started | 100 | Share palettes |
| Medium | Create asset collection grouping | `src/engine/editor/asset_palette.c` | Not Started | 80 | Group related assets |

### 5.3 Placement Grid and Alignment

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Implement configurable grid | `src/engine/editor/placement_grid.c` | Not Started | 150 | Grid size adjustment |
| High | Add snap-to-grid modes | `src/engine/editor/placement_grid.c` | Not Started | 100 | None/Coarse/Fine |
| High | Implement face-to-face snapping | `src/engine/editor/placement_grid.c` | Not Started | 120 | Modular asset snapping |
| Medium | Create custom pivot point handling | `src/engine/editor/placement_grid.c` | Not Started | 80 | Pivot selection |
| Medium | Add alignment tool | `src/engine/editor/placement_grid.c` | Not Started | 100 | Align multiple assets |

### 5.4 Quick Property Adjustment

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create simple property panel | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 300 | Quick edit UI |
| High | Implement position controls | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 100 | XYZ input or drag |
| High | Implement rotation controls | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 80 | Euler angle controls |
| High | Implement scale controls | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 80 | With lock uniform button |
| High | Add material quick picker | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 100 | Color picker |
| High | Add physics material dropdown | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 50 | Material preset |
| Medium | Implement keyboard shortcuts | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 80 | G/R/S shortcuts |
| Medium | Add property value reset button | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 30 | Reset to default |
| Medium | Create property animation | `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift` | Not Started | 100 | Interpolate values |

### 5.5 Scene Organization

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create scene hierarchy view | `src/frontend/engine/studio/Sources/SceneGraph/SceneOrganizer.swift` | Not Started | 300 | Expandable tree |
| High | Implement drag-to-reorder | `src/frontend/engine/studio/Sources/SceneGraph/SceneOrganizer.swift` | Not Started | 100 | Reorder in hierarchy |
| High | Add visibility toggle | `src/frontend/engine/studio/Sources/SceneGraph/SceneOrganizer.swift` | Not Started | 50 | Eye icon |
| High | Add lock/unlock toggle | `src/frontend/engine/studio/Sources/SceneGraph/SceneOrganizer.swift` | Not Started | 50 | Lock icon |
| Medium | Implement smart grouping | `src/frontend/engine/studio/Sources/SceneGraph/SceneOrganizer.swift` | Not Started | 150 | Group by type/proximity/custom |
| Medium | Add layer system | `src/frontend/engine/studio/Sources/SceneGraph/SceneOrganizer.swift` | Not Started | 120 | Layer management |
| Medium | Create outline search | `src/frontend/engine/studio/Sources/SceneGraph/SceneOrganizer.swift` | Not Started | 80 | Search in outline |

### 5.6 Scene Templates and Presets

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create starter templates | `src/engine/editor/scene_templates.c` | Not Started | 150 | 5-6 templates |
| Medium | Implement preset system | `src/engine/editor/scene_templates.c` | Not Started | 120 | Save/load presets |
| Low | Add template customization | `src/engine/editor/scene_templates.c` | Not Started | 100 | Edit template options |
| Low | Create template preview | `src/engine/editor/scene_templates.c` | Not Started | 80 | Thumbnail |

### 5.7 Asset Instance Management

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create instance tracking | `src/engine/ecs/asset_instance_manager.c` | Not Started | 150 | Instance counts |
| High | Implement batch operations | `src/engine/ecs/asset_instance_manager.c` | Not Started | 200 | Replace/scale/rotate all |
| Medium | Add instance prefab override system | `src/engine/ecs/asset_instance_manager.c` | Not Started | 120 | Instance overrides |
| Medium | Create instance sync to prefab | `src/engine/ecs/asset_instance_manager.c` | Not Started | 100 | Update instances from prefab |

---

## Phase 6: Animation Integration

### 6.1 Animation Property Assignment

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create animation assignment UI | `src/frontend/engine/studio/Sources/Animation/AssetAnimationSetup.swift` | Not Started | 250 | Animation browser + assign |
| High | Implement animation preview | `src/frontend/engine/studio/Sources/Animation/AssetAnimationSetup.swift` | Not Started | 100 | Preview in viewport |
| High | Add animation speed/playback controls | `src/frontend/engine/studio/Sources/Animation/AssetAnimationSetup.swift` | Not Started | 100 | Speed slider, play/pause |
| Medium | Create animation loop toggle | `src/frontend/engine/studio/Sources/Animation/AssetAnimationSetup.swift` | Not Started | 50 | Loop on/off |

### 6.2 Timeline Integration

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Extend timeline for asset placement keyframes | `src/engine/animation/animation_timeline.c` | Not Started | 200 | Asset placement tracks |
| High | Extend timeline for animation keyframes | `src/engine/animation/animation_timeline.c` | Not Started | 150 | Animation tracks |
| High | Extend timeline for material animation | `src/engine/animation/animation_timeline.c` | Not Started | 120 | Material property tracks |
| High | Extend timeline for camera animation | `src/engine/animation/animation_timeline.c` | Not Started | 120 | Camera tracks |
| High | Extend timeline for light animation | `src/engine/animation/animation_timeline.c` | Not Started | 100 | Light tracks |
| Medium | Implement scrubbing and playback | `src/engine/animation/animation_timeline.c` | Not Started | 100 | Timeline playback |
| Medium | Add keyframe selection and manipulation | `src/engine/animation/animation_timeline.c` | Not Started | 120 | Keyframe editing |
| Medium | Create curve editor | `src/engine/animation/animation_timeline.c` | Not Started | 150 | FCurve editing |
| Medium | Support multiple animation tracks | `src/engine/animation/animation_timeline.c` | Not Started | 100 | Track management |

### 6.3 Skeletal Animation in Viewport

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Implement real-time skeletal animation rendering | `src/engine/renderer/skeletal_render.c` | Not Started | 200 | GPU skeleton update |
| High | Add bone visualization | `src/engine/renderer/skeletal_render.c` | Not Started | 150 | Skeleton wireframe |
| High | Add joint position indicators | `src/engine/renderer/skeletal_render.c` | Not Started | 80 | Joint visualizations |
| Medium | Add bone weight heat maps | `src/engine/renderer/skeletal_render.c` | Not Started | 120 | Weight visualization |
| Medium | Create inverse kinematics preview | `src/engine/renderer/skeletal_render.c` | Not Started | 100 | IK solver preview |
| Medium | Implement animation layer preview | `src/engine/renderer/skeletal_render.c` | Not Started | 100 | Blending visualization |

### 6.4 Particle Animation Preview

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create particle effect preview | `src/engine/vfx/particle_preview.c` | Not Started | 150 | Preview rendering |
| Medium | Implement particle spawning at asset location | `src/engine/vfx/particle_preview.c` | Not Started | 100 | Position-based spawning |
| Medium | Add emission rate adjustment | `src/engine/vfx/particle_preview.c` | Not Started | 80 | Real-time rate control |
| Low | Create particle lifetime visualization | `src/engine/vfx/particle_preview.c` | Not Started | 80 | Timeline visualization |
| Low | Implement particle property tweaking UI | `src/engine/vfx/particle_preview.c` | Not Started | 100 | Property panel |

### 6.5 Audio and Animation Synchronization

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create audio trigger system for animations | `src/engine/audio/audio_animation_sync.c` | Not Started | 150 | Trigger system |
| Medium | Implement beat detection and sync | `src/engine/audio/audio_animation_sync.c` | Not Started | 120 | Beat detection |
| Medium | Add sound effect placement in timeline | `src/engine/audio/audio_animation_sync.c` | Not Started | 100 | Audio track |
| Low | Create voice sync for skeletal animation | `src/engine/audio/audio_animation_sync.c` | Not Started | 150 | Voice-sync system |
| Low | Implement audio analysis visualization | `src/engine/audio/audio_animation_sync.c` | Not Started | 100 | Waveform display |

### 6.6 Animation Blending and Transitions

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Extend blend tree editor | `src/engine/animation/blend_tree_editor.c` | Not Started | 250 | Visual state machine |
| Medium | Implement visual blend state machine | `src/engine/animation/blend_tree_editor.c` | Not Started | 150 | State diagram editor |
| Medium | Implement state previewing | `src/engine/animation/blend_tree_editor.c` | Not Started | 100 | Preview state |
| Low | Create automatic transition detection | `src/engine/animation/blend_tree_editor.c` | Not Started | 100 | Auto-transition suggestion |
| Low | Add transition preview in viewport | `src/engine/animation/blend_tree_editor.c` | Not Started | 80 | Show transition animation |

### 6.7 Cinematics and Camera Animation

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create camera animation keyframing | `src/engine/animation/cinematic_controller.c` | Not Started | 150 | Camera keyframe system |
| Medium | Implement camera path creation | `src/engine/animation/cinematic_controller.c` | Not Started | 150 | Spline-based paths |
| Medium | Add camera animation preview | `src/engine/animation/cinematic_controller.c` | Not Started | 100 | Preview playback |
| Low | Create dolly and pan templates | `src/engine/animation/cinematic_controller.c` | Not Started | 80 | Animation presets |
| Low | Implement letterbox aspect ratio control | `src/engine/animation/cinematic_controller.c` | Not Started | 100 | Cinematic aspect |

---

## Phase 7: Advanced Features

### 7.1 Procedural Generation Integration

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create procedural asset generator UI | `src/engine/procedural_generation/procedural_tools.c` | Not Started | 200 | Generator UI |
| Medium | Implement terrain generation template | `src/engine/procedural_generation/procedural_tools.c` | Not Started | 150 | Terrain procedural gen |
| Medium | Implement building generation template | `src/engine/procedural_generation/procedural_tools.c` | Not Started | 150 | Building generation |
| Low | Implement tree generation template | `src/engine/procedural_generation/procedural_tools.c` | Not Started | 120 | Tree procedural gen |
| Low | Implement rock/cave formation template | `src/engine/procedural_generation/procedural_tools.c` | Not Started | 150 | Cave generation |
| Low | Add noise parameter exposure | `src/engine/procedural_generation/procedural_tools.c` | Not Started | 120 | Noise controls |
| Low | Create generation preview | `src/engine/procedural_generation/procedural_tools.c` | Not Started | 100 | Preview generation |
| Low | Implement seed-based reproducibility | `src/engine/procedural_generation/procedural_tools.c` | Not Started | 80 | Seed system |

### 7.2 Physics-Based Placement

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create gravity simulation for placement | `src/engine/editor/physics_placement.c` | Not Started | 150 | Gravity sim |
| Medium | Implement settling animation | `src/engine/editor/physics_placement.c` | Not Started | 100 | Settle animation |
| Medium | Add physics constraints | `src/engine/editor/physics_placement.c` | Not Started | 120 | Constraint system |
| Low | Create collision group assignment | `src/engine/editor/physics_placement.c` | Not Started | 80 | Group UI |
| Low | Implement rigidbody property presets | `src/engine/editor/physics_placement.c` | Not Started | 100 | Preset system |

### 7.3 Advanced Snapping and Alignment

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Implement vertex snapping | `src/engine/editor/advanced_snapping.c` | Not Started | 150 | Model-to-model snap |
| High | Add edge-to-edge alignment | `src/engine/editor/advanced_snapping.c` | Not Started | 120 | Edge alignment |
| High | Create surface normal alignment | `src/engine/editor/advanced_snapping.c` | Not Started | 100 | Normal-based placement |
| Medium | Implement distribute spacing | `src/engine/editor/advanced_snapping.c` | Not Started | 120 | Distribute assets |
| Medium | Implement circular arrangement | `src/engine/editor/advanced_snapping.c` | Not Started | 100 | Circular layout |
| Medium | Implement grid arrangement | `src/engine/editor/advanced_snapping.c` | Not Started | 100 | Grid layout |
| Medium | Implement array duplication | `src/engine/editor/advanced_snapping.c` | Not Started | 120 | Array duplicate |
| Low | Add magnetic snapping | `src/engine/editor/advanced_snapping.c` | Not Started | 100 | Magnetic snap |

### 7.4 Scripting and Interactivity

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create visual scripting interface | `src/engine/scripting/visual_script_editor.c` | Not Started | 300 | Node graph editor |
| Medium | Implement event nodes | `src/engine/scripting/visual_script_editor.c` | Not Started | 150 | Event node types |
| Medium | Implement action nodes | `src/engine/scripting/visual_script_editor.c` | Not Started | 150 | Action node types |
| Medium | Implement logic nodes | `src/engine/scripting/visual_script_editor.c` | Not Started | 120 | Logic node types |
| Medium | Implement asset event binding | `src/engine/scripting/visual_script_editor.c` | Not Started | 200 | Bind events to assets |
| Low | Add Lua script inline editing | `src/engine/scripting/visual_script_editor.c` | Not Started | 150 | Lua editor |
| Low | Create script debugging | `src/engine/scripting/visual_script_editor.c` | Not Started | 150 | Debugger UI |

### 7.5 Lighting and Environment Control

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create quick light placement UI | `src/engine/editor/lighting_editor.c` | Not Started | 150 | Light palette |
| High | Implement light intensity adjustment | `src/engine/editor/lighting_editor.c` | Not Started | 80 | Intensity control |
| High | Implement light color picker | `src/engine/editor/lighting_editor.c` | Not Started | 100 | Color picker |
| High | Implement shadow quality selection | `src/engine/editor/lighting_editor.c` | Not Started | 80 | Shadow quality UI |
| Medium | Implement environment lighting | `src/engine/editor/lighting_editor.c` | Not Started | 120 | Ambient + IBL |
| Medium | Implement skybox selection | `src/engine/editor/lighting_editor.c` | Not Started | 100 | Skybox browser |
| Medium | Implement ambient lighting | `src/engine/editor/lighting_editor.c` | Not Started | 80 | Ambient controls |
| Medium | Implement global illumination settings | `src/engine/editor/lighting_editor.c` | Not Started | 100 | GI controls |
| Low | Add time-of-day simulation | `src/engine/editor/lighting_editor.c` | Not Started | 150 | Time-of-day system |
| Low | Create lighting preset templates | `src/engine/editor/lighting_editor.c` | Not Started | 100 | Lighting presets |

### 7.6 Material and Shader Editor Enhancement

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Extend material editor | `src/engine/renderer/material_editor_advanced.c` | Not Started | 250 | Material editor |
| Medium | Implement real-time shader compilation | `src/engine/renderer/material_editor_advanced.c` | Not Started | 150 | Hot reload |
| Medium | Implement shader graph visual editing | `src/engine/renderer/material_editor_advanced.c` | Not Started | 300 | Graph editor |
| Medium | Implement material parameter exposure | `src/engine/renderer/material_editor_advanced.c` | Not Started | 120 | Parameter UI |
| Medium | Implement texture assignment | `src/engine/renderer/material_editor_advanced.c` | Not Started | 100 | Texture browser |
| Low | Implement normal map properties | `src/engine/renderer/material_editor_advanced.c` | Not Started | 80 | Normal map UI |
| Low | Implement PBR properties | `src/engine/renderer/material_editor_advanced.c` | Not Started | 100 | PBR controls |
| Low | Create material preview viewport | `src/engine/renderer/material_editor_advanced.c` | Not Started | 100 | Material preview |
| Low | Implement material variant system | `src/engine/renderer/material_editor_advanced.c` | Not Started | 120 | Variant creation |
| Low | Add material animation curves | `src/engine/renderer/material_editor_advanced.c` | Not Started | 120 | Animated materials |

### 7.7 Performance Optimization Tools

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create optimization suggestions | `src/engine/tools/performance_optimizer.c` | Not Started | 200 | Suggestion system |
| Medium | Implement LOD recommendations | `src/engine/tools/performance_optimizer.c` | Not Started | 150 | LOD suggestion |
| Medium | Implement batching opportunities detection | `src/engine/tools/performance_optimizer.c` | Not Started | 150 | Batch detection |
| Medium | Implement overdraw detection | `src/engine/tools/performance_optimizer.c` | Not Started | 120 | Overdraw analysis |
| Low | Implement memory usage analysis | `src/engine/tools/performance_optimizer.c` | Not Started | 120 | Memory profiler |
| Low | Implement batch mesh generation | `src/engine/tools/performance_optimizer.c` | Not Started | 200 | Mesh batching tool |
| Low | Add level-of-detail automation | `src/engine/tools/performance_optimizer.c` | Not Started | 180 | LOD automation |
| Low | Create draw call reduction recommendations | `src/engine/tools/performance_optimizer.c` | Not Started | 100 | Draw call analysis |
| Low | Implement performance profiler integration | `src/engine/tools/performance_optimizer.c` | Not Started | 150 | Profiler integration |

### 7.8 Asset Validation and Health Check

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create missing material detection | `src/engine/tools/asset_health_check.c` | Not Started | 100 | Material validation |
| Medium | Implement broken texture reference detection | `src/engine/tools/asset_health_check.c` | Not Started | 100 | Texture validation |
| Medium | Implement invalid physics shape detection | `src/engine/tools/asset_health_check.c` | Not Started | 100 | Physics validation |
| Medium | Implement scale inconsistency detection | `src/engine/tools/asset_health_check.c` | Not Started | 80 | Scale validation |
| Medium | Implement layer assignment validation | `src/engine/tools/asset_health_check.c` | Not Started | 80 | Layer validation |
| Low | Implement auto-repair | `src/engine/tools/asset_health_check.c` | Not Started | 150 | Auto-repair system |
| Low | Add health report generation | `src/engine/tools/asset_health_check.c` | Not Started | 100 | Report generation |
| Low | Create migration tools | `src/engine/tools/asset_health_check.c` | Not Started | 150 | Asset migration |

### 7.9 Version Control and Asset Management

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Low | Create asset versioning system | `src/engine/tools/asset_version_control.c` | Not Started | 200 | Version tracking |
| Low | Implement version revert | `src/engine/tools/asset_version_control.c` | Not Started | 120 | Revert to version |
| Low | Implement version comparison | `src/engine/tools/asset_version_control.c` | Not Started | 150 | Diff visualization |
| Low | Implement check-in/check-out | `src/engine/tools/asset_version_control.c` | Not Started | 200 | Lock system |
| Low | Add conflict resolution | `src/engine/tools/asset_version_control.c` | Not Started | 150 | Conflict handling |
| Low | Create asset merge tools | `src/engine/tools/asset_version_control.c` | Not Started | 150 | Merge system |

### 7.10 Networking and Collaboration (Future)

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Low | Design collaborative editing system | `src/engine/networking/collaborative_editing.c` | Not Started | 400 | Collab architecture |
| Low | Implement real-time asset placement sync | `src/engine/networking/collaborative_editing.c` | Not Started | 300 | Network sync |
| Low | Implement user presence visualization | `src/engine/networking/collaborative_editing.c` | Not Started | 200 | User cursors |
| Low | Implement change broadcast | `src/engine/networking/collaborative_editing.c` | Not Started | 250 | Broadcast system |
| Low | Implement conflict resolution | `src/engine/networking/collaborative_editing.c` | Not Started | 200 | Resolve conflicts |
| Low | Implement session management | `src/engine/networking/collaborative_editing.c` | Not Started | 150 | Session system |
| Low | Create permission system | `src/engine/networking/collaborative_editing.c` | Not Started | 150 | Permission system |
| Low | Add activity history | `src/engine/networking/collaborative_editing.c` | Not Started | 150 | History tracking |

---

## Phase 8: Testing & Optimization

### 8.1 Integration Testing

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Create integration test suite | `tests/integration/` | Not Started | 500+ | All integration tests |
| High | Asset pipeline end-to-end tests | `tests/integration/test_asset_pipeline.c` | Not Started | 200 | E2E asset tests |
| High | Drag-and-drop workflow tests | `tests/integration/test_drag_drop.swift` | Not Started | 300 | UI workflow tests |
| High | Animation playback tests | `tests/integration/test_animation.c` | Not Started | 200 | Animation tests |
| High | Physics interaction tests | `tests/integration/test_physics.c` | Not Started | 200 | Physics tests |
| High | Multi-asset operation tests | `tests/integration/test_batch_ops.c` | Not Started | 150 | Batch op tests |
| Medium | UI state consistency tests | `tests/integration/test_ui_state.swift` | Not Started | 200 | UI state tests |

### 8.2 Performance Optimization

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Critical | Profile asset loading performance | `tests/performance/asset_loading.c` | Not Started | 150 | Target: <100ms |
| Critical | Profile viewport rendering | `tests/performance/viewport_render.c` | Not Started | 150 | Target: 60 FPS |
| High | Profile UI responsiveness | `tests/performance/ui_responsiveness.swift` | Not Started | 150 | Target: <16ms |
| High | Profile memory usage | `tests/performance/memory_usage.c` | Not Started | 150 | Target: <1GB |
| High | Profile asset preview generation | `tests/performance/preview_gen.c` | Not Started | 150 | Target: <500ms batch |

### 8.3 Platform-Specific Testing

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | iOS testing (all devices) | `tests/platform/ios_devices.swift` | Not Started | 200 | Test all iPhones/iPads |
| High | macOS testing (Intel/ARM) | `tests/platform/macos_silicon.swift` | Not Started | 200 | Test both architectures |
| Medium | Low-memory testing | `tests/platform/low_memory.swift` | Not Started | 150 | Low-memory scenarios |
| Medium | Slow network testing | `tests/platform/slow_network.swift` | Not Started | 150 | Network degradation |
| Medium | Orientation change testing | `tests/platform/orientation.swift` | Not Started | 100 | Orientation handling |
| Medium | Multitasking testing | `tests/platform/multitasking.swift` | Not Started | 100 | Multitasking support |

### 8.4 User Acceptance Testing

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create usability test protocol | `docs/uat_protocol.md` | Not Started | - | Test protocol |
| Medium | Simple mode usability tests | `docs/uat_simple_mode.md` | Not Started | - | Simple mode tests |
| Medium | Advanced feature workflow tests | `docs/uat_advanced_mode.md` | Not Started | - | Advanced tests |
| Low | User performance feedback | `docs/uat_perf_feedback.md` | Not Started | - | Performance feedback |
| Low | UI clarity testing | `docs/uat_ui_clarity.md` | Not Started | - | UI clarity tests |

### 8.5 Documentation

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| High | Create user guide (simple mode) | `docs/user_guide_simple.md` | Not Started | - | User guide |
| High | Create advanced feature documentation | `docs/user_guide_advanced.md` | Not Started | - | Advanced guide |
| High | Create API reference | `docs/api_reference.md` | Not Started | - | API docs |
| Medium | Create troubleshooting guide | `docs/troubleshooting.md` | Not Started | - | Troubleshooting |
| Medium | Create video tutorials | `docs/video_tutorials.md` | Not Started | - | Tutorial list |

### 8.6 Release Preparation

| Priority | Task | File | Status | Est. LOC | Notes |
|----------|------|------|--------|---------|-------|
| Medium | Create release notes | `RELEASE_NOTES.md` | Not Started | - | Release notes |
| Medium | Generate marketing materials | `docs/marketing/` | Not Started | - | Marketing |
| Medium | Set up distribution | `build/distribution/` | Not Started | - | Distribution setup |
| Low | Create installer/package | `build/installer/` | Not Started | - | Installer |
| Low | Prepare support infrastructure | `docs/support/` | Not Started | - | Support docs |

---

## Summary Statistics

| Metric | Count |
|--------|-------|
| **Total TODOs** | 450+ |
| **Critical Priority** | 35+ |
| **High Priority** | 180+ |
| **Medium Priority** | 150+ |
| **Low Priority** | 85+ |
| **Estimated Total LOC** | 20,000-25,000 |
| **Average TODO LOC** | 50-100 |

---

## Quick Start: Top 10 Immediate TODOs

1. **Phase 1.1**: Audit engine systems and create dependency matrix
2. **Phase 1.2**: Assess Swift frontend for duplication
3. **Phase 1.3**: Map asset pipeline and identify types for drag-and-drop
4. **Phase 1.4**: Create master integration checklist
5. **Phase 2.1**: Enhance asset registry with types and categories
6. **Phase 2.2**: Extend asset manifest for drag-and-drop metadata
7. **Phase 3.1**: Create abstract drag-and-drop protocol
8. **Phase 3.6**: Add drag-and-drop C bridge functions
9. **Phase 4.1**: Implement asset preview rendering
10. **Phase 5.1**: Create simple environment builder UI

---

## Notes

- All estimates are rough and should be refined during implementation
- Tasks can be parallelized where there are no dependencies
- Phases 5-7 can start after Phase 4 is complete
- Phase 8 is continuous throughout all phases
- This tracker should be updated weekly with progress
- Use GitHub Issues or similar to track each TODO individually
