#include "environment/stylized/voxel_builder.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t id_map[256];
    float uvs[256][4];
    bool transparent[256];
} BlockPalette;

static BlockPalette g_palette = {0};

void voxel_builder_init_palette() {
    // Basic palette setup
    memset(g_palette.id_map, 0, sizeof(g_palette.id_map));
    g_palette.transparent[0] = true; // Air
    // ...
}

void voxel_builder_register_block(uint8_t id, float u0, float v0, float u1, float v1, bool transparent) {
    g_palette.uvs[id][0] = u0;
    g_palette.uvs[id][1] = v0;
    g_palette.uvs[id][2] = u1;
    g_palette.uvs[id][3] = v1;
    g_palette.transparent[id] = transparent;
}

void voxel_builder_generate_mesh(const uint8_t *voxels, int width, int height, int depth, void **out_mesh) {
    // Simplified Greedy Meshing Stub
    // Real implementation would iterate voxels, combine adjacent faces of same type
    // and generate optimized geometry
    
    // For now, simple cube generation per voxel
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (z * height + y) * width + x;
                uint8_t id = voxels[idx];
                if (id == 0) continue; // Skip air
                
                // Check neighbors for culling
                // if (is_solid(x+1, y, z)) cull_face(RIGHT);
                // ...
            }
        }
    }
}

bool voxel_builder_destroy_block(uint8_t *voxels, int width, int height, int depth, float hit_x, float hit_y, float hit_z) {
    int x = (int)hit_x;
    int y = (int)hit_y;
    int z = (int)hit_z;
    
    if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth) {
        int idx = (z * height + y) * width + x;
        if (voxels[idx] != 0) {
            voxels[idx] = 0;
            // Spawn particles
            // particle_system_spawn(hit_x, hit_y, hit_z, BLOCK_BREAK_EFFECT);
            return true;
        }
    }
    return false;
}
