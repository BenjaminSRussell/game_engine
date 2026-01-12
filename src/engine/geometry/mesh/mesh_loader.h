#ifndef GEOMETRY_MESH_LOADER_H
#define GEOMETRY_MESH_LOADER_H

#include "geometry/mesh.h"

// Loads a mesh from an OBJ file.
// Returns NULL if loading fails.
mesh_t* mesh_load_obj(const char* filename);

#endif // GEOMETRY_MESH_LOADER_H
