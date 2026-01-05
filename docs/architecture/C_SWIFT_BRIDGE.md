# C ↔ Swift Bridge

## Overview

The C/Swift bridge enables the Swift frontend to interact with the C game engine. This document explains the integration layer and best practices.

## Bridge Components

### 1. Bridging Header
**Location**: `App/Sources/Minecraft-Bridging-Header.h`

This header exposes C functions and types to Swift:

```c
// Minecraft-Bridging-Header.h
#ifndef MINECRAFT_BRIDGING_HEADER_H
#define MINECRAFT_BRIDGING_HEADER_H

// Engine Core
#include "engine/core/engine.h"
#include "engine/core/logger.h"

// Renderer
#include "engine/renderer/core/renderer.h"
#include "engine/renderer/core/camera.h"

// Physics
#include "engine/physics/physics_core.h"

// Game
#include "game/minecraftv2/game.h"
#include "game/minecraftv2/player/player.h"
#include "game/minecraftv2/world/world.h"

#endif
```

### 2. Swift Bridge Layer
**Location**: `src/engine/platform/swift_bridge.c`

Provides C functions specifically designed for Swift interop:

```c
// swift_bridge.c
#include "swift_bridge.h"
#include "engine/core/engine.h"

// Initialize engine from Swift
void swift_engine_init(int width, int height) {
    EngineConfig config = {
        .window_width = width,
        .window_height = height,
        .enable_vsync = true
    };
    engine_init(&config);
}

// Update engine (called from Swift render loop)
void swift_engine_update(float delta_time) {
    engine_update(delta_time);
}

// Inject touch input from Swift
void swift_input_touch(int touch_id, float x, float y, int phase) {
    InputEvent event = {
        .type = INPUT_TOUCH,
        .touch = {
            .id = touch_id,
            .x = x,
            .y = y,
            .phase = phase
        }
    };
    input_inject_event(&event);
}
```

## Data Type Mapping

### Primitives
| C Type | Swift Type |
|--------|------------|
| `int` | `Int32` |
| `float` | `Float` |
| `double` | `Double` |
| `bool` | `Bool` |
| `char*` | `UnsafePointer<CChar>` |

### Structs
C structs are directly accessible in Swift:

```c
// C
typedef struct {
    float x, y, z;
} Vec3;
```

```swift
// Swift
var position = Vec3(x: 1.0, y: 2.0, z: 3.0)
```

### Enums
C enums become Swift enums:

```c
// C
typedef enum {
    BLOCK_AIR = 0,
    BLOCK_STONE = 1,
    BLOCK_DIRT = 2
} BlockType;
```

```swift
// Swift
let blockType: BlockType = BLOCK_STONE
```

## Common Patterns

### Pattern 1: Calling C Functions

```swift
// Swift
class GameEngine {
    func initialize() {
        // Call C function directly
        swift_engine_init(1920, 1080)
    }
    
    func update(deltaTime: Float) {
        swift_engine_update(deltaTime)
    }
}
```

### Pattern 2: Passing Strings

```swift
// Swift → C
let filename = "world.dat"
filename.withCString { cString in
    world_load(cString)
}

// C → Swift
let cString = world_get_name()
let swiftString = String(cString: cString!)
```

### Pattern 3: Working with Pointers

```swift
// Get pointer from C
var health: Float = 0
player_get_health(&health)
print("Health: \(health)")

// Pass array to C
var vertices: [Float] = [1.0, 2.0, 3.0]
vertices.withUnsafeBufferPointer { buffer in
    mesh_set_vertices(buffer.baseAddress, Int32(buffer.count))
}
```

### Pattern 4: Callbacks

```c
// C - Define callback type
typedef void (*EventCallback)(int event_type, void* data);

void engine_set_callback(EventCallback callback) {
    g_callback = callback;
}
```

```swift
// Swift - Create callback
let callback: @convention(c) (Int32, UnsafeMutableRawPointer?) -> Void = { eventType, data in
    print("Event: \(eventType)")
}

engine_set_callback(callback)
```

### Pattern 5: Opaque Types

For complex C types, use opaque pointers:

```c
// C
typedef struct World World; // Opaque

World* world_create(void);
void world_destroy(World* world);
void world_update(World* world, float dt);
```

```swift
// Swift wrapper
class WorldManager {
    private var world: OpaquePointer?
    
    init() {
        world = world_create()
    }
    
    deinit {
        if let world = world {
            world_destroy(world)
        }
    }
    
    func update(deltaTime: Float) {
        world_update(world, deltaTime)
    }
}
```

## Memory Management

### Rules

1. **C owns C memory**: Always `free()` what you `malloc()` in C
2. **Swift owns Swift objects**: ARC manages Swift class instances
3. **Bridge carefully**: Don't pass Swift class instances to C
4. **Match pairs**: Every `create()` needs a `destroy()`

### Example: Safe Memory Management

```c
// C - Allocate and return pointer
char* world_get_name(void) {
    char* name = malloc(256);
    strcpy(name, "MyWorld");
    return name;
}

void world_free_name(char* name) {
    free(name);
}
```

```swift
// Swift - Use and free
let namePtr = world_get_name()
let name = String(cString: namePtr!)
world_free_name(namePtr)
```

### Better: Use Stack Allocation

```c
// C - Fill caller's buffer
void world_get_name(char* buffer, int buffer_size) {
    strncpy(buffer, "MyWorld", buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}
```

```swift
// Swift - No manual memory management
var buffer = [CChar](repeating: 0, count: 256)
buffer.withUnsafeMutableBufferPointer { ptr in
    world_get_name(ptr.baseAddress, 256)
}
let name = String(cString: buffer)
```

## Performance Considerations

### Minimize Bridge Crossings

❌ **Bad**: Call C function per vertex
```swift
for vertex in vertices {
    mesh_add_vertex(vertex.x, vertex.y, vertex.z)
}
```

✅ **Good**: Batch call
```swift
vertices.withUnsafeBufferPointer { buffer in
    mesh_set_vertices(buffer.baseAddress, Int32(buffer.count))
}
```

### Cache C Data in Swift

❌ **Bad**: Query C every frame
```swift
func update() {
    let health = player_get_health() // C call every frame
    healthLabel.text = "\(health)"
}
```

✅ **Good**: Cache and update on change
```swift
private var cachedHealth: Float = 100.0

func onHealthChanged(newHealth: Float) {
    cachedHealth = newHealth
    healthLabel.text = "\(newHealth)"
}

func update() {
    // Use cached value, no C call
    healthLabel.text = "\(cachedHealth)"
}
```

## Thread Safety

### Main Thread Rule
**Always call C engine functions from the main thread** unless explicitly documented as thread-safe.

```swift
// ✅ Good: Call from main thread
DispatchQueue.main.async {
    engine_update(deltaTime)
}

// ❌ Bad: Call from background thread
DispatchQueue.global().async {
    engine_update(deltaTime) // CRASH!
}
```

### Thread-Safe C Functions
Some C functions may be thread-safe (document them):

```c
// Thread-safe: Can be called from any thread
void logger_log(const char* message) {
    pthread_mutex_lock(&log_mutex);
    // ... write to log
    pthread_mutex_unlock(&log_mutex);
}
```

## Error Handling

### C Error Codes

```c
// C
typedef enum {
    ERROR_NONE = 0,
    ERROR_FILE_NOT_FOUND = 1,
    ERROR_OUT_OF_MEMORY = 2
} ErrorCode;

ErrorCode world_load(const char* filename);
```

```swift
// Swift wrapper
enum WorldError: Error {
    case fileNotFound
    case outOfMemory
    case unknown(Int32)
}

func loadWorld(filename: String) throws {
    let result = filename.withCString { world_load($0) }
    
    switch result {
    case ERROR_NONE:
        return
    case ERROR_FILE_NOT_FOUND:
        throw WorldError.fileNotFound
    case ERROR_OUT_OF_MEMORY:
        throw WorldError.outOfMemory
    default:
        throw WorldError.unknown(result)
    }
}
```

## Debugging

### Print from C to Swift Console

```c
// C
#include <stdio.h>

void debug_print(const char* message) {
    printf("[C] %s\n", message);
    fflush(stdout); // Important: flush to see in Xcode console
}
```

### Breakpoints
- Set breakpoints in both C and Swift code
- Xcode debugger works seamlessly across the bridge
- Use `po` command for Swift objects, `p` for C variables

### Common Issues

**Issue**: Crash when calling C function
- **Check**: Is the function name correct?
- **Check**: Are you passing the right types?
- **Check**: Is the C library linked?

**Issue**: String garbled
- **Check**: Is the C string null-terminated?
- **Check**: Are you using the right encoding?

**Issue**: Memory leak
- **Check**: Are you freeing C allocations?
- **Check**: Are you releasing opaque pointers?

## Best Practices

### ✅ Do

1. **Wrap C APIs in Swift classes**
   ```swift
   class PhysicsWorld {
       private var handle: OpaquePointer?
       // Clean Swift API
   }
   ```

2. **Use Swift types in Swift code**
   ```swift
   struct Position {
       var x, y, z: Float
       
       var cVec3: Vec3 {
           Vec3(x: x, y: y, z: z)
       }
   }
   ```

3. **Document thread safety**
   ```swift
   /// Thread-safe: Can be called from any thread
   func log(_ message: String) { ... }
   ```

### ❌ Don't

1. **Don't pass Swift classes to C**
   ```swift
   // Bad
   class Player { ... }
   engine_set_player(player) // Will crash!
   ```

2. **Don't store Swift closures in C**
   ```swift
   // Bad
   let closure = { print("Hi") }
   engine_set_callback(closure) // Will crash!
   ```

3. **Don't ignore memory management**
   ```swift
   // Bad
   let ptr = world_create()
   // Forgot to call world_destroy(ptr) - leak!
   ```

## Example: Complete Integration

```swift
// Swift wrapper for C engine
class GameEngine {
    private var initialized = false
    
    func initialize(width: Int, height: Int) throws {
        guard !initialized else { return }
        
        let result = swift_engine_init(Int32(width), Int32(height))
        guard result == 0 else {
            throw EngineError.initFailed
        }
        
        initialized = true
    }
    
    func update(deltaTime: Float) {
        guard initialized else { return }
        swift_engine_update(deltaTime)
    }
    
    func shutdown() {
        guard initialized else { return }
        swift_engine_shutdown()
        initialized = false
    }
    
    deinit {
        shutdown()
    }
}
```

## Resources

- [Swift Interoperability](https://developer.apple.com/documentation/swift/imported_c_and_objective-c_apis)
- [Using Swift with C](https://www.swift.org/documentation/cxx-interop/)
- [Memory Management in Swift](https://docs.swift.org/swift-book/LanguageGuide/AutomaticReferenceCounting.html)
