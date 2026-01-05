# Minecraft V2 → Drag-and-Drop 3D Asset Editor Roadmap

## Executive Summary

Transform the existing game engine into a unified drag-and-drop 3D asset creation and environment building system. This roadmap preserves all existing engine functionality while adding an intuitive interface for both casual and advanced users.

**Key Principles:**
- ✅ Zero functionality loss from existing engine
- ✅ Backwards compatible with all current systems
- ✅ Unified interface (iOS + macOS)
- ✅ Drag-and-drop interaction model
- ✅ Scalable from simple to advanced features
- ✅ Performance-optimized for real-time editing

---

## Phase 1: Audit and Integration Baseline

### Objective
Create a comprehensive map of all existing systems and identify integration gaps/dependencies.

### Tasks

#### 1.1 Engine System Audit
- [ ] **TODO**: Document all 80+ engine subsystems with:
  - Input/output interfaces
  - Dependencies on other systems
  - Exposed C APIs
  - Configuration requirements
- [ ] **TODO**: Create dependency graph for:
  - Rendering pipeline dependencies
  - Physics initialization requirements
  - Audio system prerequisites
  - Animation state machine initialization
- [ ] **TODO**: Verify all editor integrations still work:
  - Material editor
  - Animation timeline
  - Terrain editor
  - Physics debug visualization
- **Status Tracker**: `docs/integration_audit.md`

#### 1.2 Swift Frontend Assessment
- [ ] **TODO**: Audit all 181 Swift files:
  - Identify duplicate functionality between iOS and macOS
  - Map all C bridge calls
  - Document data flow between Swift and C
- [ ] **TODO**: Create feature matrix:
  - Feature name | iOS | macOS | Status
  - (Example: 3D Viewport | ❌ | ✅ | Needs iOS impl)
- [ ] **TODO**: Identify missing drag-and-drop infrastructure:
  - UIDragInteraction / NSDraggingDestination implementation
  - Drop target zones
  - Drag preview generation
- **Status Tracker**: `docs/frontend_audit.md`

#### 1.3 Asset Pipeline Review
- [ ] **TODO**: Map current asset loading:
  - Asset manifest format
  - Serialization pipeline
  - Caching mechanisms
- [ ] **TODO**: Identify asset types that need drag-and-drop:
  - 3D Models (meshes, LODs)
  - Textures and materials
  - Animation clips
  - Particle effects
  - Audio files
  - Prefabs and compositions
- [ ] **TODO**: Document asset hot-reload capabilities
- **Status Tracker**: `docs/asset_pipeline_audit.md`

#### 1.4 Integration Points Documentation
- [ ] **TODO**: Create detailed integration checklist with all systems:
  - Rendering → Asset drag-and-drop
  - Physics → Asset interaction
  - Animation → Timeline + viewport
  - Audio → Spatial positioning
  - Scripting → Event system
  - Networking → Multi-user support (future)
- [ ] **TODO**: Document all TODO locations already in codebase
- **Status Tracker**: `docs/integration_checklist.md`

---

## Phase 2: Unified Asset Pipeline

### Objective
Create a single, robust asset management system that serves both the engine and the drag-and-drop interface.

### Tasks

#### 2.1 Asset Registry Enhancement
- [ ] **TODO** in `src/engine/resource/asset_manager.c`:
  - Add asset type enumeration for all drag-and-drop types
  - Implement asset categorization (3D Model, Texture, Animation, etc.)
  - Add asset tagging system for filtering
  - Create asset search/discovery API
  - Implement asset versioning for backwards compatibility

#### 2.2 Drag-and-Drop Asset Metadata
- [ ] **TODO** in `src/engine/core/serialization/asset_manifest.c`:
  - Extend asset manifest to include:
    - Drag-and-drop preview (thumbnail UUID)
    - Supported operations (copy, reference, instance)
    - Physics properties (if applicable)
    - Animation state defaults
    - Layer/group assignments
  - Add quick-load metadata for UI responsiveness
  - Implement async metadata loading

#### 2.3 Asset Instancing System
- [ ] **TODO** in `src/engine/ecs/entity_component_system.c`:
  - Create "AssetInstance" component
  - Implement asset reference vs. copy mechanics
  - Add instance override system
  - Support multi-level prefab nesting
  - Implement instance-to-asset synchronization

#### 2.4 Asset Streaming and Caching
- [ ] **TODO** in `src/engine/streaming/chunk_streaming.c`:
  - Optimize for rapid asset loading (< 100ms for typical assets)
  - Implement smart caching based on viewport
  - Add memory pressure notifications
  - Create asset preloading hints
  - Implement LRU eviction for asset cache

#### 2.5 Asset Validation Pipeline
- [ ] **TODO** in `src/engine/tools/asset_validator.c`:
  - Create validators for each asset type
  - Implement dependency checking (broken references detection)
  - Add asset health reporting
  - Create migration tools for legacy assets
  - Implement automatic asset repair

**Status Tracker**: `docs/asset_pipeline_implementation.md`

---

## Phase 3: Swift Frontend Unification

### Objective
Create a single, unified drag-and-drop interface that works seamlessly on iOS and macOS.

### Tasks

#### 3.1 Unified Drag-and-Drop System
- [ ] **TODO** in `src/frontend/engine/studio/Sources/Systems/DragDropSystem.swift`:
  - Create abstract DragDropTarget protocol
  - Implement platform-specific adapters:
    - iOS: UIDragInteraction + UIDropInteraction
    - macOS: NSDraggingDestination protocol
  - Create uniform drag event handling
  - Implement preview rendering (platform-agnostic)
  - Add drag state machine (started, over, can-drop, dropped)

#### 3.2 Unified Asset Browser
- [ ] **TODO** in `src/frontend/engine/studio/Sources/Components/AssetBrowser.swift`:
  - Create single asset browser view
  - Implement platform-specific layout:
    - iOS: Grid with drag handles
    - macOS: Split view (categories | grid/list)
  - Add search/filter functionality
  - Implement multi-asset selection
  - Create asset preview panel
  - Add tagging and categorization UI

#### 3.3 Unified Viewport
- [ ] **TODO** in `src/frontend/engine/studio/Sources/SceneView/UnifiedViewport.swift`:
  - Create shared rendering backend (Metal)
  - Platform-specific gesture handling:
    - iOS: Pan, pinch, rotation gestures
    - macOS: Mouse + trackpad events
  - Implement drop zone visualization
  - Add snap/alignment grid display
  - Create placement feedback (preview ghosts)
  - Implement undo/redo for viewport actions

#### 3.4 Property Inspector Unification
- [ ] **TODO** in `src/frontend/engine/studio/Sources/Components/PropertyInspector.swift`:
  - Create single property editor view
  - Platform-specific layouts:
    - iOS: Sliding panel or modal
    - macOS: Side panel
  - Implement all property types (vec3, quat, material, etc.)
  - Add live preview for property changes
  - Create collapsible sections
  - Implement drag-to-modify for numeric values (iOS + macOS)

#### 3.5 UI Layer Architecture
- [ ] **TODO** in `src/frontend/engine/studio/Sources/UI/UIArchitecture.swift`:
  - Create abstract UI component protocol
  - Implement platform adapters for:
    - Buttons, toggles, sliders
    - Dialogs and popovers
    - Context menus
    - Notifications
  - Ensure consistent styling across platforms
  - Implement dark/light theme support

#### 3.6 C/Swift Bridge Enhancement
- [ ] **TODO** in `include/swift_bridge_wrapper.h`:
  - Add drag-and-drop specific C functions:
    - `asset_get_drag_preview(asset_handle)`
    - `viewport_raycast_for_drop(ray)`
    - `asset_create_instance_at_position(asset, position, rotation)`
    - `get_drop_zone_list()`
  - Implement batch asset operations
  - Add progress callbacks for async operations
  - Create thread-safe callbacks for Swift events

**Status Tracker**: `docs/frontend_unification.md`

---

## Phase 4: 3D Asset Rendering & Interaction Layer

### Objective
Enable real-time manipulation of 3D assets with visual feedback and physics preview.

### Tasks

#### 4.1 Asset Preview Rendering
- [ ] **TODO** in `src/engine/renderer/asset_preview_renderer.c`:
  - Create off-screen rendering for asset previews
  - Implement quick thumbnail generation
  - Add custom lighting for previews
  - Cache preview images for UI performance
  - Support animated previews for skeletal meshes

#### 4.2 Transform Gizmo System
- [ ] **TODO** in `src/engine/editor/transform_gizmo.c`:
  - Extend existing gizmo system for drag-and-drop:
    - Translation gizmo
    - Rotation gizmo (all axes, plane, free rotate)
    - Scale gizmo (uniform, per-axis)
  - Implement touch-friendly gizmo sizing
  - Add coordinate system toggle (world, local, parent)
  - Create smart axis highlighting on hover
  - Implement snapping with visual feedback:
    - Grid snapping
    - Angle snapping
    - Vertex snapping

#### 4.3 Physics Asset Preview
- [ ] **TODO** in `src/engine/physics/physics_preview.c`:
  - Create preview physics bodies for assets
  - Implement soft-body visualization
  - Add collision wireframe display
  - Show physics properties inline:
    - Mass, friction, restitution
    - Collision groups/masks
  - Implement physics material swatches
  - Add quick physics property assignment UI

#### 4.4 Placement Feedback System
- [ ] **TODO** in `src/engine/editor/placement_feedback.c`:
  - Implement ghost preview (translucent asset at drop location)
  - Add placement validity indicator:
    - Green: Valid placement
    - Yellow: Warning (overlapping, unusual)
    - Red: Invalid (colliding, out of bounds)
  - Create snap target visualization
  - Implement alignment grid
  - Add distance/measurement display

#### 4.5 Selection and Highlight System
- [ ] **TODO** in `src/engine/renderer/selection_highlight.c`:
  - Implement outline rendering for selected assets
  - Add selection glow effect
  - Create hierarchy breadcrumb visualization
  - Implement multi-selection highlighting
  - Add selection cycling (overlap resolution)

#### 4.6 Interactive Viewport Features
- [ ] **TODO** in `src/engine/editor/viewport_editor.c`:
  - Create orthographic mode for precise placement
  - Add reference images/grids
  - Implement camera bookmarks
  - Create focus-on-selection with framing
  - Add viewport split view (top/front/side + perspective)
  - Implement fog of war for visibility testing

**Status Tracker**: `docs/rendering_interaction_layer.md`

---

## Phase 5: Environment Builder

### Objective
Create an intuitive environment creation system for building game worlds and scenes.

### Tasks

#### 5.1 Simple Environment Builder UI
- [ ] **TODO** in `src/frontend/engine/studio/Sources/Gameplay/EnvironmentBuilder.swift`:
  - Create main environment builder view
  - Implement two modes:
    - **Simple Mode** (default):
      - Palette of pre-made assets
      - Drag-and-drop onto grid
      - Quick property adjustment
      - Save as preset
    - **Advanced Mode**:
      - Full system access
      - Component editing
      - Script assignment
      - Physics tuning
  - Add mode toggle
  - Implement scene templates

#### 5.2 Asset Palette System
- [ ] **TODO** in `src/engine/editor/asset_palette.c`:
  - Create categorized asset palettes:
    - Terrain (blocks, natural elements)
    - Structures (buildings, props)
    - Vegetation (trees, plants)
    - Interactive (doors, switches)
    - Effects (particles, lights)
  - Implement custom palette creation
  - Add palette search and filtering
  - Support palette import/export
  - Create asset collection grouping

#### 5.3 Placement Grid and Alignment
- [ ] **TODO** in `src/engine/editor/placement_grid.c`:
  - Implement configurable grid:
    - Grid size adjustment (0.1 - 10.0 units)
    - 2D grid for simple mode
    - 3D grid for advanced mode
  - Add snap-to-grid modes:
    - None, Coarse, Fine
  - Implement face-to-face snapping for modular assets
  - Create custom pivot point handling
  - Add alignment tool (align multiple assets)

#### 5.4 Quick Property Adjustment
- [ ] **TODO** in `src/frontend/engine/studio/Sources/Components/QuickPropertyPanel.swift`:
  - Create simple property panel for most common edits:
    - Position (3 fields or drag slider)
    - Rotation (Euler angles with visual feedback)
    - Scale (vector with lock uniform button)
    - Material (quick color picker)
    - Physics material (dropdown preset)
  - Implement keyboard shortcuts:
    - G = grab/move
    - R = rotate
    - S = scale
  - Add property value reset button
  - Create property animation (interpolation between values)

#### 5.5 Scene Organization
- [ ] **TODO** in `src/frontend/engine/studio/Sources/SceneGraph/SceneOrganizer.swift`:
  - Create scene hierarchy view:
    - Expandable groups
    - Drag-to-reorder
    - Visibility toggle (eye icon)
    - Lock/unlock toggle
    - Delete button
  - Implement smart grouping:
    - Group by type
    - Group by spatial proximity
    - Custom groups
  - Add layer system:
    - Show/hide by layer
    - Lock/unlock by layer
    - Assign/remove from layer
  - Create outline search

#### 5.6 Scene Templates and Presets
- [ ] **TODO** in `src/engine/editor/scene_templates.c`:
  - Create starter templates:
    - Empty scene
    - Default terrain
    - Urban environment
    - Natural landscape
    - Interior space
  - Implement preset system:
    - Save current scene as preset
    - Load preset into current scene
    - Merge preset with existing scene
  - Add template customization options
  - Create template preview/thumbnail

#### 5.7 Asset Instance Management
- [ ] **TODO** in `src/engine/ecs/asset_instance_manager.c`:
  - Create instance tracking:
    - All instances of an asset
    - Instance counts and statistics
    - Instance variation tracking
  - Implement batch operations:
    - Replace all instances of asset A with asset B
    - Scale all instances by factor
    - Rotate all instances
    - Change material for all instances
  - Add instance prefab override system
  - Create instance sync to prefab

**Status Tracker**: `docs/environment_builder_implementation.md`

---

## Phase 6: Animation Integration

### Objective
Seamlessly integrate animation system with asset placement and real-time playback.

### Tasks

#### 6.1 Animation Property Assignment
- [ ] **TODO** in `src/frontend/engine/studio/Sources/Animation/AssetAnimationSetup.swift`:
  - Create animation assignment UI:
    - Browse available animations
    - Assign to asset
    - Set default animation
    - Create animation overrides
  - Implement animation preview in viewport
  - Add animation speed/playback controls
  - Create animation loop toggle

#### 6.2 Timeline Integration
- [ ] **TODO** in `src/engine/animation/animation_timeline.c`:
  - Extend timeline system for:
    - Asset placement keyframes
    - Animation keyframe insertion
    - Material animation
    - Camera animation
    - Light animation
  - Implement scrubbing and playback
  - Add keyframe selection and manipulation
  - Create curve editor for interpolation
  - Support multiple animation tracks

#### 6.3 Skeletal Animation in Viewport
- [ ] **TODO** in `src/engine/renderer/skeletal_render.c`:
  - Implement real-time skeletal animation rendering
  - Add bone visualization:
    - Skeleton wireframe (toggle)
    - Joint position indicators
    - Bone weight heat maps
  - Create inverse kinematics preview
  - Implement animation layer preview (blending visualization)

#### 6.4 Particle Animation Preview
- [ ] **TODO** in `src/engine/vfx/particle_preview.c`:
  - Create particle effect preview in viewport
  - Implement particle spawning at asset location
  - Add emission rate adjustment
  - Create particle lifetime visualization
  - Implement particle property tweaking UI

#### 6.5 Audio and Animation Synchronization
- [ ] **TODO** in `src/engine/audio/audio_animation_sync.c`:
  - Create audio trigger system for animations
  - Implement beat detection and sync
  - Add sound effect placement in animation timeline
  - Create voice sync for skeletal animation
  - Implement audio analysis visualization

#### 6.6 Animation Blending and Transitions
- [ ] **TODO** in `src/engine/animation/blend_tree_editor.c`:
  - Extend blend tree editor for:
    - Visual blend state machine
    - Drag-and-drop state creation
    - Transition rule editing
    - Parameter value adjustment
  - Implement state previewing
  - Create automatic transition detection
  - Add transition preview in viewport

#### 6.7 Cinematics and Camera Animation
- [ ] **TODO** in `src/engine/animation/cinematic_controller.c`:
  - Create camera animation keyframing
  - Implement camera path creation:
    - Spline-based paths
    - Camera focus points
    - Look-at targets
  - Add camera animation preview
  - Create dolly and pan animation templates
  - Implement letterbox and aspect ratio control

**Status Tracker**: `docs/animation_integration.md`

---

## Phase 7: Advanced Features

### Objective
Layer in sophisticated features for power users while maintaining simplicity for casual users.

### Tasks

#### 7.1 Procedural Generation Integration
- [ ] **TODO** in `src/engine/procedural_generation/procedural_tools.c`:
  - Create procedural asset generator UI
  - Implement templates:
    - Terrain generation
    - Building generation
    - Tree generation
    - Rock/cave formation
  - Add noise parameter exposure:
    - Perlin noise
    - Worley noise
    - Fractal Brownian motion
  - Create generation preview
  - Implement seed-based reproducibility

#### 7.2 Physics-Based Placement
- [ ] **TODO** in `src/engine/editor/physics_placement.c`:
  - Create gravity simulation for asset placement
  - Implement settling animation for dropped assets
  - Add physics constraints:
    - Stand on surface
    - Align with surface normal
    - Avoid overlap
  - Create collision group assignment
  - Implement rigidbody property presets

#### 7.3 Advanced Snapping and Alignment
- [ ] **TODO** in `src/engine/editor/advanced_snapping.c`:
  - Implement vertex snapping (model-to-model)
  - Add edge-to-edge alignment
  - Create surface normal alignment
  - Implement multi-asset alignment tools:
    - Distribute spacing
    - Circular/grid arrangement
    - Array duplication
  - Add magnetic snapping with distance preview

#### 7.4 Scripting and Interactivity
- [ ] **TODO** in `src/engine/scripting/visual_script_editor.c`:
  - Create visual scripting interface:
    - Node graph UI
    - Event nodes (on-drop, on-click, on-trigger)
    - Action nodes (animate, play-sound, move)
    - Logic nodes (if, loop, switch)
  - Implement asset event binding:
    - Drop triggers script
    - Hover triggers script
    - Click triggers script
  - Add Lua script inline editing
  - Create script debugging and logging

#### 7.5 Lighting and Environment Control
- [ ] **TODO** in `src/engine/editor/lighting_editor.c`:
  - Extend existing lighting system for:
    - Quick light placement (sun, point, spot)
    - Light intensity adjustment
    - Color picker
    - Shadow quality selection
  - Implement environment lighting:
    - Skybox selection
    - Ambient lighting
    - Global illumination settings
  - Add time-of-day simulation
  - Create lighting preset templates

#### 7.6 Material and Shader Editor Enhancement
- [ ] **TODO** in `src/engine/renderer/material_editor_advanced.c`:
  - Extend material editor:
    - Real-time shader compilation
    - Shader graph visual editing
    - Material parameter exposure
    - Texture assignment
    - Normal map and PBR properties
  - Create material preview viewport
  - Implement material variant system
  - Add material animation curves

#### 7.7 Performance Optimization Tools
- [ ] **TODO** in `src/engine/tools/performance_optimizer.c`:
  - Create optimization suggestions:
    - LOD assignment recommendations
    - Batching opportunities
    - Overdraw detection
    - Memory usage analysis
  - Implement batch mesh generation
  - Add level-of-detail automation
  - Create draw call reduction recommendations
  - Implement performance profiler integration

#### 7.8 Asset Validation and Health Check
- [ ] **TODO** in `src/engine/tools/asset_health_check.c`:
  - Create health check system:
    - Missing material detection
    - Broken texture references
    - Invalid physics shapes
    - Scale inconsistencies
    - Layer assignment validation
  - Implement auto-repair for common issues
  - Add health report generation
  - Create migration tools for asset updates

#### 7.9 Version Control and Asset Management
- [ ] **TODO** in `src/engine/tools/asset_version_control.c`:
  - Create asset versioning:
    - Save history per asset
    - Revert to previous version
    - Version comparison
    - Asset diff visualization
  - Implement check-in/check-out system
  - Add conflict resolution
  - Create asset merge tools

#### 7.10 Networking and Collaboration (Future)
- [ ] **TODO** in `src/engine/networking/collaborative_editing.c`:
  - Design collaborative editing system:
    - Real-time asset placement sync
    - User presence visualization
    - Change broadcast
    - Conflict resolution
  - Implement session management
  - Create permission system
  - Add activity history

**Status Tracker**: `docs/advanced_features.md`

---

## Phase 8: Testing & Optimization

### Objective
Ensure all systems work together seamlessly and perform optimally.

### Tasks

#### 8.1 Integration Testing
- [ ] **TODO**: Create integration test suite:
  - Asset pipeline end-to-end tests
  - Drag-and-drop workflow tests
  - Animation playback tests
  - Physics interaction tests
  - Multi-asset operation tests
  - UI state consistency tests

#### 8.2 Performance Optimization
- [ ] **TODO**: Profile and optimize:
  - Asset loading performance (target: < 100ms)
  - Viewport rendering (target: 60 FPS)
  - UI responsiveness (target: < 16ms frame time)
  - Memory usage (target: < 1GB for typical scene)
  - Asset preview generation (target: < 500ms batch)

#### 8.3 Platform-Specific Testing
- [ ] **TODO**: Test on all platforms:
  - iOS (all supported devices)
  - macOS (Intel and Apple Silicon)
  - Edge cases (low memory, slow network)
  - Orientation changes and multitasking

#### 8.4 User Acceptance Testing
- [ ] **TODO**: Create user testing protocol:
  - Simple mode usability tests
  - Advanced feature workflow tests
  - Performance feedback
  - UI clarity and discoverability

#### 8.5 Documentation
- [ ] **TODO**: Create comprehensive docs:
  - User guide for simple mode
  - Advanced feature documentation
  - API reference for extensions
  - Troubleshooting guide
  - Video tutorials

#### 8.6 Release Preparation
- [ ] **TODO**: Prepare for release:
  - Create release notes
  - Generate marketing materials
  - Set up distribution
  - Create installer/package
  - Prepare support infrastructure

**Status Tracker**: `docs/testing_optimization.md`

---

## Integration Checklist by System

This section maps each engine system to the drag-and-drop architecture:

### Rendering System
- [x] Metal backend support (already implemented)
- [ ] TODO: Asset preview rendering
- [ ] TODO: Transform gizmo rendering
- [ ] TODO: Selection highlight rendering
- [ ] TODO: Placement feedback rendering
- [ ] TODO: UI layer rendering

### Physics System
- [x] Core rigid body dynamics (already implemented)
- [ ] TODO: Physics preview visualization
- [ ] TODO: Placement physics simulation
- [ ] TODO: Asset collision preview
- [ ] TODO: Physics material presets

### Animation System
- [x] Skeletal animation (already implemented)
- [ ] TODO: Animation assignment UI
- [ ] TODO: Timeline integration
- [ ] TODO: Animation preview in viewport
- [ ] TODO: Particle animation preview

### Audio System
- [x] Spatial audio (already implemented)
- [ ] TODO: Audio-animation sync UI
- [ ] TODO: Audio trigger in timeline
- [ ] TODO: Spatial audio preview in viewport

### ECS System
- [x] Component system (already implemented)
- [ ] TODO: Asset instance component
- [ ] TODO: Drag-and-drop instance creation
- [ ] TODO: Instance override system

### AI System
- [x] NPC behavior (already implemented)
- [ ] TODO: NPC asset placement
- [ ] TODO: AI behavior editing UI
- [ ] TODO: NPC preview and testing

### Scripting System
- [x] Lua integration (already implemented)
- [ ] TODO: Visual script editor
- [ ] TODO: Event binding UI
- [ ] TODO: Script debugging tools

### Networking System
- [x] Multiplayer framework (already implemented)
- [ ] TODO: Collaborative editing
- [ ] TODO: Real-time sync for drag-and-drop
- [ ] TODO: User presence visualization

### UI System
- [x] In-game UI (already implemented)
- [ ] TODO: Editor UI unification
- [ ] TODO: Drag-and-drop UI components
- [ ] TODO: Platform-specific adapters

### Asset System
- [x] Asset manifest (already implemented)
- [ ] TODO: Asset registry enhancement
- [ ] TODO: Drag-and-drop metadata
- [ ] TODO: Asset instancing system
- [ ] TODO: Streaming and caching

---

## Critical Path and Dependencies

### Must Complete First (Critical Path)
1. Phase 1: Audit (no dependencies)
2. Phase 2: Asset Pipeline (depends on Phase 1)
3. Phase 3: Swift Frontend (depends on Phases 1-2)
4. Phase 4: Rendering Layer (depends on Phases 2-3)

### Can Proceed in Parallel
- Phase 5 & 6 can start after Phase 4
- Phase 7 can start after Phase 5
- Phase 8 is continuous throughout

### Optional/Future
- Phase 7.10: Collaboration (requires Phase 8 foundation)

---

## File Organization for Implementation

### New Directories to Create
```
docs/
├── integration_audit.md                 (Phase 1.1)
├── frontend_audit.md                    (Phase 1.2)
├── asset_pipeline_audit.md              (Phase 1.3)
├── integration_checklist.md             (Phase 1.4)
├── asset_pipeline_implementation.md     (Phase 2)
├── frontend_unification.md              (Phase 3)
├── rendering_interaction_layer.md       (Phase 4)
├── environment_builder_implementation.md (Phase 5)
├── animation_integration.md             (Phase 6)
├── advanced_features.md                 (Phase 7)
└── testing_optimization.md              (Phase 8)

src/engine/editor/
├── drag_drop_system.c/h                 (NEW)
├── placement_feedback.c/h               (NEW)
├── placement_grid.c/h                   (NEW)
└── [extend existing systems]

src/frontend/engine/studio/Sources/
├── DragDrop/                            (NEW)
│   ├── DragDropSystem.swift
│   └── DragDropTarget.swift
├── EnvironmentBuilder/                  (NEW)
│   ├── EnvironmentBuilder.swift
│   └── SimpleMode.swift
└── [extend existing systems]
```

---

## Success Metrics

- [ ] All 40,000+ lines of existing code functional
- [ ] Asset loading < 100ms average
- [ ] Viewport runs at 60 FPS
- [ ] Simple mode usable in < 5 minutes
- [ ] Advanced mode covers all engine features
- [ ] Drag-and-drop works on iOS and macOS
- [ ] Zero data loss in workflows
- [ ] Full undo/redo support
- [ ] 90%+ user satisfaction in testing

---

## Notes

This roadmap is designed to be incremental and non-destructive. Each phase builds on previous work while maintaining full backward compatibility. The simplicity of the simple mode can always be enhanced without affecting the advanced mode or the underlying engine.

**Key Implementation Philosophy:**
- Start simple, add complexity incrementally
- Always test that existing features still work
- Use feature flags for incomplete features
- Keep tight feedback loops during development
- Validate assumptions early with user testing
