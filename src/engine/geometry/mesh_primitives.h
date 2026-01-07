#ifndef GEOMETRY_MESH_PRIMITIVES_H
#define GEOMETRY_MESH_PRIMITIVES_H

#include "geometry_types.h"

/**
 * Procedural Mesh Generators
 * 
 * Generate common primitive shapes for testing and prototyping
 */

// Primitive generators (all generate UVs, normals, and tangents)
mesh_t* mesh_create_cube(f32 size);
mesh_t* mesh_create_sphere(f32 radius, u32 segments);
mesh_t* mesh_create_cylinder(f32 radius, f32 height, u32 segments);
mesh_t* mesh_create_plane(f32 width, f32 depth, u32 subdivisions);
mesh_t* mesh_create_cone(f32 radius, f32 height, u32 segments);
mesh_t* mesh_create_torus(f32 major_radius, f32 minor_radius, u32 major_segments, u32 minor_segments);

#endif // GEOMETRY_MESH_PRIMITIVES_H
