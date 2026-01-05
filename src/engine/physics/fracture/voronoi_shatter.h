#ifndef VORONOI_SHATTER_H
#define VORONOI_SHATTER_H

#include <stdint.h>

// Returns the ID of the new fractured mesh
uint64_t voronoi_shatter_mesh(uint64_t source_mesh_id, uint32_t fragment_count);

#endif // VORONOI_SHATTER_H
