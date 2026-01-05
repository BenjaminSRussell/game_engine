# Minecraft v2 - Spatial Audio Configuration

Xcode project for the professional spatial audio configuration UI.

## Features

- **Dolby Atmos** support with object-based rendering
- **DTS:X** support with Neural:X upmixing
- **Custom Surround** with user-configurable speaker layouts
- Real-time speaker visualization
- Professional audio meters with peak hold
- Format-specific control panels

## Building

1. Open this folder in Xcode
2. Select "AudioConfigApp" scheme
3. Build and run (⌘R)

## Integration

The SwiftUI app communicates with the C/C++ audio engine via:
- C API bridge: `src/engine/audio/spatial/spatial_audio_bridge.h`
- Swift bridge: `SpatialAudioEngine.swift`

## Files

**SwiftUI Views:**
- `SpatialAudioView.swift` - Main interface
- `SpeakerVisualizerView.swift` - 3D speaker layout visualization
- `AudioMetersGridView.swift` - Professional VU meters
- `FormatControlsViews.swift` - Format-specific controls
- `SpatialAudioEngine.swift` - Swift-C++ bridge

**C/C++ Backend:**
- `spatial_audio_config.h/c` - Configuration system
- `dolby_atmos_renderer.h/c` - Atmos rendering engine
- `spatial_audio_bridge.h/c` - C API for Swift

**Swift files** are located in `editor/AudioConfig/`.
**C/C++ files** are located in `src/engine/audio/spatial/`.
