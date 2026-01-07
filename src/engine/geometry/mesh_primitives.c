#include "geometry/mesh_primitives.h"
#include "geometry/mesh.h"
#include <core/memory.h>
#include <include/math/math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ----------------------------------------------------------------------------
// Cube
// ----------------------------------------------------------------------------

mesh_t* mesh_create_cube(f32 size) {
    mesh_t* mesh = mesh_create("Cube");
    
    f32 hs = size * 0.5f; // Half size
    
    // 24 vertices (4 per face) for proper normals
    vertex_t vertices[24];
    u32 vertex_idx = 0;
    
    // Front face (+Z)
    vertices[vertex_idx++] = (vertex_t){{-hs, -hs, hs}, {0, 0, 1}, {0, 0}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{ hs, -hs, hs}, {0, 0, 1}, {1, 0}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{ hs,  hs, hs}, {0, 0, 1}, {1, 1}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{-hs,  hs, hs}, {0, 0, 1}, {0, 1}, {1, 0, 0, 1}};
    
    // Back face (-Z)
    vertices[vertex_idx++] = (vertex_t){{ hs, -hs, -hs}, {0, 0, -1}, {0, 0}, {-1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{-hs, -hs, -hs}, {0, 0, -1}, {1, 0}, {-1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{-hs,  hs, -hs}, {0, 0, -1}, {1, 1}, {-1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{ hs,  hs, -hs}, {0, 0, -1}, {0, 1}, {-1, 0, 0, 1}};
    
    // Top face (+Y)
    vertices[vertex_idx++] = (vertex_t){{-hs, hs, -hs}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{ hs, hs, -hs}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{ hs, hs,  hs}, {0, 1, 0}, {1, 1}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{-hs, hs,  hs}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}};
    
    // Bottom face (-Y)
    vertices[vertex_idx++] = (vertex_t){{-hs, -hs,  hs}, {0, -1, 0}, {0, 0}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{ hs, -hs,  hs}, {0, -1, 0}, {1, 0}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{ hs, -hs, -hs}, {0, -1, 0}, {1, 1}, {1, 0, 0, 1}};
    vertices[vertex_idx++] = (vertex_t){{-hs, -hs, -hs}, {0, -1, 0}, {0, 1}, {1, 0, 0, 1}};
    
    // Right face (+X)
    vertices[vertex_idx++] = (vertex_t){{hs, -hs,  hs}, {1, 0, 0}, {0, 0}, {0, 0, 1, 1}};
    vertices[vertex_idx++] = (vertex_t){{hs, -hs, -hs}, {1, 0, 0}, {1, 0}, {0, 0, 1, 1}};
    vertices[vertex_idx++] = (vertex_t){{hs,  hs, -hs}, {1, 0, 0}, {1, 1}, {0, 0, 1, 1}};
    vertices[vertex_idx++] = (vertex_t){{hs,  hs,  hs}, {1, 0, 0}, {0, 1}, {0, 0, 1, 1}};
    
    // Left face (-X)
    vertices[vertex_idx++] = (vertex_t){{-hs, -hs, -hs}, {-1, 0, 0}, {0, 0}, {0, 0, -1, 1}};
    vertices[vertex_idx++] = (vertex_t){{-hs, -hs,  hs}, {-1, 0, 0}, {1, 0}, {0, 0, -1, 1}};
    vertices[vertex_idx++] = (vertex_t){{-hs,  hs,  hs}, {-1, 0, 0}, {1, 1}, {0, 0, -1, 1}};
    vertices[vertex_idx++] = (vertex_t){{-hs,  hs, -hs}, {-1, 0, 0}, {0, 1}, {0, 0, -1, 1}};
    
    // Indices (6 faces * 2 triangles * 3 indices = 36)
    u32 indices[36] = {
        0, 1, 2,  0, 2, 3,    // Front
        4, 5, 6,  4, 6, 7,    // Back
        8, 9, 10, 8, 10, 11,  // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };
    
    mesh_allocate_buffers(mesh, 24, 36);
    mesh_set_vertices(mesh, vertices, 24, 0);
    mesh_set_indices(mesh, indices, 36, 0);
    mesh_calculate_bounds(mesh);
    
    return mesh;
}

// ----------------------------------------------------------------------------
// Sphere
// ----------------------------------------------------------------------------

mesh_t* mesh_create_sphere(f32 radius, u32 segments) {
    mesh_t* mesh = mesh_create("Sphere");
    
    u32 rings = segments;
    u32 sectors = segments * 2;
    
    u32 vertex_count = (rings + 1) * (sectors + 1);
    u32 index_count = rings * sectors * 6;
    
    vertex_t* vertices = (vertex_t*)MALLOC_GEOMETRY(sizeof(vertex_t) * vertex_count);
    u32* indices = (u32*)MALLOC_GEOMETRY(sizeof(u32) * index_count);
    
    u32 v_idx = 0;
    f32 R = 1.0f / (f32)(rings - 1);
    f32 S = 1.0f / (f32)(sectors - 1);
    
    for (u32 r = 0; r < rings + 1; ++r) {
        for (u32 s = 0; s < sectors + 1; ++s) {
            f32 y = sinf(-M_PI * 0.5f + M_PI * r * R);
            f32 x = cosf(2.0f * M_PI * s * S) * sinf(M_PI * r * R);
            f32 z = sinf(2.0f * M_PI * s * S) * sinf(M_PI * r * R);
            
            vertices[v_idx].position = (Vec3){x * radius, y * radius, z * radius};
            vertices[v_idx].normal = (Vec3){x, y, z};
            vertices[v_idx].uv = (Vec2){s * S, r * R};
            v_idx++;
        }
    }
    
    u32 i_idx = 0;
    for (u32 r = 0; r < rings; ++r) {
        for (u32 s = 0; s < sectors; ++s) {
            u32 cur_row = r * (sectors + 1);
            u32 next_row = (r + 1) * (sectors + 1);
            
            indices[i_idx++] = cur_row + s;
            indices[i_idx++] = next_row + s;
            indices[i_idx++] = next_row + (s + 1);
            
            indices[i_idx++] = cur_row + s;
            indices[i_idx++] = next_row + (s + 1);
            indices[i_idx++] = cur_row + (s + 1);
        }
    }
    
    mesh_allocate_buffers(mesh, vertex_count, index_count);
    mesh_set_vertices(mesh, vertices, vertex_count, 0);
    mesh_set_indices(mesh, indices, index_count, 0);
    mesh_calculate_bounds(mesh);
    mesh_calculate_tangents(mesh);
    
    FREE(vertices);
    FREE(indices);
    
    return mesh;
}

// ----------------------------------------------------------------------------
// Plane
// ----------------------------------------------------------------------------

mesh_t* mesh_create_plane(f32 width, f32 depth, u32 subdivisions) {
    mesh_t* mesh = mesh_create("Plane");
    
    u32 vertex_count = (subdivisions + 1) * (subdivisions + 1);
    u32 index_count = subdivisions * subdivisions * 6;
    
    vertex_t* vertices = (vertex_t*)MALLOC_GEOMETRY(sizeof(vertex_t) * vertex_count);
    u32* indices = (u32*)MALLOC_GEOMETRY(sizeof(u32) * index_count);
    
    f32 hw = width * 0.5f;
    f32 hd = depth * 0.5f;
    
    u32 v_idx = 0;
    for (u32 z = 0; z <= subdivisions; ++z) {
        for (u32 x = 0; x <= subdivisions; ++x) {
            f32 px = -hw + (width * x / (f32)subdivisions);
            f32 pz = -hd + (depth * z / (f32)subdivisions);
            
            vertices[v_idx].position = (Vec3){px, 0, pz};
            vertices[v_idx].normal = (Vec3){0, 1, 0};
            vertices[v_idx].uv = (Vec2){x / (f32)subdivisions, z / (f32)subdivisions};
            vertices[v_idx].tangent = (Vec4){1, 0, 0, 1};
            v_idx++;
        }
    }
    
    u32 i_idx = 0;
    for (u32 z = 0; z < subdivisions; ++z) {
        for (u32 x = 0; x < subdivisions; ++x) {
            u32 tl = z * (subdivisions + 1) + x;
            u32 tr = tl + 1;
            u32 bl = (z + 1) * (subdivisions + 1) + x;
            u32 br = bl + 1;
            
            indices[i_idx++] = tl;
            indices[i_idx++] = bl;
            indices[i_idx++] = tr;
            
            indices[i_idx++] = tr;
            indices[i_idx++] = bl;
            indices[i_idx++] = br;
        }
    }
    
    mesh_allocate_buffers(mesh, vertex_count, index_count);
    mesh_set_vertices(mesh, vertices, vertex_count, 0);
    mesh_set_indices(mesh, indices, index_count, 0);
    mesh_calculate_bounds(mesh);
    
    FREE(vertices);
    FREE(indices);
    
    return mesh;
}

// ----------------------------------------------------------------------------
// Cylinder
// ----------------------------------------------------------------------------

mesh_t* mesh_create_cylinder(f32 radius, f32 height, u32 segments) {
    mesh_t* mesh = mesh_create("Cylinder");
    
    u32 vertex_count = (segments + 1) * 2 + segments * 2; // Sides + caps
    u32 index_count = segments * 6 + segments * 6; // Sides + caps
    
    vertex_t* vertices = (vertex_t*)MALLOC_GEOMETRY(sizeof(vertex_t) * vertex_count);
    u32* indices = (u32*)MALLOC_GEOMETRY(sizeof(u32) * index_count);
    
    f32 half_height = height * 0.5f;
    u32 v_idx = 0;
    
    // Cylinder sides
    for (u32 i = 0; i <= segments; ++i) {
        f32 angle = 2.0f * M_PI * i / (f32)segments;
        f32 x = cosf(angle);
        f32 z = sinf(angle);
        
        // Bottom vertex
        vertices[v_idx].position = (Vec3){x * radius, -half_height, z * radius};
        vertices[v_idx].normal = (Vec3){x, 0, z};
        vertices[v_idx].uv = (Vec2){i / (f32)segments, 0};
        v_idx++;
        
        // Top vertex
        vertices[v_idx].position = (Vec3){x * radius, half_height, z * radius};
        vertices[v_idx].normal = (Vec3){x, 0, z};
        vertices[v_idx].uv = (Vec2){i / (f32)segments, 1};
        v_idx++;
    }
    
    // Caps (simplified - center + ring)
    u32 cap_start = v_idx;
    vertices[v_idx++] = (vertex_t){{0, half_height, 0}, {0, 1, 0}, {0.5f, 0.5f}, {1, 0, 0, 1}};
    vertices[v_idx++] = (vertex_t){{0, -half_height, 0}, {0, -1, 0}, {0.5f, 0.5f}, {1, 0, 0, 1}};
    
    // Side indices
    u32 i_idx = 0;
    for (u32 i = 0; i < segments; ++i) {
        u32 bottom = i * 2;
        u32 top = bottom + 1;
        u32 next_bottom = (i + 1) * 2;
        u32 next_top = next_bottom + 1;
        
        indices[i_idx++] = bottom;
        indices[i_idx++] = next_bottom;
        indices[i_idx++] = top;
        
        indices[i_idx++] = top;
        indices[i_idx++] = next_bottom;
        indices[i_idx++] = next_top;
    }
    
    mesh_allocate_buffers(mesh, v_idx, i_idx);
    mesh_set_vertices(mesh, vertices, v_idx, 0);
    mesh_set_indices(mesh, indices, i_idx, 0);
    mesh_calculate_bounds(mesh);
    mesh_calculate_tangents(mesh);
    
    FREE(vertices);
    FREE(indices);
    
    return mesh;
}

// ----------------------------------------------------------------------------
// Cone
// ----------------------------------------------------------------------------

mesh_t* mesh_create_cone(f32 radius, f32 height, u32 segments) {
    mesh_t* mesh = mesh_create("Cone");
    
    u32 vertex_count = segments + 2; // Base ring + apex + base center
    u32 index_count = segments * 3 + segments * 3; // Sides + base
    
    vertex_t* vertices = (vertex_t*)MALLOC_GEOMETRY(sizeof(vertex_t) * vertex_count);
    u32* indices = (u32*)MALLOC_GEOMETRY(sizeof(u32) * index_count);
    
    u32 v_idx = 0;
    
    // Apex
    vertices[v_idx++] = (vertex_t){{0, height, 0}, {0, 1, 0}, {0.5f, 1}, {1, 0, 0, 1}};
    
    // Base ring
    u32 base_start = v_idx;
    for (u32 i = 0; i < segments; ++i) {
        f32 angle = 2.0f * M_PI * i / (f32)segments;
        f32 x = cosf(angle) * radius;
        f32 z = sinf(angle) * radius;
        
        // Calculate normal (points outward and up)
        Vec3 to_apex = {-x, height, -z};
        f32 len = sqrtf(to_apex.x * to_apex.x + to_apex.y * to_apex.y + to_apex.z * to_apex.z);
        Vec3 normal = {to_apex.x / len, to_apex.y / len, to_apex.z / len};
        
        vertices[v_idx].position = (Vec3){x, 0, z};
        vertices[v_idx].normal = normal;
        vertices[v_idx].uv = (Vec2){i / (f32)segments, 0};
        v_idx++;
    }
    
    // Base center
    u32 base_center = v_idx;
    vertices[v_idx++] = (vertex_t){{0, 0, 0}, {0, -1, 0}, {0.5f, 0.5f}, {1, 0, 0, 1}};
    
    // Side indices
    u32 i_idx = 0;
    for (u32 i = 0; i < segments; ++i) {
        u32 current = base_start + i;
        u32 next = base_start + ((i + 1) % segments);
        
        indices[i_idx++] = 0; // Apex
        indices[i_idx++] = current;
        indices[i_idx++] = next;
        
        // Base
        indices[i_idx++] = base_center;
        indices[i_idx++] = next;
        indices[i_idx++] = current;
    }
    
    mesh_allocate_buffers(mesh, v_idx, i_idx);
    mesh_set_vertices(mesh, vertices, v_idx, 0);
    mesh_set_indices(mesh, indices, i_idx, 0);
    mesh_calculate_bounds(mesh);
    mesh_calculate_tangents(mesh);
    
    FREE(vertices);
    FREE(indices);
    
    return mesh;
}

// ----------------------------------------------------------------------------
// Torus
// ----------------------------------------------------------------------------

mesh_t* mesh_create_torus(f32 major_radius, f32 minor_radius, u32 major_segments, u32 minor_segments) {
    mesh_t* mesh = mesh_create("Torus");
    
    u32 vertex_count = (major_segments + 1) * (minor_segments + 1);
    u32 index_count = major_segments * minor_segments * 6;
    
    vertex_t* vertices = (vertex_t*)MALLOC_GEOMETRY(sizeof(vertex_t) * vertex_count);
    u32* indices = (u32*)MALLOC_GEOMETRY(sizeof(u32) * index_count);
    
    u32 v_idx = 0;
    for (u32 i = 0; i <= major_segments; ++i) {
        f32 theta = 2.0f * M_PI * i / (f32)major_segments;
        f32 cos_theta = cosf(theta);
        f32 sin_theta = sinf(theta);
        
        for (u32 j = 0; j <= minor_segments; ++j) {
            f32 phi = 2.0f * M_PI * j / (f32)minor_segments;
            f32 cos_phi = cosf(phi);
            f32 sin_phi = sinf(phi);
            
            // Position
            f32 x = (major_radius + minor_radius * cos_phi) * cos_theta;
            f32 y = minor_radius * sin_phi;
            f32 z = (major_radius + minor_radius * cos_phi) * sin_theta;
            
            // Normal
            f32 nx = cos_phi * cos_theta;
            f32 ny = sin_phi;
            f32 nz = cos_phi * sin_theta;
            
            vertices[v_idx].position = (Vec3){x, y, z};
            vertices[v_idx].normal = (Vec3){nx, ny, nz};
            vertices[v_idx].uv = (Vec2){i / (f32)major_segments, j / (f32)minor_segments};
            v_idx++;
        }
    }
    
    // Indices
    u32 i_idx = 0;
    for (u32 i = 0; i < major_segments; ++i) {
        for (u32 j = 0; j < minor_segments; ++j) {
            u32 current = i * (minor_segments + 1) + j;
            u32 next = current + minor_segments + 1;
            
            indices[i_idx++] = current;
            indices[i_idx++] = next;
            indices[i_idx++] = current + 1;
            
            indices[i_idx++] = current + 1;
            indices[i_idx++] = next;
            indices[i_idx++] = next + 1;
        }
    }
    
    mesh_allocate_buffers(mesh, v_idx, i_idx);
    mesh_set_vertices(mesh, vertices, v_idx, 0);
    mesh_set_indices(mesh, indices, i_idx, 0);
    mesh_calculate_bounds(mesh);
    mesh_calculate_tangents(mesh);
    
    FREE(vertices);
    FREE(indices);
    
    return mesh;
}
