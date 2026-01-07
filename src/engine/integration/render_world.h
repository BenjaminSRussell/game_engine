#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "render_config.h"

// Fix Component typedef collision with Carbon/ECS
#define Component CarbonComponent
#include <Metal/Metal.h>
#include <simd/simd.h>
#undef Component

#include <stdbool.h>

// Include system definitions for value types
#include "rendering/3d_rendering/lighting/shadows/shadow_atlas.h"
#include "rendering/3d_rendering/rendering/deferred/deferred_lighting.h"
#include "rendering/3d_rendering/effects/gpu_particles/gpu_particle_system.h"
#include "rendering/3d_rendering/environment/atmosphere/sky/sky_atmosphere/atmosphere_system.h"
#include "rendering/3d_rendering/postprocess/postprocessing/anti_aliasing/taa_resolve.h"
#include "rendering/3d_rendering/postprocess/bloom/bloom_types.h"
#include "rendering/3d_rendering/editor/debug/debug_renderer.h"
#include "rendering/3d_rendering/editor/debug/gpu_profiler.h"
#include "rendering/3d_rendering/editor/debug/cpu_gpu_timing.h"
#include "rendering/3d_rendering/editor/debug/performance_analyzer.h"

// Forward declarations for external types
typedef struct Scene scene_t;
struct Camera; // Use struct Camera directly to avoid collision

// Forward declarations for pointer types
typedef struct render_graph render_graph_t;
typedef struct gbuffer gbuffer_t; 
typedef struct transform_gizmo transform_gizmo_t;

// Wrapper structs 
typedef struct shadow_system {
    lighting_shadow_atlas_handle_t atlas;
} shadow_system_t;

typedef struct light_system {
    deferred_lighting_t* deferred;
} light_system_t;

typedef struct material_system {
    void* internal_state;
} material_system_t;

typedef struct ocean_system {
    void* internal_state; 
} ocean_system_t;

typedef struct taa_state {
    postprocessing_taa_resolve_handle_t handle;
} taa_state_t;

typedef struct bloom_state {
    bloom_system_t* system;
} bloom_state_t;

// Main Render World Structure
typedef struct render_world {
    // Core
    id<MTLDevice> device;
    uint32_t width;
    uint32_t height;

    // Systems
    render_graph_t* graph;
    gbuffer_t* gbuffer;
    shadow_system_t* shadows;
    light_system_t* lights;
    material_system_t* materials;

    // Effect systems
    gpu_particle_system_t* particles;
    ocean_system_t* ocean;
    atmosphere_system_t* atmosphere;

    // Post-processing
    taa_state_t* taa;
    bloom_state_t* bloom;

    // Editor
    debug_renderer_t* debug;
    transform_gizmo_t* gizmo;

    // Profiling
    gpu_profiler_t* gpu_profiler;
    cpu_gpu_timing_t* cpu_gpu_timing;
    performance_analyzer_t* perf_analyzer;

    // Config
    render_config_t config;

} render_world_t;

// API
render_world_t* render_world_create(id<MTLDevice> device, uint32_t width, uint32_t height);
void render_world_destroy(render_world_t* world);
void render_world_resize(render_world_t* world, uint32_t width, uint32_t height);
void render_world_render(render_world_t* world, const scene_t* scene, const struct Camera* camera);

#endif // RENDER_WORLD_H
