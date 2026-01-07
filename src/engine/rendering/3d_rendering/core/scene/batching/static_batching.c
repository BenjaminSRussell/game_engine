/*
 * static_batching.c
 * Static Geometry Batching (Merge at Load Time)
 *
 * Part of the Scene Management subsystem
 * Advanced 3D Rendering Engine
 */

#include "static_batching.h"
#include <stdlib.h>

/*
 * Only used for strictly static geometry that shares materials.
 */

void static_batching_merge(
    void** source_meshes, 
    uint32_t count, 
    void** out_merged_mesh
) {
    // Calculate total vertices/indices
    // Allocate new merged mesh
    // Transform vertices of source meshes into world space (if baking transform)
    // Append to merged mesh
    // Return result
}
