# Ray Tracing Subsystem

This subsystem implements hardware-accelerated ray tracing for the 3D Rendering Engine.

## Features

### 1. Acceleration Structures (`acceleration_structures/`)
- **BLAS (Bottom-Level)**: Built per-mesh. Optimized for static geometry.
- **TLAS (Top-Level)**: Built per-frame. Handles dynamic object transforms.
- **Compaction**: Reduces memory footprint of static BLAS.
- **Updates**: various strategies for refitting/rebuilding.

### 2. Ray-Traced Shadows (`shadows_rt/`)
- Hard shadows via direct visibility rays.
- Soft shadows support (skeleton implemented).
- Denoising support (skeleton implemented).

### 3. Ray-Traced Reflections (`reflections_rt/`)
- Mirror-like reflections.
- Glossy reflections (roughness support).
- Denoising for rough surfaces.

### 4. Global Illumination (`gi_rt/`)
- **DDGI (Dynamic Diffuse Global Illumination)**: Probe-based solution.
- **Probes**: Managed in a grid, updated via ray tracing.
- **Sampling**: Efficient irradiance and visibility sampling.

## Usage

Initialize the subsystems in order:
1. `acceleration_structures_blas_builder_init()`
2. `acceleration_structures_tlas_builder_init()`
3. `raytracing_rt_shadow_rays_init()`
4. `raytracing_rt_reflection_rays_init()`
5. `raytracing_ddgi_probes_init()`

## Testing

Integration tests are located in `src/engine/rendering/3d_rendering/test/test_raytracing.c`.
Benchmarks are in `src/engine/rendering/3d_rendering/test/benchmark_raytracing.c`.
