# Minecraft v2 Engine - Architecture & Infrastructure Guide

**Date**: January 13, 2026
**Status**: Infrastructure mapping and foundation building
**Purpose**: Create cohesive, navigable codebase with clear dependency hierarchy

---

## I. System Architecture Overview

### Critical Dependency Chain (Foundation First)

```
LAYER 0: Platform Abstractions
├── sys (system calls, OS-level abstractions)
├── core/platform (platform-specific code)
└── core/simd (SIMD operations)

LAYER 1: Core Data Structures & Utilities
├── include (public API definitions)
├── core/containers (arrays, maps, queues, etc.)
├── core/string (string utilities)
├── core/memory (memory management)
└── core/serialization (data persistence)

LAYER 2: Math & Physics Foundations
├── math (vector, matrix, quaternion operations)
├── physics (collision, dynamics, constraints)
└── core/algorithms (pathfinding, optimization)

LAYER 3: Core Engine Systems
├── core (main engine core)
├── ecs (entity-component system)
├── core/scene_graph (hierarchical transforms)
└── core/profiling (performance monitoring)

LAYER 4: Rendering & Graphics
├── rendering (GPU abstractions)
├── engine/renderer (rendering pipeline)
├── engine/assets (asset management)
└── engine/editor/ui (UI system)

LAYER 5: Gameplay Systems
├── engine/animation (skeletal animation)
├── engine/audio (spatial audio)
├── engine/ai (NPC behavior)
├── engine/physics (advanced physics)
└── engine/gameplay (game mechanics)

LAYER 6: Game Content & Frontend
├── game (blockgame implementation)
├── frontend (UI/input layer)
├── networking (multiplayer)
└── scripting (game scripting)
```

---

## II. Current Subsystem Status

### Fully Implemented (Ready to Use)
- **core/platform**: Platform abstractions ✅
- **core/simd**: SIMD utilities ✅
- **core/memory**: Memory management (partial) ✅
- **core/containers**: Data structures ✅
- **core/string**: String utilities ✅
- **math**: Vector/matrix math (partial) ✅
- **physics**: Physics core (partial) ✅
- **ecs**: Entity-component system ✅
- **core/scene_graph**: Transform hierarchy ✅

### Partially Implemented (Need Completion)
- **core/serialization**: Data I/O (70%)
- **core/algorithms**: Pathfinding (60%)
- **rendering**: GPU abstractions (75%)
- **engine/renderer**: Rendering pipeline (65%)
- **engine/animation**: Skeletal animation (70%)
- **engine/audio**: Audio system (50%)
- **engine/ai**: AI systems (60%)
- **engine/gameplay**: Game mechanics (55%)
- **game**: Blockgame content (50%)
- **networking**: Multiplayer (40%)

### Missing/Stub Implementation (Critical Gaps)
- **core/unified_types**: Type system consolidation (0%)
- **core/memory/allocator**: Unified allocator (0%)
- **rendering/shaders**: Compute shaders (TAA, SSAO, SSR) (0%)
- **physics/ccd**: Continuous collision detection (0%)
- **physics/deterministic**: Network synchronization (0%)
- **core/profiling**: GPU profiling (0%)
- **testing**: Integration tests (20%)

---

## III. Essential Base Dependencies

### Dependency Resolution Order

#### Phase 1: Absolute Foundations (Must implement first)
1. **sys/** - OS abstractions (DONE)
2. **core/simd/** - SIMD operations (DONE)
3. **core/platform/** - Platform code (DONE)
4. **core/containers/** - Data structures (DONE)
5. **include/** - Public API (DONE)

#### Phase 2: Core Infrastructure
1. **core/memory/** - Memory management (70% DONE)
   - Allocators (pool, linear, general)
   - Leak detection
   - Statistics

2. **core/string/** - String utilities (DONE)
   - String pool
   - String operations
   - Formatting

3. **math/** - Math library (70% DONE)
   - **Unified type system** (NEEDED)
   - Vector operations (v2, v3, v4)
   - Quaternions
   - Matrices (3x3, 4x4)

#### Phase 3: Advanced Core
1. **core/serialization/** - Data I/O (70% DONE)
   - Binary serialization
   - JSON serialization
   - Schema validation

2. **core/algorithms/** - Algorithm library (60% DONE)
   - Pathfinding (A*, JPS)
   - Graph algorithms
   - Spatial queries

3. **core/profiling/** - Performance monitoring (NEEDED)
   - CPU profiling
   - Memory profiling
   - GPU profiling

#### Phase 4: Engine Core
1. **ecs/** - Entity system (DONE)
   - Entity creation/destruction
   - Component attachment
   - System scheduling

2. **core/scene_graph/** - Transform hierarchy (DONE)
   - Spatial hierarchy
   - Transform calculations
   - Dirty tracking

3. **core/** - Main engine (75% DONE)
   - Engine initialization
   - Frame loop
   - Subsystem management

#### Phase 5: Graphics Pipeline
1. **rendering/** - GPU abstractions (75% DONE)
   - Device management
   - Buffer allocation
   - Synchronization

2. **engine/renderer/** - Rendering pipeline (65% DONE)
   - Renderpass management
   - Shader compilation
   - Draw command recording

3. **engine/assets/** - Asset system (DONE)
   - Asset loading
   - Resource management
   - Caching

#### Phase 6: Gameplay Systems
1. **physics/** - Physics engine (70% DONE)
   - Rigid body simulation
   - Collision detection
   - **CCD** (NEEDED)
   - **Deterministic replay** (NEEDED)

2. **engine/animation/** - Animation system (70% DONE)
   - Skeletal animation
   - **GPU skinning** (NEEDED)
   - Animation blending

3. **engine/audio/** - Audio system (50% DONE)
   - Audio playback
   - Spatial audio
   - Mixing

4. **engine/ai/** - AI systems (60% DONE)
   - Behavior trees
   - Decision making
   - **Group behaviors** (NEEDED)

---

## IV. Dependency Graph

### Core Dependencies

```
include (PUBLIC API)
    ↓
sys (PLATFORM) ← core/platform
    ↓
core/simd (SIMD) ← core/containers
    ↓
core/containers (DATA STRUCTURES)
    ├─→ core/string (STRINGS)
    ├─→ core/memory (MEMORY)
    └─→ core/serialization (I/O)

math (MATH LIBRARY)
    ├─→ core/simd
    └─→ core/containers

physics (PHYSICS)
    ├─→ math
    ├─→ core/containers
    ├─→ core/algorithms
    └─→ core/memory

ecs (ENTITY SYSTEM)
    ├─→ core/containers
    ├─→ core/memory
    └─→ core/serialization

core/scene_graph (TRANSFORMS)
    ├─→ math
    ├─→ ecs
    └─→ core/containers

core (ENGINE CORE)
    ├─→ ecs
    ├─→ core/scene_graph
    ├─→ physics
    ├─→ math
    ├─→ core/memory
    ├─→ core/serialization
    └─→ core/profiling

rendering (GPU ABSTRACTIONS)
    ├─→ core
    ├─→ math
    └─→ core/containers

engine/renderer (RENDERING PIPELINE)
    ├─→ rendering
    ├─→ core
    ├─→ math
    └─→ engine/assets

engine/animation (ANIMATION)
    ├─→ core
    ├─→ math
    ├─→ ecs
    └─→ core/profiling

engine/audio (AUDIO)
    ├─→ core
    ├─→ math
    └─→ core/memory

engine/ai (AI)
    ├─→ core
    ├─→ ecs
    ├─→ physics
    ├─→ core/algorithms
    └─→ core/profiling

engine/gameplay (GAMEPLAY)
    ├─→ core
    ├─→ ecs
    ├─→ physics
    ├─→ engine/animation
    ├─→ engine/audio
    └─→ engine/ai

game (BLOCKGAME)
    ├─→ engine/gameplay
    ├─→ engine/renderer
    ├─→ ecs
    └─→ core

networking (MULTIPLAYER)
    ├─→ core
    ├─→ ecs
    ├─→ core/serialization
    └─→ physics (for deterministic sync)

frontend (UI/INPUT)
    ├─→ core
    ├─→ game
    ├─→ networking
    └─→ engine/renderer
```

---

## V. Implementation Priority (Building Missing Infrastructure)

### CRITICAL PATH (Week 1-2)

#### 1. **Unified Type System** (200 LOC)
**File**: `src/core/math/types.h`
**Purpose**: Single source of truth for all vector/matrix types
**Depends on**: Nothing
**Impacts**: All math, physics, rendering
**Structure**:
```c
// Aligned types for SIMD
typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Vec4;
typedef struct { float x, y, z, w; } Quat;
typedef float Mat3[9];  // Column-major
typedef float Mat4[16]; // Column-major

// Conversion functions
Vec2 vec3_to_vec2(Vec3 v);
Vec4 vec3_to_vec4(Vec3 v, float w);
// ... etc
```

#### 2. **Memory Allocator Consolidation** (300 LOC)
**File**: `src/core/memory/unified_allocator.c`
**Purpose**: Single allocator interface for all allocation types
**Depends on**: sys/, core/containers/
**Impacts**: Memory management everywhere
**Structure**:
```c
typedef enum {
    ALLOC_GENERAL,   // General purpose
    ALLOC_POOL,      // Fixed-size pools
    ALLOC_LINEAR,    // Linear arena
    ALLOC_TEMP,      // Temporary/frame allocator
} AllocationType;

typedef struct {
    void* (*allocate)(size_t size, AllocationType type);
    void (*free)(void* ptr, AllocationType type);
    void (*reset)(AllocationType type);
    AllocatorStats (*get_stats)(void);
} Allocator;

extern Allocator g_allocator;
```

#### 3. **Core Profiling System** (300 LOC)
**File**: `src/core/profiling/profiler.h` + `.c`
**Purpose**: CPU and memory profiling hooks
**Depends on**: sys/, core/memory/
**Impacts**: Performance monitoring everywhere
**Structure**:
```c
typedef struct {
    const char* name;
    uint64_t start_time;
    uint64_t elapsed_us;
    uint32_t call_count;
} ProfileScope;

void profiler_start_scope(const char* name);
void profiler_end_scope(void);
void profiler_dump_stats(void);
```

#### 4. **Logging Infrastructure** (200 LOC)
**File**: `src/core/logging/logger.h` + `.c`
**Purpose**: Unified logging across all systems
**Depends on**: sys/, core/string/
**Impacts**: Debugging everywhere
**Structure**:
```c
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,
} LogLevel;

void logger_init(void);
void logger_log(LogLevel level, const char* module, const char* fmt, ...);
void logger_dump_to_file(const char* filepath);

#define LOG_DEBUG(module, fmt, ...) logger_log(LOG_DEBUG, module, fmt, __VA_ARGS__)
#define LOG_ERROR(module, fmt, ...) logger_log(LOG_ERROR, module, fmt, __VA_ARGS__)
```

---

### HIGH PRIORITY (Week 3-4)

#### 5. **Physics CCD System** (400 LOC)
**File**: `src/engine/physics/continuous_collision.c`
**Purpose**: Swept collision detection for fast objects
**Depends on**: math/, physics/ core, core/algorithms/
**Impacts**: Fast-moving projectiles, impacts
**Key Features**:
- Swept sphere tests
- Swept box tests
- Time-of-impact calculation
- Constraint generation

#### 6. **Physics Deterministic Replay** (350 LOC)
**File**: `src/engine/physics/deterministic_replay.c`
**Purpose**: Network-synchronized physics
**Depends on**: physics/, core/serialization/
**Impacts**: Multiplayer gameplay
**Key Features**:
- Input recording
- Simulation snapshots
- Divergence detection
- Resynchronization

---

### MEDIUM PRIORITY (Week 5-8)

#### 7. **Rendering Compute Shaders** (700 LOC)
**Files**:
- `src/engine/renderer/shaders/taa_compute.glsl`
- `src/engine/renderer/shaders/ssao_compute.glsl`
- `src/engine/renderer/shaders/ssr_compute.glsl`

#### 8. **Testing Infrastructure** (1,500 LOC)
**Files**:
- `tests/unit/math_tests.c`
- `tests/integration/physics_tests.c`
- `tests/integration/rendering_tests.c`
- `tests/stress/load_test.c`

---

## VI. Navigation Guide

### Finding Systems by Purpose

**I need to modify rendering...**
→ Start at: `src/engine/renderer/`
→ Public API: `src/include/rendering.h`
→ Dependencies: `src/rendering/`, `src/math/`, `src/core/`

**I need to add game mechanics...**
→ Start at: `src/engine/gameplay/`
→ Public API: `src/include/gameplay.h`
→ Dependencies: `src/ecs/`, `src/physics/`, `src/engine/animation/`

**I need to optimize physics...**
→ Start at: `src/engine/physics/`
→ Public API: `src/include/physics.h`
→ Dependencies: `src/math/`, `src/core/algorithms/`, `src/core/simd/`

**I need to add UI elements...**
→ Start at: `src/engine/ui/`
→ Public API: `src/include/ui.h`
→ Dependencies: `src/rendering/`, `src/core/`, `src/math/`

**I need to debug...**
→ Use: `src/core/logging/logger.h`
→ Use: `src/core/profiling/profiler.h`
→ Use: `src/core/memory/leak_detector.h`

### File Organization Convention

```
src/[SYSTEM]/
├── include/
│   └── [system].h          # Public API
├── core/
│   └── [system]_core.c     # Core implementation
├── [feature]/
│   ├── [feature].c         # Feature implementation
│   └── [feature].h         # Feature public API
├── tests/
│   └── [system]_test.c     # System tests
└── [system].c              # System initialization
```

---

## VII. Module Interface Template

Every new module should expose this interface:

```c
// src/include/[system].h
#ifndef [SYSTEM]_H
#define [SYSTEM]_H

#include "core/common.h"

// Initialization
void [system]_init(void);
void [system]_shutdown(void);

// Main functionality
// (specific to system)

// Status/diagnostics
void [system]_debug_print(void);
struct [System]Stats [system]_get_stats(void);

#endif // [SYSTEM]_H
```

---

## VIII. Circular Dependency Prevention

### Rules
1. **Higher layers depend on lower layers only**
   - Game depends on Engine, not vice versa
   - Engine depends on Core, not vice versa
   - Core depends on Platform, not vice versa

2. **Use abstraction layers**
   - Platform layer (sys/) abstracts OS differences
   - Rendering layer abstracts GPU differences
   - Audio layer abstracts sound system differences

3. **Forward declarations for optional references**
   ```c
   // In header:
   typedef struct Entity Entity;  // Forward decl

   // In implementation:
   #include "entity.h"  // Real definition
   ```

4. **Use callback patterns for reverse dependencies**
   ```c
   // Instead of physics.h #including game.h:
   typedef void (*PhysicsCollisionCallback)(Entity* a, Entity* b);
   void physics_set_collision_callback(PhysicsCollisionCallback cb);
   ```

---

## IX. Infrastructure Build Checklist

- [ ] **Phase 1: Core Foundations**
  - [ ] Unified type system (types.h)
  - [ ] Unified allocator
  - [ ] Profiling system
  - [ ] Logging system
  - [ ] Testing framework

- [ ] **Phase 2: Physics Completion**
  - [ ] CCD implementation
  - [ ] Deterministic replay
  - [ ] Physics tests

- [ ] **Phase 3: Rendering Completion**
  - [ ] Compute shaders (TAA, SSAO, SSR)
  - [ ] GPU profiling
  - [ ] Rendering tests

- [ ] **Phase 4: Gameplay Systems**
  - [ ] Game features completion
  - [ ] AI systems completion
  - [ ] Audio system completion

- [ ] **Phase 5: Multiplayer**
  - [ ] Network synchronization
  - [ ] Lag compensation
  - [ ] Matchmaking

- [ ] **Phase 6: Polish & Optimization**
  - [ ] Performance optimization
  - [ ] Memory profiling
  - [ ] Final integration tests

---

## X. Quick Reference

### System Entry Points
```
src/core/main.c              - Engine entry point
src/game/main.c              - Game entry point
src/tests/run_tests.c        - Test runner
```

### Critical Headers
```
src/include/core.h           - Core engine
src/include/math.h           - Math library
src/include/physics.h        - Physics engine
src/include/rendering.h      - Graphics
src/include/gameplay.h       - Game systems
src/include/ui.h             - UI system
```

### Configuration
```
CMakeLists.txt               - Build configuration
src/core/config.h            - Engine configuration
```

---

**Next Steps**: Implement Phase 1 core systems (unified types, allocators, logging, profiling) before tackling Phase 2-3 features.
