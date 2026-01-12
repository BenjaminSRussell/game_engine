# Rendering Pipeline Architecture Documentation

## Overview
The Minecraft v2 engine uses a **hybrid rendering approach** with Metal as the primary backend on macOS/iOS and Vulkan as a secondary backend for cross-platform support. The architecture combines forward rendering for transparent objects with deferred rendering for opaque geometry.

## Current Architecture

### 1. Rendering Backends
- **Metal Backend** (Primary): `src/engine/backend/metal/`
  - Fully implemented for macOS/iOS
  - Comprehensive feature set including mesh shaders, ray tracing, and indirect command buffers
  - Synchronization issues present in frame management
  - 24 implementation files covering all major rendering subsystems

- **Vulkan Backend** (Secondary): `src/engine/backend/vulkan/`
  - Partially implemented with 36 files
  - Many files are disabled or contain minimal implementations
  - Validation layer system present but incomplete
  - Deferred renderer designed for Vulkan but backend non-functional

### 2. Rendering Pipeline Stages

#### Geometry Stage (Deferred)
- **G-Buffer Layout**: 5 attachments (Position+Depth, Normal+Roughness, Albedo, Material+AO+Emissive, Depth)
- **Primary Renderer**: `DeferredRenderer` in `include/renderer/deferred_renderer.h`
- **Backend Support**: Designed for Vulkan, Metal adaptation needed

#### Lighting Stage (Deferred)
- **PBR Materials**: Full metallic-roughness workflow
- **Light Types**: Point, directional, spot lights with shadow mapping
- **Global Illumination**: Voxel cone tracing implementation

#### Forward Stage
- **Transparent Objects**: Alpha-blended particles, sprites, effects
- **UI Rendering**: 2D sprites and text rendering
- **Special Effects**: Weather particles, VFX systems

### 3. Renderer Implementations Analysis

#### Particle Renderers (45+ files found)
**Active Implementations:**
- `src/engine/core/services/particle_renderer.c` - Main particle renderer (355 lines)
- `src/engine/effects/svg_particles/svg_particle_renderer.c` - SVG-based particles (226 lines)
- `src/engine/effects/gpu_particles/` - GPU-accelerated particle system
- `src/engine/effects/particles/` - CPU particle system (mostly disabled)

**Duplicate Issues:**
- Multiple particle systems with overlapping functionality
- GPU particles exist in both `core/services/` and `effects/gpu_particles/`
- SVG particles separate from main particle pipeline
- Weather particles integrated into main renderer but also separate systems

#### Voxel Renderers (20+ files found)
**Active Implementations:**
- `src/engine/rendering/voxel_renderer.h` - Main voxel renderer interface
- `src/engine/core/services/voxel_renderer.c` - Core implementation
- `src/engine/core/services/voxel_renderer_metal.c` - Metal-specific implementation
- `src/engine/environment/terrain/core/voxel_mesher.c` - Terrain meshing

**Duplicate Issues:**
- Multiple voxel renderer interfaces with similar functionality
- Metal-specific implementation separate from core renderer
- Terrain mesher duplicates voxel meshing logic

#### Sprite Renderers (9+ files found)
**Active Implementations:**
- `src/engine/core/services/sprite_3d_renderer.c` - 3D sprite rendering
- `src/engine/core/services/sprite_renderer_2_5d.c` - 2.5D sprite rendering
- `src/engine/rendering/sprite_3d_renderer.c` - Alternative implementation (disabled)
- `src/engine/rendering/sprite_renderer_2_5d.c` - Alternative implementation

**Duplicate Issues:**
- Multiple sprite renderers with overlapping 2D/3D functionality
- Core services and rendering modules contain duplicate implementations
- Several implementations are disabled but still present

## Critical Issues

### 1. Vulkan Backend Status
- **Current State**: Non-functional, most files disabled
- **Impact**: Deferred rendering pipeline unusable on non-Apple platforms
- **Decision Point**: Either complete implementation or remove entirely

### 2. Metal Backend Synchronization
- **Issue**: Frame synchronization problems in `mtl_frame_sync.c`
- **Impact**: Potential visual artifacts and performance issues
- **Files Involved**: 24 Metal backend files need synchronization review

### 3. Renderer Fragmentation
- **Particle Systems**: 45+ files with massive duplication
- **Voxel Systems**: 20+ files with overlapping functionality  
- **Sprite Systems**: 9+ files with redundant implementations

## Recommended Architecture Changes

### Phase 1: Backend Consolidation
1. **Remove Vulkan Backend**: Delete 36 Vulkan files and deferred renderer
2. **Fix Metal Synchronization**: Resolve frame sync issues in Metal backend
3. **Unify Rendering Pipeline**: Use Metal for all rendering stages

### Phase 2: Renderer Consolidation
1. **Particle Systems**: Consolidate to single GPU particle system
2. **Voxel Renderers**: Single unified voxel renderer with Metal backend
3. **Sprite Renderers**: Single sprite renderer supporting 2D/2.5D/3D

### Phase 3: Pipeline Optimization
1. **Unified Material System**: Single PBR material pipeline
2. **Consolidated Command Buffer**: Single command encoding system
3. **Performance Monitoring**: Unified statistics and profiling

## Implementation Priority

**High Priority (Blocks all visual systems):**
- Fix Metal backend synchronization
- Remove or complete Vulkan backend
- Consolidate particle renderers

**Medium Priority (Performance/maintenance):**
- Consolidate voxel renderers
- Remove redundant sprite renderers
- Document final architecture

**Low Priority (Optimization):**
- End-to-end pipeline testing
- Performance optimization
- Advanced feature integration

## File Structure (Post-Consolidation)

```
src/engine/
├── backend/
│   └── metal/ (24 files - keep all)
├── rendering/
│   ├── particle_renderer.h/.c (1 unified system)
│   ├── voxel_renderer.h/.c (1 unified system)
│   ├── sprite_renderer.h/.c (1 unified system)
│   └── pipeline_manager.h/.c (new unified pipeline)
└── effects/
    └── (consolidated effects systems)
```

This consolidation will reduce the rendering codebase from ~100+ files to approximately 30-40 core files while maintaining all functionality.
