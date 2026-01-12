# Terrain Runtime API Documentation

## Overview

The Terrain Runtime API provides comprehensive runtime modification capabilities for terrain systems in the game engine. It allows for real-time terrain manipulation including height modification, texture painting, vegetation management, and erosion simulation.

## Features

### Core Functionality
- **Height Modification**: Modify terrain height at specific positions or regions
- **Texture Painting**: Apply textures to terrain surfaces
- **Vegetation Management**: Add and remove vegetation instances
- **Erosion Simulation**: Apply hydraulic and thermal erosion effects
- **Batch Operations**: Execute multiple modifications efficiently
- **History Management**: Undo/redo functionality for terrain modifications
- **Query Operations**: Query terrain properties (height, texture, normal, slope)

### Performance Features
- **Asynchronous Operations**: All modifications are executed asynchronously
- **Memory Management**: Efficient memory usage with garbage collection
- **Statistics Tracking**: Comprehensive performance and usage statistics
- **Error Handling**: Robust error handling and recovery

## Getting Started

### Initialization

```swift
import Engine

// Create terrain data
let terrainData = TerrainData(size: SIMD2<Int>(1024, 1024))

// Initialize the API
let terrainAPI = TerrainRuntimeAPI.shared
try terrainAPI.initialize(with: terrainData)
```

### Basic Height Modification

```swift
// Modify height at a specific position
let position = SIMD2<Float>(100, 100)
let result = try await terrainAPI.modifyHeight(at: position, delta: 5.0)

if result.success {
    print("Height modified successfully")
}
```

### Texture Painting

```swift
// Paint texture at a specific position
let position = SIMD2<Float>(50, 50)
let textureIndex = 2 // Grass texture
let result = try await terrainAPI.paintTexture(at: position, textureIndex: textureIndex, radius: 10.0)
```

### Vegetation Management

```swift
// Add vegetation
let position = SIMD2<Float>(75, 75)
let result = try await terrainAPI.addVegetation(at: position, type: .oak, density: 0.8)

// Remove vegetation
let result = try await terrainAPI.removeVegetation(at: position, radius: 15.0)
```

## API Reference

### Height Operations

#### `modifyHeight(at:delta:radius:operation:)`
Modifies terrain height at a specific position.

**Parameters:**
- `position: SIMD2<Float>` - World position to modify
- `delta: Float` - Height change amount
- `radius: Float` - Radius of effect (default: 1.0)
- `operation: HeightOperation` - Type of operation (default: .add)

**Returns:** `TerrainModificationResult`

#### `setHeight(at:height:radius:)`
Sets terrain height to a specific value.

**Parameters:**
- `position: SIMD2<Float>` - World position
- `height: Float` - Target height value
- `radius: Float` - Radius of effect (default: 1.0)

**Returns:** `TerrainModificationResult`

#### `modifyHeightRegion(rect:delta:operation:)`
Modifies terrain height in a rectangular region.

**Parameters:**
- `rect: CGRect` - Rectangle defining the region
- `delta: Float` - Height change amount
- `operation: HeightOperation` - Type of operation (default: .add)

**Returns:** `TerrainModificationResult`

### Texture Operations

#### `paintTexture(at:textureIndex:radius:strength:)`
Paints texture at a specific position.

**Parameters:**
- `position: SIMD2<Float>` - World position
- `textureIndex: Int` - Index of texture to apply
- `radius: Float` - Radius of effect (default: 1.0)
- `strength: Float` - Painting strength (0.0-1.0, default: 1.0)

**Returns:** `TerrainModificationResult`

#### `paintTextureRegion(rect:textureIndex:strength:)`
Paints texture in a rectangular region.

**Parameters:**
- `rect: CGRect` - Rectangle defining the region
- `textureIndex: Int` - Index of texture to apply
- `strength: Float` - Painting strength (0.0-1.0, default: 1.0)

**Returns:** `TerrainModificationResult`

### Vegetation Operations

#### `addVegetation(at:type:density:)`
Adds vegetation at a specific position.

**Parameters:**
- `position: SIMD2<Float>` - World position
- `type: VegetationType` - Type of vegetation
- `density: Float` - Density of vegetation (default: 1.0)

**Returns:** `TerrainModificationResult`

#### `removeVegetation(at:radius:)`
Removes vegetation in a circular region.

**Parameters:**
- `position: SIMD2<Float>` - Center position
- `radius: Float` - Radius of removal

**Returns:** `TerrainModificationResult`

### Erosion Operations

#### `applyHydraulicErosion(at:intensity:iterations:)`
Applies hydraulic erosion to terrain.

**Parameters:**
- `position: SIMD2<Float>` - Center position
- `intensity: Float` - Erosion intensity (0.0-1.0)
- `iterations: Int` - Number of iterations (default: 10)

**Returns:** `TerrainModificationResult`

#### `applyThermalErosion(at:intensity:iterations:)`
Applies thermal erosion to terrain.

**Parameters:**
- `position: SIMD2<Float>` - Center position
- `intensity: Float` - Erosion intensity (0.0-1.0)
- `iterations: Int` - Number of iterations (default: 10)

**Returns:** `TerrainModificationResult`

### Query Operations

#### `getHeight(at:)`
Gets terrain height at a specific position.

**Parameters:**
- `position: SIMD2<Float>` - World position

**Returns:** `Float?` - Height value or nil if out of bounds

#### `getTexture(at:)`
Gets texture index at a specific position.

**Parameters:**
- `position: SIMD2<Float>` - World position

**Returns:** `Int?` - Texture index or nil if out of bounds

#### `getNormal(at:)`
Gets terrain normal at a specific position.

**Parameters:**
- `position: SIMD2<Float>` - World position

**Returns:** `SIMD3<Float>?` - Normalized normal vector or nil if out of bounds

#### `getSlope(at:)`
Gets terrain slope at a specific position.

**Parameters:**
- `position: SIMD2<Float>` - World position

**Returns:** `Float?` - Slope angle in radians or nil if out of bounds

### Batch Operations

#### `executeBatch(_:)`
Executes multiple modifications as a single batch.

**Parameters:**
- `modifications: [TerrainModification]` - Array of modifications

**Returns:** `[TerrainModificationResult]` - Array of results

#### `applyHeightmap(_:offset:scale:)`
Applies a heightmap to terrain.

**Parameters:**
- `heightmap: [[Float]]` - 2D array of height values
- `offset: SIMD2<Float>` - Offset position (default: .zero)
- `scale: Float` - Height scale factor (default: 1.0)

**Returns:** `TerrainModificationResult`

### History Management

#### `undo()`
Undoes the last modification.

**Returns:** `TerrainModificationResult?` - Result of undo operation or nil if no history

#### `redo()`
Redoes the last undone modification.

**Returns:** `TerrainModificationResult?` - Result of redo operation or nil if no redo history

#### `clearHistory()`
Clears the modification history.

## Data Structures

### TerrainModification
Represents a terrain modification operation.

```swift
struct TerrainModification {
    let id: UUID
    let type: ModificationType
    let position: SIMD2<Float>
    let parameters: [String: Any]
    let timestamp: Date
}
```

### TerrainModificationResult
Represents the result of a terrain modification.

```swift
struct TerrainModificationResult {
    let modification: TerrainModification
    let success: Bool
    let executionTime: Double
    let affectedArea: CGRect
    let message: String
}
```

### TerrainData
Represents terrain data structure.

```swift
struct TerrainData {
    let size: SIMD2<Int>
    var heightmap: [[Float]]
    var textureMap: [[Int]]
    var vegetationMap: [[VegetationInstance]]
}
```

## Enums

### HeightOperation
Types of height modification operations.

```swift
enum HeightOperation: String, CaseIterable {
    case add = "add"
    case subtract = "subtract"
    case multiply = "multiply"
    case set = "set"
    case min = "min"
    case max = "max"
}
```

### VegetationType
Types of vegetation.

```swift
enum VegetationType: String, CaseIterable, Codable {
    case oak = "oak"
    case pine = "pine"
    case birch = "birch"
    case bush = "bush"
    case grass = "grass"
    case flower = "flower"
}
```

### TerrainAPIError
Error types for terrain API operations.

```swift
enum TerrainAPIError: LocalizedError {
    case notInitialized
    case alreadyInitialized
    case invalidPosition
    case invalidParameters
    case operationFailed(String)
}
```

## Performance Considerations

### Memory Usage
- The API maintains modification history which can consume memory
- Consider calling `clearHistory()` periodically for long-running applications
- Batch operations are more memory-efficient than individual modifications

### Execution Time
- All operations are asynchronous and non-blocking
- Large modifications may take longer to execute
- Use batch operations for better performance with multiple modifications

### Thread Safety
- The API is designed to be thread-safe
- Multiple concurrent modifications are supported
- Statistics and history are updated atomically

## Integration Examples

### Swift Integration
```swift
class TerrainController {
    private let terrainAPI = TerrainRuntimeAPI.shared
    
    func setupTerrain() async {
        let terrainData = TerrainData(size: SIMD2<Int>(2048, 2048))
        try! terrainAPI.initialize(with: terrainData)
    }
    
    func createMountain(at position: SIMD2<Float>) async {
        // Create a mountain by raising terrain
        _ = try! await terrainAPI.modifyHeight(at: position, delta: 50.0, radius: 20.0)
        
        // Add some erosion for realism
        _ = try! await terrainAPI.applyThermalErosion(at: position, intensity: 0.3, iterations: 50)
        
        // Add vegetation
        _ = try! await terrainAPI.addVegetation(at: position, type: .pine, density: 0.7)
    }
}
```

### Objective-C Integration
```objective-c
#import "TerrainRuntimeAPIBridge.h"

@interface TerrainController : NSObject
@end

@implementation TerrainController

- (void)setupTerrain {
    BOOL success = [TerrainRuntimeAPIBridge.shared initializeTerrainWithWidth:1024 height:1024];
    if (success) {
        NSLog(@"Terrain initialized successfully");
    }
}

- (void)createMountainAtX:(float)x y:(float)y {
    [TerrainRuntimeAPIBridge.shared modifyHeightWithX:x y:y delta:50.0 radius:20.0];
}

@end
```

## Testing

The API includes comprehensive unit tests covering:
- Initialization and shutdown
- All modification operations
- Query operations
- Batch operations
- History management
- Error handling
- Performance testing

Run tests with:
```bash
swift test --filter TerrainRuntimeAPITests
```

## Best Practices

1. **Always check initialization status** before performing operations
2. **Use batch operations** for multiple modifications
3. **Handle errors gracefully** and provide user feedback
4. **Monitor performance statistics** for optimization
5. **Clear history periodically** for long-running applications
6. **Use appropriate radius values** to avoid performance issues
7. **Validate parameters** before calling API methods

## Troubleshooting

### Common Issues

**API not initialized error**
- Ensure `initialize(with:)` is called before any operations
- Check if `shutdown()` was called inadvertently

**Performance issues**
- Reduce modification radius for large operations
- Use batch operations instead of individual calls
- Monitor memory usage and clear history if needed

**Invalid position errors**
- Ensure positions are within terrain bounds
- Check coordinate system (world vs. heightmap coordinates)

**Memory leaks**
- Call `shutdown()` when done with the API
- Clear modification history periodically
- Monitor memory usage statistics

## Future Enhancements

Planned features for future versions:
- Multi-threaded terrain generation
- GPU-accelerated modifications
- Real-time collaboration support
- Advanced erosion algorithms
- Terrain LOD integration
- Streaming terrain support
- Custom modification plugins
