#include "mesh_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to parse OBJ file
mesh_t* mesh_load_obj(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return NULL;
    }

    // Temporary buffers for parsing
    // In a production system, use dynamic arrays or a resizable vector implementation
    // For now, we'll use fixed size buffers with capacity checks
    const u32 MAX_TEMP_VERTICES = 65536;
    Vec3* temp_positions = (Vec3*)calloc(MAX_TEMP_VERTICES, sizeof(Vec3));
    Vec2* temp_uvs = (Vec2*)calloc(MAX_TEMP_VERTICES, sizeof(Vec2));
    Vec3* temp_normals = (Vec3*)calloc(MAX_TEMP_VERTICES, sizeof(Vec3));

    u32 pos_count = 0;
    u32 uv_count = 0;
    u32 normal_count = 0;

    // We will build the final mesh directly
    // Assuming triangles only for simplicity
    const u32 MAX_FINAL_VERTICES = 65536;
    const u32 MAX_INDICES = 65536 * 3;

    vertex_t* final_vertices = (vertex_t*)calloc(MAX_FINAL_VERTICES, sizeof(vertex_t));
    u32* final_indices = (u32*)calloc(MAX_INDICES, sizeof(u32));
    u32 final_vertex_count = 0;
    u32 final_index_count = 0;

    if (!temp_positions || !temp_uvs || !temp_normals || !final_vertices || !final_indices) {
        printf("Memory allocation failed for mesh loader buffers\n");
        if (file) fclose(file);
        if (temp_positions) free(temp_positions);
        if (temp_uvs) free(temp_uvs);
        if (temp_normals) free(temp_normals);
        if (final_vertices) free(final_vertices);
        if (final_indices) free(final_indices);
        return NULL;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Skip comments
        if (line[0] == '#') continue;

        if (strncmp(line, "v ", 2) == 0) {
            if (pos_count >= MAX_TEMP_VERTICES) continue;
            sscanf(line + 2, "%f %f %f", &temp_positions[pos_count].x, &temp_positions[pos_count].y, &temp_positions[pos_count].z);
            pos_count++;
        } else if (strncmp(line, "vt ", 3) == 0) {
            if (uv_count >= MAX_TEMP_VERTICES) continue;
            sscanf(line + 3, "%f %f", &temp_uvs[uv_count].x, &temp_uvs[uv_count].y);
            uv_count++;
        } else if (strncmp(line, "vn ", 3) == 0) {
            if (normal_count >= MAX_TEMP_VERTICES) continue;
            sscanf(line + 3, "%f %f %f", &temp_normals[normal_count].x, &temp_normals[normal_count].y, &temp_normals[normal_count].z);
            normal_count++;
        } else if (strncmp(line, "f ", 2) == 0) {
            // Parse faces (triangles only for now)
            // Supports v, v/vt, v/vt/vn, v//vn formats
            u32 v[3], vt[3], vn[3];
            int matches = sscanf(line + 2, "%u/%u/%u %u/%u/%u %u/%u/%u",
                                 &v[0], &vt[0], &vn[0],
                                 &v[1], &vt[1], &vn[1],
                                 &v[2], &vt[2], &vn[2]);

            if (matches == 9) { // v/vt/vn
                for (int i = 0; i < 3; i++) {
                    if (final_vertex_count >= MAX_FINAL_VERTICES) break;

                    final_vertices[final_vertex_count].position = temp_positions[v[i] - 1];
                    final_vertices[final_vertex_count].uv = temp_uvs[vt[i] - 1];
                    final_vertices[final_vertex_count].normal = temp_normals[vn[i] - 1];
                    final_vertices[final_vertex_count].tangent = (Vec4){0,0,0,1}; // Calculated later

                    final_indices[final_index_count++] = final_vertex_count++;
                }
            } else {
                // Try other formats or implement triangulation for quads if needed
                // For MVP, assume triangulated OBJ with full attributes
            }
        }
    }

    fclose(file);
    free(temp_positions);
    free(temp_uvs);
    free(temp_normals);

    // Create the mesh object
    mesh_t* mesh = mesh_create(filename);
    mesh_allocate_buffers(mesh, final_vertex_count, final_index_count);
    mesh_set_vertices(mesh, final_vertices, final_vertex_count, 0);
    mesh_set_indices(mesh, final_indices, final_index_count, 0);

    // Setup default submesh
    submesh_t submesh = {0};
    submesh.index_start = 0;
    submesh.index_count = final_index_count;
    submesh.vertex_start = 0;
    submesh.vertex_count = final_vertex_count;
    mesh_add_submesh(mesh, submesh);

    // Recalculate bounds
    mesh_calculate_bounds(mesh);
    // Recalculate tangents (requires normals and UVs which we parsed)
    mesh_calculate_tangents(mesh);

    free(final_vertices);
    free(final_indices);

    return mesh;
}
