#include <common.h>
#include <rendering/mesh.h>
#include <rendering/renderer.h>
#include <stdlib.h>
#include <string.h>

// Basic Mesh system implementation

void mesh_init(Mesh *mesh, u32 vertex_capacity, u32 index_capacity) {
    if (!mesh) return;

    memset(mesh, 0, sizeof(Mesh));
    
    mesh->vertex_capacity = vertex_capacity;
    mesh->index_capacity = index_capacity;

    if (vertex_capacity > 0) {
        mesh->vertices = (Vertex*)calloc(vertex_capacity, sizeof(Vertex));
    }
    
    if (index_capacity > 0) {
        mesh->indices = (u32*)calloc(index_capacity, sizeof(u32));
    }

    mesh->is_uploaded = false;
}

void mesh_free(Mesh *mesh) {
    if (!mesh) return;

    if (mesh->vertices) {
        free(mesh->vertices);
        mesh->vertices = NULL;
    }
    
    if (mesh->indices) {
        free(mesh->indices);
        mesh->indices = NULL;
    }

    mesh->vertex_count = 0;
    mesh->vertex_capacity = 0;
    mesh->index_count = 0;
    mesh->index_capacity = 0;
    mesh->is_uploaded = false;
}

void mesh_optimize_vertex_cache(Mesh *mesh) {
    (void)mesh;
    // Placeholder: In a real implementation, we would reorder indices
    // to optimize for GPU vertex cache locality (e.g., Forsyth algorithm).
}
