# Developer Quick Start Guide - Minecraft v2 Infrastructure

**For**: Developers working on the engine
**Date**: January 13, 2026
**Version**: 1.0

---

## 🚀 Getting Started in 5 Minutes

### Step 1: Understand the Layer System

The engine is organized in **7 layers**, each building on the previous:

```
🎮 GAME (Layer 7)
    ↓ depends on
📱 GAMEPLAY (Layer 6)
    ↓ depends on
🎨 GRAPHICS + 📊 RENDERING (Layer 5)
    ↓ depends on
🎯 ENGINE CORE (Layer 4)
    ↓ depends on
🧮 DATA STRUCTURES (Layer 3)
    ↓ depends on
⚙️ CORE INFRASTRUCTURE (Layer 2) ← YOU ARE HERE
    ↓ depends on
💻 PLATFORM (Layer 1)
```

**Key Rule**: Lower layers NEVER depend on higher layers.

### Step 2: Include the Right Headers

```c
// If you need math operations:
#include "core/math/unified_types.h"

// If you need memory management:
#include "core/memory/unified_allocator.h"

// If you need logging/debugging:
#include "core/logging/logger.h"

// If you need ECS (entities):
#include "core/ecs/entity.h"

// If you need physics:
#include "engine/physics/physics.h"
```

### Step 3: Use Consistent Types

```c
// ✅ CORRECT - Using unified types
Vec3 position = vec3(1.0f, 2.0f, 3.0f);
Vec3 velocity = vec3(0.5f, 0.0f, -0.5f);
Vec3 new_pos = vec3_add(position, velocity);

// ❌ WRONG - Mixing types
float pos[3] = {1.0f, 2.0f, 3.0f};  // Don't do this
struct { float x, y, z; } vel;       // Don't do this
```

### Step 4: Use the Allocator System

```c
// ✅ CORRECT - Using unified allocator
void* buffer = alloc(4096, ALLOC_GENERAL);
Vec3* vertices = ALLOC_ARRAY(Vec3, 100, ALLOC_PERSIST);
free_alloc(buffer, ALLOC_GENERAL);

// ❌ WRONG - Using malloc/free directly
void* buffer = malloc(4096);  // Don't do this
Vec3* vertices = (Vec3*)malloc(sizeof(Vec3) * 100);  // Don't do this
free(buffer);
```

### Step 5: Use Proper Logging

```c
// ✅ CORRECT - Using unified logger
LOG_INFO("PHYSICS", "Body created: %s", name);
LOG_ERROR("RENDER", "Shader compilation failed");
LOG_DEBUG("ANIM", "Frame %d", frame);

// ❌ WRONG - Using printf
printf("Body created: %s\n", name);  // Don't do this
fprintf(stderr, "Error!\n");          // Don't do this
```

---

## 📚 Common Tasks

### Task: Create a 3D Vertex Array

```c
#include "core/math/unified_types.h"
#include "core/memory/unified_allocator.h"

// Create array
Vec3* vertices = ALLOC_ARRAY(Vec3, 1024, ALLOC_GENERAL);
if (!vertices) {
    LOG_ERROR("RENDER", "Failed to allocate vertex buffer");
    return false;
}

// Fill with data
for (int i = 0; i < 1024; i++) {
    vertices[i] = vec3(
        (float)(i % 32) / 32.0f,
        (float)(i / 32) / 32.0f,
        0.0f
    );
}

// Use the data
render_vertices(vertices, 1024);

// Clean up
free_alloc(vertices, ALLOC_GENERAL);
```

### Task: Implement Physics Body Movement

```c
#include "core/math/unified_types.h"
#include "engine/physics/physics.h"

void update_body(PhysicsBody* body, float dt) {
    // Get current position
    Vec3 pos = physics_body_get_position(body);

    // Calculate new position
    Vec3 vel = physics_body_get_velocity(body);
    Vec3 accel = physics_body_get_acceleration(body);

    // Update velocity
    vel = vec3_add(vel, vec3_mul(accel, dt));

    // Update position
    pos = vec3_add(pos, vec3_mul(vel, dt));

    // Apply new state
    physics_body_set_position(body, pos);
    physics_body_set_velocity(body, vel);

    // Log if debug enabled
    LOG_DEBUG("PHYSICS", "Body: pos=(%f, %f, %f)", pos.x, pos.y, pos.z);
}
```

### Task: Create a Rotated Object

```c
#include "core/math/unified_types.h"
#include "engine/renderer/renderer.h"

void create_rotated_cube(void) {
    // Create rotation around Y axis
    Quat rotation = quat_from_axis_angle(
        vec3_up(),           // Axis: up
        3.14159f / 4.0f      // Angle: 45 degrees
    );

    // Create transformation
    Mat4 transform = mat4_from_quat(rotation);

    // Add translation
    Mat4 translation = mat4_translation(vec3(5.0f, 0.0f, 0.0f));

    // Combine transformations (M = T * R)
    Mat4 final = mat4_mul(translation, transform);

    // Render with transformation
    renderer_set_transform(&final);
    renderer_draw_mesh(&cube_mesh);
}
```

### Task: Log Performance Information

```c
#include "core/logging/logger.h"

void report_frame_stats(float fps, uint32_t draw_calls, uint64_t memory_used) {
    LOG_INFO("FRAME", "FPS: %.1f | DrawCalls: %u | Memory: %lu MB",
             fps, draw_calls, memory_used / (1024 * 1024));

    if (draw_calls > 2000) {
        LOG_WARN("RENDER", "High draw call count: %u", draw_calls);
    }

    if (memory_used > 512 * 1024 * 1024) {
        LOG_ERROR("MEMORY", "High memory usage: %lu MB",
                  memory_used / (1024 * 1024));
    }
}
```

---

## 🔍 Debugging Tips

### Enable Debug Logging

```c
// In your main initialization:
logger_init("engine.log");
logger_set_level(LOG_DEBUG);
logger_set_module_filter("PHYSICS,RENDER,ANIM");
```

### Check Memory Usage

```c
// Anywhere in your code:
AllocationMetrics metrics = allocator_get_metrics();
printf("Total memory: %zu MB\n", metrics.total_engine_memory / (1024*1024));
printf("Active allocations: %u\n", metrics.active_allocations);
```

### Assert Preconditions

```c
void process_entity(Entity* entity) {
    LOG_ASSERT(entity != NULL, "GAME", "Entity cannot be NULL");
    LOG_ASSERT(entity->id != 0, "GAME", "Entity ID must be non-zero");

    // Safe to proceed
    update_entity(entity);
}
```

### Log Groups for Complex Operations

```c
void complex_operation(void) {
    logger_group_begin("Complex Operation");

    LOG_DEBUG("SYSTEM", "Step 1: Initialize");
    init_step();

    LOG_DEBUG("SYSTEM", "Step 2: Process");
    process_step();

    LOG_DEBUG("SYSTEM", "Step 3: Finalize");
    finalize_step();

    logger_group_end();
}
```

---

## 📋 Checklist: Adding a New System

When you add a new subsystem to the engine:

- [ ] **Header Organization**
  ```c
  // Create: src/include/[system].h
  // Contains: Public API only

  // Create: src/engine/[system]/[system].h
  // Contains: Internal types (not exported)

  // Create: src/engine/[system]/[system].c
  // Contains: Implementation
  ```

- [ ] **Proper Dependencies**
  ```c
  // Only include from lower layers:
  #include "core/math/unified_types.h"
  #include "core/memory/unified_allocator.h"
  #include "core/logging/logger.h"

  // DO NOT include from higher layers:
  // #include "game/game.h"  // ❌ WRONG
  ```

- [ ] **Memory Management**
  ```c
  // Use unified allocator:
  Entity* entity = ALLOC_OBJ(Entity, ALLOC_GENERAL);

  // Choose correct type:
  // ALLOC_GENERAL: Most things
  // ALLOC_PERSIST: Game state
  // ALLOC_TEMP: Per-frame temporary
  ```

- [ ] **Logging**
  ```c
  // Log errors:
  LOG_ERROR("SYSTEM", "Failed to initialize");

  // Log important events:
  LOG_INFO("SYSTEM", "System initialized");

  // Log debug info:
  LOG_DEBUG("SYSTEM", "Detailed info: %d", value);
  ```

- [ ] **Testing**
  ```c
  // Create: tests/unit/[system]_test.c
  // Test all public APIs
  // Test error conditions
  // Test performance
  ```

---

## 🎯 Design Patterns

### Pattern 1: Deferred Initialization

```c
// In .h file:
typedef struct {
    bool initialized;
    uint32_t data;
    void* internal;
} System;

// In .c file:
bool system_init(void) {
    if (g_system.initialized) {
        LOG_WARN("SYSTEM", "Already initialized");
        return false;
    }

    g_system.internal = alloc(sizeof(SystemImpl), ALLOC_PERSIST);
    g_system.initialized = true;

    LOG_INFO("SYSTEM", "Initialized");
    return true;
}
```

### Pattern 2: Thread-Safe Queuing

```c
// Use ALLOC_TEMP for per-frame allocations:
void frame_update(void) {
    // Work items created during frame
    WorkItem* items = ALLOC_ARRAY(WorkItem, 100, ALLOC_TEMP);

    // Process items
    for (int i = 0; i < 100; i++) {
        process_work_item(&items[i]);
    }

    // Automatically freed when frame ends:
    allocator_reset(ALLOC_TEMP);
}
```

### Pattern 3: Module Filtering

```c
// In main.c:
logger_init("engine.log");

// Only log specific systems during debugging:
if (DEBUG_MODE) {
    logger_set_module_filter("PHYSICS,AI");  // Only these
}
```

---

## 🚨 Common Mistakes

| ❌ Wrong | ✅ Correct | Why |
|---------|-----------|-----|
| `float v[3]` | `Vec3 v` | Unified types |
| `malloc()` | `alloc()` | Memory tracking |
| `printf()` | `LOG_INFO()` | Filtering, routing |
| `v[0]`, `v[1]` | `v.x`, `v.y` | Consistency |
| Circle dependency | Linear dependency | Maintainability |
| Direct math | Inline functions | Performance |
| No logging | Comprehensive logs | Debuggability |
| Global state | Parameter passing | Testability |

---

## 📞 Getting Help

### Where to Find Things

| Need | Look In |
|------|----------|
| Math types | `src/core/math/unified_types.h` |
| Memory allocation | `src/core/memory/unified_allocator.h` |
| Logging | `src/core/logging/logger.h` |
| ECS system | `src/core/ecs/` |
| Physics | `src/engine/physics/` |
| Rendering | `src/engine/renderer/` |
| Architecture | `docs/ARCHITECTURE_AND_INFRASTRUCTURE.md` |
| Build status | `INFRASTRUCTURE_BUILD_PROGRESS.md` |

### Documentation

- **Architecture Guide**: `ARCHITECTURE_AND_INFRASTRUCTURE.md`
- **Implementation Status**: `INFRASTRUCTURE_BUILD_PROGRESS.md`
- **Missing Features**: `CRITICAL_MISSING_FEATURES.md`
- **Integration Report**: `INTEGRATION_REPORT.md`

---

## 🎓 Learning Path

1. **Day 1**: Read this quick start guide
2. **Day 2**: Study `unified_types.h` and do math exercises
3. **Day 3**: Study `unified_allocator.h` and trace memory usage
4. **Day 4**: Study `logger.h` and add logging to a system
5. **Day 5**: Create a simple system using all three

---

## ✨ Tips for Success

1. **Consistent Naming**: Use full module names in logs
   ```c
   LOG_INFO("PHYSICS_BODY", "Created");  // ✅
   LOG_INFO("PB", "Created");             // ❌
   ```

2. **Meaningful Log Levels**: Use correct levels
   ```c
   LOG_DEBUG("PHYSICS", "Value: %f");      // ✅ Detail
   LOG_INFO("PHYSICS", "Body created");    // ✅ Important
   LOG_WARN("PHYSICS", "High force: %f");  // ✅ Concern
   LOG_ERROR("PHYSICS", "Invalid state");  // ✅ Problem
   ```

3. **Memory Budgets**: Use appropriate allocators
   ```c
   void* permanent = alloc(1000, ALLOC_PERSIST);  // Game state
   void* frame_temp = alloc(500, ALLOC_TEMP);     // Temporary
   ```

4. **Defensive Coding**: Always check allocations
   ```c
   void* ptr = alloc(size, type);
   if (!ptr) {
       LOG_ERROR("SYSTEM", "Allocation failed");
       return false;
   }
   ```

---

**Ready to start coding?** Pick a system from `CRITICAL_MISSING_FEATURES.md` and get started!
