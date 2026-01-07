#include "geometry/lod/lod_generator.h"
#include "geometry/mesh.h"
#include "geometry/geometry_types.h"
#include <core/logger.h>
#include <core/asserts.h>
#include <stdio.h>
#include <stdlib.h>

// Mock mesh creation for testing
mesh_data_t* create_test_mesh_data() {
    mesh_data_t* mesh = (mesh_data_t*)calloc(1, sizeof(mesh_data_t));
    mesh->vertex_count = 4;
    mesh->index_count = 12; // 4 faces * 3
    mesh->vertex_stride = sizeof(vertex_t);
    mesh->vertex_data_size = mesh->vertex_count * mesh->vertex_stride;
    mesh->index_data_size = mesh->index_count * sizeof(uint32_t);
    mesh->index_type = INDEX_TYPE_UINT32;
    
    mesh->vertex_data = malloc(mesh->vertex_data_size);
    mesh->index_data = malloc(mesh->index_data_size);
    
    // Tetrahedron
    vertex_t* verts = (vertex_t*)mesh->vertex_data;
    // V0: Top
    verts[0].position = (Vec3){0, 1, 0};
    // V1: Front Left
    verts[1].position = (Vec3){-1, -1, 1};
    // V2: Front Right
    verts[2].position = (Vec3){1, -1, 1};
    // V3: Back
    verts[3].position = (Vec3){0, -1, -1};
    
    uint32_t* indices = (uint32_t*)mesh->index_data;
    // Face 0: 0-1-2
    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    // Face 1: 0-2-3
    indices[3] = 0; indices[4] = 2; indices[5] = 3;
    // Face 2: 0-3-1
    indices[6] = 0; indices[7] = 3; indices[8] = 1;
    // Face 3: 1-3-2 (Base)
    indices[9] = 1; indices[10] = 3; indices[11] = 2;
    
    return mesh;
}

void test_lod_generation() {
    LOG_INFO("Starting LOD Generation Test...");
    
    mesh_data_t* source = create_test_mesh_data();
    if (!source) {
        LOG_ERROR("Failed to create source mesh");
        return;
    }
    
    // Test QEM Simplification
    // Try to reduce to 50% (should target 2 faces)
    // Note: Tetrahedron is minimal convex shape, can't reduce further without becoming a plane (2 faces back-to-back)
    // or collapsing to line/point. 
    // QEM should be able to collapse to 1 face or 0 if allowed.
    // Our implementation clamps min vertices to 3.
    
    mesh_data_t* simplified = qem_simplify(source, 0.5f); // Target 2 vertices? No, ratio is vertex count
    // 4 verts * 0.5 = 2 verts. Clamped to 3.
    // So result should have 3 verts, 1 face.
    
    if (simplified) {
        LOG_INFO("Simplification result:");
        LOG_INFO("  Vertices: %d -> %d", source->vertex_count, simplified->vertex_count);
        LOG_INFO("  Indices: %d -> %d", source->index_count, simplified->index_count);
        
        if (simplified->vertex_count != 3) {
            LOG_ERROR("Expected 3 vertices, got %d", simplified->vertex_count);
        } else {
            LOG_INFO("Vertex count verification passed");
        }
        
        // Free valid result
        free(simplified->vertex_data);
        free(simplified->index_data);
        free(simplified);
    } else {
        LOG_ERROR("Simplification failed");
    }
    
    // Test Chain Generation
    float ratios[] = {0.75f, 0.5f, 0.25f};
    lod_chain_t* chain = lod_generate_chain(source, ratios, 3);
    
    if (chain) {
        LOG_INFO("LOD Chain Generated:");
        LOG_INFO("  Levels: %d", chain->lod_count);
        for (uint32_t i = 0; i < chain->lod_count; i++) {
             LOG_INFO("  LOD %d: %d verts, %.2f distance, error %.4f", 
                 i, 
                 chain->lods[i]->vertex_count, 
                 chain->switch_distances[i],
                 chain->geometric_errors[i]);
        }
        lod_chain_destroy(chain);
    } else {
        LOG_ERROR("LOD Chain generation failed");
    }
    
    // Cleanup
    free(source->vertex_data);
    free(source->index_data);
    free(source);
    
    LOG_INFO("LOD Generation Test Completed.");
}

int main() {
    test_lod_generation();
    return 0;
}
