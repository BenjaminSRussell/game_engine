/*
 * surface_atlas.c
 * Lumen Surface Cache Atlas Management
 *
 * Part of the Lumen GI subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/lumen/surface_cache/surface_atlas.h"
#include "lighting/lightmaps/lightmap_packer.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * Manages the large texture atlas that stores surface radiance/albedo/normals
 * for "Surface Cards".
 */

typedef struct surface_atlas_ctx {
    uint32_t width; 
    uint32_t height;
    void* albedo_atlas;
    void* emission_atlas;
    void* depth_atlas;
    bool initialized;
} surface_atlas_ctx_t;

static surface_atlas_ctx_t g_atlas = {0};

int surface_atlas_init(uint32_t size) {
    g_atlas.width = size;
    g_atlas.height = size;
    // Create textures
    
    // Init Packer
    lightmap_packer_init(size, size);
    
    g_atlas.initialized = true;
    return 0;
}

bool surface_atlas_allocate(uint32_t w, uint32_t h, uint32_t* out_x, uint32_t* out_y) {
    if (!g_atlas.initialized) return false;
    
    int x, y;
    if (lightmap_packer_pack((int)w, (int)h, &x, &y)) {
        *out_x = (uint32_t)x;
        *out_y = (uint32_t)y;
        return true;
    }
    return false;
}

void surface_atlas_shutdown(void) {
    lightmap_packer_shutdown();
    // Destroy textures
    g_atlas.initialized = false;
}
