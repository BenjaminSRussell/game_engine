# Renderer Subsystem Architecture

> **Priority 1** | Est. 12,000+ files | 1.5M+ LOC | ⭐⭐⭐⭐⭐ Complexity

The renderer is the largest and most complex subsystem, handling all visual output.

---

## Directory Structure

```
Renderer/
├── Private/                    # All .c implementation files
├── Public/                     # All .h header files
│
├── Core/                       # Rendering fundamentals
│   ├── Commands/               # Command buffers, draw calls
│   ├── Resources/              # GPU resource management
│   ├── Pipeline/               # Pipeline state objects
│   └── Synchronization/        # Fences, semaphores
│
├── RenderGraph/                # Frame graph system
│   ├── Builder/                # Graph construction
│   ├── Compiler/               # Pass optimization
│   ├── Executor/               # Graph execution
│   └── Resources/              # Transient resources
│
├── Deferred/                   # Deferred rendering pipeline
│   ├── GBuffer/                # Geometry buffer
│   │   ├── Albedo/
│   │   ├── Normal/
│   │   ├── Roughness/
│   │   └── Metallic/
│   ├── Lighting/               # Deferred lighting pass
│   │   ├── Directional/
│   │   ├── Point/
│   │   ├── Spot/
│   │   └── Area/
│   └── Composition/            # Final composition
│
├── Forward/                    # Forward+ rendering
│   ├── Clustered/              # Clustered lighting
│   ├── Transparent/            # Transparency handling
│   └── Decals/                 # Forward decals
│
├── RayTracing/                 # Hardware ray tracing
│   ├── BVH/                    # Acceleration structures
│   │   ├── Builder/
│   │   ├── Updater/
│   │   └── Compaction/
│   ├── Intersection/           # Ray-primitive tests
│   ├── Shading/                # Ray shading
│   ├── Denoiser/               # Noise reduction
│   │   ├── Temporal/
│   │   ├── Spatial/
│   │   └── AI/                 # ML denoising
│   └── GI/                     # Global illumination
│       ├── RTGI/               # Ray traced GI
│       ├── DDGI/               # Dynamic diffuse GI
│       └── Irradiance/         # Irradiance probes
│
├── Shadows/                    # Shadow systems
│   ├── Cascaded/               # CSM for directional
│   │   ├── Fitting/
│   │   ├── Splitting/
│   │   └── Filtering/
│   ├── Virtual/                # Virtual shadow maps
│   │   ├── Pages/
│   │   ├── Caching/
│   │   └── Streaming/
│   ├── RayTraced/              # RT shadows
│   └── Contact/                # Contact shadows
│
├── PostProcess/                # Post-processing effects
│   ├── Bloom/
│   │   ├── Downsample/
│   │   ├── Upsample/
│   │   └── Composite/
│   ├── SSAO/                   # Ambient occlusion
│   │   ├── GTAO/
│   │   ├── HBAO/
│   │   └── SSAO_Classic/
│   ├── SSR/                    # Screen-space reflections
│   │   ├── HiZ/
│   │   ├── Tracing/
│   │   └── Resolve/
│   ├── TAA/                    # Temporal AA
│   │   ├── Jitter/
│   │   ├── History/
│   │   └── Resolve/
│   ├── DOF/                    # Depth of field
│   │   ├── CoC/
│   │   ├── Bokeh/
│   │   └── Blur/
│   ├── MotionBlur/
│   │   ├── Velocity/
│   │   ├── TileMax/
│   │   └── Gather/
│   ├── Tonemapping/
│   │   ├── ACES/
│   │   ├── Reinhard/
│   │   └── FilmicBT2020/
│   ├── ColorGrading/
│   │   ├── LUT/
│   │   ├── WhiteBalance/
│   │   └── Shadows_Highlights/
│   └── ChromaticAberration/
│
├── Atmosphere/                 # Atmospheric rendering
│   ├── Sky/
│   │   ├── Procedural/
│   │   ├── HDRI/
│   │   └── PhysicallyBased/
│   ├── Fog/
│   │   ├── Volumetric/
│   │   ├── Height/
│   │   └── Distance/
│   ├── Clouds/
│   │   ├── Volumetric/
│   │   ├── RayMarching/
│   │   └── LightScattering/
│   └── Scattering/
│       ├── Rayleigh/
│       └── Mie/
│
├── Materials/                  # Material system
│   ├── PBR/
│   │   ├── Standard/
│   │   ├── ClearCoat/
│   │   ├── Subsurface/
│   │   └── Anisotropic/
│   ├── Shaders/
│   │   ├── Compiler/
│   │   ├── Cache/
│   │   └── HotReload/
│   ├── Nodes/                  # Material graph nodes
│   │   ├── Math/
│   │   ├── Texture/
│   │   ├── Utility/
│   │   └── Output/
│   └── Instances/
│
├── Textures/                   # Texture systems
│   ├── Virtual/                # Virtual texturing
│   │   ├── Pages/
│   │   ├── Feedback/
│   │   └── Streaming/
│   ├── Compression/
│   │   ├── BC7/
│   │   ├── ASTC/
│   │   └── ETC2/
│   ├── Streaming/
│   │   ├── Priority/
│   │   ├── Budget/
│   │   └── Prefetch/
│   └── Atlas/
│
├── Geometry/                   # Geometry processing
│   ├── Nanite/                 # Virtualized geometry
│   │   ├── Clusters/
│   │   ├── LOD/
│   │   ├── Culling/
│   │   └── Streaming/
│   ├── LOD/                    # Traditional LOD
│   │   ├── Generation/
│   │   ├── Selection/
│   │   └── Transitions/
│   ├── Instancing/
│   │   ├── Static/
│   │   ├── Dynamic/
│   │   └── GPU_Driven/
│   └── Mesh/
│       ├── Processing/
│       ├── Optimization/
│       └── Splitting/
│
├── Culling/                    # Visibility determination
│   ├── Frustum/
│   ├── Occlusion/
│   │   ├── HZB/                # Hierarchical-Z buffer
│   │   ├── Software/
│   │   └── GPU/
│   ├── Distance/
│   └── Portal/
│
├── VRS/                        # Variable rate shading
│   ├── Image/
│   ├── Primitive/
│   └── Adaptive/
│
├── Upscaling/                  # Resolution scaling
│   ├── DLSS/
│   ├── FSR/
│   ├── XeSS/
│   └── MetalFX/
│
├── Debug/                      # Debug visualization
│   ├── Wireframe/
│   ├── Normals/
│   ├── UVs/
│   ├── Overdraw/
│   └── Performance/
│
└── Water/                      # Water rendering
    ├── Ocean/
    ├── River/
    ├── Caustics/
    └── Foam/
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| Renderer/Core/ | src/Renderer/core/ | 41 |
| Renderer/Deferred/ | src/Renderer/deferred/ | 23 |
| Renderer/PostProcess/ | src/Renderer/postprocess/, src/engine/postprocess/ | 260 |
| Renderer/RayTracing/ | src/Renderer/raytracing/ | 97 |
| Renderer/Materials/ | src/Renderer/materials/ | 164 |
| Renderer/Shadows/ | src/Renderer/shadows/ | 1 |
| Renderer/Culling/ | src/Renderer/culling/ | 5 |
| Renderer/VRS/ | src/Renderer/vrs/ | 24 |
| Renderer/Geometry/ | src/Renderer/Geometry/ | 264 |

---

## Key Files to Create

Each leaf directory needs:
1. `{feature}_types.h` - Type definitions
2. `{feature}_api.h` - Public API
3. `{feature}_internal.h` - Internal API
4. `{feature}.c` - Implementation (<500 LOC)
5. `{feature}_utils.c` - Utilities (optional)
