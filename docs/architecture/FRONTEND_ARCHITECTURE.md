# Frontend Architecture (Swift)

## Overview

The Swift frontend provides the **user interface** and **platform integration** for the C game engine. It consists of two main applications:

1. **iOS App** (`src/frontend/engine/ios_app/`) - Mobile game interface
2. **Studio** (`src/frontend/engine/studio/`) - Desktop game editor

## iOS App (24 Swift Files)

### Purpose
Mobile game interface for iOS devices with touch controls, Metal rendering, and native platform features.

### Key Components

#### Core App Structure
- `AppDelegate.swift` - App lifecycle management
- `SceneDelegate.swift` - Scene lifecycle (iOS 13+)
- `GameStateManager.swift` - Game state coordination
- `LoadingViewController.swift` - Loading screens

#### Engine Integration
- `AssetBrowserBridge.swift` - Bridge to C asset system
- `AssetManager.swift` - Swift-side asset management
- `VoxelEngine.swift` - Voxel rendering interface
- `ControllerManager.swift` - Game controller support

#### Editors
- `AssetEditorViewController.swift` - Asset editing UI
- `BlueprintEditorView.swift` - Blueprint/schematic editor
- `MaterialEditorView.swift` - Material/texture editor
- `TerrainEditorView.swift` - Terrain sculpting tools

#### Engine Wrappers
- `BlueprintEngine.swift` - Blueprint system wrapper
- `MaterialEngine.swift` - Material system wrapper
- `TerrainEngine.swift` - Terrain system wrapper

#### UI Components
- `PropertyInspectorView.swift` - Property editing panel
- `DrawingTools.swift` - Drawing/painting tools
- `DockingManager.swift` - Window docking system
- `ThemeManager.swift` - UI theming

#### Settings
- `SettingsViewController.swift` - Game settings UI

#### Sprite System
- `SpriteFrame.swift` - Sprite frame data
- `SpriteLayer.swift` - Sprite layering

## Studio (63 Swift Files)

### Purpose
Full-featured game editor for macOS with advanced tools for content creation, animation, audio, and scripting.

### Directory Structure

```
studio/
├── App.swift                    # Main app entry point
├── ContentView.swift            # Root view
├── DocumentationView.swift      # Documentation browser
├── ProjectExplorerView.swift    # Project file browser
├── Package.swift                # Swift package manifest
│
├── Editors/
│   ├── 2D/
│   │   └── SpriteEditorView.swift
│   └── 3D/
│       └── ModelEditorView.swift
│
└── Sources/
    ├── Animation/              # Animation tools (4 files)
    ├── Audio/                  # Audio tools (4 files)
    ├── AssetManagement/        # Asset pipeline (1 file)
    ├── Components/             # UI components (many files)
    └── [Various APIs]          # Engine API wrappers
```

### Key Subsystems

#### Animation System (4 files)
- `AnimationTimeline.swift` - Timeline editor
- `DopeSheet.swift` - Keyframe editor (dope sheet)
- `FCurveEditor.swift` - Function curve editor
- `NLAEditor.swift` - Non-linear animation editor

#### Audio System (4 files)
- `AudioAnalyzer.swift` - Audio waveform analysis
- `AudioAnimationDriver.swift` - Audio-driven animation
- `AudioTimingSystem.swift` - Audio timing/sync
- `BackgroundMusicSystem.swift` - Music playback
- `BeatCharacterController.swift` - Rhythm-based controls

#### Asset Management
- `AssetManagementSystem.swift` - Asset pipeline
- `AssetDefinitions.swift` - Asset type definitions

#### Engine API Wrappers
The studio provides Swift wrappers for C engine systems:
- `BiomeAPI.swift` - Biome system
- `ChunkAPI.swift` - Chunk management
- `CameraController.swift` - Camera controls
- `CameraBookmarksAPI.swift` - Camera bookmarks
- `CameraDirectorAPI.swift` - Cinematic camera
- `BoundsVisualizationAPI.swift` - Debug bounds rendering

#### Component System
- `ComponentSystem.swift` - ECS component management
- `ComponentInspectors.swift` - Component property editors
- `StatRow.swift` - Statistics display

#### Command System
- `CommandManager.swift` - Undo/redo system

## Swift ↔ C Bridge

### Bridging Header
All C functions are exposed to Swift via:
```
App/Sources/Minecraft-Bridging-Header.h
```

### Common Bridge Patterns

#### Calling C from Swift
```swift
// Swift code
class VoxelEngine {
    func initialize() {
        // Call C function
        engine_init()
    }
    
    func update(deltaTime: Float) {
        engine_update(deltaTime)
    }
}
```

#### Passing Data to C
```swift
// Swift → C
let position = SIMD3<Float>(x: 1.0, y: 2.0, z: 3.0)
player_set_position(position.x, position.y, position.z)
```

#### Receiving Data from C
```swift
// C → Swift
var health: Float = 0
player_get_health(&health)
print("Health: \(health)")
```

### Memory Management

**Important**: When bridging between Swift and C:
- Swift manages Swift objects
- C manages C structs/pointers
- Use `UnsafePointer` for C pointers in Swift
- Always match `malloc`/`free` pairs in C
- Don't pass Swift class instances to C (use structs)

## UI Architecture

### SwiftUI vs UIKit

**iOS App**: Primarily UIKit (view controllers)
- Better for game UI with custom rendering
- Direct Metal integration
- Fine-grained control over view lifecycle

**Studio**: Primarily SwiftUI
- Modern declarative UI
- Easier to build complex editor layouts
- Better for desktop/macOS

### Metal Integration

The Swift frontend uses Metal for rendering:

```swift
import MetalKit

class GameView: MTKView {
    override func draw(_ rect: CGRect) {
        // Get Metal drawable from C engine
        let drawable = currentDrawable!
        
        // C engine renders to Metal texture
        engine_render_frame(drawable.texture)
        
        // Present
        drawable.present()
    }
}
```

## Platform-Specific Features

### iOS-Specific
- Touch gesture recognition
- Haptic feedback (`UIImpactFeedbackGenerator`)
- Safe area handling (notch support)
- Background/foreground transitions
- Game controller support

### macOS-Specific
- Multi-window support
- Menu bar integration
- Keyboard shortcuts
- Drag-and-drop from Finder
- File system access

## Best Practices

### ✅ Do

1. **Keep UI logic in Swift**
   ```swift
   // Good: UI state in Swift
   @State private var selectedTool: Tool = .brush
   ```

2. **Call C for game logic**
   ```swift
   // Good: Game logic in C
   func placeBrick(at position: SIMD3<Float>) {
       world_place_block(position.x, position.y, position.z, BLOCK_BRICK)
   }
   ```

3. **Use Swift for platform features**
   ```swift
   // Good: Platform-specific in Swift
   func triggerHaptic() {
       let generator = UIImpactFeedbackGenerator(style: .medium)
       generator.impactOccurred()
   }
   ```

### ❌ Don't

1. **Don't duplicate C logic in Swift**
   ```swift
   // Bad: Physics in Swift (should be in C)
   func calculateTrajectory() { ... }
   ```

2. **Don't bypass the bridge**
   ```swift
   // Bad: Direct memory access
   let ptr = UnsafeMutableRawPointer(...)
   ```

3. **Don't mix UI rendering in C**
   ```c
   // Bad: UI in C (should be in Swift)
   void draw_menu_button() { ... }
   ```

## File Organization

### Naming Conventions
- **Views**: `*View.swift` (e.g., `TerrainEditorView.swift`)
- **Controllers**: `*ViewController.swift` (iOS) or `*Controller.swift` (macOS)
- **Managers**: `*Manager.swift` (e.g., `AssetManager.swift`)
- **Engines**: `*Engine.swift` (wrappers for C systems)
- **APIs**: `*API.swift` (C bridge wrappers)

### Directory Organization
```
src/frontend/engine/
├── ios_app/              # iOS-specific code
│   └── Sources/          # All Swift files here
│
└── studio/               # macOS editor
    ├── App.swift         # Root level files
    ├── ContentView.swift
    └── Sources/          # Organized by subsystem
        ├── Animation/
        ├── Audio/
        └── ...
```

## Testing

### Unit Tests
Swift code can be unit tested independently:
```swift
import XCTest

class AssetManagerTests: XCTestCase {
    func testAssetLoading() {
        let manager = AssetManager()
        XCTAssertNotNil(manager.loadAsset("test.png"))
    }
}
```

### Integration Tests
Test Swift ↔ C bridge:
```swift
func testEngineIntegration() {
    engine_init()
    engine_update(0.016) // 60 FPS
    XCTAssertTrue(engine_is_running())
    engine_shutdown()
}
```

## Future Considerations

- **Cross-platform**: Consider adding Android frontend (Kotlin/Java)
- **Web**: WebAssembly build with JavaScript frontend
- **Modding**: Expose more C APIs to Swift for user mods
- **Performance**: Profile Swift ↔ C bridge overhead
