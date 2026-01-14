# Editor Subsystem Architecture

> **Priority 12** | Est. 4,000+ files | 400K+ LOC | ⭐⭐⭐ Complexity

Swift-based editor with C engine integration via bridge layer.

---

## Directory Structure

```
Editor/
├── Core/                       # Editor framework
│   ├── Application/            # EditorApp main
│   ├── Bridge/                 # Swift-C engine bridge
│   │   ├── EngineBridge/
│   │   ├── SceneBridge/
│   │   └── AssetBridge/
│   ├── Commands/               # Undo/redo command system
│   ├── Selection/              # Selection management
│   └── Settings/               # Editor preferences
│
├── Viewport/                   # 3D viewport
│   ├── Rendering/              # Engine viewport rendering
│   ├── Camera/                 # Editor camera controls
│   ├── Gizmos/                 # Transform gizmos
│   ├── Selection/              # Click-to-select raycast
│   ├── Grid/                   # Ground grid
│   └── Overlays/               # Debug visualizations
│
├── AssetBrowser/               # Content browser
│   ├── Thumbnails/             # Asset preview generation
│   ├── Importing/              # Asset import pipeline
│   │   ├── Mesh/
│   │   ├── Texture/
│   │   ├── Audio/
│   │   └── Animation/
│   ├── Metadata/               # Asset metadata
│   └── Search/                 # Asset search/filter
│
├── Inspector/                  # Property editor
│   ├── PropertyGrid/           # Property display
│   ├── Editors/                # Type-specific editors
│   │   ├── Vector/
│   │   ├── Color/
│   │   ├── Curve/
│   │   └── Reference/
│   └── Reflection/             # Type reflection system
│
├── SceneHierarchy/             # Scene tree
│   ├── Tree/                   # Tree view
│   ├── DragDrop/               # Reparenting
│   └── Context/                # Right-click menu
│
├── Profiler/                   # Performance tools
│   ├── FrameTime/              # Per-system timing
│   ├── Memory/                 # Memory usage
│   ├── GPU/                    # GPU profiling
│   └── Timeline/               # Frame timeline
│
├── Tools/                      # Specialized editors
│   ├── MaterialEditor/
│   │   ├── Graph/
│   │   ├── Preview/
│   │   └── Nodes/
│   ├── AnimationEditor/
│   │   ├── Timeline/
│   │   ├── Curves/
│   │   └── Preview/
│   ├── TerrainEditor/
│   ├── ParticleEditor/
│   └── DialogEditor/
│
├── PlayMode/                   # In-editor play
│   ├── Simulation/             # PIE (Play-in-Editor)
│   ├── Pause/
│   └── Step/
│
└── SwiftUI/                    # Swift UI components
    ├── Panels/
    ├── Windows/
    ├── Dialogs/
    └── DesignSystem/
```

---

## Design Notes (from design doc)

> "The Swift editor allows constructing a world much faster than writing code. 
> A tester can import a character model, place some lights, and press Play 
> to spawn in as the player and roam the world."

> "The editor's profiler could measure frame times of each system – 
> in Unreal, profiling and debugging is crucial."

---

## Engine Integration

```
┌─────────────────────────────────────────────┐
│              Swift Editor (SwiftUI)          │
├─────────────────────────────────────────────┤
│              Engine Bridge (C API)           │
├─────────────────────────────────────────────┤
│              C Engine Runtime                │
└─────────────────────────────────────────────┘
```

The bridge exposes:
- Scene manipulation (add/remove entities)
- Asset operations (import, preview)
- Playback controls (play, pause, step)
- Performance metrics (FPS, memory, GPU)
