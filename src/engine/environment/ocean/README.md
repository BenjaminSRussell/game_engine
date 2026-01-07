# Ocean FFT System

**Realistic ocean wave simulation using Phillips spectrum and GPU-accelerated FFT**

---

## Overview

The Ocean FFT system generates realistic ocean waves using:
- **Phillips spectrum** for physically-based wave statistics
- **Fast Fourier Transform (FFT)** on GPU for real-time performance
- **Choppy waves** (Gerstner-style) for realistic wave shapes
- **Dynamic foam** from wave breaking detection
- **Multi-scale cascades** for near/medium/far detail

---

## Features

### ✅ Implemented

- Phillips spectrum generation with wind parameters
- Radix-2 FFT compute shaders (Metal)
- Time-varying wave displacement (height + horizontal)
- Normal map & foam generation
- Tessellated ocean mesh with LOD
- Enhanced water rendering (Fresnel, reflections, foam)
- Multi-cascade system (3 levels)

### ⏳ Pending

- Metal backend integration (texture/buffer creation)
- FFT pipeline dispatch
- Cascade blending in shaders
- Environment map reflections
- Underwater refraction

---

## Usage

### Initialization

```c
#include "environment/ocean/ocean_fft.h"
#include "environment/ocean/ocean_mesh.h"

// Create ocean system
OceanFFTConfig config = ocean_fft_default_config();
OceanFFTSystem* ocean = ocean_fft_create(device, queue, &config);

// Create ocean mesh
OceanMeshConfig mesh_config = ocean_mesh_default_config();
OceanMesh* mesh = ocean_mesh_create(device, &mesh_config);

// Configure spectrum (optional)
PhillipsSpectrumParams params = ocean_fft_default_spectrum();
params.wind_speed = 25.0f;  // m/s
params.wind_direction_x = 1.0f;
params.wind_direction_z = 0.0f;
ocean_fft_set_spectrum_params(ocean, &params);
```

### Per-Frame Update

```c
// Update ocean simulation (runs FFT pipeline)
ocean_fft_update(ocean, delta_time);

// Select LOD based on camera distance
float distance = length(camera_pos - ocean_center);
uint32_t lod = ocean_mesh_select_lod(mesh, distance);

// Get output textures
MTLTextureRef displacement = ocean_fft_get_displacement_texture(ocean, 0);
MTLTextureRef normals = ocean_fft_get_normal_texture(ocean, 0);
```

### Rendering

```c
// Bind vertex/index buffers
MTLBufferRef vb = ocean_mesh_get_vertex_buffer(mesh, lod);
MTLBufferRef ib = ocean_mesh_get_index_buffer(mesh, lod);
uint32_t count = ocean_mesh_get_index_count(mesh, lod);

// Bind displacement & normal textures
[encoder setVertexTexture:displacement atIndex:2];  // cascade 0
[encoder setFragmentTexture:normals atIndex:5];     // cascade 0

// Draw ocean
[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                    indexCount:count
                     indexType:MTLIndexTypeUInt32
                   indexBuffer:ib
             indexBufferOffset:0];
```

### Cleanup

```c
ocean_fft_destroy(ocean);
ocean_mesh_destroy(mesh);
```

---

## Configuration

### Ocean FFT Config

```c
typedef struct OceanFFTConfig {
    uint32_t fft_resolution;     // 128, 256, or 512 (default: 256)
    float gravity;                // m/s² (default: 9.81)
    float time_scale;             // Time multiplier (default: 1.0)
    
    uint32_t cascade_count;       // 1-4 (default: 3)
    float cascade_scales[4];      // Tile sizes (default: 250, 1000, 4000)
    
    bool enable_foam;             // Generate foam (default: true)
    bool enable_normals;          // Generate normals (default: true)
} OceanFFTConfig;
```

### Phillips Spectrum Params

```c
typedef struct PhillipsSpectrumParams {
    float wind_speed;             // m/s (default: 20)
    float wind_direction_x;       // Normalized (default: 1.0)
    float wind_direction_z;       // Normalized (default: 0.0)
    
    float amplitude;              // Wave height scale (default: 1.0)
    float wave_suppression;       // Small wave cutoff (default: 0.001)
    float fetch;                  // Wave development distance (default: 100000)
    float choppy_factor;          // Horizontal displacement (default: 1.5)
} PhillipsSpectrumParams;
```

### Mesh Config

```c
typedef struct OceanMeshConfig {
    uint32_t base_resolution;     // Base grid resolution (default: 128)
    uint32_t lod_count;           // Number of LODs (default: 3)
    float lod_distances[5];       // Distance thresholds (default: 500, 2000, 10000)
    float tile_size;              // Physical size in meters (default: 1000)
} OceanMeshConfig;
```

---

## Technical Details

### Phillips Spectrum

The Phillips spectrum defines the amplitude of ocean waves based on wind:

```
P(k) = A * |k̂·ŵ|² / |k|⁴ * exp(-1/(kL)²) * exp(-k²l²)
```

- `k`: Wave vector (direction and frequency)
- `w`: Wind direction
- `L = V²/g`: Largest wave size (depends on wind speed V)
- `l`: Small wave suppression factor

### FFT Pipeline

1. **Spectrum Update:** Compute h(k,t) = h0(k)*exp(iωt) + h0*(-k)*exp(-iωt)
2. **Horizontal FFT:** Process each row
3. **Vertical FFT:** Process each column
4. **Displacement Assembly:** Combine height, Dx, Dz
5. **Normal Generation:** Compute gradients via finite differences
6. **Foam Generation:** Detect wave breaking from Jacobian determinant

### Choppy Waves

Horizontal displacement creates realistic wave shapes:

```
Position = (x + λ*Dx, height, z + λ*Dz)
```

Where λ (choppy_factor) typically ranges from 1.0 to 2.0.

### Cascades

Multiple FFT systems with different tile sizes provide multi-scale detail:

- **Cascade 0** (250m): Fine ripples and small waves
- **Cascade 1** (1000m): Medium waves
- **Cascade 2** (4000m): Large ocean swells

---

## Performance

### Targets

| Metric | Target | Status |
|--------|--------|--------|
| FFT Resolution | 256×256 | ✅ Configurable |
| FFT Time | <2ms | ⏳ To be profiled |
| Total Update | <3ms | ⏳ To be profiled |
| Frame Rate | 60 FPS | ⏳ To be verified |
| Memory | <100MB | ✅ ~20-25MB estimated |

### Optimization

- Shared memory in FFT shaders for cache efficiency
- Ping-pong buffers for in-place FFT
- LOD system reduces vertex count at distance
- Cascade blending (future) adapts detail to camera

---

## Shaders

### Compute Shaders (Metal)

Located in `assets/shaders/ocean/`:

- `fft_utils.metal` - Complex math, twiddle factors
- `fft_horizontal.comp.metal` - Horizontal FFT pass
- `fft_vertical.comp.metal` - Vertical FFT pass
- `wave_spectrum_update.comp.metal` - Time-varying spectrum
- `wave_displacement.comp.metal` - Post-FFT assembly
- `wave_normals.comp.metal` - Normal map generation
- `wave_foam.comp.metal` - Foam from Jacobian

### Vertex/Fragment Shaders (GLSL)

Located in `assets/shaders/`:

- `ocean_surface.vert` - Displacement mapping
- `ocean_surface.frag` - Water shading (Fresnel, foam)

---

## Directory Structure

```
src/engine/rendering/3d_rendering/environment/ocean/
├── ocean_fft.h              # Main ocean FFT system API
├── ocean_fft.c              # System initialization & update
├── phillips_spectrum.h      # Spectrum generation API
├── phillips_spectrum.c      # Phillips equation
├── ocean_mesh.h             # Grid mesh & LOD
└── ocean_mesh.c             # Mesh generation

assets/shaders/ocean/
├── fft_utils.metal
├── fft_horizontal.comp.metal
├── fft_vertical.comp.metal
├── wave_spectrum_update.comp.metal
├── wave_displacement.comp.metal
├── wave_normals.comp.metal
└── wave_foam.comp.metal
```

---

## References

- Tessendorf, J. (2001). "Simulating Ocean Water" (SIGGRAPH course notes)
- Cooley & Tukey (1965). "An Algorithm for the Machine Calculation of Complex Fourier Series"
- Schlick, C. (1994). "An Inexpensive BRDF Model for Physically-based Rendering"

---

## Status

**Implementation:** 75+ out of ~100 TODOs complete  
**Core Algorithms:** ✅ Fully implemented  
**Metal Integration:** ⏳ Pending (texture/buffer creation, pipeline dispatch)  
**Rendering:** ✅ Shaders complete, integration pending

See [walkthrough.md](file:///Users/benjaminrussell/.gemini/antigravity/brain/9c26c9c4-7cdf-4b1c-b30b-6085433fb605/walkthrough.md) for detailed implementation notes.
