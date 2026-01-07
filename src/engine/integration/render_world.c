#include "render_world.h"
#include <stdlib.h>
#include <string.h>

// Includes are now largely in .h, but we might need more for implementation details
// #include "rendering/3d_rendering/rendering/render_graph/render_graph.h" 

// External includes
#include "core/scene.h"
#include "renderer/camera.h"

// Forward decl of scene_render_frame from scene_renderer.c
void scene_render_frame(render_world_t* world, const scene_t* scene, const struct Camera* camera);

render_world_t* render_world_create(id<MTLDevice> device, uint32_t width, uint32_t height) {
    render_world_t* world = (render_world_t*)calloc(1, sizeof(render_world_t));
    if (!world) return NULL;

    world->device = device;
    world->width = width;
    world->height = height;

    // Initialize Config
    render_config_init_defaults(&world->config);

    // Initialize Systems
    // 1. Shadows
    world->shadows = (shadow_system_t*)calloc(1, sizeof(shadow_system_t));
    lighting_shadow_atlas_init();
    lighting_shadow_atlas_desc_t shadow_desc = { .flags = 0 };
    lighting_shadow_atlas_create(&world->shadows->atlas, &shadow_desc);

    // 2. Lights
    world->lights = (light_system_t*)calloc(1, sizeof(light_system_t));
    world->lights->deferred = rendering_deferred_lighting_create(device, MTLPixelFormatRGBA16Float, MTLPixelFormatDepth32Float);

    // 3. Atmosphere
    world->atmosphere = (atmosphere_system_t*)calloc(1, sizeof(atmosphere_system_t));
    atmosphere_init(world->atmosphere, (void*)device); // Cast to match void* signature logic if needed

    // 4. Particles
    world->particles = (gpu_particle_system_t*)calloc(1, sizeof(gpu_particle_system_t));
    // gpu_particle_system_init(world->particles, device); 

    // 5. TAA
    world->taa = (taa_state_t*)calloc(1, sizeof(taa_state_t));
    postprocessing_taa_resolve_init();
    postprocessing_taa_resolve_desc_t taa_desc = { .flags = 0 };
    postprocessing_taa_resolve_create(&world->taa->handle, &taa_desc);

    // 6. Bloom
    world->bloom = (bloom_state_t*)calloc(1, sizeof(bloom_state_t));
    world->bloom->system = (bloom_system_t*)calloc(1, sizeof(bloom_system_t));
    // bloom_init(world->bloom->system, device, width, height); 

    // 7. Materials
    world->materials = (material_system_t*)calloc(1, sizeof(material_system_t));
    // materials_material_instance_init();

    // 8. Ocean
    world->ocean = (ocean_system_t*)calloc(1, sizeof(ocean_system_t));

    // 9. Debug
    world->debug = debug_renderer_create(device, 100000); // Allocate space for 100k lines

    // 10. Profiling
    world->gpu_profiler = gpu_profiler_create(device);
    world->cpu_gpu_timing = cpu_gpu_timing_create();
    world->perf_analyzer = performance_analyzer_create();

    return world;
}

void render_world_destroy(render_world_t* world) {
    if (!world) return;

    if (world->shadows) {
        lighting_shadow_atlas_destroy(world->shadows->atlas);
        lighting_shadow_atlas_shutdown();
        free(world->shadows);
    }
    
    if (world->lights) {
        rendering_deferred_lighting_destroy(world->lights->deferred);
        free(world->lights);
    }

    if (world->atmosphere) {
        atmosphere_shutdown(world->atmosphere);
        free(world->atmosphere);
    }

    if (world->taa) {
        postprocessing_taa_resolve_destroy(world->taa->handle);
        postprocessing_taa_resolve_shutdown();
        free(world->taa);
    }

    if (world->bloom) {
        free(world->bloom->system);
        free(world->bloom);
    }
    
    if (world->particles) free(world->particles);
    if (world->materials) free(world->materials);
    if (world->ocean) free(world->ocean);

    // Profiling cleanup
    if (world->gpu_profiler) gpu_profiler_destroy(world->gpu_profiler);
    if (world->cpu_gpu_timing) cpu_gpu_timing_destroy(world->cpu_gpu_timing);
    if (world->perf_analyzer) performance_analyzer_destroy(world->perf_analyzer);

    free(world);
}

void render_world_resize(render_world_t* world, uint32_t width, uint32_t height) {
    if (!world) return;
    world->width = width;
    world->height = height;
}

void render_world_render(render_world_t* world, const scene_t* scene, const struct Camera* camera) {
    scene_render_frame(world, scene, camera);
}
