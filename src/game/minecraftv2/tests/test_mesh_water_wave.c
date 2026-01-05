#include "../include/mesh/mesh.h>
#include "../include/chunk/chunk.h>
#include "../include/block/block.h>
#include "../include/block/water_physics.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // Create a minimal chunk
    Chunk chunk = {0};
    chunk.pos = chunk_pos(0,0,0);
    chunk.blocks = malloc(sizeof(BlockID) * CHUNK_SIZE_CUBE);
    chunk.skylight = malloc(CHUNK_SIZE_CUBE);
    chunk.blocklight = malloc(CHUNK_SIZE_CUBE);
    chunk.metadata = malloc(CHUNK_SIZE_CUBE);

    // Initialize all to air
    for (u32 i = 0; i < CHUNK_SIZE_CUBE; i++) {
        chunk.blocks[i] = BLOCK_AIR;
        chunk.skylight[i] = 0;
        chunk.blocklight[i] = 0;
        chunk.metadata[i] = 0;
    }

    // Place a single water block at (1,1,1)
    chunk_set_block(&chunk, 1, 1, 1, BLOCK_WATER);

    Mesh mesh;
    mesh_init(&mesh, 1024, 1024);

    // Setup a minimal block registry with defaults (including water)
    BlockRegistry registry;
    block_registry_init(&registry, 128);
    block_registry_init_defaults(&registry);

    MeshOptions options = {0};
    options.greedy_meshing = false;
    options.ambient_occlusion = false;
    options.face_culling = false;

    mesh_generate_chunk(&mesh, &chunk, &registry, options);

    // Expect at least one vertex and at least one vertex to have non-zero wave_phase
    if (mesh.vertex_count == 0) {
        fprintf(stderr, "FAIL: No vertices generated for water block\n");
        return 1;
    }

    bool found_wave = false;
    for (u32 i = 0; i < mesh.vertex_count; i++) {
        if (mesh.vertices[i].wave_phase != 0.0f) {
            found_wave = true;
            break;
        }
    }

    if (!found_wave) {
        fprintf(stderr, "FAIL: No wave phase assigned to water vertices\n");
        return 1;
    }

    printf("PASS: water wave phase present (vertex_count=%u)\n", mesh.vertex_count);

    // Cleanup
    mesh_free(&mesh);
    free(chunk.blocks);
    free(chunk.skylight);
    free(chunk.blocklight);
    free(chunk.metadata);
    block_registry_free(&registry);

    return 0;
}
