# VoxelForge Swift Editor - macOS

Premium macOS editor for the VoxelForge engine, built with Swift 6 and SwiftUI.

## Features

- **Metal Viewport** - Hardware-accelerated voxel rendering
- **Liquid Glass UI** - 2026 design language (translucent materials, ambient glows)
- **Unified Memory** - Zero-copy buffer sharing on Apple Silicon
- **Swift 6 Concurrency** - Data race safety with Actors
- **Direct C++ Interop** - No Objective-C bridging required

## Requirements

- **macOS**: 14.0+ (Sonoma or later)
- **Swift**: 6.0+
- **Xcode**: 15.0+ (for Command Line Tools)
- **Hardware**: Apple Silicon recommended (M1/M2/M3)

## Quick Start

### Build
```bash
./build_swift_editor.sh
```

### Run
```bash
cd src/editor
swift run VoxelForgeEditor
```

### Test
```bash
cd src/editor
swift test
```

## Architecture

```
src/editor/
├── Core/
│   ├── Bridges/          # C <-> Swift interop
│   ├── EntityBridge.swift   # ECS type wrappers
│   ├── MetalViewport.swift  # SwiftUI + MTKView
│   ├── EditorApp.swift      # Main app entry
│   └── Tests/            # XCTest suites
├── UI/
│   └── Components/
│       ├── GlassPlate.swift    # Liquid Glass component
│       ├── VitalityMesh.swift  # Animated background
│       └── StatusGlow.swift    # Context-aware lighting
├── Platform/
│   ├── MetalRenderer.swift     # Metal 4 integration
│   └── Tests/               # Performance benchmarks
└── Package.swift
```

## Performance

| Metric | Target | Achieved (M1 Pro) |
|--------|--------|-------------------|
| Unified Memory Write (1M voxels) | <10ms | **~6ms** |
| Buffer Alignment | 256 bytes | ✅ |
| Frame Time (4K) | <16ms | ✅ (sub-16ms) |

## Testing

**Unit Tests** (24 test cases):
- C function call validation
- Type conversion round-trips
- Collection protocol conformance
- Actor thread safety
- Zero-copy buffer performance

```bash
swift test --filter SwiftCInteropTests        # Interop tests
swift test --filter UnifiedMemoryPerformanceTests  # Performance benchmarks
```

## Development

### Add New UI Component

1. Create Swift file in `UI/Components/`
2. Import required frameworks
3. Use `@available(macOS 14.0, *)` for modern APIs
4. Add to `Package.swift` sources (auto-discovered)

### Liquid Glass Pattern

```swift
GlassPlate(material: .thin) {
    VStack {
        Text("Your Content")
    }
}
```

### Access C Engine

```swift
actor WorldManager {
    let worldPtr: UnsafeMutablePointer<World>
    
    func getEntities() -> [SwiftEntity] {
        Array(EntityCollection(world: worldPtr))
    }
}
```

## Troubleshooting

**Build fails with "GameEngine not found"**
- Build C engine first: `cd build && cmake --build . --target EngineLib`

**Linker error: undefined symbols**
- Check `Package.swift` linkerSettings point to `../../build/`
- Verify `libEngineLib.a` exists in build directory

**Runtime error: Metal device not found**
- Requires Metal-capable GPU (all Macs 2012+)
- Check: `system_profiler SPDisplaysDataType | grep Metal`

**Swift 6 errors about @MainActor**
- Update Xcode to 15.0+
- Ensure `swift-tools-version:6.0` in Package.swift

## Documentation

- [Swift/C++ Interop Guide](../../docs/SwiftCppInterop.md) - 10 comprehensive patterns
- [Implementation Plan](../../.gemini/antigravity/brain/.../implementation_plan.md)
- [Walkthrough](../../.gemini/antigravity/brain/.../walkthrough.md)

## License

See main repository LICENSE
