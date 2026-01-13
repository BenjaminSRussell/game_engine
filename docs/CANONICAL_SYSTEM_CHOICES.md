# Canonical System Choices - Minecraft v2 Infrastructure

This document defines the canonical choices for all major systems in Minecraft v2, providing clear direction for development and eliminating duplicate implementations.

## Phase 1: System Stabilization (COMPLETED)

### Physics System
**Canonical Choice**: `physics_consolidated.h`
- **Unified API**: Single interface consolidating all physics subsystems
- **Components**: Rigid bodies, soft bodies, fluids, cloth, vehicles
- **Features**: Collision detection, raycasting, block physics integration
- **Performance**: Multi-threaded with GPU acceleration support
- **Deprecated**: All separate physics headers (advanced_physics.h, block_physics.h, etc.)

### Particle System
**Canonical Choice**: `particle_system_unified.h`
- **Unified API**: Single particle system replacing 20+ duplicate implementations
- **Components**: Emitters, particles, GPU acceleration when available
- **Features**: Sorting, collision, wind, gravity
- **Performance**: CPU-based with optional GPU acceleration
- **Deprecated**: All separate particle system files (gpu_particles.h, svg_particles.h, etc.)

### Audio System
**Canonical Choice**: `audio_system_unified.h`
- **Unified API**: Single audio system consolidating all audio subsystems
- **Components**: 3D audio, effects, streaming, mixing
- **Features**: HRTF, Doppler, reverb, spatial audio
- **Performance**: Hardware-accelerated with fallback support
- **Deprecated**: All separate audio headers (advanced_audio.h, audio_core.h, etc.)

### Memory Management
**Canonical Choice**: `unified_memory_allocator.h`
- **Unified API**: Single memory allocator consolidating all memory systems
- **Strategies**: Pool, stack, arena, tracked allocations
- **Features**: Leak detection, performance metrics, guard pages
- **Performance**: Optimized for different allocation patterns
- **Deprecated**: All separate memory allocators

## Phase 2: Enable Excluded Systems

### NPC System
**Canonical Choice**: ECS-based NPC system
- **Integration**: Use current ECS architecture for NPC components
- **Components**: AI, behavior trees, perception, pathfinding
- **Features**: GOAP planning, state machines, interaction systems
- **Performance**: Batched updates with spatial partitioning

### Networking Subsystem
**Canonical Choice**: Entity replication system
- **Protocol**: Custom binary protocol with reliability layers
- **Features**: Delta compression, prioritization, client prediction
- **Performance**: Bandwidth-optimized with interpolation

### Editor Infrastructure
**Canonical Choice**: Node-based editor system
- **Architecture**: Component-based with visual scripting
- **Features**: Real-time editing, hot reload, asset management
- **Performance**: Efficient undo/redo with change tracking

### Mobile Platform Support
**Canonical Choice**: Cross-platform rendering abstraction
- **Graphics**: Metal/Vulkan with OpenGL fallback
- **Input**: Touch input with gesture recognition
- **Performance**: Optimized for mobile hardware constraints

## Phase 3: Complete Gameplay Features

### Player Systems
**Canonical Choice**: Component-based player architecture
- **Food System**: 8-item nutrition system with status effects
- **Magic System**: 6-slot spell system with mana management
- **Combat System**: Single unified combat interface
- **Integration**: All systems work through unified player component

### Vehicle System
**Canonical Choice**: Physics-based vehicle system
- **Integration**: Uses unified physics system
- **Features**: 10-item vehicle system with realistic physics
- **Performance**: Optimized for multiple vehicle types

### Spirit Model System
**Canonical Choice**: Animation-based spirit system
- **Integration**: Uses unified animation system
- **Features**: 8-item spirit system with procedural effects
- **Performance**: GPU-accelerated with LOD support

### Animation IK
**Canonical Choice**: FABRIK-based IK system
- **Integration**: Uses unified animation system
- **Features**: Full-body IK with analysis and testing
- **Performance**: Optimized for real-time use

## Migration Guidelines

### For Developers
1. **Use canonical headers**: Always include unified system headers
2. **Follow API patterns**: Use established function signatures
3. **Deprecate old code**: Mark old implementations as deprecated
4. **Update build system**: Remove references to deprecated systems

### For System Integration
1. **ECS-first**: All gameplay systems use ECS architecture
2. **Component-based**: Modular design with clear interfaces
3. **Performance-aware**: Consider memory layout and cache efficiency
4. **Cross-platform**: Ensure compatibility across all supported platforms

## Quality Standards

### Code Quality
- **Single source of truth**: Each system has one canonical implementation
- **Clear interfaces**: Well-documented APIs with consistent naming
- **Error handling**: Comprehensive error checking and recovery
- **Testing**: Unit tests for all major functionality

### Performance Standards
- **Memory efficiency**: Minimal allocations with pooling where appropriate
- **Thread safety**: All systems designed for multi-threaded use
- **GPU utilization**: Accelerate computationally expensive operations
- **Profiling**: Built-in performance metrics and debugging

### Documentation Standards
- **API documentation**: Complete function documentation with examples
- **Architecture docs**: System design and integration guides
- **Migration guides**: Clear paths from old to new systems
- **Performance guides**: Optimization recommendations and best practices

## Future Considerations

### Extensibility
- **Plugin architecture**: Allow for system extensions without core changes
- **Configuration**: Runtime configuration for different use cases
- **Modulation**: Support for different game modes and features
- **Scalability**: Design for both small and large-scale deployments

### Maintenance
- **Automated testing**: CI/CD pipeline with comprehensive test coverage
- **Performance monitoring**: Runtime performance tracking and alerting
- **Deprecation policy**: Clear timeline for removing old systems
- **Version compatibility**: Maintain API compatibility where possible

---

**Document Status**: Active
**Last Updated**: Phase 1 Complete, Phase 2-3 In Progress
**Next Review**: After Phase 2 completion
