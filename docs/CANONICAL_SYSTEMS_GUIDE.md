# Canonical Systems Guide

This document outlines the canonical system choices made during the infrastructure cleanup and consolidation phase of the Minecraft v2 project. These choices represent the standardized, production-ready implementations that should be used throughout the engine.

## Table of Contents

1. [Memory Management](#memory-management)
2. [Error Handling](#error-handling)
3. [AI Systems](#ai-systems)
4. [Physics Systems](#physics-systems)
5. [Particle Systems](#particle-systems)
6. [Rendering Pipeline](#rendering-pipeline)
7. [Audio Systems](#audio-systems)
8. [Performance Optimization](#performance-optimization)
9. [Logging System](#logging-system)
10. [Migration Guidelines](#migration-guidelines)

---

## Memory Management

### Canonical System: Unified Memory Manager
**Location**: `src/core/unified_memory.h` and `src/core/unified_memory.c`

### Features
- **Multi-strategy allocation**: Default, tracked, pooled, arena, stack, and GPU memory strategies
- **Comprehensive tracking**: Allocation metadata, leak detection, corruption detection
- **Performance optimization**: Memory pools, alignment, guard pages
- **Thread safety**: Full pthread-based synchronization
- **Debug support**: Extensive validation and profiling capabilities

### Usage
```c
// Basic allocation
void* ptr = UNIFIED_ALLOC(size);

// Type-safe allocation
MyStruct* obj = UNIFIED_NEW(MyStruct);

// Tracked allocation with type information
void* tracked = UNIFIED_ALLOC_TRACKED(size, "MyType");

// Array allocation
MyStruct* array = UNIFIED_NEW_ARRAY(MyStruct, count);

// Memory cleanup
UNIFIED_FREE(ptr);
```

### Migration Path
Replace all `malloc`, `calloc`, `realloc`, `free`, and custom memory functions with unified memory macros. The system provides compatibility macros for seamless migration.

---

## Error Handling

### Canonical System: Unified Error Handler
**Location**: `src/core/error_handling.h` and `src/core/error_handling.c`

### Features
- **Standardized error codes**: 50+ specific error codes across all categories
- **Error context tracking**: File, line, function, timestamp, thread ID
- **Error chaining**: Support for causal error relationships
- **Recovery strategies**: Configurable error handling strategies
- **Integration with logging**: Seamless integration with unified logging system

### Usage
```c
// Basic error reporting
REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid input parameter");

// Conditional error reporting
REPORT_ERROR_IF(condition, ERROR_OUT_OF_MEMORY, "Memory allocation failed");

// Return on error pattern
RETURN_ERROR_IF_NULL(ptr, ERROR_NULL_POINTER, "Pointer cannot be null");

// Error with context
error_context_t* error = error_create(ERROR_OPERATION_FAILED, "Description", 
                                     __FILE__, __FUNCTION__, __LINE__);
error_report(error);
```

### Migration Path
Replace all ad-hoc error handling with standardized error codes and reporting functions. The system provides legacy compatibility for existing error handling patterns.

---

## AI Systems

### Canonical System: Unified AI System
**Location**: `src/ai/ai_system.h` and `src/ai/ai_system.c`

### Features
- **Behavior Trees**: Complete node system with decorators, blackboards, utility selectors
- **GOAP Planning**: Goal-oriented action planning with world state management
- **Pathfinding**: A* navigation with navmesh support
- **Agent Management**: Unified agent lifecycle and state management
- **Performance Optimization**: Caching, batching, async operations

### Usage
```c
// Create AI world
ai_world_t* world = ai_world_create();

// Create agent
ai_agent_t* agent = ai_world_create_agent(world, config);

// Create behavior tree
behavior_tree_t* bt = ai_agent_create_behavior_tree(agent);

// Create GOAP planner
goap_planner_t* planner = ai_agent_create_goap_planner(agent);

// Update AI system
ai_world_update(world, dt);
```

### Migration Path
Consolidate all AI implementations (behavior trees, GOAP, pathfinding) into the unified AI system. The system provides compatibility layers for existing AI code.

---

## Physics Systems

### Canonical System: Unified Physics API
**Location**: `src/physics/physics_api.h` and `src/physics/physics_api.c`

### Features
- **Rigid Body Dynamics**: Complete rigid body simulation with constraints
- **Collision Detection**: Broadphase and narrowphase collision detection
- **Character Controllers**: Specialized character physics
- **Constraint Solvers**: Multiple constraint types (joints, springs, etc.)
- **Performance Optimization**: Multi-threading, spatial partitioning, GPU acceleration

### Usage
```c
// Create physics world
physics_world_t* world = physics_world_create(config);

// Create rigid body
physics_body_t* body = physics_body_create(shape, mass);

// Add to world
physics_world_add_body(world, body);

// Step simulation
physics_world_step(world, dt);
```

### Migration Path
Replace all physics system calls with unified physics API. The system provides legacy macros for backward compatibility.

---

## Particle Systems

### Canonical System: Unified Particle System
**Location**: `src/effects/unified_particle_system.h`

### Features
- **Multi-backend rendering**: CPU, GPU, and hybrid rendering modes
- **Advanced emitters**: Comprehensive emitter configuration and presets
- **Particle effects**: Gravity, wind, turbulence, collisions, attractions
- **Weather system**: Rain, snow, dust, smoke, fog effects
- **Performance optimization**: Culling, batching, instancing

### Usage
```c
// Create particle system
particle_system_t* system = particle_system_create(RENDER_MODE_HYBRID, 1000);

// Add emitter
particle_emitter_config_t config;
particle_emitter_config_fire(&config);
uint32_t emitter_id = particle_system_add_emitter(system, &config);

// Update and render
particle_system_update(system, dt);
particle_system_render(system, view_matrix, projection_matrix);
```

### Migration Path
Consolidate all particle implementations into the unified system. Use the consolidation script to identify and remove duplicates.

---

## Rendering Pipeline

### Canonical System: Unified Rendering Pipeline
**Location**: `src/rendering/unified_rendering_pipeline.h`

### Features
- **Multi-backend support**: OpenGL, Vulkan, Metal, DirectX12
- **Deferred rendering**: GBuffer, deferred lighting, post-processing
- **Forward rendering**: Transparent objects, UI, debug rendering
- **Shader management**: Unified shader loading and compilation
- **Performance optimization**: Culling, batching, instancing

### Usage
```c
// Create rendering pipeline
render_config_t config = {0};
unified_rendering_pipeline_t* pipeline = unified_rendering_pipeline_create(&config);

// Initialize
unified_rendering_pipeline_initialize(pipeline);

// Render frame
unified_rendering_pipeline_begin_frame(pipeline);
unified_rendering_pipeline_execute(pipeline);
unified_rendering_pipeline_end_frame(pipeline);
```

### Migration Path
Replace all renderer-specific code with unified rendering pipeline calls. The system provides legacy renderer interface compatibility.

---

## Audio Systems

### Canonical System: Unified Audio System
**Location**: `src/audio/unified_audio_system.h`

### Features
- **Multi-backend support**: OpenAL, XAudio2, Core Audio, ALSA, PulseAudio
- **3D Spatial Audio**: HRTF, VBAP, ambisonics support
- **DSP Effects**: Reverb, echo, chorus, filters, compression, EQ
- **Audio Streaming**: Large file streaming with dynamic loading
- **Performance optimization**: Multi-threading, buffering, compression

### Usage
```c
// Create audio system
audio_config_t config = {0};
unified_audio_system_t* audio = unified_audio_system_create(&config);

// Load sound
uint32_t buffer_id = unified_audio_load_buffer(audio, "sound.wav");

// Play sound
AUDIO_PLAY_SOUND(audio, buffer_id);

// Update
unified_audio_system_update(audio, dt);
```

### Migration Path
Replace all audio system calls with unified audio API. The system provides compatibility for existing audio code.

---

## Performance Optimization

### Canonical System: Performance Optimizer
**Location**: `src/core/performance_optimizer.h` and `src/core/performance_optimizer.c`

### Features
- **LOD System**: Automatic level-of-detail selection
- **Culling Systems**: Frustum, occlusion, and distance culling
- **Batch Rendering**: Efficient geometry batching
- **GPU Instancing**: Hardware-accelerated instancing
- **Performance Monitoring**: Real-time metrics and profiling

### Usage
```c
// Create optimizer
performance_optimizer_t* optimizer = performance_optimizer_create(PERF_OPT_DEFAULT);

// Add LOD object
lod_object_t object = {0};
performance_optimizer_add_lod_object(optimizer, &object);

// Update
performance_optimizer_update(optimizer, dt);
```

### Migration Path
Integrate performance optimization calls into existing rendering and game logic code.

---

## Logging System

### Canonical System: Unified Logging
**Location**: `src/core/common/unified_logging.h`

### Features
- **Multi-channel logging**: Separate channels for different subsystems
- **Log levels**: Trace, debug, info, warning, error, fatal
- **Multiple outputs**: Console, file, debugger, network
- **Performance optimized**: Efficient formatting and output
- **Thread safety**: Full concurrent logging support

### Usage
```c
// Basic logging
LOG_CORE_INFO("System initialized");
LOG_PHYSICS_ERROR("Physics simulation failed");

// Channel-specific logging
LOG_AUDIO_DEBUG("Audio buffer loaded");
LOG_RENDERING_WARNING("Shader compilation failed");

// Performance logging
LOG_PROFILE_START("update");
// ... code ...
LOG_PROFILE_END("update");
```

### Migration Path
Replace all logging calls with unified logging macros. The system provides compatibility macros for existing logging systems.

---

## Migration Guidelines

### General Migration Strategy

1. **Assessment**: Use the provided migration scripts to identify areas needing migration
2. **Incremental Migration**: Migrate one system at a time to minimize disruption
3. **Testing**: Test each migration step thoroughly before proceeding
4. **Documentation**: Update code documentation to reflect new systems
5. **Cleanup**: Remove old system code after successful migration

### Migration Scripts

- **Memory Migration**: `tools/memory_migration.py` - Consolidates memory allocation calls
- **Particle Consolidation**: `tools/particle_consolidation.py` - Identifies and removes duplicate particle systems

### Compatibility Considerations

- All canonical systems provide legacy compatibility layers
- Migration can be done incrementally without breaking existing functionality
- Existing code can continue to work during the migration period
- Performance improvements are realized gradually as migration progresses

### Best Practices

1. **Use Type-Safe APIs**: Prefer type-safe allocation and creation functions
2. **Enable Debug Features**: Use debug and validation features during development
3. **Monitor Performance**: Use built-in profiling and monitoring tools
4. **Follow Error Handling**: Use standardized error reporting and handling
5. **Document Changes**: Update documentation to reflect system changes

---

## System Dependencies

```
Unified Logging (Foundation)
    ↓
Unified Memory Management
    ↓
Error Handling System
    ↓
All Other Systems (AI, Physics, Rendering, Audio, etc.)
```

The systems are designed with clear dependency hierarchies. The logging system provides the foundation, followed by memory management and error handling, which all other systems depend on.

---

## Performance Characteristics

### Memory Management
- **Allocation overhead**: ~5-10% for tracked allocations
- **Memory usage**: 2-5% overhead for metadata
- **Performance**: Comparable to direct malloc/free for hot paths

### Error Handling
- **Overhead**: Minimal when no errors occur
- **Performance**: Fast error code lookup and reporting
- **Memory**: Small static footprint for error tables

### AI Systems
- **Performance**: Optimized for large numbers of agents
- **Memory**: Efficient data structures with minimal overhead
- **Scalability**: Designed for thousands of concurrent agents

### Physics Systems
- **Performance**: Multi-threaded with GPU acceleration options
- **Accuracy**: High-fidelity simulation with configurable precision
- **Scalability**: Handles thousands of rigid bodies efficiently

### Rendering Pipeline
- **Performance**: Modern GPU pipeline with minimal CPU overhead
- **Flexibility**: Supports multiple rendering backends
- **Features**: Complete modern rendering feature set

### Audio Systems
- **Latency**: Low-latency audio processing
- **Quality**: High-fidelity spatial audio and DSP
- **Performance**: Multi-threaded with streaming support

---

## Conclusion

The canonical systems described in this guide represent the culmination of the infrastructure cleanup and consolidation effort. They provide:

- **Consistency**: Standardized APIs across all subsystems
- **Performance**: Optimized implementations with profiling support
- **Maintainability**: Clean, well-documented code with clear interfaces
- **Extensibility**: Modular design for future enhancements
- **Reliability**: Comprehensive error handling and validation

By adopting these canonical systems, the Minecraft v2 project achieves a solid, production-ready foundation for future development.

---

*This document is maintained as part of the Minecraft v2 project infrastructure. Last updated during the consolidation phase.*
