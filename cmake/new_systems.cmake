# CMake additions for all new systems
# Add to cmake/sources.cmake

# New rendering systems
set(RENDERING_METALFX_SOURCES
    src/engine/backend/metal/mtl_metalfx.m
    src/engine/include/rendering/metal_fx.h
)

set(RENDERING_RAYTRACING_SOURCES
    src/engine/backend/metal/mtl_raytracing.m
    src/engine/include/rendering/metal_raytracing.h
)

set(RENDERING_VRS_SOURCES
    src/engine/backend/metal/mtl_vrs.m
    src/engine/include/rendering/metal_vrs.h
)

set(RENDERING_MESH_SHADERS_SOURCES
    src/engine/include/rendering/metal_mesh_shaders.h
)

set(RENDERING_INTEGRATION_SOURCES
    src/engine/include/rendering/metal_integration.h
)

# Nanite
set(GEOMETRY_NANITE_SOURCES
    src/engine/geometry/nanite/nanite_cluster.c
    src/engine/geometry/nanite/nanite_render.m
    src/engine/include/geometry/nanite/nanite_cluster.h
)

# Lumen
set(LIGHTING_LUMEN_SOURCES
    src/engine/lighting/lumen/lumen_gi.m
    src/engine/include/lighting/lumen/lumen_gi.h
)

# World building
set(ENVIRONMENT_TERRAIN_SOURCES
    src/engine/environment/terrain/heightmap_terrain.m
    src/engine/include/environment/terrain/heightmap_terrain.h
)

set(ENVIRONMENT_FOLIAGE_SOURCES
    src/engine/environment/vegetation/foliage_system.m
    src/engine/include/environment/vegetation/foliage_system.h
)

# Animation
set(ANIMATION_BLUEPRINTS_SOURCES
    src/engine/animation/animation_blueprints.c
    src/engine/include/animation/animation_blueprints.h
)

# Audio (extends existing spatial_audio.c)
# No new sources - extends existing

# Physics
set(PHYSICS_DESTRUCTION_SOURCES
    src/engine/physics/destruction/chaos_destruction.m
    src/engine/include/physics/destruction/chaos_destruction.h
)

# VFX
set(EFFECTS_NIAGARA_SOURCES
    src/engine/effects/particles/niagara_emitter.m
    src/engine/include/effects/particles/niagara_emitter.h
)

# Networking
set(NETWORK_REPLICATION_SOURCES
    src/engine/network/replication/net_replication.c
    src/engine/include/network/replication/net_replication.h
)

# Pipeline
set(TOOLS_USD_SOURCES
    src/engine/tools/pipeline/usd_importer.m
    src/engine/include/tools/pipeline/usd_importer.h
)

# Profiling
set(TOOLS_PROFILER_SOURCES
    src/engine/tools/profiler/gpu_profiler.m
    src/engine/include/tools/profiler/gpu_profiler.h
)

# Mesh shaders  
set(RENDERING_MESH_SHADERS_SOURCES
    src/engine/backend/metal/mtl_mesh_shaders.m
    src/engine/include/rendering/metal_mesh_shaders.h
)

# Metal integration layer
set(RENDERING_INTEGRATION_SOURCES
    src/engine/backend/metal/mtl_integration.m
    src/engine/include/rendering/metal_integration.h
)

# Metal shaders
set(SHADER_SOURCES
    ${SHADER_SOURCES}
    assets/shaders/nanite/nanite_gpu_culling.metal
    assets/shaders/lighting/lumen_trace.metal
)

# Add all to ENGINE_SOURCES
set(ENGINE_SOURCES
    ${ENGINE_SOURCES}
    ${RENDERING_METALFX_SOURCES}
    ${RENDERING_RAYTRACING_SOURCES}
    ${RENDERING_VRS_SOURCES}
    ${RENDERING_MESH_SHADERS_SOURCES}
    ${RENDERING_INTEGRATION_SOURCES}
    ${GEOMETRY_NANITE_SOURCES}
    ${LIGHTING_LUMEN_SOURCES}
    ${ENVIRONMENT_TERRAIN_SOURCES}
    ${ENVIRONMENT_FOLIAGE_SOURCES}
    ${ANIMATION_BLUEPRINTS_SOURCES}
    ${AUDIO_SPATIAL_PREMIUM_SOURCES}
    ${PHYSICS_DESTRUCTION_SOURCES}
    ${EFFECTS_NIAGARA_SOURCES}
    ${NETWORK_REPLICATION_SOURCES}
    ${TOOLS_USD_SOURCES}
    ${TOOLS_PROFILER_SOURCES}
)
