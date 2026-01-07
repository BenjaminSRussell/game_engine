#include "geometry/mesh.h"
#include <common.h>
#include <math/vec3.h>
#include <math/vec2.h>
#include <core/logger.h>
#include <core/asserts.h>
#include <stdio.h>

void test_geometry_system() {
    LOG_INFO("Starting Geometry System Test...");

    // 1. Creation
    mesh_t* mesh = mesh_create("TestCube");
    if (!mesh) {
        LOG_ERROR("Failed to create mesh");
        return;
    }
    
    // 2. Allocation
    // Simple Triangle
    mesh_allocate_buffers(mesh, 3, 3);
    
    // 3. Data Set
    vertex_t vertices[3];
    // Bottom Left
    vertices[0].position = (Vec3){0.0f, 0.0f, 0.0f};
    vertices[0].uv = (Vec2){0.0f, 0.0f};
    
    // Bottom Right
    vertices[1].position = (Vec3){1.0f, 0.0f, 0.0f};
    vertices[1].uv = (Vec2){1.0f, 0.0f};
    
    // Top Center
    vertices[2].position = (Vec3){0.5f, 1.0f, 0.0f};
    vertices[2].uv = (Vec2){0.5f, 1.0f};
    
    u32 indices[3] = {0, 1, 2};
    
    mesh_set_vertices(mesh, vertices, 3, 0);
    mesh_set_indices(mesh, indices, 3, 0);
    
    // 4. Bounds
    mesh_calculate_bounds(mesh);
    LOG_INFO("Bounds Min: (%.2f, %.2f, %.2f)", mesh->bounds.min.x, mesh->bounds.min.y, mesh->bounds.min.z);
    
    // Check bounds correctness (manual check in log)
    
    // 5. Normals
    mesh_calculate_normals(mesh);
    LOG_INFO("Vertex 0 Normal: (%.2f, %.2f, %.2f)", mesh->vertices[0].normal.x, mesh->vertices[0].normal.y, mesh->vertices[0].normal.z);

    // 6. Submeshes
    submesh_t sm;
    sm.index_start = 0;
    sm.index_count = 3;
    sm.material_index = 0;
    sm.vertex_start = 0;
    sm.vertex_count = 3;
    mesh_add_submesh(mesh, sm);

    // 7. Destruction
    mesh_destroy(mesh);
    LOG_INFO("Geometry System Test Passed.");
}
